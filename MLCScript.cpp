#include "MLCScript.h"
#include "Util.h"
#define ASSUMED_HLT_VOLUME_ML 32000.0f
#define ASSUMED_DEGREES_HEAT_LOSS 0.5f
#define MAX_TEMPERATURE 79.0f

static void readNextNullTerminatedString(char* outBuffer, uint8_t length)
{
    while (Serial.available() < length - 1) {}

    uint8_t i;

    for (i = 0; i < length - 1; i++)
        outBuffer[i] = Serial.read();

    outBuffer[i] = '\0';
}

static void readNextFourBytes(char* outBuffer) {
    while (Serial.available() < 4) {}

    for (uint8_t i = 0; i < 4; i++)
        outBuffer[i] = Serial.read();
}

static void writeSuccess(void) {
    Serial.println("OK");
}

MLCScript::MLCScript(void (*pauseCallback)())
    : m_pauseCallback (pauseCallback)
{
    reset();
}

void MLCScript::writeUnknownCommand(char* command) {
    char msg[21] = "Unknown command: XXX";
    msg[17] = command[0];
    msg[18] = command[1];
    msg[19] = command[2];
    return writeFailure(msg);
}

void MLCScript::writeFailure(char* msg) {
    Serial.print("ERROR: ");
    Serial.println(msg);
    Serial.flush();
    m_numStatements = 0;
    m_completed = true;
}

void MLCScript::reset() {
	m_counter = m_numStatements =
        m_activeStatementIndex = m_mashWaterVolume = 0;
    m_completed = false;
    m_hltTemperature = m_grainTemperature = NAN;
}

void MLCScript::step(uint32_t elapsedMillis, float hltTemperature, float grainTemperature)
{
    if (m_activeStatementIndex >= m_numStatements) {
        m_completed = true;
        return;
    }

    ScriptStatement* currentStatement = &m_statements[m_activeStatementIndex];
    uint32_t command = currentStatement->command.uint;

    m_hltTemperature = hltTemperature;
    m_grainTemperature = grainTemperature;
    
    if (command == STRUINT("INI") ||
        command == STRUINT("MSH") ||
        command == STRUINT("SPG")) {

        if (command == STRUINT("INI"))
            m_mode = INITIALIZE;
        else if (command == STRUINT("MSH"))
            m_mode = MASHING;
        else if (command == STRUINT("SPG"))
            m_mode = SPARGING;

        m_activeStatementIndex++;
        step(elapsedMillis, hltTemperature, grainTemperature);
    }
    else if (command == STRUINT("PAU")) {
        m_pauseCallback();
        m_activeStatementIndex++;
        step(0, hltTemperature, grainTemperature);
    }
    else if (command == STRUINT("MWV")) {
        m_mashWaterVolume = currentStatement->f1.volume;
        m_activeStatementIndex++;
        step(elapsedMillis, hltTemperature, grainTemperature);
    }
    else if (command == STRUINT("HEA")) {
        float currentTemperature = (m_mode == MASHING) ?
                                       grainTemperature
                                     : hltTemperature;
        if (currentTemperature >= currentTemperatureTarget()) {
            m_activeStatementIndex++;
            step(0, hltTemperature, grainTemperature);
        }
    }
    else if (command == STRUINT("HLD")) {
        m_counter += elapsedMillis;

        if (m_counter >= currentStatement->f2.time) {
            m_counter = 0;
            m_activeStatementIndex++;
            step(0, hltTemperature, grainTemperature);
        }
    }
    else
        return writeUnknownCommand(currentStatement->command.str);
}

bool MLCScript::completed()
{
    return m_completed;
}

Mode MLCScript::mode()
{
    return m_mode;
}

void MLCScript::readFromSerial()
{
    reset();
    ScriptStatement currentStatement;
    for (uint8_t i = 0; i < NUM_STATEMENTS; i++) {
        readNextNullTerminatedString(currentStatement.command.str, 4);
        uint32_t command = currentStatement.command.uint;
        if (command == STRUINT("INI") ||
            command == STRUINT("MSH") ||
            command == STRUINT("SPG") ||
            command == STRUINT("PAU")) {
            currentStatement.f1.temperature = NAN;
            writeSuccess();
        }
        else if (command == STRUINT("MWV") ||
            command == STRUINT("HEA")) {
            writeSuccess();
            readNextFourBytes((char*)&currentStatement.f1);
            writeSuccess();
        }
        else if (command == STRUINT("HLD")) {
            writeSuccess();
            readNextFourBytes((char*)&currentStatement.f1);
            writeSuccess();
            readNextFourBytes((char*)&currentStatement.f2);
            writeSuccess();
        }
        else if (command == STRUINT("END"))
            return writeSuccess();
        else
            return writeUnknownCommand(currentStatement.command.str);

        m_statements[m_numStatements] = currentStatement;
        m_numStatements++;
    }

    readNextNullTerminatedString(currentStatement.command.str, 4);
    if (currentStatement.command.uint != STRUINT("END"))
        return writeFailure((char*)"Too many statements in script.");
    else
        return writeSuccess();
}

float MLCScript::currentTemperatureSetpoint(void)
{
    return m_statements[m_activeStatementIndex].f1.temperature;
}

float MLCScript::currentTemperatureTarget(void)
{
    float setpoint = currentTemperatureSetpoint(),
          offset = 0.0f;

    if (m_mode == MASHING) {
        offset = m_mashWaterVolume / ASSUMED_HLT_VOLUME_ML
                    * (setpoint - m_grainTemperature)
                    + ASSUMED_DEGREES_HEAT_LOSS;
    }

    if (setpoint + offset > MAX_TEMPERATURE)
        return MAX_TEMPERATURE;
    else
        return setpoint + offset;
}

uint32_t MLCScript::timeInCurrentInterval(void)
{
    return m_counter;
}

uint32_t MLCScript::currentIntervalDuration(void)
{
    ScriptStatement* currentStatement = &m_statements[m_activeStatementIndex];

    if (currentStatement->command.uint == STRUINT("HLD"))
        return currentStatement->f2.time;
    else
        return 0;
}

void MLCScript::currentCommand(char* outBuffer) {
    char defaultCommand[] = "NON";
    char* command;
    if (m_numStatements > 0)
        command = m_statements[m_activeStatementIndex].command.str;
    else
        command = defaultCommand;

    for (int i = 0; i < 4; i++)
        outBuffer[i] = command[i];
}

