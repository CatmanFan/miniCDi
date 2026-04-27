#ifndef MINICDI_MCD212
#define MINICDI_MCD212

#include "cdi/common.hpp"

class MCD212
{
	M68kCpu *mpu;
	MiniCDIConfig* emuConfig;
	VideoCDI::Video video;
	int ns;

	size_t linesV, line;
	uint64_t frames;

	uint8_t* DRAM;

	// internal registers
	uint8_t* CSR1R;
	uint8_t* CSR1W;
	uint8_t* CSR2R;
	uint8_t* CSR2W;
	uint8_t* DCR[2];
	uint8_t* DDR[2];
	uint8_t* VSR[2];
	uint8_t* DCP[2];

	// bits (components) of internal registers
	uint8_t DA,			/** (Display Active)	1 = fetching information from video memory **/
			PA,			/** (Parity)			0 = even frame; 1 = odd frame (interlaced mode only) **/
			DD,			/** (DTACK Delay)		1 = DTACK active **/
			DD1, DD2,	/**	selects DTack type **/
			TD,			/** (Type DRAM)			0 = 256KB * 4 or 256KB * 16; 1 = 1MB * 4 **/
			ST,			/**	(Standard)			1 = screen height -= 40 in PAL, screen width shifted to 360 in NTSC or bitmap width made to 384 in PAL **/
			BE[2],		/** (Bus Error) **/
			DE,			/** (Display Enable)	1 = DRAM display access and synchronisation output **/
			CF,			/** (Crystal Frequency)	0 = PAL (28MHz); 1 = NTSC (30MHz) **/
			FD,			/** (Frame Duration)	0 = 50fps; 1 = 60fps **/
			SM,			/** (Scan Mode)			0 = non-interlaced; 1 = interlaced **/
			CM1, CM2,	/** (Color Mode)		0 = 8bpp & CLK/4; 1 = 4bpp & CLK/2 **/
			IC1, IC2,	/** (ICA) 0 = corresponding ICA off, 1 = corresponding ICA on **/
			DC1, DC2,	/** (DCA) 0 = corresponding DCA off, 1 = corresponding DCA on **/
			IT1, IT2,	/** (Interrupt) **/
			DI1, DI2,	/** (Disable Interrupts) **/
			MF1[2],		/** (Mosaic Factor) separate for each channel **/
			MF2[2],
			FT1[2],		/** (File Type) separate for each channel **/
			FT2[2];

	void IR_disassemble()
	{
		PA = (READ16(CSR1R, 0) >> 5) & 0b01u;
		DA = (READ16(CSR1R, 0) >> 7) & 0b01u;
		BE[0] = READ16(CSR2R, 0) & 0b01u;
		IT1 = (READ16(CSR2R, 0) >> 2) & 0b01u;
		IT2 = (READ16(CSR2R, 0) >> 1) & 0b01u;
		BE[1] = READ16(CSR1W, 0) & 0b01u;
		ST = (READ16(CSR1W, 0) >> 1) & 0b01u;
		DD = (READ16(CSR1W, 0) >> 3) & 0b01u;
		TD = (READ16(CSR1W, 0) >> 5) & 0b01u;
		DD2 = (READ16(CSR1W, 0) >> 8) & 0b01u;
		DD1 = (READ16(CSR1W, 0) >> 9) & 0b01u;
		DI1 = (READ16(CSR1W, 0) >> 15) & 0b01u;
		DI2 = (READ16(CSR2W, 0) >> 15) & 0b01u;
		DC1 = (READ16(DCR[0], 0) >> 7) & 0b01u;
		DC2 = (READ16(DCR[1], 0) >> 7) & 0b01u;
		IC1 = (READ16(DCR[0], 0) >> 8) & 0b01u;
		IC2 = (READ16(DCR[1], 0) >> 8) & 0b01u;
		CM1 = (READ16(DCR[0], 0) >> 10) & 0b01u;
		CM2 = (READ16(DCR[1], 0) >> 10) & 0b01u;
		SM = (READ16(DCR[0], 0) >> 12) & 0b01u;
		FD = (READ16(DCR[0], 0) >> 13) & 0b01u;
		CF = (READ16(DCR[0], 0) >> 14) & 0b01u;
		DE = (READ16(DCR[0], 0) >> 15) & 0b01u;
		FT2[0] = (READ16(DDR[0], 0) >> 7) & 0b01u;
		FT1[0] = (READ16(DDR[0], 0) >> 8) & 0b01u;
		MF2[0] = (READ16(DDR[0], 0) >> 9) & 0b01u;
		MF1[0] = (READ16(DDR[0], 0) >> 10) & 0b01u;
		FT2[1] = (READ16(DDR[1], 0) >> 7) & 0b01u;
		FT1[1] = (READ16(DDR[1], 0) >> 8) & 0b01u;
		MF2[1] = (READ16(DDR[1], 0) >> 9) & 0b01u;
		MF1[1] = (READ16(DDR[1], 0) >> 10) & 0b01u;
	}

