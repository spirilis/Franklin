/* CalibrateF5529 - Perform Antenna calibration of the AS3935 using the IRQ pin attached to TA1CLK (pin 5),
 * and the MSP430F5529's built-in DCO oscillator trimmed by a 32.768KHz XTAL as a reference.
 */
#include <Franklin.h>
#include <SPI.h>

#ifndef __MSP430F5529
#error This sketch only works on the TI MSP430F5529 USB LaunchPad.
#endif

#if F_CPU != 16000000
#error This sketch is calibrated to use a 16MHz MCLK; please select 16MHz for the F5529 LaunchPad board.
#endif

Franklin lsensor(2, 5);  // AS3935 attached to SPI pins with CS -> pin 2, IRQ -> pin 5 (TA1CLK).

void setup() {
  Serial.begin(115200);
  delay(2000);  // Give the user 2 seconds to open the serial monitor
  
  SPI.begin();
  SPI.setClockDivider(F_CPU / 2000000);  // 2MHz SPI max supported by AS3935
  SPI.setDataMode(SPI_MODE1);
  SPI.setBitOrder(MSBFIRST);

  Serial.println("CalibrateF5529 - Initializing AS3935 chip and setting up a calibration timer.");

  lsensor.begin();
  if (lsensor.getState() == FRANKLIN_STATE_UNKNOWN) {
    Serial.println("ERROR: Franklin AS3935 sensor not found!");
    while(1) delay(1000);
  }

  // Express LCO on IRQ pin
  detachInterrupt(5);
  lsensor.writePartialReg(0x00, 0, 1, 0);  // POWERDOWN = 0
  lsensor.writePartialReg(0x03, 0, 2, 6);  // LCO_FDIV = /16
  lsensor.writePartialReg(0x08, 1, 1, 7);  // DISP_LCO = 1
  Serial.println("Antenna tuning frequency should be expressed on IRQ (pin #5)");
  Serial.println();

  // Pin#5 = P1.6
  P1DIR &= ~BIT6;
  P1SEL |= BIT6;  // P1DIR.6=0, P1SEL.6=1 == TA1CLK

  performAntennaCalibration();
}

void loop() {
  delay(10000);
}

// This is the "meat" of this sketch, and it only runs once.
void performAntennaCalibration() {
  int tuncap = 0, i = 0, j = 0;
  uint16_t ta1val = 0;
  uint32_t frequencies[16], deltas[16];
  int32_t d;

  // Try all 16 antenna tuning capacitor settings
  for (tuncap = 0; tuncap < 16; tuncap++) {
    Serial.print("Tuning capacitor setting #"); Serial.print(tuncap); Serial.println(":");
    lsensor.writePartialReg(0x08, tuncap, 4, 0);
    Serial.print("analyzing frequency... ");
    Serial.flush();

    TA0CTL |= TACLR;
    TA0CTL = TASSEL_2 | ID_3;
    TA1CTL |= TACLR;
    TA1CTL = TASSEL_0 | ID_0;
    TA1CTL |= MC_2;
    TA0CTL |= MC_2;
    for (i=0; i < 50; i++) {
      while (TA0R < 40000) ;  // This samples 1/50th of a second
      TA0CTL |= TACLR;
      TA0CTL = TASSEL_2 | ID_3 | MC_2;
    }
    ta1val = TA1R;
    TA0CTL &= ~MC_3;
    TA1CTL &= ~MC_3;

    frequencies[tuncap] = (uint32_t)ta1val * 16;
    d = frequencies[tuncap] - 500000;
    if (d < 0)
      d = -d;
    deltas[tuncap] = d;
    Serial.print("frequency="); Serial.print(frequencies[tuncap]); Serial.println(" Hz");
  }

  Serial.println();

  // Find lowest delta
  j = 0;
  for (i=1; i < 16; i++) {
    if (deltas[i] < deltas[j]) {
      j = i;
    }
  }
  Serial.print("Best TUNCAP setting: "); Serial.print(j); Serial.print(" (frequency = "); Serial.print(frequencies[j]); Serial.println(" Hz)");
  Serial.print("Calibration constant: 0x");
  i = lsensor.readPartialReg(0x00, 5, 1);
  i |= (uint16_t)lsensor.readPartialReg(0x01, 3, 4) << 4;
  i |= j << 8;
  Serial.println(i, HEX);
}
