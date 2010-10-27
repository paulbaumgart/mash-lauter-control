#ifndef _MLCScript_H_
#define _MLCScript_H_

#include <WProgram.h>

#define NUM_SETPOINTS 10

class MLCScript {
	public:
		MLCScript();
        bool completed(void);
        void reset(void);
		void addSetpoint(float temperature, uint32_t durationMillis);
		void step(uint32_t elapsedMillis);
		float currentTemperatureSetpoint(void);

	private:
		uint32_t counter,
				 setpointDurations[NUM_SETPOINTS];
		float setpoints[NUM_SETPOINTS];
		uint8_t numSetpoints;
		int8_t activeSetpointIndex,
			   maxIndex;
};

#endif

