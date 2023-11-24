/*
 * This code demostrates a low frequency lockin amplifier or synchronous ligth 
 * detector inplementation using only a Arduino board and 2 additional modules:
 *   1. Atmega328p Arduino board w synchronous detector inplementation in code
 *   2. The OPT101 amplified vissible ligth photodiode 
 *   3. A laser pointer 
 * 
 * The syncronous detector components ar all realised in software 
 * using the Atmega328p builtin ADC as frontend.
 * 
 * Basic spec
 * ----------
 * ADC raw samplerate is 78.6kHz with estimated > 8 bit resoluition. 
 *    LSB is a bit unstable becuse of the high samplerate but is giving better relative 
 *    resolution wit this compared to lovering the samplerate and decimation ration
 * ADC resolution is >14 bit after oversampoling and decimation
 * Laser modulation or exitation frequency 12.8kHz, and so also the LO frequency of the sychronous detector.
 * After decimation the output samplerate from the detector is 12.8 / 4^6 = 3.125 kS/s
 * Ewerything is divided down and synchronous to the ADC raw samplerate clock.
 * 
 * Copyright (c) 2020 Rune Bekkevold
 * Released under the terms of the MIT license:
 * https://opensource.org/licenses/MIT
 * 
 */

#include <Arduino.h>

#define DEBUGPRINT 1;
#ifdef DEBUGPRINT
    #define DEBUG_PRINT(x)    Serial.print (x)
    #define DEBUG_PRINTDEC(x) Serial.print (x, DEC)
    #define DEBUG_PRINTLN(x)  Serial.println (x)
    #define DBG_PRINTF(y,x)  Serial.print(y); Serial.println(x);
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTDEC(x)
    #define DEBUG_PRINTLN(x) 
    #define DBG_PRINTF(y,x)
#endif

#define DEBUGPINS 1;
#ifdef DEBUGPINS
    #define DEBUG_PIN_TOGGLE(x)     PORTD ^= _BV(x);    // Toggle pin
    #define DEBUG_PIN_SET(x)        PORTD |= _BV(x);    // Set pin high
    #define DEBUG_PIN_CLR(x)        PORTD &= ~(_BV(x)); // Set pin low
#else
    #define DEBUG_PIN_TOGGLE(x)
    #define DEBUG_PIN_SET(x)
    #define DEBUG_PIN_CLR(x)
#endif


// -- Input and output pins -------------------- 
const uint8_t sensorPin = 0;        // Analog input connected to the photo sensor
const uint8_t ExPin = 2;            // Set D2 pin as Laser exitation output

// -- Debugg output pins ------------------------
const uint8_t sampleIndicator = 3;  // Indicator pin for sample rate
const uint8_t serialIndicator = 4;  // Indicator pin for serial write
const uint8_t ADCisrIndicator = 5;  // Indicator pin for ISR(ADC_vect) duration


// ADC resolition ioncreased by oversampling an decimation
const uint8_t ADCincreasedRes = 6;                                  // For adding 6 extra bits ADC resolution
const uint16_t decimationFactor = (int)pow(4, ADCincreasedRes);     // decimate oversampled data by 4^increasedRes

const uint8_t exPulseLen = 16;      // Exitation pulse length set in number of ADC samples
volatile bool ligthReadingReady;    // Light reading ready indicator, provided by the ADC interrupt service routine.

struct detectorRes {                // Results struct 
    uint32_t exitationLvl;
    int32_t backgroundLvl;
    int32_t differenceLvl;
};
volatile detectorRes sampleSum;     // Results object


