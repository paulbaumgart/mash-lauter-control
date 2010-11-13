#include "MLCScript.h"
#include "Util.h"
#define ASSUMED_HLT_VOLUME_ML 32000.0f
#define MAX_TEMPERATURE 79.0f

MLCScript::MLCScript() {
    reset();
}

bool MLCScript::completed() {
    return activeSetpointIndex == -1;
}

void MLCScript::reset() {
	counter = numSetpoints = activeSetpointIndex = mashWaterVolume = 0;
	maxIndex = -1;
	initialRampUpTemperature = NAN;
	initialRampUpCompleted = false;
}

void MLCScript::addSetpoint(float temperature, uint32_t durationMillis) {
	maxIndex += 1;

	if (maxIndex < NUM_SETPOINTS) {
		setpoints[maxIndex] = temperature;
		setpointDurations[maxIndex] = durationMillis;
	}
}

void MLCScript::setMashWaterVolume(uint32_t mashWaterMilliLiters) {
	mashWaterVolume = mashWaterMilliLiters;
}

void MLCScript::setMashWaterTemperature(float mashWaterCelsius) {
	mashWaterTemperature = mashWaterCelsius;
}

bool MLCScript::inInitialRampUp(void) {
	return !initialRampUpCompleted && mashWaterVolume > 0 && !ISNAN(initialRampUpTemperature);
}

void MLCScript::step(uint32_t elapsedMillis, float currentTemperature) {
	if (!initialRampUpCompleted && mashWaterVolume > 0) {
        float setpoint = setpoints[activeSetpointIndex];
        initialRampUpTemperature = setpoint +
                                   (mashWaterVolume / ASSUMED_HLT_VOLUME_ML *
                                       (setpoint - mashWaterTemperature));
        if (initialRampUpTemperature > MAX_TEMPERATURE)
            initialRampUpTemperature = MAX_TEMPERATURE;

		if (currentTemperature >= initialRampUpTemperature)
			initialRampUpCompleted = true;
		else
			return;
	}
    else if (maxIndex >= 0 && activeSetpointIndex >= 0) {
		counter += elapsedMillis;

		if (counter > setpointDurations[activeSetpointIndex]) {
			// this duration has ended
			counter = 0;
			activeSetpointIndex += 1;

			if (activeSetpointIndex > maxIndex)
				activeSetpointIndex = -1;
		}
	}
}

float MLCScript::currentTemperatureSetpoint(void) {
	if (!initialRampUpCompleted)
		return initialRampUpTemperature;
	else if (activeSetpointIndex >= 0)
		return setpoints[activeSetpointIndex];
	else
		return NAN;
}

uint32_t MLCScript::timeInCurrentInterval(void) {
    return counter;
}

uint32_t MLCScript::currentIntervalDuration(void) {
    if (activeSetpointIndex >= 0)
        return setpointDurations[activeSetpointIndex];
    else
        return 0;
}

