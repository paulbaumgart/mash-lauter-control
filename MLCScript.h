#ifndef _MLCScript_H_
#define _MLCScript_H_

#define NUM_SETPOINTS 10

typedef enum MLCMode {
	kMLCMode_Mashing = 0,
	kMLCMode_Sparging,
	kMLCMode_Count
} MLCMode;

class MLCScript {
	public:
		MLCScript(MLCMode aMode);
		void addSetpoint(float temperature, uint32_t durationMillis);
		void step(uint32_t elapsedMillis);
		float currentTemperatureSetpoint(void);
		MLCMode getMode(void);

	private:
		uint32_t counter,
				 setpointDurations[NUM_SETPOINTS];
		float setpoints[NUM_SETPOINTS];
		uint8_t numSetpoints,
				mode;
		int8_t activeSetpointIndex,
			   maxIndex;
};

#endif
