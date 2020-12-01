
// macroUNO.h
// This file goes into the "C:\Users\YOURUSER\Documents\Arduino\libraries\macroUNO"  folder.
//
// These Macros can be used with Arduino UNO board
//
// NOTE:
// ***** Macros may have to be redefined for other types of Arduino boards *****
//
// Macro Format:
// #define XXXXX  YYYYY
//   where XXXXX = the macro name you what to define (might want to use capitals)
//   where YYYYY = the code that will be substituted prior to compiling
// #define TOGGLEd2 PORTD ^= B00000100
//         TOGGLEd2 = the macro name
//         PORTD ^= B00000100 = the microcontroller code  
//
// Toggle port pins 0-7
// Port D
//#define TOGGLEd0 PORTD^= B00000001 // Toggle digital pin D0 Shared with USB RX
//#define TOGGLEd1 PORTD^= B00000010 // Toggle digital pin D1 Shared with USB TX
#define TOGGLEd2  PORTD ^= B00000100 // Toggle digital pin D2
#define TOGGLEd3  PORTD ^= B00001000 // Toggle digital pin D3
#define TOGGLEd4  PORTD ^= B00010000 // Toggle digital pin D4  
#define TOGGLEd5  PORTD ^= B00100000 // Toggle digital pin D5  
#define TOGGLEd6  PORTD ^= B01000000 // Toggle digital pin D6  
#define TOGGLEd7  PORTD ^= B10000000 // Toggle digital pin D7  
 
// Toggle port pins 0-5            
// Port B                          
#define TOGGLEd8  PORTB ^= B00000001 // Toggle digital pin D8
#define TOGGLEd9  PORTB ^= B00000010 // Toggle digital pin D9
#define TOGGLEd10 PORTB ^= B00000100 // Toggle digital pin D10
#define TOGGLEd11 PORTB ^= B00001000 // Toggle digital pin D11
#define TOGGLEd12 PORTB ^= B00010000 // Toggle digital pin D12
#define TOGGLEd13 PORTB ^= B00100000 // Toggle digital pin D13
 
// Toggle port pins 0-5            
// Port C                          
#define TOGGLEd14 PORTC ^= B00000001 // Toggle digital pin D14 A0
#define TOGGLEd15 PORTC ^= B00000010 // Toggle digital pin D15 A1
#define TOGGLEd16 PORTC ^= B00000100 // Toggle digital pin D16 A2
#define TOGGLEd17 PORTC ^= B00001000 // Toggle digital pin D17 A3
#define TOGGLEd18 PORTC ^= B00010000 // Toggle digital pin D18 A4
#define TOGGLEd19 PORTC ^= B00100000 // Toggle digital pin D19 A5

//
