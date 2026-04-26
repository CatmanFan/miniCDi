#ifndef MINICDI_MCD212
#define MINICDI_MCD212

#include "cdi/common.hpp"

/*** SUMMARY:
	The MCD212 handles the Display Control Program (DCP), which is a set of instructions executed on the VSync (Field Control Tables/FCT) or HSync (Line Control Tables/LCT).
	The hardware tables for these shall be respectively known as the Image Control Area (ICA) and Display Control Area (DCA). The video driver also maintains a "shadow FCT" for each plane.
 ***/

/* Main class */
class MCD212
{
	MiniCDIConfig* emuConfig;
	VideoCDI::Video video;

	/** IO **/
	uint32_t addr_ica[2]; // vsync
	uint32_t addr_dca[2]; // hsync
	uint32_t ica[2];
	uint32_t dca[2];

	uint8_t* ramBank1;
	uint8_t* ramBank2;

	// channel 1
	uint8_t* CSR1R;
	uint8_t* CSR1W;
	uint8_t* DCR1;
	uint8_t* DDR1;

	// channel 2
	uint8_t* CSR2R;
	uint8_t* CSR2W;
	uint8_t* DCR2;
	uint8_t* DDR2;

	// channels 1+2
	uint8_t* VSR[2];
	uint8_t* DCP[2];

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
 
	void ica_step(M68kCpu *cpu, size_t i)
	{
		ica[i] = READ32(i == 1 ? ramBank2 : ramBank1, addr_ica[i]);
		addr_ica[i] += 4;

		switch ((ica[i] & 0xFF000000) >> 24)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: // STOP
				//printf("[ICA%d] stop\n", i + 1);
				return;

			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: // NOP
				break;

			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: // RELOAD DCP
				addr_dca[i] = (ica[i] & 0x003FFFFCu);
				WRITE32(DCP[i], 0, addr_dca[i]);
				//printf("[ICA%d] dcr $%x\n", i + 1, addr_dca[i]);
				break;

			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f: // RELOAD DCP + STOP
				addr_dca[i] = (ica[i] & 0x003FFFFCu);
				WRITE32(DCP[i], 0, addr_dca[i]);
				//printf("[ICA%d] dcr_stop $%x\n", i + 1, addr_dca[i]);
				return;

			case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
			case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: // RELOAD VCR
				addr_ica[i] = (ica[i] & 0x003FFFFFu);
				//printf("[ICA%d] vcr $%x\n", i + 1, addr_ica[i]);
				break;

			case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
			case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // RELOAD VCR + STOP
				addr_ica[i] = (ica[i] & 0x003FFFFFu);
				WRITE32(VSR[i], 0, addr_ica[i]);
				//printf("[ICA%d] vcr_stop $%x\n", i + 1, addr_ica[i]);
				return;

			case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
			case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // INTERRUPT
				IT1 = !IT1;
				m68k_set_irq(cpu, 3);
				break;

			default:
				video.process(ica[i], i);
				break;
		}
	}

	void dca_cycles(size_t i)
	{
		for (size_t cycles = 0; cycles < (CF ? 16 : 8); cycles++)
		{
			dca[i] = READ32(i == 1 ? ramBank2 : ramBank1, addr_dca[i]);
			addr_dca[i] += 4;

			switch ((dca[i] & 0xFF000000) >> 24)
			{
				case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
				case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: // STOP
					//printf("[DCA%d] stop\n", i + 1);
					return;

				case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
				case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: // NOP
					break;

				case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
				case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: // RELOAD DCP
					break;

				case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
				case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f: // RELOAD DCP + STOP
					addr_dca[i] = (dca[i] & 0x003FFFFCu);
					WRITE32(DCP[i], 0, addr_dca[i]);
					//printf("[DCA%d] dcr_stop $%x\n", i + 1, addr_dca[i]);
					return;

				case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
				case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: // RELOAD VCR
					addr_ica[i] = (dca[i] & 0x003FFFFFu);
					WRITE32(VSR[i], 0, addr_ica[i]);
					//printf("[DCA%d] vcr $%x\n", i + 1, addr_ica[i]);
					break;

				case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
				case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // RELOAD VCR + STOP
					addr_ica[i] = (dca[i] & 0x003FFFFFu);
					WRITE32(VSR[i], 0, addr_ica[i]);
					//printf("[DCA%d] vcr_stop $%x\n", i + 1, addr_ica[i]);
					return;

				case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
				case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // INTERRUPT
					IT1 = !IT1;
					break;

				default:
					video.process(dca[i], i);
					break;
			}
		}
	}

