#include "MLCScript.h"

MLCScript::MLCScript(MLCMode aMode) {
	mode = aMode;	
	counter = numSetpoints = activeSetpointIndex = 0;
	maxIndex = -1;
}

MLCMode MLCScript::getMode() {
	return mode;
}

void MLCScript::addSetpoint(float temperature, uint32_t durationMillis) {
	maxIndex += 1;

	if (maxIndex < NUM_SETPOINTS) {
		setpoints[maxIndex] = temperature;
		setpointDurations[maxIndex] = durationMillis;
	}
}

void MLCScript::step(uint32_t elapsedMillis) {
	if (maxIndex >= 0 && activeSetpointIndex >= 0) {
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
	if (activeSetpointIndex >= 0)
		return setpoints[activeSetpointIndex];
	else
		return NAN;
}
