/*
 * sound-meter.ino: Arduino sound meter.
 *
 * This program continuously samples a sound ExitationOnal on analog input 0
 * and outputs the computed sound intensity through the serial port.
 *
 * The analog-to-digital converter is set to "free running mode" and
 * takes one sample every 104 us. After subtracting a DC offset
 * (constant dc_offset below), the samples are squared and low-pass
 * filtered with a time constant of 256 sample periods (26.6 ms). They
 * are then decimated by a factor 256 and transmitted as an ASCII stream
 * at 9600/8N1 through the serial port.
 *
 * The program is intended for an Arduino Uno, and is likely to work on
 * any AVR-based Arduino having an ADC and clocked at 16 MHz.
 *
 * Copyright (c) 2016 Edgar Bonet Orozco.
 * Released under the terms of the MIT license:
 * https://opensource.org/licenses/MIT
 */

#include <Arduino.h>



// Analog input connected to the microphone (between 0 and 5).
const uint8_t sensorPin = 0;
const uint8_t ExPin = 2; //LED_BUILTIN; // 2;

const uint8_t length = 8; // 2^lenggth
const uint16_t decFact = (int) pow(2, length) - 1;

// Intensity readings provided by the ADC interrupt service routine.
volatile uint32_t intensity_reading;
volatile bool intensity_reading_ready;

// Low-pass filter for the sound intensity.
// Time constant = 256 * sampling period = 26.624 ms
static int32_t low_pass(int32_t x, uint8_t len)
{
    static int32_t integral;
/*- Basic 256 point moving average
    integral = integral + currentVal - integral / 256;
    return integral / 256;
    Speeding up by avoiding division and compiler dependent multiplication optimisation 
    therefor we use the facts that X * 256 = (x << 8) and X / 256 = (x >> 8)
    
    scaling x by 256 (x << 8) to take advantage of the increased resolution
    without going floating point
*/

    // integral += (x << len) - (integral >> len);
    integral += (x << len) - (integral >> len);
    return (integral >> len);
}

// ADC interrupt service routine.
// Called each time a conversion result is available.
ISR(ADC_vect)
{
    // Read the ADC.
    int32_t sample = ADC;
    static uint32_t sample_count;
    static bool ExitationOn;
    // Toggle exitation and sample sign as fast as possible inside our sensor bandwith
    // Default 3dB bandwith for the opt101 is 14kHz so we need to be a bit slower than that
    if (!ExitationOn) { // If exitation was of for this sample, invert it
         sample = -sample;
    }

    // set up our exitation for the next number of samples
    if ((sample_count & 0b111) == 6) { // if 78.6 kS/s samplerate, divide by 6 gives 12.8kHz on the exitation pin
        if (!ExitationOn) {
            PORTD = PORTD | 0b00000100; // set exitation pin D2 high
            ExitationOn = true; 
        } else {
            PORTD = PORTD & 0b11111011; // set exitation pin D2 low
            ExitationOn = false;
        }
    }

    
    // int32_t avg_intensity = low_pass(sample, length);
    
    // // Decimate by a same length as filter
    // if ((sample_count & decFact ) == 0) {
    //     intensity_reading = avg_intensity;
    //     intensity_reading_ready = true;
    // }
    static uint32_t avg_intensity;
    
    avg_intensity += sample;
    
    if ((sample_count & 4095 ) == 0) {
        intensity_reading = (avg_intensity >> 6);
        avg_intensity = 0;
        intensity_reading_ready = true;
    }

    sample_count++;
}

void setup()
{
    // Configure the ADC in "free running mode".
    ADMUX  = _BV(REFS0)  // ref = AVCC
           | sensorPin;    // input channel
    ADCSRB = 0;          // free running mode
    ADCSRA = _BV(ADEN)   // enable
           | _BV(ADSC)   // start conversion
           | _BV(ADATE)  // auto trigger enable
           | _BV(ADIF)   // clear interrupt flag
           | _BV(ADIE)   // interrupt enable
           | 4; // prescale 16 //5; // prescaler 32. 7;  // prescaler = 128.

    // Configure the serial port.
    //Serial.begin(250000);
    Serial.begin(115200);
    pinMode(ExPin, OUTPUT);
    digitalWrite(ExPin, HIGH);
    // Serial.print("Max: ");
    // Serial.println(decFact);
    //Serial.println("Min: 6000");


}

void loop()
{
    // Wait for a reading to be available.
    while (!intensity_reading_ready) /* wait */;

    // Get a copy.
    noInterrupts();
    int32_t intensity_reading_copy = intensity_reading;
    intensity_reading_ready = false;
    interrupts();

    // Transmit to the host computer.
    //Serial.print("Test: ");
    Serial.println(intensity_reading_copy);
}