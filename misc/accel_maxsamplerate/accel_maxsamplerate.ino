#include<Wire.h>
#define LED_PIN 2
bool blinkState = false;
const int MPU_addr = 0x68; // I2C address of the MPU-6050
int16_t AcX[8192], AcY[8192], AcZ[8192];

void setup() {
  Wire.begin();
  Wire.setClock(400000L);
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0x04);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

  Wire.beginTransmission(0x68);
  Wire.write(0x1A);  // the config address
  Wire.write(0x07);  // the config value
  Wire.endTransmission(true);

  Wire.beginTransmission(0x68);
  Wire.write(0x1C);  // the config address
  Wire.write(0x00);  // the config value
  Wire.endTransmission(true);

  Serial.begin(115200);

  int ii=0;
  Serial.println("8192");
  Serial.println(millis());
  for (ii=0; ii<8192; ii++) {
      
      Wire.beginTransmission(MPU_addr);
      Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
      Wire.endTransmission(false);
      Wire.requestFrom(MPU_addr, 6, true); // request a total of 6 registers
      AcX[ii] = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
      AcY[ii] = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
      AcZ[ii] = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  }    
  Serial.println(millis());

  for (ii=0; ii<8192; ii++) {
      Serial.print(AcX[ii]);
      Serial.print(","); Serial.print(AcY[ii]);
      Serial.print(","); Serial.println(AcZ[ii]);
  }    
}

void loop() {
  
}
