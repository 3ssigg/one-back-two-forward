#include "SCD30.h"

// uv in
const int analogInPin = A0;
int isr_counter = 0;
long int uvi = 0;
int uvi_avg = 0;

// SCD30 readout
float result[3] = {0};

void setup() {
  setupTimerInterrupt();
  Serial.begin(9600);
  scd30.initialize();
}

void loop() {
  int co2 = 0; // integer value of co2 measurement
  int co2_msb = 0; // "more significant byte" 2^8 - 2^15
  int co2_lsb = 0; // "less significant byte" 2^0 - 2^7
  
  int temp = 0;
  int temp_msb = 0;
  int temp_lsb = 0;
  
  int humd = 0;
  int humd_msb = 0;
  int humd_lsb = 0;

  int uvi_avg_msb = 0;
  int uvi_avg_lsb = 0;
  
  if (scd30.isAvailable()) {
    scd30.getCarbonDioxideConcentration(result);
    //result[0] --> co2
    //result[1] --> temp
    //result[2] --> humd
  }
    
  co2 = (int)result[0]; // cast float to int
  if (co2 == 65535) co2 = 65534; 
  co2_msb = co2 >> 8;
  co2_lsb = co2 & 255;
  
  temp = (int)(result[1] * 1000.); // cast float to int
  if (temp == 65535) temp = 65534;
  temp_msb = temp >> 8;
  temp_lsb = temp & 255;
  if (co2_lsb == 255 && temp_msb == 255) co2_lsb = 254;
  
  humd = (int)(result[2] * 100.); // cast float to int
  if (humd == 65535) humd = 65534;
  humd_msb = humd >> 8;
  humd_lsb = humd & 255;
  if (temp_lsb == 255 && humd_msb == 255) temp_lsb = 254;

  // UV index
  uvi_avg_msb = uvi_avg >> 8;
  uvi_avg_lsb = uvi_avg & 255;
  if (humd_lsb == 255 && uvi_avg_msb == 255) humd_lsb = 254;

  // send start marker
  Serial.write(255);
  Serial.write(255);

  // send packed sensor data
  Serial.write(co2_msb);
  Serial.write(co2_lsb);
  Serial.write(temp_msb);
  Serial.write(temp_lsb);
  Serial.write(humd_msb);
  Serial.write(humd_lsb);
  Serial.write(uvi_avg_msb);
  Serial.write(uvi_avg_lsb);

  // send end marker
  Serial.write(0);
  
  delay(20);
}

void setupTimerInterrupt()
{
  noInterrupts();
  TCCR1A  = 0;
  TCCR1B  = 0;
  TCNT1   = 0;
  // set compare match register for 500Hz increments
  // (16*10^6) / (25*64) - 1 (must be <65536)
  OCR1A   = 499; // 9999 for 25Hz
  TCCR1A |= (1 << WGM11);
  TCCR1B |= (1 << CS11) | (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);
    interrupts();
}


// RMS Interrupt routine
ISR(TIMER1_COMPA_vect) {
  uvi += analogRead(analogInPin);
  isr_counter++;
  if(isr_counter == 1024) {
    uvi_avg = (int)(uvi / 1024);
    // reset counter and sum
    isr_counter = 0;
    uvi = 0;
  }
}
