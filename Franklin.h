/* Franklin - Austria Microsystems AS3935 Franklin Lighting Sensor driver
 * Written for Energia, compatible with Arduino except for the calibration examples.
 * Calibration examples designed for the TI MSP430 LaunchPad platforms using the
 *   MSP430's internal timer architecture.
 *
 * Calibration Constant format (uint16_t):
 *   15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
 *  +-----------------------------------------------+
 *  |  |  |  |  |  TUN_CAP  | NF_LEV |   AFE_GB     |
 *  +-----------------------------------------------+
 *  TUN_CAP[11:8] - 4 bits
 *  NF_LEV[7:5] - 3 bits
 *  AFE_GB[4:0] - 5 bits
 *
 *
 *  Important SPI information:
 *
 *  The AS3935 requires SPI_MODE1, MSBFIRST, no higher than 2MHz frequency for SPI operation.
 *  Please use SPI.setBitOrder(MSBFIRST); SPI.setDataMode(SPI_MODE1); and SPI.setClockDivider()
 *  appropriately; it may be necessary to re-set SPI.setDataMode() before each call to the Franklin
 *  library inside your code if you happen to have multiple SPI devices on the bus.
 *
 * Copyright (c) 2014, Eric Brundick <spirilis@linux.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <Arduino.h>
#include <SPI.h>

enum {
	FRANKLIN_STATE_UNKNOWN = 0,
	FRANKLIN_STATE_POWERDOWN,
	FRANKLIN_STATE_LISTENING,
	FRANKLIN_STATE_NOISY,
	FRANKLIN_STATE_LIGHTNING,
	FRANKLIN_STATE_DISTURBER
};

#define FRANKLIN_IRQ_NOISEHIGH 0x01
#define FRANKLIN_IRQ_DISTURBER 0x04
#define FRANKLIN_IRQ_LIGHTNING 0x08

class Franklin {
	private:
		int _csPin, _irqPin;
		uint16_t _calibConstant;

		uint16_t extractBitfield(uint16_t val, int bitwidth, int bitstart);
		uint16_t bitfieldMask(int bitwidth, int bitstart);
		int stormDistanceKm(uint8_t distreg);

	public:
		Franklin(int csPin, int irqPin);
		Franklin(int csPin, int irqPin, uint16_t calibration_constant);
		void setCalibration(uint16_t calibration_constant);
		uint16_t getCalibration(void);
		void resetOscillatorTrim(void);

		void begin();
		void end();
		boolean available();
		void clear();
		void power(boolean yesno);

		uint8_t readReg(uint8_t addr);
		void writeReg(uint8_t addr, uint8_t value);
		int dumpRegs(uint8_t *buf);
		void printRegs(Stream *outdev, int whichReg);  // whichReg < 0 means print them all

		uint8_t readPartialReg(uint8_t addr, int bitwidth, int bitstart);
		void writePartialReg(uint8_t addr, uint8_t value, int bitwidth, int bitstart);  // Performs a Read-Modify-Write

		int getState(void);

		void setIndoors(boolean yesno);
		void setCustomGain(uint16_t afegain);
		boolean getIndoorOutdoor(void);  // true = indoor AFE setting, false = outdoor AFE setting

		int getNoiseFloor(void);
		int setNoiseFloor(int uVrms);
		unsigned int getNoiseFloorBits(void);
		void setNoiseFloorBits(unsigned int);

		void squelchDisturbers(boolean yesno);
		boolean getSquelchDisturbers(void);

		int getSignalThreshold(void);
		void setSignalThreshold(int wdth);

		int getStormDistance(void);

		int getStrikeThreshold(void);
		int setStrikeThreshold(int numStrikes);

		int getSpikeRejection(void);
		void setSpikeRejection(int val);
};

#ifdef __cplusplus
extern "C" {
#endif

void franklin_irq_handler(void);
extern volatile boolean _franklin_pending_IRQ;

#ifdef __cplusplus
};
#endif
