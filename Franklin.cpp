/* Franklin - Austria Microsystems AS3935 Franklin Lighting Sensor driver
 * Written for Energia, compatible with Arduino except for the calibration examples.
 * Calibration examples designed for the TI MSP430 LaunchPad platforms using the
 *   MSP430's internal timer architecture.
 */

#include "Franklin.h"


#ifdef __cplusplus
extern "C" {
#endif

volatile boolean _franklin_pending_IRQ;

void franklin_irq_handler(void)
{
	_franklin_pending_IRQ = true;
	#if defined(wakeup)
	wakeup();
	#else
		#if defined(LPM4_exit)
		__bic_SR_register_on_exit(LPM4_bits);
		#endif
	#endif
}

#ifdef __cplusplus
};
#endif

Franklin::Franklin(int csPin, int irqPin, uint16_t calibration_constant)
{
	_csPin = csPin;
	_irqPin = irqPin;
	_calibConstant = calibration_constant;
	_franklin_pending_IRQ = false;
}

Franklin::Franklin(int csPin, int irqPin)
{
	_csPin = csPin;
	_irqPin = irqPin;
	_calibConstant = 0xFFFF;
	_franklin_pending_IRQ = false;
}

void Franklin::begin(void)
{
	detachInterrupt(_irqPin);
	pinMode(_irqPin, INPUT);
	pinMode(_csPin, OUTPUT);
	digitalWrite(_csPin, HIGH);

	writeReg(0x3C, 0x96);  // PRESET_DEFAULT

	if (_calibConstant != 0xFFFF) {
		setCalibration(_calibConstant);
	}


	// Calibrate internal oscillators
	writePartialReg(0x00, 1, 1, 0);  // PWD (powerdown) = 1
	writeReg(0x3D, 0x96);  // CALIB_RCO (calibrate internal oscillators)
	writePartialReg(0x08, 1, 1, 5);  // DISP_TRCO = 1
	delay(2);
	writePartialReg(0x08, 0, 1, 5);  // DISP_TRCO = 0

	// Prepare chip for service
	writePartialReg(0x00, 0, 1, 0);  // PWD (powerdown) = 0
	attachInterrupt(_irqPin, franklin_irq_handler, RISING);
	_franklin_pending_IRQ = false;
}

uint8_t Franklin::readReg(uint8_t addr)
{
	uint8_t ret;

	digitalWrite(_csPin, LOW);
	SPI.transfer(0x40 | (addr & 0x3F));  // READ mode
	ret = SPI.transfer(0xFF);
	digitalWrite(_csPin, HIGH);

	return ret;
}

void Franklin::writeReg(uint8_t addr, uint8_t value)
{
	digitalWrite(_csPin, LOW);
	SPI.transfer(addr & 0x3F);
	SPI.transfer(value);
	digitalWrite(_csPin, HIGH);
}

uint16_t Franklin::extractBitfield(uint16_t val, int bitwidth, int bitstart)
{
	if (bitwidth < 0 || bitwidth > 16 || bitstart < 0 || bitstart > 15)
		return 0;
	
	return ((val & bitfieldMask(bitwidth, bitstart)) >> bitstart);
}

uint16_t Franklin::bitfieldMask(int bitwidth, int bitstart)
{
	int i = 0;
	uint16_t mask = 0x0000;

	if (bitwidth < 0 || bitwidth > 16 || bitstart < 0 || bitstart > 15)
		return 0;

	for (i=bitstart; i < (bitstart+bitwidth); i++) {
		mask |= 1 << i;
	}
	return mask;
}

uint8_t Franklin::readPartialReg(uint8_t addr, int bitwidth, int bitstart)
{
	uint8_t val;

	val = readReg(addr);
	return extractBitfield(val, bitwidth, bitstart);
}

void Franklin::writePartialReg(uint8_t addr, uint8_t value, int bitwidth, int bitstart)
{
	uint8_t val, bitmask, wrmask;

	val = readReg(addr);
	bitmask = bitfieldMask(bitwidth, bitstart);
	wrmask = bitmask >> bitstart;
	val &= ~bitmask;
	val |= (value & wrmask) << bitstart;

	writeReg(addr, val);
}

void Franklin::setCalibration(uint16_t calibration_constant)
{
	uint16_t pval;

	// TUN_CAP
	pval = extractBitfield(calibration_constant, 4, 8);
	writePartialReg(0x08, pval, 4, 0);
	// NF_LEV
	pval = extractBitfield(calibration_constant, 3, 5);
	writePartialReg(0x01, pval, 3, 4);
	// AFE_GB
	pval = extractBitfield(calibration_constant, 5, 0);
	writePartialReg(0x00, pval, 5, 1);
}

