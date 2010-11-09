#include <WProgram.h>
#include <util/delay.h>
#include "MAX6675.h"
#include "PID.h"
#include "MLCScript.h"
#include "Util.h"

// pin aliases
#define THERMO_CS1  2
#define THERMO_DO1  3
#define THERMO_CLK1 4
#define THERMO_CS2  5
#define THERMO_DO2  6
#define THERMO_CLK2 7

#define BOBBER_SPARGING 8
#define BOBBER_MASHING  9

#define STIR_PLATE 10

#define RELAY_240V 11
#define RELAY_120V 12

// misc constants
#define MAX_TEMPERATURE 80.0f
#define MIN_TEMPERATURE 25.0f
#define MAX_MASHING_TEMPERATURE 74.0f
#define PUMP_TIMEOUT 10000
#define STIR_PLATE_PWM 30
#define STIR_PLATE_WAIT_MS 2000

// PID constants
#define PID_OUTPUT_LIMIT 1000
#define P_GAIN 250.0f
#define I_GAIN 0.0f
#define D_GAIN 0.0f

#define ON true
#define OFF false

typedef enum ProgramState {
    kProgramState_WaitingForInput = 0,
    kProgramState_Mashing,
    kProgramState_Sparging,
    kNumProgramStates
} ProgramState;

typedef union InputBuffer {
    uint32_t intVal;
    float floatVal;
    char stringVal[sizeof(uint32_t)];
} InputBuffer;

static ProgramState currentState = kProgramState_WaitingForInput;

static MAX6675 thermocouple1(THERMO_CLK1, THERMO_CS1, THERMO_DO1);
static MAX6675 thermocouple2(THERMO_CLK2, THERMO_CS2, THERMO_DO2);

static MLCScript currentScript;

static int8_t pumpState;

void setup(void) {
    // arduino function that sets up the arduino-specific things
    init();

    // set USB communication to 9600 b/s
	Serial.begin(9600);

    // set pin modes for the off-board I/O (except thermocouples)
    pinMode(BOBBER_MASHING, INPUT);
    pinMode(BOBBER_SPARGING, INPUT);
    pinMode(RELAY_240V, OUTPUT);
    pinMode(RELAY_120V, OUTPUT);

    // apparently the thermocouples need a while to stabilize
    delay(500);

    Serial.println("PROGRAM RUNNING");
}

void readNextNullTerminatedString(InputBuffer& inputBuf) {
    const uint8_t bytesToRead = sizeof(inputBuf) - 1;
    while (Serial.available() < bytesToRead);

    for (uint8_t i = 0; i < bytesToRead; ++i)
        inputBuf.stringVal[i] = Serial.read();

    inputBuf.stringVal[bytesToRead] = '\0';
}

void readNextFourBytes(InputBuffer& inputBuf) {
    while (Serial.available() < sizeof(inputBuf));

    for (uint8_t i = 0; i < sizeof(inputBuf); ++i)
        inputBuf.stringVal[i] = Serial.read();
}

void writeSuccess(void) {
    Serial.println("OK");
}

void writeFailure(void) {
    currentState = kProgramState_WaitingForInput;
    Serial.println("ERROR");
    Serial.flush();
}

void readMLCScript(void) {
    InputBuffer inputData;

    // ok, let's make sure we're on the same page here...
    readNextNullTerminatedString(inputData);
    if (strcmp (inputData.stringVal, "BEG") == 0)
        writeSuccess();
    else
        return writeFailure();

    // mode: mashing or sparging?
    readNextNullTerminatedString(inputData);
    if (strcmp (inputData.stringVal, "MSH") == 0) { // mashing
        currentState = kProgramState_Mashing;
        writeSuccess();
    }
    else if (strcmp (inputData.stringVal, "SPG") == 0) { // sparging
        currentState = kProgramState_Sparging;
        writeSuccess();
    }
    else
        return writeFailure();

    currentScript.reset();

	// mash water volume
	readNextFourBytes(inputData);
	uint32_t mashWaterVolume = inputData.intVal;
	if (mashWaterVolume >= 0.0f) {
		currentScript.setMashWaterVolume(mashWaterVolume);
		writeSuccess();
	}
	else
		return writeFailure();


    // number of (temperature, duration) pairs to expect
    readNextFourBytes(inputData);
    uint32_t scriptLength = inputData.intVal;
    if (scriptLength <= NUM_SETPOINTS) 
        writeSuccess();
    else
        return writeFailure();

    for (uint8_t i = 0; i < scriptLength; ++i) {
        readNextFourBytes(inputData);
        float temperature = inputData.floatVal;
        if (temperature <= MAX_TEMPERATURE && temperature >= MIN_TEMPERATURE)
            writeSuccess();
        else
            return writeFailure();

        readNextFourBytes(inputData);
        uint32_t durationMillis = inputData.intVal;
        // not really any bounds checking we can do on durationMillis...
        writeSuccess();

        currentScript.addSetpoint(temperature, durationMillis);
    }

    // make sure everything is synced up still 
    readNextNullTerminatedString(inputData);
    if (strcmp (inputData.stringVal, "END") == 0)
        writeSuccess();
    else
        return writeFailure();

    Serial.println("RUNNING SCRIPT");
}

