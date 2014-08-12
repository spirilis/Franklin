/* DumpRegs.ino - Basic example to dump the Franklin Lightning Sensor's registers
 *
 * This sketch initializes the IC with begin(), verifies it exists with getState() and then
 * dumps the registers' raw hex values over the Serial port (115200bps)
 *
 */
#include <Franklin.h>
#include <SPI.h>

Franklin lsensor(2, 5);  // AS3935 attached to SPI pins with CS -> pin 2, IRQ -> pin 5.

void setup() {
  uint8_t regs[0x33];
  int i;

  Serial.begin(115200);
  SPI.begin();
  SPI.setClockDivider(F_CPU / 2000000);  // 2MHz SPI max supported by AS3935
  SPI.setDataMode(SPI_MODE1);
  SPI.setBitOrder(MSBFIRST);

  Serial.println("Hi there, this is DumpRegs.ino and I am initializing your AS3935 chip-");

  lsensor.begin();
  Serial.println("Performing register dump-");
  lsensor.dumpRegs(regs);

  for (int i=0; i < 0x33; i++) {
    Serial.print("0x"); Serial.print(regs[i], HEX); Serial.print(" ");
    if (i % 8 == 7)
      Serial.println();
  }
  Serial.println();
  Serial.println("Done.");
}

void loop() {
  delay(10000);
}
