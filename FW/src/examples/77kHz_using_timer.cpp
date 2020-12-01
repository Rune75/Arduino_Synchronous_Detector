#include <Arduino.h>

const byte adcPin = 0;  // A0
const int MAX_RESULTS = 256;
volatile int results [MAX_RESULTS];
volatile int resultNumber;

// ADC complete ISR
ISR (ADC_vect)
  {
  if (resultNumber >= MAX_RESULTS)
    ADCSRA = 0;  // turn off ADC
  else
    results [resultNumber++] = ADC;
  }  // end of ADC_vect
  
EMPTY_INTERRUPT (TIMER1_COMPB_vect);
 
void setup ()
  {
  Serial.begin(115200); // set baudrate
  Serial.println();
  
  // reset Timer 1
  TCCR1A  = 0;
  TCCR1B  = 0;
  TCNT1   = 0;
  TCCR1B  = bit (CS11) | bit (WGM12);  // CTC, prescaler of 8
  TIMSK1  = bit (OCIE1B); 
  OCR1A   = 39;    
  OCR1B   = 39; // 20 uS - sampling frequency 50 kHz

  ADCSRA  =  bit (ADEN) | bit (ADIE) | bit (ADIF); // turn ADC on, want interrupt on completion
  ADCSRA |= bit (ADPS2);  // Prescaler of 16
  ADMUX   = bit (REFS0) | (adcPin & 7);
  ADCSRB  = bit (ADTS0) | bit (ADTS2);  // Timer/Counter1 Compare Match B
  ADCSRA |= bit (ADATE);   // turn on automatic triggering
}

void loop () {
  while (resultNumber < MAX_RESULTS) { }
    
  for (int i = 0; i < MAX_RESULTS; i++)
  {
    Serial.println (results [i]);
  }
  resultNumber = 0; // reset counter
  ADCSRA =  bit (ADEN) | bit (ADIE) | bit (ADIF)| bit (ADPS2) | bit (ADATE); // turn ADC ON
}