uint16_t Franklin::getCalibration(void)
{
	uint16_t calib = 0x0000;
	uint8_t reg, bf;

	// AFE_GB
	reg = readReg(0x00);
	bf = extractBitfield(reg, 5, 1);
	calib = bf;
	// NF_LEV
	reg = readReg(0x01);
	bf = extractBitfield(reg, 3, 4);
	calib |= (bf << 5);
	// TUN_CAP
	reg = readReg(0x08);
	bf = extractBitfield(reg, 4, 0);
	calib |= (bf << 8);

	return calib;
}

void Franklin::resetOscillatorTrim(void)
{
	detachInterrupt(_irqPin);

	// Calibrate internal oscillators
	writePartialReg(0x00, 1, 1, 0);  // PWD (powerdown) = 1
	writeReg(0x3D, 0x96);  // CALIB_RCO (calibrate internal oscillators)
	writePartialReg(0x08, 1, 1, 5);  // DISP_TRCO = 1
	delay(2);
	writePartialReg(0x08, 0, 1, 5);  // DISP_TRCO = 0

	writePartialReg(0x00, 0, 1, 0);  // PWD (powerdown) = 0

	attachInterrupt(_irqPin, franklin_irq_handler, RISING);
	_franklin_pending_IRQ = false;
}

int Franklin::stormDistanceKm(uint8_t distreg)
{
	if (distreg == 0x01)
		return 0;  // Storm is overhead
	if (distreg < 5)
		return -1;  // Undefined?
	if (distreg >= 5 && distreg <= 40)
		return distreg;
	return -1;  // Out of Range
}

int Franklin::getStormDistance(void)
{
	return stormDistanceKm(readReg(0x07));
}

int Franklin::dumpRegs(uint8_t *buf)
{
	int i;

	for (i=0; i <= 0x32; i++) {
		buf[i] = readReg(i);
	}
	return i;
}