//-- ADC interrupt service routine -----------------------------------------
//-- Called each time a conversion result is available.
ISR(ADC_vect)
{
    DEBUG_PIN_SET(ADCisrIndicator); //Debug pin set for indicating that a ADC conversion result is ready, and we entered the interupt routine

    int32_t sample = ADC;    // Copy the sample from the ADC.
    
    // Declare interupt function specific static variables
    static uint16_t decimationCounter;      // Keeping track of Decimation length
    static uint8_t exitationCounter;        // Keeping track of number of passed exitation pulses
    static int32_t backgroundLvlAcc;        // Cumulative sum of Background ligth level readings
    static bool ExitationOn;                // Indicates state of exitation output
    static int32_t exitationLvlAcc;         // Cumulative sum of exited light level

    decimationCounter++;
    exitationCounter++;
    
    if (!ExitationOn) {                 // If Laser exitation is off
        backgroundLvlAcc += sample;     // Acumulate ambient ligth level
    } else {
        exitationLvlAcc += sample;      // else acumulate exited ligth level
    }

    //-- store acumulators when decimationFactor number of samples is reached
    //-- Occurs when we have enough summed samples for one output sample
    if (decimationCounter == decimationFactor) 
    { 
        DEBUG_PIN_SET(sampleIndicator);             // Debug pin set
        sampleSum.exitationLvl = exitationLvlAcc;   // Output auccumulated  exited light level readings
        sampleSum.backgroundLvl = backgroundLvlAcc; // Output auccumulated  background light level readings
        backgroundLvlAcc = 0;                       // reset ambient Acc to 0 
        exitationLvlAcc = 0;                        // reset sample accumulator to previous sample;

        decimationCounter = 0;                      // Reset decimation counter
        exitationCounter = exPulseLen;              // Reset exitation counter, syncronizes exitation with sample reading

        ligthReadingReady = true;                   // Tell the world that accummulators are ready for division
    }

    // Toggle exitation every exPulseLen samples
    if (exitationCounter == exPulseLen) {
        if (!ExitationOn) {
            PORTD = PORTD | 0b00000100; // Set exitation pin D2 high
            ExitationOn = true;
        }
        else {
            PORTD = PORTD & 0b11111011; // Set exitation pin D2 low
            ExitationOn = false;
        }
        exitationCounter = 0;
    }    
    DEBUG_PIN_TOGGLE(ADCisrIndicator);  // Debug pin Toggle to indicate interupt routine has finished
}


//-- 
detectorRes sampleACCdownConvSimple()
{
    detectorRes res;
    res.exitationLvl = (uint32_t)(sampleSum.exitationLvl / pow(2, ADCincreasedRes-1)); // ##- wonder if it should not be -1 here ???????? 

    // get ambient light level
    res.backgroundLvl = (int32_t)(sampleSum.backgroundLvl / pow(2, ADCincreasedRes-1));

    res.differenceLvl = (int32_t) (res.exitationLvl - res.backgroundLvl);
    return res;
}

detectorRes sampleACCdownConv()
{
    detectorRes results;
    if (sampleSum.exitationLvl > 0)
    {   // Accumulator value is positive. cast acc to uint and rigthshift with virtual resolution invrease
        results.exitationLvl = (int32_t)(((uint32_t)sampleSum.exitationLvl) >> ADCincreasedRes);
    }
    else
    { // Accumulator value is negative, flip sign and cast acc to unsigned before right shift
        results.exitationLvl = -(int32_t)(((uint32_t)-sampleSum.exitationLvl) >> ADCincreasedRes);
    }

    // get ambient light level
    results.backgroundLvl = (sampleSum.backgroundLvl >> (ADCincreasedRes - 1)); // the ambient level measurement is only half the number of samples so we shift one bit less
    return results;
}

void setup()
{
    // Configure the ADC in "free running mode".
    ADMUX = _BV(REFS0)    // ref = AVCC
            | sensorPin;  // input channel
    ADCSRB = 0;           // free running mode
    ADCSRA = _BV(ADEN)    // enable
             | _BV(ADSC)  // start conversion
             | _BV(ADATE) // auto trigger enable
             | _BV(ADIF)  // clear interrupt flag
             | _BV(ADIE)  // interrupt enable
             | 4;         //4  prescale 16 //5; // prescaler 32. 7;  // prescaler = 128.

    // Configure the serial port.
    Serial.begin(115200);

    // configure Exitation pin mode and initial state;
    pinMode(ExPin, OUTPUT);
    digitalWrite(ExPin, HIGH);

    // Configure the other pins and initial states
    pinMode(sampleIndicator, OUTPUT);
    digitalWrite(sampleIndicator, HIGH);

    pinMode(serialIndicator, OUTPUT);
    digitalWrite(serialIndicator, HIGH);

    pinMode(ADCisrIndicator, OUTPUT);
    digitalWrite(ADCisrIndicator, HIGH);
}

void loop()
{
    // Wait for interupt routine to finsh, saying a reading a reading is available.
    while (!ligthReadingReady) /* wait for interupt */
        ;
    ligthReadingReady = false;
    
    // Get a copy of the detecor result
    //detectorRes detectorReadings = sampleACCdownConv();
    detectorRes detectorReadings = sampleACCdownConvSimple();
    DEBUG_PIN_TOGGLE(sampleIndicator);

    digitalWrite(serialIndicator, HIGH);

    //- print out results for Teleplot---
    DBG_PRINTF(">BackGround:", detectorReadings.backgroundLvl);     // Print Background level
    DBG_PRINTF(">ExitedLvl:", detectorReadings.exitationLvl);       // Primt Reflected level + ambient
    DBG_PRINTF(">Result:", detectorReadings.differenceLvl);

    digitalWrite(serialIndicator, LOW);

}
