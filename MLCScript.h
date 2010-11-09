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
		void setMashWaterVolume(uint32_t mashWaterMilliLiters);
		void setMashWaterTemperature(float mashWaterCelsius);
		bool inInitialRampUp(void);
		void step(uint32_t elapsedMillis, float currentTemperature);
		float currentTemperatureSetpoint(void);
        uint32_t timeInCurrentInterval(void);
        uint32_t currentIntervalDuration(void);

	private:
		uint32_t counter,
				 setpointDurations[NUM_SETPOINTS],
				 mashWaterVolume;
		float setpoints[NUM_SETPOINTS],
			  initialRampUpTemperature,
			  mashWaterTemperature;
		uint8_t numSetpoints;
		int8_t activeSetpointIndex,
			   maxIndex;
		bool initialRampUpCompleted;
};

#endif

