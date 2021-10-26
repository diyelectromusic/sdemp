//
// Simple DIY Electronic Music Projects
//    diyelectromusic.wordpress.com
//
// Simple Raspberry Pi Bare Metal Synthesizer Experiments
//
// Modifications and updates:
//    Copyright (C) 2021 Kevin (diyelectromusic)
//
// kernel.h
//    Based on sample 29-miniorgan/kernel.h
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
#ifndef _kernel_h
#define _kernel_h

//#define USE_SPI_MASTER_AUX		// use SPI1 (Auxiliary SPI master)

#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/types.h>
#include <circle/i2cmaster.h>
#include <circle/usb/usbhcidevice.h>
#ifndef USE_SPI_MASTER_AUX
	#include <circle/spimaster.h>
#else
	#include <circle/spimasteraux.h>
#endif
#include <sensor/mcp300x.h>
#include "circlesynth.h"

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);

	TShutdownMode Run (void);

private:
	// do not change this order
	CActLED			m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	CScreenDevice		m_Screen;
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer			m_Timer;
	CLogger			m_Logger;
	CI2CMaster		m_I2CMaster;
	CUSBHCIDevice		m_USBHCI;

#ifndef USE_SPI_MASTER_AUX
	CSPIMaster		m_SPIMaster;
#else
	CSPIMasterAUX		m_SPIMaster;
#endif

	CMCP300X		m_MCP300X;

	CCircleSynth		m_CircleSynth;
};

#endif
