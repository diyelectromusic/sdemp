#include "PicoDVIArduino.h"
#include "ZxSpectrumDisplay.h"
#include "ZxSpectrumPrepareDviScanline.h"
//#include "int50hz-pio.h"

//------------------------------
// Optional 50Hz /INT Output
// (must be via 74HCT14)
//------------------------------
//#define Z80_INT_50HZ

//#define Z80_INT_GPIO 37
#define Z80_INT_GPIO 33
#define Z80_INT_PIO  pio1
#define Z80_INT_SM   2

//------------------------------
// 64K RAM address space copy
//------------------------------
#define RAMSIZE (64*1024)  // 64K
uint8_t ram[RAMSIZE];

void int50hz_setup() {
#ifdef Z80_INT_50HZ
  if (Z80_INT_GPIO > 31) {
    pio_set_gpio_base(Z80_INT_PIO, 16);
  }
  uint offset = pio_add_program(Z80_INT_PIO, &int50hz_program);
  int50hz_program_init(Z80_INT_PIO, Z80_INT_SM, offset, Z80_INT_GPIO);
  pio_sm_put_blocking(Z80_INT_PIO, Z80_INT_SM, INT50HZ_LOOP);
  pio_sm_set_enabled(Z80_INT_PIO, Z80_INT_SM, true);
#endif
}

//------------------------------
// DVI/HDMI
//------------------------------

// TMDS bit clock 252 MHz
// DVDD 1.2V (1.1V seems ok too)
#define VREG_VSEL VREG_VOLTAGE_1_20
#define DVI_TIMING dvi_timing_640x480p_60hz

struct dvi_inst dvi0;

#define ZXS_DISP_SIZE 0x1800  // 6144
#define ZXS_DISP_ADDR 0x4000  // 16384
#define ZXS_ATTR_SIZE 0x0300  // 768
#define ZXS_ATTR_ADDR 0x5800  // 22528

uint8_t borderColour = 0;

static const struct dvi_serialiser_cfg pga2350_video_cfg = {
  .pio = DVI_DEFAULT_PIO_INST,
  .sm_tmds = {0, 1, 2},
  .pins_tmds = {40, 46, 44},
  .pins_clk = 42,
  .invert_diffpairs = false
};

//------------------------------
//  Video Output Details
//  Runs on core 1
//  Uses GPIO 40-45
//------------------------------

uint frames;

void setup() {
  pio_set_gpio_base(DVI_DEFAULT_PIO_INST, 16);
  vreg_set_voltage(VREG_VSEL);
  sleep_ms(10);

  // Run system at TMDS bit clock
  set_sys_clock_khz(DVI_TIMING.bit_clk_khz, true);

  dvi0.timing = &DVI_TIMING;
  dvi0.ser_cfg = pga2350_video_cfg;
  dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

  dvi_register_irqs_this_core(&dvi0, DMA_IRQ_1);
  dvi_start(&dvi0);

  frames=0;
}

void loop() {
  uint8_t *screenPtr = &ram[ZXS_DISP_ADDR];
  uint8_t *attrPtr = &ram[ZXS_ATTR_ADDR];

	for(int i = 0; i < DISPLAY_BLANK_LINES/2; ++i) {
		// NB: These are blank lines prior to the ZX Spectrum screen itself starting
		//     These are all passed in as "y=0" and will ignore the actual screen
		//     contents and just show the border colour.
	  zx_prepare_hdmi_scanline(&dvi0, 0, frames, screenPtr, attrPtr, 0);
	}

	for (uint y=0; y<ZX_SPECTRUM_SCREEN_HEIGHT; y++) {
		// Work through the actual ZX Spectrum display, including its
		// own view of what the screen border should be doing.
    zx_prepare_hdmi_scanline(&dvi0, y, frames, screenPtr, attrPtr, borderColour);
	}

	for(int i = 0; i < DISPLAY_BLANK_LINES/2; ++i) {
		// NB: These are residual blank lines after the ZX Spectrum screen
		//     These are all passed in as "y=239"
			zx_prepare_hdmi_scanline(&dvi0, 239, frames, screenPtr, attrPtr, 0);
	}

  frames++;
}

