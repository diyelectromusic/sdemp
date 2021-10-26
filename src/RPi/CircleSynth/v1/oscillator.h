//
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
// Simple Raspberry Pi Bare Metal Synthesizer Experiments
//
// Modifications and updates:
//    Copyright (C) 2021 Kevin (diyelectromusic)
//
// oscillator.h
//    Based on rsta2/minisynth/.../src/oscillator.h
//
// Original license and copyright statement below
//
// General purpose oscillator
//
// MiniSynth Pi - A virtual analogue synthesizer for Raspberry Pi
// Copyright (C) 2017-2020  R. Stange <rsta2@o2online.de>
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
#ifndef _oscillator_h
#define _oscillator_h

#define DEFAULT_SAMPLE_RATE 48000

enum TWaveform
{
	WaveformSine,
	WaveformSquare,
	WaveformSawtooth,
	WaveformTriangle,
	WaveformUnknown
};

class COscillator
{
public:
	COscillator (COscillator *pModulator = 0);
	~COscillator (void);

    void SetSampleRate (unsigned nSampleRate);
	void SetWaveform (TWaveform Waveform);
	void SetFrequency (float fFrequency);			// in Hz
	void SetDetune (float fDetune);				// [-1.0, 1.0]
	void SetModulationVolume (float fVolume);		// [0.0, 1.0]

	void NextSample (void);
	float GetOutputLevel (void) const;			// returns [-1.0, 1.0]

private:
	COscillator *m_pModulator;

	TWaveform m_Waveform;
	float m_fFrequency;
	float m_fMidFrequency;
	float m_fDetune;
	float m_fModulationVolume;

	unsigned m_nSampleRate;
	unsigned m_nSampleCount;

	float m_fOutputLevel;

	static float s_SineTable[];
};

#endif
