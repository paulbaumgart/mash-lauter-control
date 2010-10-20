// this library is public domain. enjoy!
// www.ladyada.net/learn/sensors/thermocouple

#include <avr/pgmspace.h>
#include <WProgram.h>
#include <util/delay.h>
#include <stdlib.h>

#include "MAX6675.h"

MAX6675::MAX6675(uint8_t SCLK, uint8_t CS, uint8_t MISO) {
    sclk = SCLK;
    cs = CS;
    miso = MISO;

    //define pin modes
    pinMode(cs, OUTPUT);
    pinMode(sclk, OUTPUT); 
    pinMode(miso, INPUT);

    digitalWrite(cs, HIGH);
}

float MAX6675::readCelsius(void) {

    uint16_t v;

    digitalWrite(cs, LOW);
    _delay_ms(1);

    v = spiread();
    v <<= 8;
    v |= spiread();

    if (v & 0x4) {
        // uh oh, no thermocouple attached!
        return NAN; 
    }

    v >>= 3;

    digitalWrite(cs, HIGH);

    return v * 0.25f;
}

byte MAX6675::spiread(void) { 
    int i;
    byte d = 0;

    for (i = 7; i >= 0; --i) {
        digitalWrite(sclk, LOW);
        _delay_ms(1);

        if (digitalRead(miso))
            d |= (1 << i);

        digitalWrite(sclk, HIGH);
        _delay_ms(1);
    }

    return d;
}