//------------------------------
//  Memory Monitor
//  Runs on core 0
//  Uses GPIO 0 to 31
//------------------------------

// GPIO Base 0 Definitions
// Start from GPIO 0 up to GPIO 31
//
#define GP_ADDR_START 0ul
#define GP_ADDR_PINS  16ul
#define GP_ADDR_MASK (0xFFFFul<<GP_ADDR_START)
// Display info is in the range 0x4000-0x5800
// So can look for addresses where:
//   A15 == 0   0x8000
//   A14 == 1   0x4000
//   A13 == 0   0x2000
//   A12 == 0/1 0x1000
#define GP_A15       (15ul+GP_ADDR_START)
#define GP_A14       (14ul+GP_ADDR_START)
#define GP_A13       (13ul+GP_ADDR_START)
#define GP_DISP_MASK ((1ul<<GP_A15)|(1ul<<GP_A14)|(1ul<<GP_A13))
#define GP_DISP_VAL  ((0ul<<GP_A15)|(1ul<<GP_A14)|(0ul<<GP_A13))

#define GP_DATA_START 16ul
#define GP_DATA_PINS  8ul
#define GP_DATA_MASK (0xFFul<<GP_DATA_START)

#define GP_RD 24ul
#define GP_WR 25ul
#define GP_M1 28ul
#define GP_RFSH 29ul  // Not connected on RC2014
#define GP_MREQ 30ul
#define GP_IORQ 31ul
#define GP_CTRL_MASK ((1ul<<GP_RD)|(1ul<<GP_WR)|(1ul<<GP_M1)|(1ul<<GP_MREQ)|(1ul<<GP_IORQ))
//#define GP_CTRL_MASK ((1ul<<GP_WR)|(1ul<<GP_MREQ))
#define GP_CTRL_PINS 6ul
//#define GP_CTRL_PINS 2ul
int ctrlPins[GP_CTRL_PINS] = {GP_RD, GP_WR, GP_M1, GP_MREQ, GP_IORQ};
//int ctrlPins[GP_CTRL_PINS] = {GP_WR, GP_MREQ};
int ctrlOver[GP_CTRL_PINS] = {
  GPIO_OVERRIDE_INVERT,  // /RD = HIGH
  GPIO_OVERRIDE_NORMAL,  // /WR = LOW
  GPIO_OVERRIDE_INVERT,  // /M1 = HIGH
//  GPIO_OVERRIDE_INVERT,  // /RFSH = HIGH
  GPIO_OVERRIDE_NORMAL,  // /MREQ = LOW
  GPIO_OVERRIDE_INVERT,  // /IORQ = HIGH
};

// Assumes all GPIO below 32 for the moment
#define GP_BASE0_MASK (GP_ADDR_MASK | GP_DATA_MASK | GP_CTRL_MASK)

void setup1() {
  for (int i=0; i<RAMSIZE; i++) {
    ram[i] = 0x00;
  }

  // Now initialise the memory reader
  // Pins in range lower than 32 can be initialised at once
  gpio_init_mask(GP_BASE0_MASK);
  gpio_set_dir_in_masked(GP_BASE0_MASK);

  // Set the GPIO properties so that all desired control signals
  // will read as zero when in the right state
  for (int i=0; i<GP_CTRL_PINS; i++) {
    gpio_set_inover(ctrlPins[i], ctrlOver[i]);
  }
}

void loop1() {
  while (1) {
    uint32_t gpio32 = gpio_get_all();

    // All GPIO ctrl pins will read as 0 for a legit write
    if ((gpio32 & GP_CTRL_MASK) == 0)
    {
      if ((gpio32 & GP_DISP_MASK) == GP_DISP_VAL) {
        // Grab the data off the bus and update RAM
        uint16_t addr16 = (GP_ADDR_MASK & gpio32) >> GP_ADDR_START;
        uint8_t data8 = (GP_DATA_MASK & gpio32) >> GP_DATA_START;
        ram[addr16] = data8;
      }
    }
  }
}
