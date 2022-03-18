//
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
// Simple Raspberry Pi Bare Metal Synthesizer Experiments
//
// Modifications and updates:
//    Copyright (C) 2021 Kevin (diyelectromusic)
//
// kernel.cpp
//    Based on sample 29-miniorgan/kernel.cpp
//
// Original license and copyright statement below
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2021  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include <circle/machineinfo.h>

static const char FromKernel[] = "kernel";

#define VREF			3.3f		// Reference voltage (Volt)

#define SPI_MASTER_DEVICE	0		// 0, 4, 5, 6 on Raspberry Pi 4; 0 otherwise
#define SPI_CHIP_SELECT		0		// 0 or 1, or 2 (for SPI1)
#define SPI_CLOCK_SPEED		1000000		// Hz

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (m_Options.GetLogLevel (), &m_Timer),
	m_I2CMaster (CMachineInfo::Get ()->GetDevice (DeviceI2CMaster), TRUE),
	m_USBHCI (&m_Interrupt, &m_Timer, TRUE),		// TRUE: enable plug-and-play
#ifndef USE_SPI_MASTER_AUX
	m_SPIMaster (SPI_CLOCK_SPEED, 0, 0, SPI_MASTER_DEVICE),
#else
	m_SPIMaster (SPI_CLOCK_SPEED),
#endif
	m_MCP300X (&m_SPIMaster, VREF, SPI_CHIP_SELECT, SPI_CLOCK_SPEED),
	m_CircleSynth (&m_Interrupt, &m_I2CMaster)
{
	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Logger.Initialize (&m_Screen);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_I2CMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_USBHCI.Initialize ();
	}

	if (bOK)
	{
		bOK = m_SPIMaster.Initialize ();
	}

	if (bOK)
	{
		bOK = m_CircleSynth.Initialize ();
	}

	return bOK;
}

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);
	
	m_Logger.Write (FromKernel, LogNotice, "Just play!");

	m_CircleSynth.Start ();

	for (unsigned nCount = 0; m_CircleSynth.IsActive (); nCount++)
	{
		// This must be called from TASK_LEVEL to update the tree of connected USB devices.
		boolean bUpdated = m_USBHCI.UpdatePlugAndPlay ();

		// SPI/IO handling
		// Assumes consecutive pots used by the Synth on the first channels of the MCP300X
		for (int potCh=0; potCh<SYNTH_POTS; potCh++)
		{
			unsigned nResult = m_MCP300X.DoSingleEndedConversionRaw (potCh);
			if (nResult >= 0)
			{
				m_CircleSynth.SetPot(potCh, nResult);
			}
		}

		m_Timer.MsDelay (1000);
		m_CircleSynth.Process (bUpdated);

		m_Screen.Rotor (0, nCount);
	}

	return ShutdownHalt;
}
