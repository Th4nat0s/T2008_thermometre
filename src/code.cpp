#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <Arduino.h>
#include <TinyWireM.h>
#include <Tiny4kOLED.h>
#include <TinyBME280.h>

uint16_t nextRowOfTextToDraw;
int hourCounter = 0;

#define numCycles 1   // numCycles = the number of 4 second timeouts between readings of the light value. 900 = 1 hour. 1 = 4 seconds for the impatient. :)
#define adc_disable() (ADCSRA &= ~(1 << ADEN)) // Disable ADC to save a lot of current consumption.
                                               // ADCSRA = ADC Control and Status Register A  , le bit 7 est ADEN: ADC Enable, 0 ca coupe l'adc.

int val = 0;         // variable to store the read value
int cycleCounter = 0;                   // As we take 1 light reading per hour but the maximum watchdog timer period is 8 seconds we need a counter to track the number of times the timer has timed out. 
int period = 9;                        // This = 4 second timeout. Refer to table above

void dodo(){
  // met le bme en sleep
  TinyWireM.beginTransmission(118);
  TinyWireM.write(0xf4);
  TinyWireM.write(0b00100100);
  TinyWireM.endTransmission();
  }

void debout() {
  // reveille le bme
  TinyWireM.beginTransmission(118);
  TinyWireM.write(0xf4);
  TinyWireM.write(0b00100111);
  TinyWireM.endTransmission();
}

void setup_watchdog(int ii)
{
  // 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
  // 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec

  uint8_t bb;
  // if (ii > 9 ) ii=9;
  bb=ii & 7;
  // if (ii > 7) bb|= (1<<5);
  bb|= (1<<WDCE);

  MCUSR &= ~(1<<WDRF);
  // start timed sequence
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  // set new watchdog timeout value
  WDTCSR = bb;
  WDTCSR |= _BV(WDIE);
}


void setup() {
  oled.begin();
  BME280setup();
  adc_disable();                       // ADC uses ~320uA so disable it until we need it.
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // This is the lowest power consumption mode.
  setup_watchdog(period);              // Set watchdog to wake system each 4 seconds.

  // Two fonts are supplied with this library, FONT8X16 and FONT6X8
  // Other fonts are available from the TinyOLED-Fonts library
  oled.setFont(FONT6X8);
  // If the text to scroll needs to start on-screen, it can be initialized,
  // clipped to the scrolling window
  // Start drawing the text from the beginning pixel, clip at 118 pixels.
  oled.clear();
  oled.on();
  oled.switchRenderFrame();

}

// system wakes up when watchdog is timed out

void systemSleep()
{
  sleep_enable();   // active le bit "sleep enable" seelctionné par set_sleep_mode
  sei();            // active les interuption
  sleep_cpu();      // Entre dans le mode économie.
  sleep_disable();  //when watchdog times out, desactive le bit sleep enable
}

void loop() {
 if (cycleCounter++ == numCycles)     // Is it time to read LDR value?
    {
    cycleCounter = 0;                  // Reset the count of 4 second periods.
    hourCounter++;
    if (hourCounter > 23) hourCounter = 0; // Reset the hour counter each 24 hours.
    }

  debout();
  int temperature = BME280temperature();
  unsigned int pressure = BME280pressure();
  float tempf = float(temperature)/100;
  dodo();
  String temp = "Temp: " + String(tempf) + " C";
  String pres = "Pres: " + String(pressure) + " Pa";

  oled.on();
  oled.clear();////
  oled.setCursor(0,0 );  
  oled.print("MyHome Fridge");
  oled.setCursor(0,1 );  
  oled.print(temp);
  oled.setCursor(0,2 );  
  oled.print(pres);
  // oled.setCursor(0,3);  
  oled.switchFrame();
  delay(3000);
  //  oled.off();

 systemSleep() ;
  }

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect)
{
 // nothing here
}