void Franklin::printRegs(Stream *outdev, int whichReg)
{
	uint8_t regbuf[0x33];
	int i, j;
	uint32_t k;
	boolean indoors = false;

	dumpRegs(regbuf);
	// 0x00 - AFE_GB, PWD
	if (whichReg < 0 || whichReg == 0x00) {
		i = extractBitfield(regbuf[0x00], 5, 1);
		outdev->print("AFE_GB: "); outdev->print(i, BIN);
		switch (i) {
			case 0x12:
				outdev->println(" (INDOOR)");
				indoors = true;
				break;
			case 0x0E:
				outdev->println(" (OUTDOOR)");
				indoors = false;
				break;
			default:
				outdev->println(" (CUSTOM SETTING)");
				indoors = false;
		}
		outdev->print("PWD :"); outdev->print(regbuf[0x00] & 0x01, DEC);
		if (regbuf[0x00] & 0x01)
			outdev->println(" (POWERDOWN)");
		else
			outdev->println(" (POWERUP)");
	}

	// 0x01 - NF_LEV, WDTH
	if (whichReg < 0 || whichReg == 0x01) {
		i = extractBitfield(regbuf[0x01], 3, 4);
		outdev->print("Noise Floor (NF_LEV): ");
		if (indoors) {
			switch (i) {
				case 0x00:
					j = 28;
					break;
				case 0x01:
					j = 45;
					break;
				case 0x02:
					j = 62;
					break;
				case 0x03:
					j = 78;
					break;
				case 0x04:
					j = 95;
					break;
				case 0x05:
					j = 112;
					break;
				case 0x06:
					j = 130;
					break;
				case 0x07:
					j = 146;
					break;
			}
		} else {
			switch (i) {
				case 0x00:
					j = 390;
					break;
				case 0x01:
					j = 630;
					break;
				case 0x02:
					j = 860;
					break;
				case 0x03:
					j = 1100;
					break;
				case 0x04:
					j = 1140;
					break;
				case 0x05:
					j = 1570;
					break;
				case 0x06:
					j = 1800;
					break;
				case 0x07:
					j = 2000;
					break;
			}
		}
		outdev->print(j); outdev->println(" uVrms");

		i = extractBitfield(regbuf[0x01], 4, 0);
		outdev->print("Watchdog Threshold (WDTH): "); outdev->println(i);
	}

	// 0x02 - MIN_NUM_LIGH, SREJ
	if (whichReg < 0 || whichReg == 0x02) {
		i = extractBitfield(regbuf[0x02], 2, 4);
		switch (i) {
			case 0x00:
				j = 1;
				break;
			case 0x01:
				j = 5;
				break;
			case 0x02:
				j = 9;
				break;
			case 0x03:
				j = 16;
		}
		outdev->print("Minimum # of Lightning (MIN_NUM_LIGH): "); outdev->println(j);
		i = extractBitfield(regbuf[0x02], 4, 0);
		outdev->print("Spike Rejection (SREJ): "); outdev->println(i);
	}

	// 0x03 - LCO_FDIV, MASK_DIST, INT
	if (whichReg < 0 || whichReg == 0x03) {
		i = extractBitfield(regbuf[0x03], 2, 6);
		outdev->print("LCO Output Divider: /");
		switch (i) {
			case 0x00:
				j = 16;
				break;
			case 0x01:
				j = 32;
				break;
			case 0x02:
				j = 64;
				break;
			case 0x03:
				j = 128;
				break;
		}
		outdev->println(j);

		i = extractBitfield(regbuf[0x03], 1, 5);
		outdev->print("Mask Disturber (MASK_DIST): ");
		if (i)
			outdev->println("ENABLED");
		else
			outdev->println("DISABLED");

		i = extractBitfield(regbuf[0x03], 4, 0);
		outdev->print("IRQ: ");
		if (i & FRANKLIN_IRQ_NOISEHIGH)
			outdev->print("NOISE_TOO_HIGH ");
		if (i & FRANKLIN_IRQ_DISTURBER)
			outdev->print("DISTURBER_DETECTED ");
		if (i & FRANKLIN_IRQ_LIGHTNING)
			outdev->print("LIGHTNING_DETECTED ");
		if (i == 0)
			outdev->print("NONE ");
		outdev->println();
	}

	// 0x04, 0x05, 0x06 - Energy of Single Lightning
	if (whichReg < 0 || whichReg == 0x04 || whichReg == 0x05 || whichReg == 0x06) {
		k = regbuf[0x04] | (regbuf[0x05] << 8) | (regbuf[0x06] << 16);
		outdev->print("Last Single Lightning Energy: "); outdev->println(k, DEC);
	}

	// 0x07 - Distance Estimation of incoming storm
	if (whichReg < 0 || whichReg == 0x07) {
		outdev->print("Distance Estimate of Incoming Storm: ");
		i = stormDistanceKm(regbuf[0x07]);
		if (i < 0)
			outdev->println("No Storm Detected/Out of Range");
		if (i == 0)
			outdev->println("Storm Overhead");
		if (i > 0) {
			outdev->print(i); outdev->println("km");
		}
	}

	// 0x08 - Tuning capacitor, display calibration frequencies on IRQ pin
	if (whichReg < 0 || whichReg == 0x08) {
		if (regbuf[0x08] & 0x80)
			outdev->println("DISP_LCO active (Antenna LC oscillator frequency expressed on IRQ pin");
		if (regbuf[0x08] & 0x40)
			outdev->println("DISP_SRCO active (32.768KHz Sleep Oscillator expressed on IRQ pin");
		if (regbuf[0x08] & 0x20)
			outdev->println("DISP_TRCO active (1.1MHz High-Frequency Oscillator expressed on IRQ pin");
		i = extractBitfield(regbuf[0x08], 4, 0);
		outdev->print("Tuning Capacitors: ");
		outdev->print(i * 8);
		outdev->println("pF");
	}

	// 0x09-0x32 are the hardcoded Lightning Detection Look-Up tables
	if (whichReg < 0 || whichReg >= 0x09) {
		outdev->println("Hardcoded Lightning Detection Look-Up Table:");
		for (i=0x09; i <= 0x32; i++) {
			outdev->print("0x"); outdev->print(regbuf[i], HEX); outdev->print(" ");
			if ( (i-0x09) % 8 == 7 )
				outdev->println();
		}
		outdev->println();
	}
	outdev->println("--------");
}

void Franklin::setIndoors(boolean yesno)
{
	if (yesno)
		writePartialReg(0x00, 0x12, 5, 1);
	else
		writePartialReg(0x00, 0x0E, 5, 1);
}

void Franklin::setCustomGain(uint16_t afegain)
{
	writePartialReg(0x00, afegain, 5, 1);
}

boolean Franklin::getIndoorOutdoor(void)
{
	int i;

	i = readPartialReg(0x00, 5, 1);
	if (i >= 0x12)
		return true;
	return false;
}

int Franklin::getNoiseFloor(void)
{
	boolean indoors = getIndoorOutdoor();
	int i, j;

	i = readPartialReg(0x01, 3, 4);
	if (indoors) {
		switch (i) {
			case 0x00:
				j = 28;
				break;
			case 0x01:
				j = 45;
				break;
			case 0x02:
				j = 62;
				break;
			case 0x03:
				j = 78;
				break;
			case 0x04:
				j = 95;
				break;
			case 0x05:
				j = 112;
				break;
			case 0x06:
				j = 130;
				break;
			case 0x07:
				j = 146;
				break;
		}
	} else {
		switch (i) {
			case 0x00:
				j = 390;
				break;
			case 0x01:
				j = 630;
				break;
			case 0x02:
				j = 860;
				break;
			case 0x03:
				j = 1100;
				break;
			case 0x04:
				j = 1140;
				break;
			case 0x05:
				j = 1570;
				break;
			case 0x06:
				j = 1800;
				break;
			case 0x07:
				j = 2000;
				break;
		}
	}
	return j;
}

