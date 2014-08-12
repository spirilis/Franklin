/* PrintRegs.ino - Basic example to dump the Franklin Lightning Sensor's registers
 *
 * This sketch initializes the IC with begin(), verifies it exists with getState() and then
 * dumps the registers in a readable format to Serial (115200bps).
 *
 */
#include <Franklin.h>
#include <SPI.h>

Franklin lsensor(2, 5);  // AS3935 attached to SPI pins with CS -> pin 2, IRQ -> pin 5.

void setup() {
  Serial.begin(115200);
  SPI.begin();
  SPI.setClockDivider(F_CPU / 2000000);  // 2MHz SPI max supported by AS3935
  SPI.setDataMode(SPI_MODE1);
  SPI.setBitOrder(MSBFIRST);

  Serial.println("Hi there, this is PrintRegs.ino and I am initializing your AS3935 chip-");

  lsensor.begin();
  if (lsensor.getState() == FRANKLIN_STATE_UNKNOWN) {
    Serial.println("ERROR: Franklin AS3935 sensor not found!");
    while(1) delay(1000);
  }
  Serial.println("Sensor found; printing register information every 10 seconds");
}

void loop() {

  int state = lsensor.getState();
  switch (state) {
    case FRANKLIN_STATE_UNKNOWN:
      Serial.println("AS3935 State: UNKNOWN");
      break;
    case FRANKLIN_STATE_POWERDOWN:
      Serial.println("AS3935 State: POWERDOWN");
      break;
    case FRANKLIN_STATE_LISTENING:
      Serial.println("AS3935 State: LISTENING");
      break;
    case FRANKLIN_STATE_NOISY:
      Serial.println("AS3935 State: NOISY");
      break;
    case FRANKLIN_STATE_LIGHTNING:
      Serial.println("AS3935 State: LIGHTNING");
      break;
    case FRANKLIN_STATE_DISTURBER:
      Serial.println("AS3935 State: DISTURBER");
      break;
  }

  lsensor.printRegs(&Serial, -1);  // Output details via Serial, and output info for all registers (-1)
  delay(10000);

}
