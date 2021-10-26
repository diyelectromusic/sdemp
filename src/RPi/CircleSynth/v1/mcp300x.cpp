//
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
// Simple Raspberry Pi Bare Metal Synthesizer Experiments
//
// Modifications and updates:
//    Copyright (C) 2021 Kevin (diyelectromusic)
//
// mcp300x.cpp
//    Based on addon/sensors/mcp300x.cpp
//
// Original license and copyright statement below
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2021  R. Stange <rsta2@o2online.de>
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
#include <sensor/mcp300x.h>
#include <circle/macros.h>
#include <assert.h>

#define ERR	0xFF

u8 CMCP300X::s_DifferentialMap[CMCP300X::Channels][CMCP300X::Channels] =
{
	{ERR, 0, ERR, ERR, ERR, ERR, ERR, ERR},
	{1, ERR, ERR, ERR, ERR, ERR, ERR, ERR},
	{ERR, ERR, ERR, 2, ERR, ERR, ERR, ERR},
	{ERR, ERR, 3, ERR, ERR, ERR, ERR, ERR},
	{ERR, ERR, ERR, ERR, ERR, 4, ERR, ERR},
	{ERR, ERR, ERR, ERR, 5, ERR, ERR, ERR},
	{ERR, ERR, ERR, ERR, ERR, ERR, ERR, 6},
	{ERR, ERR, ERR, ERR, ERR, ERR, 7, ERR}
};

CMCP300X::CMCP300X (CSPIMaster *pSPIMaster, float fVREF, unsigned nChipSelect, unsigned nClockSpeed)
:	m_pSPIMaster (pSPIMaster),
	m_pSPIMasterAUX (0),
	m_fVREF (fVREF),
	m_nChipSelect (nChipSelect),
	m_nClockSpeed (nClockSpeed)
{
}

CMCP300X::CMCP300X (CSPIMasterAUX *pSPIMasterAUX, float fVREF, unsigned nChipSelect,
		    unsigned nClockSpeed)
:	m_pSPIMaster (0),
	m_pSPIMasterAUX (pSPIMasterAUX),
	m_fVREF (fVREF),
	m_nChipSelect (nChipSelect),
	m_nClockSpeed (nClockSpeed)
{
}

CMCP300X::~CMCP300X (void)
{
	m_pSPIMaster = 0;
	m_pSPIMasterAUX = 0;
}

unsigned CMCP300X::AnalogRead (unsigned nChannel)
{
	assert (nChannel < Channels);
	return DoConversion (0b1000 | nChannel);
}

float CMCP300X::DoSingleEndedConversion (unsigned nChannel)
{
	assert (nChannel < Channels);
	u16 usResult = DoConversion (0b1000 | nChannel);
	return m_fVREF * usResult / 1024.0f;
}

float CMCP300X::DoDifferentialConversion (unsigned nChannelPlus, unsigned nChannelMinus)
{
	assert (nChannelPlus < Channels);
	assert (nChannelMinus < Channels);
	u8 nControl = s_DifferentialMap[nChannelPlus][nChannelMinus];
	assert (nControl <= 7);
    u16 usResult = DoConversion (nControl);
	return m_fVREF * usResult / 1024.0f;
}

unsigned CMCP300X::DoConversion (u8 nControl)
{
	u8 TxBuffer[3] ALIGN (4) = {0x01, (u8) (nControl << 4), 0x00};
	u8 RxBuffer[3] ALIGN (4);

	if (m_pSPIMaster)
	{
		m_pSPIMaster->SetClock (m_nClockSpeed);
		m_pSPIMaster->SetMode (0, 0);

		if (m_pSPIMaster->WriteRead (m_nChipSelect, TxBuffer, RxBuffer, 3) != 3)
		{
			return ResultSPIError;
		}
	}
	else
	{
		assert (m_pSPIMasterAUX != 0);
		m_pSPIMasterAUX->SetClock (m_nClockSpeed);

		if (m_pSPIMasterAUX->WriteRead (m_nChipSelect, TxBuffer, RxBuffer, 3) != 3)
		{
			return ResultSPIError;
		}
	}

	u16 usResult = RxBuffer[1] & 3;
	usResult <<= 8;
	usResult |= RxBuffer[2];

	return usResult;
}
