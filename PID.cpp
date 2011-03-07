#include "PID.h"

PID::PID() {
    this->setpoint = this->pGain = this->iGain = this->dGain
        = this->outputLimit = this->integral = this->previousError = 0;
}

PID::PID(float setpoint, float pGain, float iGain, float dGain, float outputLimit) {
    this->setpoint = setpoint;
    this->pGain = pGain;
    this->iGain = iGain;
    this->dGain = dGain;
    this->outputLimit = outputLimit;

    this->integral = this->previousError = 0;
}

float PID::nextControlOutput(float inputValue, uint32_t elapsedMillis) {
    float error        = setpoint - inputValue,
          elapsedSecs  = elapsedMillis / 1000.0f,
          proportional = error,
          derivative, 
          output;

    if (elapsedSecs > 0)
        derivative = (error - previousError) / elapsedSecs;
    else
        derivative = 0;

    // "Usually you can just set the integrator minimum and maximum so that the
    // integrator output matches the drive minimum and maximum."
    // (http://igor.chudov.com/manuals/Servo-Tuning/PID-without-a-PhD.pdf)
    integral += error * elapsedSecs;
    if (integral * iGain > outputLimit)
        integral = outputLimit / iGain;
    else if (integral < 0)
        integral = 0;

    previousError = error;

	if (inputValue >= setpoint)
		output = 0;
	else
		output = pGain * proportional + iGain * integral + dGain * derivative;

    if (output > outputLimit)
        output = outputLimit;
    else if (output < 0)
        output = 0;

    return output;
}