	void IR_reassemble()
	{
		WRITE16(CSR1R, 0, (PA << 5) | (DA << 7));
		WRITE16(CSR2R, 0, BE[0] | (IT2 << 1) | (IT1 << 2));
		WRITE16(CSR1W, 0, BE[1] | (ST << 1) | (DD << 3) | (TD << 5) | (DD2 << 8) | (DD1 << 9) | (DI1 << 15));
		WRITE16(CSR2W, 0, (DI2 << 15));
		WRITE16(DCR[0], 0, (DC1 << 7) | (IC1 << 8) | (CM1 << 10) | (SM << 12) | (FD << 13) | (CF << 14) | (DE << 15));
		WRITE16(DCR[1], 0, (DC2 << 7) | (IC2 << 8) | (CM2 << 10));
		WRITE16(DDR[0], 0, (FT2[0] << 7) | (FT1[0] << 8) | (MF2[0] << 9) | (MF1[0] << 10));
		WRITE16(DDR[1], 0, (FT2[1] << 7) | (FT1[1] << 8) | (MF2[1] << 9) | (MF1[1] << 10));
	}

	void vsr_set(size_t path, uint32_t value)
	{
		WRITE16(DCR[path], 0, (READ16(DCR[path], 0) & 0xFB00) | ((value >> 10) & 0x3F));
		WRITE16(VSR[path], 0, value);
	}

	void dcp_set(size_t path, uint32_t value)
	{
		WRITE16(DDR[path], 0, (READ16(DDR[path], 0) & 0x0F00) | ((value >> 10) & 0x3F));
		WRITE16(DCP[path], 0, value);
	}

	void ica_execute(size_t path)
	{
		uint32_t addr = SM && !PA ? 0x404 : 0x400;
		if (path == 1) addr += (512*1024);

		for (int cycles = 0; cycles < (CF ? 120 : 112); cycles++)
		{
			uint32_t inst = READ32(DRAM, addr);
			addr += 4;

			switch ((inst & 0xFF000000) >> 24)
			{
				case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
				case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: // STOP
					//printf("[ICA%d] stop\n", path+1);
					return;

				case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
				case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: // NOP
					break;

				case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
				case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: // RELOAD DCP
					dcp_set(path, inst & 0x003FFFFCu);
					//printf("[ICA%d] dcr $%x\n", path+1, inst & 0x003FFFFCu);
					break;

				case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
				case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f: // RELOAD DCP + STOP
					dcp_set(path, inst & 0x003FFFFCu);
					//printf("[ICA%d] dcr_stop $%x\n", path+1, inst & 0x003FFFFCu);
					return;

				case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
				case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: // RELOAD VCR
					addr = (inst & 0x003FFFFFu);
					//printf("[ICA%d] vcr $%x\n", path+1, inst & 0x003FFFFFu);
					break;

				case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
				case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // RELOAD VCR + STOP
					vsr_set(path, inst & 0x003FFFFFu);
					//printf("[ICA%d] vcr_stop $%x\n", path+1, inst & 0x003FFFFFu);
					return;

				case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
				case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // INTERRUPT
					if (path)
					{
						IT2 = 1;
						if (!DI2)
							m68k_set_irq(mpu, 3);
					}
					else
					{
						IT1 = 1;
						if (!DI1)
							m68k_set_irq(mpu, 3);
					}
					break;

				default:
					video.process(inst, path);
					break;
			}
		}
	}