void outputStatus(float temperature1, float temperature2, uint32_t heaterDutyCycleMillis) {
    Serial.print(currentState == kProgramState_Mashing ? "MASHING" : "SPARGING");
    Serial.print(',');
    Serial.print(currentScript.timeInCurrentInterval());
    Serial.print(',');
    Serial.print(currentScript.currentIntervalDuration());
    Serial.print(',');
    Serial.print(temperature1);
    Serial.print(',');
    Serial.print(temperature2);
    Serial.print(',');
    Serial.print(currentScript.currentTemperatureSetpoint());
    Serial.print(',');
    Serial.print(heaterDutyCycleMillis);
    Serial.print(',');
    Serial.print(pumpState == HIGH ? "ON" : "OFF");
    Serial.println("");
}

void runPumpBasedOnMashingBobberPosition(uint32_t elapsedMillis) {
    static int16_t pumpTimeout = 0;
    int8_t bobberState = digitalRead(BOBBER_MASHING);

    pumpState = HIGH;

    if (bobberState == HIGH) { // bobber signal HIGH means water level is low
        if (pumpTimeout > 0)
            pumpTimeout -= elapsedMillis;
        else
            pumpState = LOW;
    }
    else
        pumpTimeout = PUMP_TIMEOUT;

    digitalWrite(RELAY_120V, pumpState);
}

void runPumpBasedOnSpargingBobberPosition(uint32_t elapsedMillis) {
    int8_t bobberState = digitalRead(BOBBER_SPARGING);
    digitalWrite(RELAY_120V, bobberState);
}

void runHeaterForDutyCycleMillis(uint32_t dutyCycleMillis) {
    if (dutyCycleMillis <= 1000) {
        if (dutyCycleMillis > 0) {
            digitalWrite(RELAY_240V, HIGH);
            _delay_ms(dutyCycleMillis);
        }

        digitalWrite(RELAY_240V, LOW);
        _delay_ms(1000 - dutyCycleMillis);
    }
    else
		writeFailure();
}

float dutyCycleBasedOnPIDControl(float currentTemperature, uint32_t elapsedMillis) {
    static PID pid;
    static float temperatureSetpoint = 0.0f;
    
    float previousTemperatureSetpoint = temperatureSetpoint;

    temperatureSetpoint = currentScript.currentTemperatureSetpoint();
	if (ISNAN(temperatureSetpoint)) {
		writeFailure();
		return 0.0f;
	}
	
    if (previousTemperatureSetpoint != temperatureSetpoint) {
        PID newPID(temperatureSetpoint, P_GAIN, I_GAIN, D_GAIN, PID_OUTPUT_LIMIT);
        pid = newPID;
    }

    return pid.nextControlOutput(currentTemperature, elapsedMillis);
}

void readTemperaturesFromThermocouples(float* temp1Pointer, float* temp2Pointer) {
	analogWrite(STIR_PLATE, 0);
	_delay_ms(STIR_PLATE_WAIT_MS);
	*temp1Pointer = thermocouple1.readCelsius();
	*temp2Pointer = thermocouple2.readCelsius();
	analogWrite(STIR_PLATE, STIR_PLATE_PWM);
}

int main() {
    setup();

    uint32_t lastTime              = millis(),
             currentTime           = 0,
             elapsedMillis         = 0,
             heaterDutyCycleMillis = 0;

    float temperature1       = 0.0f,
		  temperature2       = 0.0f,
		  currentTemperature = 0.0f;

    while (1) {
        currentTime = millis();
        elapsedMillis = currentTime - lastTime;

		// MASHING
		// =======
		// Thermocouples:
		//   The thermocouple in the grain bed
		//   (thermocouple2) is used as the input for PID control,
		//   and the thermocouple in the HLT (thermocouple1) is just
		//   used to ensure the wort does not exceed the
		//   MAX_MASHING_TEMPERATURE.
		// Bobbers:
		//   The bobber in the grant (BOBBER_MASHING) turns the
		//   pump on or off, with some hysteresis.
		//   
		// SPARGING
		// ========
		// Thermocouples:
		//   The thermocouple in the HLT (thermocouple1) is the only
		//   one relevant for the sparging process. It is used as the
		//   input for PID control.
		// Bobbers:
		//   The bobber in the mash tun (BOBBER_SPARGING) turns the
		//   pump on or off, with some hysteresis.
        if (currentState == kProgramState_Mashing || currentState == kProgramState_Sparging) {

            // end mashing/sparging if the currentScript has completed
            if (currentScript.completed()) {
                currentState = kProgramState_WaitingForInput;
                Serial.println("END OF SCRIPT");
            }
			else { // run the control system
				readTemperaturesFromThermocouples(&temperature1, &temperature2);

				if (currentState == kProgramState_Mashing) {
					currentTemperature = temperature2;
					if (temperature1 >= MAX_MASHING_TEMPERATURE)
						heaterDutyCycleMillis = 0;
					else
						heaterDutyCycleMillis = dutyCycleBasedOnPIDControl(currentTemperature, elapsedMillis);
				}
				else {
					currentTemperature = temperature1;
					heaterDutyCycleMillis = dutyCycleBasedOnPIDControl(currentTemperature, elapsedMillis);
				}
				currentScript.step(elapsedMillis, currentTemperature);
	
				outputStatus(temperature1, temperature2, heaterDutyCycleMillis);

				runPumpBasedOnMashingBobberPosition(elapsedMillis);
				runHeaterForDutyCycleMillis(heaterDutyCycleMillis);
			}
		}
		else if (currentState == kProgramState_WaitingForInput) {
			readMLCScript();
			currentTime = millis(); // don't include time to read script in time steps
		}

        lastTime = currentTime;        
    }
}

