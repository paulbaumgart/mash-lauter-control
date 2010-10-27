#include "PID.h"

PID::PID (float setpoint, float pGain, float iGain, float dGain, float outputLimit) {
    this->setpoint = setpoint;
    this->pGain = pGain;
    this->iGain = iGain;
    this->dGain = dGain;
    this->outputLimit = outputLimit;

    this->integral = this->previousError = 0;
}

float PID::nextControlOutput (float inputValue, float timeElapsed) {
    float error = setpoint - inputValue,
          proportional = error,
          derivative = (error - previousError) / timeElapsed,
          output;

    // "Usually you can just set the integrator minimum and maximum so that the
    // integrator output matches the drive minimum and maximum."
    // (http://igor.chudov.com/manuals/Servo-Tuning/PID-without-a-PhD.pdf)
    integral += error * timeElapsed;
    if (integral > outputLimit)
        integral = outputLimit;
    else if (integral < 0)
        integral = 0;

    previousError = error;

    output = pGain * proportional + iGain * integral + dGain * derivative;
    if (output > outputLimit)
        output = outputLimit;
    else if (output < 0)
        output = 0;

    return output;
}

