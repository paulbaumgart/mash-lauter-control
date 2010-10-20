#ifndef _PID_H_
#define _PID_H_

typedef struct PID {
    double setpoint;

    double previous_error,
           integral,
           derivative;

    double p_weight,
           i_weight,
           d_weight;
} PID;

#endif

