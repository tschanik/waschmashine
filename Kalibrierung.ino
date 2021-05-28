
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads1115; // Construct an ads1115

//Netzspannung 230 V ±10 %

void setup() {
  Serial.begin(115200);
  
  // ads.setGain(GAIN_FOUR); // 4x gain   +/- 1.024V  1 bit = 0.03125mV
  ads1115.setGain(GAIN_FOUR);
  ads1115.setDataRate(RATE_ADS1115_860SPS);
  ads1115.begin(); // Initialize ads1015 at the default address 0x48
  
  delay(200);
}

void loop() {

  int16_t results;
  float voltage;
  float current;
  float voltage_sum = 0;
  float sum = 0;
  long time_check = millis();
  int i;
  //0,02sec * 1000msec/1sec
  
  for(i=0; i < 860; i++){
    
    results = ads1115.readADC_Differential_0_1();
    // 
    if (abs(results) <= 6) {
      results = 0;
    }
    // 16 bit -> 2^(16-1) = 32768 bits  4x gain: 1.024V -> 1.024V/32768bits
    voltage = results * (1.024*1000)/32768; // [mv]
    voltage_sum += sq(voltage); // [mV^2]
 
  }  
  
  voltage = sqrt(voltage_sum/i); // [mV]

  //Serial.println(voltage,6);
  
  // y=m*x+b
  // Hersteller sagt m= 30 A/V 
  current = 0.030616891 * voltage;
    
  Serial.print("Voltage: ");
  Serial.print(voltage, 6);
  Serial.print(" mV | ");
  Serial.print("Strom: ");
  Serial.print(current, 6);
  Serial.print(" A | ");
  Serial.print("Power: ");
  Serial.print(230*current,6);
  Serial.println(" W"); 
  
}
