// this library is public domain. enjoy!
// www.ladyada.net/learn/sensors/thermocouple

#ifndef _MAX6675_H_
#define _MAX6675_H_

class MAX6675 {
    public:
        MAX6675(uint8_t SCLK, uint8_t CS, uint8_t MISO);

        double readCelsius(void);
        double readFarenheit(void);
    private:
        uint8_t sclk, miso, cs;
        uint8_t spiread(void);
};

#endif

