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

#define BOBBER_MLT 8
#define BOBBER_GRANT  9

#define STIR_PLATE 10

#define RELAY_120V 11
#define RELAY_240V 12

// misc constants
#define PUMP_TIMEOUT 10000
#define STIR_PLATE_PWM 90 
#define STIR_PLATE_START_MS 4000
#define STIR_PLATE_STOP_MS 5500
#define HEATER_PERIOD_MS 1000

// PID constants
#define PID_OUTPUT_LIMIT 1000
#define P_GAIN 250.0f

static void pause();

// Thermocouple 1 is in the Hot-Liquor Tank
static MAX6675 thermocouple1(THERMO_CLK1, THERMO_CS1, THERMO_DO1);

// Thermocouple 2 is in the grain bed
static MAX6675 thermocouple2(THERMO_CLK2, THERMO_CS2, THERMO_DO2);

static MLCScript currentScript(pause);

static int pumpState;
static int16_t pumpTimeout;

void setup(void)
{
    // arduino function that sets up the arduino-specific things
    init();

    // set USB communication to 9600 b/s
    Serial.begin(9600);

    // set pin modes for the off-board I/O (except thermocouples)
    pinMode(BOBBER_GRANT, INPUT);
    pinMode(BOBBER_MLT, INPUT);
    pinMode(STIR_PLATE, OUTPUT);
    pinMode(RELAY_120V, OUTPUT);
    pinMode(RELAY_240V, OUTPUT);

    // apparently the thermocouples need a while to stabilize
    delay(500);
}


void writeSuccess(void)
{
    Serial.println("OK");
}

void writeFailure(const char* msg)
{
    Serial.print("ERROR: ");
    Serial.println(msg);
    Serial.flush();
    pause();
}

void outputStatus(float temperature1, float temperature2, uint32_t heaterDutyCycleMillis)
{
    char buffer[4];
    currentScript.currentCommand(buffer);

    Serial.print(currentScript.mode() == MASHING ? "MASHING"
               : currentScript.mode() == SPARGING ? "SPARGING"
               : currentScript.mode() == INITIALIZE ? "INITIALIZE"
               : "UNKNOWN MODE");
    Serial.print(',');
    Serial.print(buffer);
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
    Serial.print(currentScript.currentTemperatureTarget());
    Serial.print(',');
    Serial.print(heaterDutyCycleMillis);
    Serial.print(',');
    Serial.print(pumpState == HIGH ? "ON" : "OFF");
    Serial.print(',');
    Serial.print(digitalRead(BOBBER_GRANT) == HIGH ? "DOWN" : "UP");
    Serial.print(',');
    Serial.print(digitalRead(BOBBER_MLT) == HIGH ? "DOWN" : "UP");
    Serial.println("");
}

void runPumpBasedOnBobberPosition(uint32_t elapsedMillis)
{
    if (currentScript.mode() == MASHING) {
        int bobberState = digitalRead(BOBBER_GRANT);
        
        pumpState = HIGH;
        if (bobberState == HIGH) { // bobber signal HIGH means water level is low
            if (pumpTimeout > 0)
                pumpTimeout -= elapsedMillis;
            else 
                pumpState = LOW;
        }
        else
            pumpTimeout = PUMP_TIMEOUT;
    }
    else if (currentScript.mode() == SPARGING) {
        int bobberState = digitalRead(BOBBER_MLT);
        pumpState = bobberState;
    }
    else {
        pumpState = LOW;
    }

    digitalWrite(RELAY_120V, pumpState);
}

void runHeaterForDutyCycleMillis(uint32_t dutyCycleMillis)
{
    if (dutyCycleMillis <= HEATER_PERIOD_MS) {
        if (dutyCycleMillis > 0) {
            digitalWrite(RELAY_240V, HIGH);
            _delay_ms(dutyCycleMillis);
        }

        digitalWrite(RELAY_240V, LOW);
        _delay_ms(HEATER_PERIOD_MS - dutyCycleMillis);
    }
    else {
        writeFailure("Duty cycle must be <= 1000");
    }
}


void pause()
{
    Serial.flush();
    Serial.println("PAUSED");
    pumpState = LOW;
    pumpTimeout = 0;
    digitalWrite(RELAY_120V, pumpState);
    digitalWrite(STIR_PLATE, LOW);
    while (Serial.available() < 1) {}
    Serial.flush();
}

float dutyCycleBasedOnProportionalControl(float currentTemperature)
{
    static PID pid;
    static float temperatureTarget = 0.0f;
    
    float previousTemperatureTarget = temperatureTarget;

    temperatureTarget = currentScript.currentTemperatureTarget();
    if (ISNAN(temperatureTarget)) {
        writeFailure("Temperature setpoint is NAN. Are thermocouples connected?");
        return 0.0f;
    }
    
    if (previousTemperatureTarget != temperatureTarget) {
        PID newPID(temperatureTarget, P_GAIN, 0, 0, PID_OUTPUT_LIMIT);
        pid = newPID;
    }

    return pid.nextControlOutput(currentTemperature, 0);
}

void readTemperaturesFromThermocouples(float& temperature1, float& temperature2)
{
    temperature1 = thermocouple1.readCelsius();
    temperature2 = thermocouple2.readCelsius();
    if (ISNAN(temperature1))
        return writeFailure("Thermocouple 1 reading is NAN.");
    else if (ISNAN(temperature2))
        return writeFailure("Thermocouple 2 reading is NAN.");
}

uint32_t step(float temperature1, float temperature2)
{
    static uint32_t lastTime       = millis();
    uint32_t currentTime           = millis(),
             elapsedMillis         = currentTime - lastTime,
             heaterDutyCycleMillis = 0;


    currentScript.step(elapsedMillis, temperature1, temperature2);

    if (currentScript.completed())
        return 0;

    currentTime = millis(); // ignore time taken for script to step

    heaterDutyCycleMillis = dutyCycleBasedOnProportionalControl(temperature1);

    runPumpBasedOnBobberPosition(elapsedMillis);
    runHeaterForDutyCycleMillis(heaterDutyCycleMillis);

    outputStatus(temperature1, temperature2, heaterDutyCycleMillis);

    lastTime = currentTime;
    return millis() - lastTime;
}

int main()
{
    setup();

    uint32_t counter = 0;

    float temperature1 = 0.0f,
          temperature2 = 0.0f;

    readTemperaturesFromThermocouples(temperature1, temperature2);

    currentScript.readFromSerial();

    if (ISNAN(temperature1))
        writeFailure("Thermocouple 1 reading is NAN.");
    if (ISNAN(temperature2))
        writeFailure("Thermocouple 2 reading is NAN.");


    while (true) {
        if (!currentScript.completed()) {
            readTemperaturesFromThermocouples(temperature1, temperature2);

            // let stir plate run
            analogWrite(STIR_PLATE, STIR_PLATE_PWM);
            counter = 0;
            while (counter < STIR_PLATE_START_MS - HEATER_PERIOD_MS &&
                   !currentScript.completed()) {
                counter += step(temperature1, temperature2);
            }

            _delay_ms(STIR_PLATE_START_MS - counter);

            // wait for stir plate to stop
            analogWrite(STIR_PLATE, 0);
            counter = 0;
            while (counter < STIR_PLATE_STOP_MS - HEATER_PERIOD_MS &&
                   !currentScript.completed()) {
                counter += step(temperature1, temperature2);
            }
            
            _delay_ms(STIR_PLATE_STOP_MS - counter);
        }
    }
}

