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

// Analog input connected to the photo sensor
const uint8_t sensorPin = 0;
const uint8_t ExPin = 2; // Using D2 pin for Laser exitation output

// ADC resolition ioncrease by oversampling an decimation
const uint8_t ADCincreasedRes = 6;                                  // adding 6 extra bits ADC resolution
const uint16_t decimationFactor = (int)pow(4, ADCincreasedRes) - 1; // decimate oversampled data by 4^increasedRes

// Light readings provided by the ADC interrupt service routine.
volatile int32_t lightReading;              // Our detectir reading result goes here
volatile int32_t backGroundLvlReading;      // Our ambient ligth measurement result goes here
volatile bool ligthReadingReady;            

// ADC interrupt service routine.
// Called each time a conversion result is available.
ISR(ADC_vect)
{
    // Read a sample from the ADC.
    int32_t sample = ADC;
    static uint16_t sample_count;
    static uint32_t backgroundLvlAcc;

    static bool ExitationOn;
    
    // Mixer Function:
    // Toggle exitation and sample sign as fast as possible inside our sensor bandwith
    // Default 3dB bandwith for the opt101 is 14kHz so we need to be a bit slower than that
    // This is a simple square wave mixer, I supose we could experiment with a sinewave LUT here
    // but then how can we avoid floating point to make it fast enough?
    if (!ExitationOn)
    { // If exitation was off when the current sample was aquired, invert the sample sign
        backgroundLvlAcc += sample;   // lets also get a measurment of the ambient ligth level by averaging only the samples when the exitation is off
        sample = -sample;             // invert sample sign.
    }

    // Set up our exitation state for the next 6 samples
    // because for 78.6 kS/s samplerate, division by 6 gives 12.8kHz on the exitation pin,
    // so shold be well within the bandwith of the OPT101 sensor.
    if ((sample_count & 0b111) == 6) // toggle exitation every 6th sample This wil actually be divide by 7
    {
        if (!ExitationOn)
        {
            PORTD = PORTD | 0b00000100; // set exitation pin D2 high
            ExitationOn = true;
        }
        else
        {
            PORTD = PORTD & 0b11111011; // set exitation pin D2 low
            ExitationOn = false;
        }
    }

    static int32_t ADCsampleAccumulator;
    ADCsampleAccumulator += sample; // sum up our samples for one decimation lenght

    if ((sample_count & decimationFactor) == 0) // This decimation length mask wil inly work as intended if it is power of 2, need to consider this
    { // everytime sample counter reaches decimationFactor number of samples
        if (ADCsampleAccumulator > 0)
        { // Accumulator value is posiotve
            lightReading = (int32_t)(((uint32_t)ADCsampleAccumulator) >> ADCincreasedRes);
        }
        else
        { // Accumulator value is negative, flip sign and cast acc to unsigned before right shift
            lightReading = -(int32_t)(((uint32_t)-ADCsampleAccumulator) >> ADCincreasedRes);
        }
        ADCsampleAccumulator = sample; // reset sample accumulator to previous sample;

        // get ambient light level
        backGroundLvlReading = (backgroundLvlAcc >> (ADCincreasedRes - 1)); // the ambient level measurement is only haf the number of samples
        backgroundLvlAcc = 0;                                               // reset ambient Acc to 0 //(backGroundLvlReading >> ADCincreasedRes);
        ligthReadingReady = true;
    }

    sample_count++;
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
}

void loop()
{
    // Wait for interupt action to hsppen and saying a reading a reading is available.
    while (!ligthReadingReady) /* wait */
        ;

    // Get a copy of the detecor result
    noInterrupts();
    int32_t lighthReading_copy = lightReading;
    int32_t backGroundLvlReading_copy = backGroundLvlReading;
    ligthReadingReady = false;
    interrupts();

    // Transmit data.
    Serial.print(lighthReading_copy);           // Transmit detection level 
    Serial.print(",");                  
    Serial.println(backGroundLvlReading_copy);  // Transmit ambient level
}
