//
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
// Simple Raspberry Pi Bare Metal Synthesizer Experiments
//
// Modifications and updates:
//    Copyright (C) 2021 Kevin (diyelectromusic)
//
// circlesynth.h
//    Based on sample 29-miniorgan/miniorgan.h
//
// Original license and copyright statement below
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2017-2021  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _circlesynth_h
#define _circlesynth_h

// define only one
//#define USE_I2S
//#define USE_HDMI

#ifdef USE_I2S
	#include <circle/i2ssoundbasedevice.h>
	#define SOUND_CLASS	CI2SSoundBaseDevice
	#define SAMPLE_RATE	192000
	#define CHUNK_SIZE	8192
	#define DAC_I2C_ADDRESS	0		// I2C slave address of the DAC (0 for auto probing)
#elif defined (USE_HDMI)
	#include <circle/hdmisoundbasedevice.h>
	#define SOUND_CLASS	CHDMISoundBaseDevice
	#define SAMPLE_RATE	48000
	#define CHUNK_SIZE	(384 * 10)
#else
	#include <circle/pwmsoundbasedevice.h>
	#define SOUND_CLASS	CPWMSoundBaseDevice
	#define SAMPLE_RATE	48000
	#define CHUNK_SIZE	2048
#endif

#include <circle/interrupt.h>
#include <circle/i2cmaster.h>
#include <circle/usb/usbmidi.h>
#include <circle/usb/usbkeyboard.h>
#include <circle/serial.h>
#include <circle/types.h>
#include "oscillator.h"

struct TNoteInfo
{
	char	Key;
	u8	KeyNumber;	// MIDI number
};

// The number of expected potentiomer controllers
#define SYNTH_POTS    4
#define WAVE_POT      0
#define INTENSITY_POT 1
#define RATE_POT      2
#define RATIO_POT     3

class CCircleSynth : public SOUND_CLASS
{
public:
	CCircleSynth (CInterruptSystem *pInterrupt, CI2CMaster *pI2CMaster);
	~CCircleSynth (void);

	boolean Initialize (void);

	void Process (boolean bPlugAndPlayUpdated);

	unsigned GetChunk (u32 *pBuffer, unsigned nChunkSize);
	
	void SetPot(unsigned nPot, unsigned nValue);

	int Osc2Level (float fOscOutput);

private:
	static void MIDIPacketHandler (unsigned nCable, u8 *pPacket, unsigned nLength);

	static void KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6]);

	static void USBDeviceRemovedHandler (CDevice *pDevice, void *pContext);
	
private:
	CUSBMIDIDevice     * volatile m_pMIDIDevice;
	CUSBKeyboardDevice * volatile m_pKeyboard;

	CSerialDevice m_Serial;
	boolean m_bUseSerial;
	unsigned m_nSerialState;
	u8 m_SerialMessage[3];

	int      m_nLowLevel;
	int      m_nNullLevel;
	int      m_nHighLevel;
	int      m_nCurrentLevel;
	unsigned m_nSampleCount;
	float    m_fFrequency;
	float    m_fRatio;
	float    m_fIntensity;
	float    m_fRate;

	u8 m_ucKeyNumber;
	
	TWaveform m_Waveform;

	unsigned m_nPots[SYNTH_POTS];

	static const float s_KeyFrequency[];
	static const TNoteInfo s_Keys[];
#define FM_RATIOS 8
	float m_fmRatios[FM_RATIOS] = {1.0, 2.0, 3.0, 5.0, 7.0, 9.0, 11.0, 13.0};

	static CCircleSynth *s_pThis;
	
	COscillator m_Mod;
	COscillator m_VCO2;
	COscillator m_VCO1;
};

#endif
