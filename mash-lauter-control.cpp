#include <WProgram.h>
#include "MAX6675.h"
#include "PID.h"
#include "MLCScript.h"

// pin aliases
#define THERMO_CS1  2
#define THERMO_DO1  3
#define THERMO_CLK1 4
#define THERMO_CS2  5
#define THERMO_DO2  6
#define THERMO_CLK2 7

#define BOBBER_MASHING  8
#define BOBBER_SPARGING 9

#define RELAY_240V 10
#define RELAY_120V 11

// misc constants
#define MAX_TEMPERATURE 80.0f
#define MIN_TEMPERATURE 30.0f
#define MAX_MASHING_TEMPERATURE 74.0f

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
}

void readNextFourBytes(InputBuffer *inputBuf) {
    uint8_t i;

    // this should probably time out or something
    while (Serial.available() < sizeof(*inputBuf));

    for (i = 0; i < sizeof(*inputBuf); ++i)
        inputBuf->stringVal[i] = Serial.read();
}

void writeSuccess(void) {
    Serial.print("OK!");
}

void writeFailure(void) {
    currentState = kProgramState_WaitingForInput;
    Serial.print("ERR");
}

void readMLCScript(void) {
    InputBuffer inputData;

    // ok, let's make sure we're on the same page here...
    readNextFourBytes(&inputData);
    if (strcmp (inputData.stringVal, "BEG") == 0)
        writeSuccess();
    else
        return writeFailure();

    // mode: mashing or sparging?
    readNextFourBytes(&inputData);
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

    // number of (temperature, duration) pairs to expect
    readNextFourBytes(&inputData);
    uint32_t scriptLength = inputData.intVal;

    for (uint8_t i = 0; i < scriptLength; ++i) {
        readNextFourBytes(&inputData);
        float temperature = inputData.floatVal;
        if (temperature <= MAX_TEMPERATURE && temperature >= MIN_TEMPERATURE)
            writeSuccess();
        else
            return writeFailure();

        readNextFourBytes(&inputData);
        uint32_t durationMillis = inputData.intVal;
        // not really any bounds checking we can do on durationMillis...
        writeSuccess();

        currentScript.addSetpoint(temperature, durationMillis);
    }

    // make sure everything is synced up still 
    readNextFourBytes(&inputData);
    if (strcmp (inputData.stringVal, "END") == 0)
        writeSuccess();
    else
        return writeFailure();
}

int main() {
    setup();

    double lastTime = millis(),
           currentTime = 0.0,
           elapsedTime = 0.0;

    while (1) {
        currentTime = millis();
        elapsedTime = lastTime - currentTime;

        switch (currentState) {
            case kProgramState_WaitingForInput:
                readMLCScript();
                break;
            case kProgramState_Mashing:
                // Thermocouples:
                //   The thermocouple in the grain bed
                //   (thermocouple2) is used as the input for PID control,
                //   and the thermocouple in the HLT (thermocouple1) is just
                //   used to ensure the wort does not exceed the
                //   MAX_MASHING_TEMPERATURE.
                // Bobbers:
                //   The bobber in the grant (BOBBER_MASHING) turns the
                //   pump on or off, with some hysteresis.

                break;
            case kProgramState_Sparging:
                // Thermocouples:
                //   The thermocouple in the HLT (thermocouple1) is the only
                //   one relevant for the sparging process. It is used as the
                //   input for PID control.
                // Bobbers:
                //   The bobber in the mash tun (BOBBER_SPARGING) turns the
                //   pump on or off, with some hysteresis.

                break;
            default: // make the compiler happy
                break;
        }

        lastTime = currentTime;        
    }
}

