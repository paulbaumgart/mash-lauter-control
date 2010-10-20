#include <WProgram.h>
#include "MAX6675.h"
#include "PID.h"

// pin aliases
#define THERMO_CS1  2
#define THERMO_DO1  3
#define THERMO_CLK1 4
#define THERMO_CS2  5
#define THERMO_DO2  6
#define THERMO_CLK2 7

#define BOBBER1     8
#define BOBBER2     9

#define RELAY_240V  10
#define RELAY_120V  11

typedef enum ProgramState {
    kProgramState_WaitingForInput = 0,
    kProgramState_Mashing,
    kProgramState_Sparging,
    kNumProgramStates
} ProgramState;

static PID pid;
static ProgramState currentState = kProgramState_WaitingForInput;

static MAX6675 thermocouple1(THERMO_CLK1, THERMO_CS1, THERMO_DO1);
static MAX6675 thermocouple2(THERMO_CLK2, THERMO_CS2, THERMO_DO2);

static int paused = false;

void setup() {
    // arduino function that sets up the arduino-specific things
    init();

    // set USB communication to 9600 b/s
	Serial.begin(9600);

    // set pin modes for the off-board I/O (except thermocouples)
    pinMode(BOBBER1, INPUT);
    pinMode(BOBBER2, INPUT);
    pinMode(RELAY_240V, OUTPUT);
    pinMode(RELAY_120V, OUTPUT);

    // clear out the pid struct
    memset(&pid, 0, sizeof(PID));

    // apparently the thermocouples need a while to stabilize
    delay(500);
}


int main() {
    setup();

    double lastTime = millis(),
           currentTime = 0.0,
           elapsedTime = 0.0;

    while (1) {
        currentTime = millis();
        elapsedTime = lastTime - currentTime;

        //handle_input();

        if (!paused) {
            switch (currentState) {
                case kProgramState_WaitingForInput:
                    break;
                case kProgramState_Mashing:
                    break;
                case kProgramState_Sparging:
                    break;
                default: // make the compiler happy
                    break;
            }
        }

        lastTime = currentTime;        
    }
}