public:
	MCD212(uint8_t *memory, size_t start, MiniCDIConfig *config)
	{
		emuConfig = config;
		ramBank1 = memory;
		ramBank2 = memory + (512*1024);

		CSR1R = memory + (start + 0x11);
		CSR1W = memory + (start + 0x10);
		DCR1 = memory + (start + 0x12);
		VSR[0] = memory + (start + 0x14);
		DDR1 = memory + (start + 0x18);
		DCP[0] = memory + (start + 0x1A);

		CSR2R = memory + (start + 0x01);
		CSR2W = memory + (start + 0x00);
		DCR2 = memory + (start + 0x02);
		VSR[1] = memory + (start + 0x04);
		DDR2 = memory + (start + 0x08);
		DCP[1] = memory + (start + 0x0A);
	}

	void reset()
	{
		WRITE16(CSR1W, 0, 0x0000);
		WRITE16(CSR2W, 0, 0x0000);
		WRITE16(DCR1, 0, 0x0000);
		WRITE16(DCR2, 0, 0x0000);
		WRITE16(DDR1, 0, READ16(DDR1, 0) & 0x003F);
		WRITE16(DDR2, 0, 0x0000);

		video.reset();
	}

	void step(M68kCpu *cpu)
	{
		// Disassemble internal register values
		PA = (READ16(CSR1R, 0) >> 5) & 0x01;
		DA = (READ16(CSR1R, 0) >> 7) & 0x01;
		BE[0] = READ16(CSR2R, 0) & 0x01;	
		IT1 = (READ16(CSR2R, 0) >> 2) & 0x01;
		IT2 = (READ16(CSR2R, 0) >> 1) & 0x01;
		BE[1] = READ16(CSR1W, 0) & 0x01;	
		ST = (READ16(CSR1W, 0) >> 1) & 0x01;
		DD = (READ16(CSR1W, 0) >> 3) & 0x01;
		TD = (READ16(CSR1W, 0) >> 5) & 0x01;
		DD2 = (READ16(CSR1W, 0) >> 8) & 0x01;
		DD1 = (READ16(CSR1W, 0) >> 9) & 0x01;
		DI1 = (READ16(CSR1W, 0) >> 15) & 0x01;
		DI2 = (READ16(CSR2W, 0) >> 15) & 0x01;
		DC1 = (READ16(DCR1, 0) >> 7) & 0x01;
		DC2 = (READ16(DCR2, 0) >> 7) & 0x01;
		IC1 = (READ16(DCR1, 0) >> 8) & 0x01;
		IC2 = (READ16(DCR2, 0) >> 8) & 0x01;
		CM1 = (READ16(DCR1, 0) >> 10) & 0x01;
		CM2 = (READ16(DCR2, 0) >> 10) & 0x01;
		SM = (READ16(DCR1, 0) >> 12) & 0x01;
		FD = (READ16(DCR1, 0) >> 13) & 0x01;
		CF = (READ16(DCR1, 0) >> 14) & 0x01;
		DE = (READ16(DCR1, 0) >> 15) & 0x01;
		FT2[0] = (READ16(DDR1, 0) >> 7) & 0x01;
		FT1[0] = (READ16(DDR1, 0) >> 8) & 0x01;
		MF2[0] = (READ16(DDR1, 0) >> 9) & 0x01;
		MF1[0] = (READ16(DDR1, 0) >> 10) & 0x01;
		FT2[1] = (READ16(DDR2, 0) >> 7) & 0x01;
		FT1[1] = (READ16(DDR2, 0) >> 8) & 0x01;
		MF2[1] = (READ16(DDR2, 0) >> 9) & 0x01;
		MF1[1] = (READ16(DDR2, 0) >> 10) & 0x01;

		/*******************************************************/
		addr_ica[1] = addr_ica[0] = SM && !PA ? 0x404 : 0x400;

		for (size_t cycles = 0; cycles < (CF ? 120 : 112); cycles++)
		{
			ica_step(cpu, 0);
			ica_step(cpu, 1);

			video.draw(CF == 1 ? VideoCDI::NTSCTV : VideoCDI::PAL, VideoCDI::NormalRes, cycles);
		}

		// PA = DA ? PA ? 0 : 1 : PA;
		DA = !DA;

		dca_cycles(0);
		dca_cycles(1);
		/*******************************************************/

		// Reassemble to internal registers
		WRITE16(CSR1R, 0, (PA << 5) | (DA << 7));
		WRITE16(CSR2R, 0, BE[0] | (IT2 << 1) | (IT1 << 2));
		WRITE16(CSR1W, 0, BE[1] | (ST << 1) | (DD << 3) | (TD << 5) | (DD2 << 8) | (DD1 << 9) | (DI1 << 15));
		WRITE16(CSR2W, 0, (DI2 << 15));
		WRITE16(DCR1, 0, (DC1 << 7) | (IC1 << 8) | (CM1 << 10) | (SM << 12) | (FD << 13) | (CF << 14) | (DE << 15));
		WRITE16(DCR2, 0, (DC2 << 7) | (IC2 << 8) | (CM2 << 10));
		WRITE16(DDR1, 0, (FT2[0] << 7) | (FT1[0] << 8) | (MF2[0] << 9) | (MF1[0] << 10));
		WRITE16(DDR2, 0, (FT2[1] << 7) | (FT1[1] << 8) | (MF2[1] << 9) | (MF1[1] << 10));

		/*printf("[VDSC viewer]\n");
		printf("ICA1  %8x = %8x    ICA2  %8x = %8x\n", addr_ica[0], ica[0], addr_ica[1], ica[1]);
		printf("DCA1  %8x = %8x    DCA2  %8x = %8x\n", addr_dca[0], dca[0], addr_dca[1], dca[1]);
		printf("CSR1R %4x  Display Active: %d; Parity: %d;\n", READ16(CSR1R, 0), DA, PA);
		printf("CSR2R %4x  Interrupt 1: %s; Interrupt 2: %s; Bus Error: %s;\n", READ16(CSR2R, 0), IT1 ? "yes" : "no", IT2 ? "yes" : "no", BE[0] ? "yes" : "no");
		printf("CSR2W %4x  Disable Interrupts 1: %s;\n", READ16(CSR2W, 0), DI1 ? "yes" : "no");*/
	}

	uint32_t* get_display()
	{
		return video.get_display();
	}

	bool check_vsync()
	{
		return false;
	}
};

#endif