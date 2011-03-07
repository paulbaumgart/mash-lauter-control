#ifndef _MLCScript_H_
#define _MLCScript_H_

#include <WProgram.h>

#define NUM_STATEMENTS 32

typedef struct {
    union {
        char str[4];
        uint32_t uint;
    } command;
    union {
        uint32_t volume;
        float temperature;
    } f1;
    union {
        uint32_t time;
    } f2;
} ScriptStatement;

typedef enum Mode {
    INITIALIZE = 0,
    MASHING,
    SPARGING,
    MODE_COUNT
} Mode;

class MLCScript {
public:
    MLCScript(void (*pauseCallback)());
    void reset(void);
    void step(uint32_t elapsedMillis, float hltTemperature, float grainTemperature);
    Mode mode(void);
    void readFromSerial(void);
    float currentTemperatureSetpoint(void);
    float currentTemperatureTarget(void);
    uint32_t timeInCurrentInterval(void);
    uint32_t currentIntervalDuration(void);
    bool completed(void);
    void currentCommand(char* outBuffer);

private:
    void writeFailure(char*);
    void writeUnknownCommand(char*);
    enum Mode m_mode;
    uint32_t m_counter,
             m_mashWaterVolume;
    ScriptStatement m_statements[NUM_STATEMENTS];
    uint8_t m_numStatements;
    int8_t m_activeStatementIndex;
    bool m_completed;
    float m_hltTemperature;
    float m_grainTemperature;
    void (*m_pauseCallback)();
};

#endif

