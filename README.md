# Arduino Synchronous Detector

This code demostrates a low frequency lockin amplifier or synchronous ligth detector inplementation using only a Arduino board and 2 additional modules: 
  1. The OPT101 amplified vissible light photodiode 
  2. A laser pointer 
  
The syncronous detector components are all realised in software 
using the Atmega328p builtin ADC as frontend and one GPIO for driving the laserdiode.

## Basic principle

![Basic principle](specs/lockin_detector-Signals.drawio.png)


## Analog implementation
![Analog Synchronous detector](specs/Analog_detector.png)


## Full microcontroller inplementation with Atmega328p
![Arduino Synchronous detector](specs/Arduino_detector.png)

## Expanded view of actual implementation
By filtering the reflected signal while under exitation and the reflected signal when exitation is off in two separate arrays we are able to also measure and log the backround ligth level.
![Signals_alt](specs/lockin_detector-Signals_alt.drawio.png)


##  Sampling and signal bandwith (work in progress)
ADC raw samplerate with prescaler set to 16 in free running mode is 16 MHz/(16*13) ≈ 77kHz. estimated > 8 bit resoluition. 
(LSB is a bit unstable because of the high samplerate).
The resulution is then increased by 6 bit trough downsampling by 4^6 = 4096 giving us a downsampled samplerate of 77k / 4096 = 18.79Hz
The exitation pulse length is set to 16 samples and 50% dudty cycle so the Exitation pulse rate is 77kHz/(2*16) = ~2404Hz.
The bandwith of the opt101 sensor is 14KHz so 2.4kHz exitation frequency is wel within this band.

Ewerything is divided down from and synchronous to the ADC raw samplerate.
  
  
