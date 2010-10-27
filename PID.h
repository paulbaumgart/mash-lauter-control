#ifndef _PID_H_
#define _PID_H_

#include <WProgram.h>

class PID {
    public:
        PID (float setpoint, float pGain, float iGain, float dGain, float outputLimit); 
        float nextControlOutput (float inputValue, float timeElapsed);

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