	void dca_execute(size_t path)
	{
		uint32_t addr = SM && !PA ? 0x404 : 0x400;
		uint32_t inst = READ32(DRAM, addr);
		addr += 4;

		switch ((inst & 0xFF000000) >> 24)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: // STOP
				//printf("[DCA%d] stop\n", path+1);
				return;

			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: // NOP
				break;

			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: // RELOAD DCP
				break;

			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f: // RELOAD DCP + STOP
				dcp_set(path, inst & 0x003FFFFCu);
				//printf("[DCA%d] dcr_stop $%x\n", path+1, inst & 0x003FFFFCu);
				return;

			case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
			case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: // RELOAD VCR
				vsr_set(path, inst & 0x003FFFFFu);
				//printf("[DCA%d] vcr $%x\n", path+1, inst & 0x003FFFFFu);
				break;

			case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
			case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // RELOAD VCR + STOP
				vsr_set(path, inst & 0x003FFFFFu);
				//printf("[DCA%d] vcr_stop $%x\n", path+1, inst & 0x003FFFFFu);
				return;

			case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
			case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // INTERRUPT
				if (path) { IT2 = !IT2; }
				else	  { IT1 = !IT1; }
				break;

			default:
				video.process(inst, path);
				break;
		}
	}

	/** Draws a video line **/
	void execute()
	{
		// Disassemble internal register values
		IR_disassemble();

		/*******************************************************/
		if (linesV++ <= (FD ? 262 : 312))
		{
			if (linesV == 1 && DE)
			{
				if (IC1)
					ica_execute(0);
				if (IC2)
					ica_execute(1);
			}
		}
		else
		{
			DA = 1;
			PA = SM ? (frames % 2 == 0 ? 0 : 1) : 1;

			if (line == 0) {
				video.set_mode(CF == 1 ? VideoCDI::NTSCTV : VideoCDI::PAL, VideoCDI::NormalRes);
				if (frames % 2 == 0 && SM)
					line = 1;
			}

			if (DE)
			{
				uint32_t vsr1 = (READ16(DCR[0], 0) << 16) | READ16(VSR[0], 0);
				uint32_t vsr2 = (READ16(DCR[1], 0) << 16) | READ16(VSR[1], 0);

				// render line onto bitmap
				vsr_set(0, vsr1 + video.FG[0].width);
				vsr_set(1, vsr2 + video.FG[0].height);

				for (size_t period = 0; period < (CF ? 16 : 8); period++)
				{
					if (IC1 && DC1)
						dca_execute(0);
					if (IC2 && DC2)
						dca_execute(1);
				}
			}

			if (linesV >= (FD ? 262 : 312))
			{
				// add cursor to video?

				printf("\x1b[%d;%dH", 11, 0);
				printf("VSync %lld\n", frames);
				DA = 0;
				frames++;
				linesV = 0;
				line = 0;
			}
		}
		/*******************************************************/

		// Reassemble to internal registers
		IR_reassemble();

		/*printf("\n[VDSC viewer]\n");
		printf("CSR1R %4x  Display Active: %d; Parity: %d;\n", READ16(CSR1R, 0), DA, PA);
		printf("CSR2R %4x  Interrupt 1: %s; Interrupt 2: %s; Bus Error: %s;\n", READ16(CSR2R, 0), IT1 ? "yes" : "no", IT2 ? "yes" : "no", BE[0] ? "yes" : "no");
		printf("CSR1W %4x  Disable Interrupts 1: %s; Bus Error: %s\n", READ16(CSR1W, 0), DI1 ? "yes" : "no", BE[1] ? "enabled" : "disabled");
		printf("CSR2W %4x  Disable Interrupts 2: %s;\n", READ16(CSR2W, 0), DI2 ? "yes" : "no");*/
	}

public:
	void init(M68kCpu *mpu, uint8_t *memory, size_t start, MiniCDIConfig *config)
	{
		this->mpu = mpu;
		emuConfig = config;
		ns = 0;

		DRAM = memory;

		CSR1R = memory + (start + 0x11);
		CSR1W = memory + (start + 0x10);
		DCR[0] = memory + (start + 0x12);
		VSR[0] = memory + (start + 0x14);
		DDR[0] = memory + (start + 0x18);
		DCP[0] = memory + (start + 0x1A);

		CSR2R = memory + (start + 0x01);
		CSR2W = memory + (start + 0x00);
		DCR[1] = memory + (start + 0x02);
		VSR[1] = memory + (start + 0x04);
		DDR[1] = memory + (start + 0x08);
		DCP[1] = memory + (start + 0x0A);
	}

	void reset()
	{
		IR_disassemble();
		// clear bits
		DI1 = 0; DD1 = 0; DD2 = 0; TD = 0; DD = 0; ST = 0; BE[0] = 0; BE[1] = 0;
		DI2 = 0;
		DE = 0; CF = 0; FD = 0; SM = 0; CM1 = 0; IC1 = 0; DC1 = 0;
		CM2 = 0; IC2 = 0; DC2 = 0;
		MF1[0] = 0; MF2[0] = 0; FT1[0] = 0; FT2[0] = 0;
		MF1[1] = 0; MF2[1] = 0; FT1[1] = 0; FT2[1] = 0;

		// initialization
		CF = FD = emuConfig->pal ? 0 : 1;
		SM = /* to-do: interlace */ 0;
		DE = 1;
		IC1 = 1;
		IC2 = 1;
		DC1 = 1;
		DC2 = 1;
		IR_reassemble();

		frames = 0;
		linesV = 0;
		line = 0;

		video.reset();
	}

	void increment_time(int ns)
	{
		this->ns += ns;
		if (this->ns >= (emuConfig->pal || !CF ? 64000 : 63560))
		{
			execute();
			this->ns -= (emuConfig->pal || !CF ? 64000 : 63560);
		}
	}

	uint32_t* get_display()
	{
		return video.get_display();
	}

	size_t get_display_width()
	{
		return video.get_display_width();
	}

	bool check_vsync()
	{
		return DA == 0;
	}
};

#endif