int Franklin::setNoiseFloor(int uVrms)
{
	boolean indoors = getIndoorOutdoor();
	int i;
	uint8_t nf_lev = 0;

	if (uVrms < 0)
		return -1;
	if (indoors) {
		if (uVrms <= 28) {
			i = 28;
			nf_lev = 0x00;
		} else if (uVrms <= 45) {
			i = 45;
			nf_lev = 0x01;
		} else if (uVrms <= 62) {
			i = 62;
			nf_lev = 0x02;
		} else if (uVrms <= 78) {
			i = 78;
			nf_lev = 0x03;
		} else if (uVrms <= 95) {
			i = 95;
			nf_lev = 0x04;
		} else if (uVrms <= 112) {
			i = 112;
			nf_lev = 0x05;
		} else if (uVrms <= 130) {
			i = 130;
			nf_lev = 0x06;
		} else if (uVrms <= 146) {
			i = 146;
			nf_lev = 0x07;
		} else {
			return -1;
		}
	} else {
		if (uVrms <= 390) {
			i = 390;
			nf_lev = 0x00;
		} else if (uVrms <= 630) {
			i = 630;
			nf_lev = 0x01;
		} else if (uVrms <= 860) {
			i = 860;
			nf_lev = 0x02;
		} else if (uVrms <= 1100) {
			i = 1100;
			nf_lev = 0x03;
		} else if (uVrms <= 1140) {
			i = 1140;
			nf_lev = 0x04;
		} else if (uVrms <= 1570) {
			i = 1570;
			nf_lev = 0x05;
		} else if (uVrms <= 1800) {
			i = 1800;
			nf_lev = 0x06;
		} else if (uVrms <= 2000) {
			i = 2000;
			nf_lev = 0x07;
		} else {
			return -1;
		}
	}
	writePartialReg(0x01, nf_lev, 3, 4);
	return i;
}

void Franklin::squelchDisturbers(boolean yesno)
{
	if (yesno)
		writePartialReg(0x03, 1, 1, 5);
	else
		writePartialReg(0x03, 0, 1, 5);
}

int Franklin::getSignalThreshold(void)
{
	return readPartialReg(0x01, 4, 0);
}

void Franklin::setSignalThreshold(int wdth)
{
	if (wdth < 0 || wdth > 15)
		return;
	
	writePartialReg(0x01, wdth, 4, 0);
}

int Franklin::getStrikeThreshold(void)
{
	uint8_t reg = readPartialReg(0x02, 2, 4);
	switch (reg) {
		case 0x00:
			return 1;
		case 0x01:
			return 5;
		case 0x02:
			return 9;
		case 0x03:
			return 16;
	}
	return -1;
}

int Franklin::setStrikeThreshold(int numStrikes)
{
	uint8_t mnl = 0x00;
	int i = 1;

	if (numStrikes < 0 || numStrikes > 16)
		return -1;
	
	if (numStrikes > 1) {
		mnl = 0x01;
		i = 5;
	}
	if (numStrikes > 5) {
		mnl = 0x02;
		i = 9;
	}
	if (numStrikes > 9) {
		mnl = 0x03;
		i = 16;
	}
	
	writePartialReg(0x02, mnl, 2, 4);
	return i;
}

int Franklin::getSpikeRejection(void)
{
	return readPartialReg(0x02, 4, 0);
}

void Franklin::setSpikeRejection(int val)
{
	if (val < 0 || val > 15)
		return;
	
	writePartialReg(0x02, val, 4, 0);
}


void Franklin::clear(void)
{
}

int Franklin::getState(void)
{
	uint8_t irq, pwd;

	// Sanity check - if this fails, we have no idea that the transceiver is wired correctly!
	if (readReg(0x09) != 0xAD)
		return FRANKLIN_STATE_UNKNOWN;

	irq = readPartialReg(0x03, 4, 0);
	_franklin_pending_IRQ = false;

	if (!irq) {
		pwd = readPartialReg(0x00, 1, 0);
		if (pwd)
			return FRANKLIN_STATE_POWERDOWN;
		return FRANKLIN_STATE_LISTENING;
	}
	if (irq & FRANKLIN_IRQ_LIGHTNING)
		return FRANKLIN_STATE_LIGHTNING;
	if (irq & FRANKLIN_IRQ_DISTURBER)
		return FRANKLIN_STATE_DISTURBER;
	if (irq & FRANKLIN_IRQ_NOISEHIGH)
		return FRANKLIN_STATE_NOISY;
}

boolean Franklin::available(void)
{
	return _franklin_pending_IRQ;
}
