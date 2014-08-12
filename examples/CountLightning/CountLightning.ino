/* CountLightning.ino - Continuously monitor the Lightning Sensor and count total lightning events found.
 * Also log the closest Estimated Distance reading ever found.
 *
 * This sketch assumes you have a proper calibration value for the sensor, and it tunes the
 * Watchdog (setSignalThreshold) with a hardcoded value which you should discover by yourself
 * using the PrintRegs example (watch for a setSignalThreshold setting that does not generate
 * too many Disturber events on its own).
 *
 */
#include <Franklin.h>
#include <SPI.h>

Franklin lsensor(2, 5, 0x732);  // AS3935 attached to SPI pins with CS -> pin 2, IRQ -> pin 5.

void setup() {
  Serial.begin(115200);
  SPI.begin();
  SPI.setClockDivider(F_CPU / 2000000);  // 2MHz SPI max supported by AS3935
  SPI.setDataMode(SPI_MODE1);
  SPI.setBitOrder(MSBFIRST);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  Serial.println("This is CountLightning.ino - Here we will count lightning events as they come through.");

  lsensor.begin();
  if (lsensor.getState() == FRANKLIN_STATE_UNKNOWN) {
    Serial.println("ERROR: Franklin AS3935 sensor not found!");
    while(1) delay(1000);
  }
  Serial.print("Sensor found; tuning parameters: ");
  Serial.print("SignalThreshold=3 ");
  lsensor.setSignalThreshold(3);

  Serial.print("SquelchDisturbers=ON ");
  lsensor.squelchDisturbers(true);

  Serial.println();
}

void loop() {
  static uint16_t lightning_count = 0;
  static uint16_t min_distance = 0xFFFF;
  static uint32_t lastmillis = 0;
  int state, i;

  while (!lsensor.available()) {
    if (millis()-lastmillis > 300000) {  // 5 minutes
      lastmillis = millis();

      if (lightning_count) {
        lightning_count = 0;
      } else {
        digitalWrite(GREEN_LED, LOW);
      }
      lastmillis = millis();
    }
  }

  state = lsensor.getState();
  switch (state) {
    case FRANKLIN_STATE_UNKNOWN:
      //Serial.println("AS3935 State: UNKNOWN");
      break;
    case FRANKLIN_STATE_POWERDOWN:
      //Serial.println("AS3935 State: POWERDOWN");
      break;
    case FRANKLIN_STATE_LISTENING:
      //Serial.println("AS3935 State: LISTENING");
      digitalWrite(RED_LED, LOW);
      break;
    case FRANKLIN_STATE_NOISY:
      //Serial.println("AS3935 State: NOISY");
      digitalWrite(RED_LED, HIGH);
      break;
    case FRANKLIN_STATE_LIGHTNING:
      //Serial.println("AS3935 State: LIGHTNING");
      lightning_count++;
      Serial.print("LIGHTNING: "); Serial.print(lightning_count); Serial.print(" count, ");
      i = lsensor.getStormDistance();
      if (i >= 0) {
        if (i < min_distance)
          min_distance = i;
      }
      if (min_distance < 0xFFFF) {
        Serial.print("minimum detected distance = "); Serial.print(min_distance); Serial.print("km");
      } else {
        Serial.print("no distance known.");
      }
      Serial.println();
      digitalWrite(RED_LED, LOW);
      digitalWrite(GREEN_LED, HIGH);
      break;
    case FRANKLIN_STATE_DISTURBER:
      //Serial.println("AS3935 State: DISTURBER");
      digitalWrite(RED_LED, LOW);
      break;
  }
}
