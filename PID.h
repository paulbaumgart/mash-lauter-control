#ifndef _PID_H_
#define _PID_H_

#include <WProgram.h>

class PID {
    public:
        PID();
        PID(float setpoint, float pGain, float iGain, float dGain, float outputLimit);
        float nextControlOutput(float inputValue, uint32_t elapsedMillis);

    private:
        float setpoint,
              previousError,
              integral,
              pGain,
              iGain,
              dGain,
              outputLimit;
};

#endif

