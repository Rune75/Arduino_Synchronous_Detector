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

const uint8_t dbgPins = 1;  // 
const uint8_t dbgPlot = 0;  // 


// Analog input connected to the photo sensor
const uint8_t sensorPin = 0;
const uint8_t ExPin = 2; // Using D2 pin for Laser exitation output

// Indicator pins for debugging
const uint8_t sampleIndicator = 3; // Indicator pin for sample rate
const uint8_t serialIndicator = 4; // Indicator pin for serial write
const uint8_t ADCisrIndicator = 5; // Indicator pin for ISR(ADC_vect) duration


// ADC resolition ioncrease by oversampling an decimation
const uint8_t ADCincreasedRes = 6;                                  // adding 6 extra bits ADC resolution
const uint16_t decimationFactor = (int)pow(4, ADCincreasedRes); // decimate oversampled data by 4^increasedRes
const uint8_t exPulseLen = 16;

// Light readings provided by the ADC interrupt service routine.
volatile bool ligthReadingReady;    

struct detectorRes {
    uint32_t exitationLvl;
    int32_t backgroundLvl;
};

volatile detectorRes sampleSum;

// ADC interrupt service routine.
// Called each time a conversion result is available.
ISR(ADC_vect)
{
    //PORTD ^= _BV(ADCisrIndicator);
    PORTD = PORTD | _BV(ADCisrIndicator);


    // Read a sample from the ADC.
    int32_t sample = ADC;
    static uint16_t decimationCounter;
    static uint8_t exitationCounter;
    static int32_t backgroundLvlAcc;

    static bool ExitationOn;
    static int32_t exitationLvlAcc;
    decimationCounter++;
    exitationCounter++;
    
    // Mixer Function:
    // Toggle exitation and sample sign as fast as possible inside our sensor bandwith
    // Default 3dB bandwith for the opt101 is 14kHz so we need to be a bit slower than that.
    // This is a simple square wave mixer, I supose we could experiment with a sinewave LUT here
    // but then how can we avoid floating point to make it fast enough?
    if (!ExitationOn)
    { // If exitation was off when the current sample was aquired, invert the sample sign
        //sample = -sample;             // invert sample sign. this migth need some delay to aligne with the delayed responce from the sensor        
        backgroundLvlAcc += sample;   // lets also get a measurment of the ambient ligth level by averaging only the samples when the exitation is off
    } else {
        exitationLvlAcc += sample; // sum up our samples for one decimation lenght
    }


    if (decimationCounter == decimationFactor) 
    { // everytime sample counter reaches decimationFactor number of samples
        PORTD = PORTD | _BV(sampleIndicator);
        sampleSum.exitationLvl = exitationLvlAcc;
        sampleSum.backgroundLvl = backgroundLvlAcc;
        backgroundLvlAcc = 0;           // reset ambient Acc to 0 //(backGroundLvlReading >> ADCincreasedRes);
        decimationCounter = 0;          // Reset decimation counter
        exitationCounter = exPulseLen;  // Reset exitation counter 

        exitationLvlAcc = 0; //sample; // reset sample accumulator to previous sample;
        ligthReadingReady = true;
    }

    // Set up our exitation state for the next 6 samples
    // because for 76.9 kS/s samplerate, division by 12 gives 6.41kHz on the exitation pin,
    // so shold be well within the bandwith of the OPT101 sensor.
    if (exitationCounter == exPulseLen)     // toggle exitation every exPulseLen sample
    {
        if (!ExitationOn)
        {
            PORTD = PORTD | 0b00000100; // Set exitation pin D2 high
            ExitationOn = true;
        }
        else
        {
            PORTD = PORTD & 0b11111011; // Set exitation pin D2 low
            ExitationOn = false;
        }
        exitationCounter = 0;
    }    
    PORTD ^= _BV(ADCisrIndicator);

}

detectorRes sampleACCdownConvSimple()
{
    detectorRes results;
    results.exitationLvl = (uint32_t)(sampleSum.exitationLvl / pow(2, ADCincreasedRes-1));

    // get ambient light level
    results.backgroundLvl = (int32_t)(sampleSum.backgroundLvl / pow(2, ADCincreasedRes-1)); // the ambient level measurement is only haf the number of samples so we shift one bit less
    return results;
}

detectorRes sampleACCdownConv()
{
    detectorRes results;
    if (sampleSum.exitationLvl > 0)
    {   // Accumulator value is posiotve. cast acc to uint and rigthshift with virtual resolution invrease
        results.exitationLvl = (int32_t)(((uint32_t)sampleSum.exitationLvl) >> ADCincreasedRes);
    }
    else
    { // Accumulator value is negative, flip sign and cast acc to unsigned before right shift
        results.exitationLvl = -(int32_t)(((uint32_t)-sampleSum.exitationLvl) >> ADCincreasedRes);
    }

    // get ambient light level
    results.backgroundLvl = (sampleSum.backgroundLvl >> (ADCincreasedRes - 1)); // the ambient level measurement is only haf the number of samples so we shift one bit less
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

    pinMode(sampleIndicator, OUTPUT);
    digitalWrite(sampleIndicator, HIGH);

    pinMode(serialIndicator, OUTPUT);
    digitalWrite(serialIndicator, HIGH);

    pinMode(ADCisrIndicator, OUTPUT);
    digitalWrite(ADCisrIndicator, HIGH);
    
}

void loop()
{
    // Wait for interupt action to hsppen and saying a reading a reading is available.
    while (!ligthReadingReady) /* wait */
        ;

    // Get a copy of the detecor result
    //noInterrupts();
    //detectorRes detectorReadings = sampleACCdownConv();
    detectorRes detectorReadings = sampleACCdownConvSimple();
    ligthReadingReady = false;
    //interrupts();
    PORTD ^= _BV(sampleIndicator);

    digitalWrite(serialIndicator, HIGH);
    // Transmit data.
    if (dbgPlot) Serial.print(detectorReadings.backgroundLvl);   // Transmit detection level 
    if (dbgPlot) Serial.print(",");                
    if (dbgPlot) Serial.print(detectorReadings.exitationLvl);       // Transmit ambient level
    if (dbgPlot) Serial.print(",");                
    Serial.println((detectorReadings.exitationLvl - detectorReadings.backgroundLvl)/1);   // Transmit detection level 


    digitalWrite(serialIndicator, LOW);

}
