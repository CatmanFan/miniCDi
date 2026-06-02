#ifndef MINICDI_MCD212
#define MINICDI_MCD212

/*****
  DISCLAIMER:
  Sourced partially from official documentation of MCD212 by Motorola, as
  well as the specifications of the Green Book.
 *****/

#define MCD212_VSYNC_LINES		(FD ? 262 : 312)
#define MCD212_HSYNC_CYCLES		(CF ? 120 : 112)
#define MCD212_INACTIVE_VLINES	(FD ? 262 - 240 : ST ? 312 - 240 : 312 - 280)

class MCD212
{
	SCC68070 *cpu;
	uint8_t* memory;

	VDSC::Decoder vdsc;

	size_t linesV, line;
	bool interlace;

	// internal registers
	uint32_t VSR[2];
	uint32_t DCP[2];

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
			CM[2],		/** (Color Mode)		0 = 8bpp & CLK/4 pixel output; 1 = 4bpp & CLK/2 pixel output **/
			IC[2],		/** (ICA) 0 = corresponding ICA off, 1 = corresponding ICA on **/
			DC[2],		/** (DCA) 0 = corresponding DCA off, 1 = corresponding DCA on **/
			IT[2],		/** (Interrupt) **/
			DI[2],		/** (Disable Interrupts) **/
			MF1[2],		/** (Mosaic Factor) separate for each channel **/
			MF2[2],
			FT1[2],		/** (File Type) separate for each channel **/
			FT2[2];

	template <size_t Path>
	void vsr_set(uint32_t value) {
		VSR[Path] = value & 0x003FFFFFu;
		IC[Path] = 1;
	}

	template <size_t Path>
	void dcp_set(uint32_t value) {
		DCP[Path] = value & 0x003FFFFCu;
		DC[Path] = 1;
	}

	template <size_t Path>
	void ICA_execute()
	{
		uint32_t addr = SM && !PA ? (Path ? 0x200404 : 0x404) : (Path ? 0x200400 : 0x400);

		for (int i = 0; i < MCD212_HSYNC_CYCLES * MCD212_INACTIVE_VLINES; i++)
		{
			uint32_t inst = (memory[addr] << 24) | (memory[addr+1] << 16) | (memory[addr+2] << 8) | memory[addr+3];
			addr += 4;

			switch (inst >> 24 & 0xFF)
			{
				case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
				case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: // STOP
					//MiniCDI::Log("[ICA%d] stop", Path+1);
					return;

				case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
				case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: // NOP
					break;

				case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
				case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: // RELOAD DCP
					//MiniCDI::Log("[ICA%d] dcr $%x", Path+1, inst & 0x003FFFFCu);
					dcp_set<Path>(inst);
					break;

				case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
				case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f: // RELOAD DCP + STOP
					//MiniCDI::Log("[ICA%d] dcr_stop $%x", Path+1, inst & 0x003FFFFCu);
					dcp_set<Path>(inst);
					return;

				case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
				case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: // RELOAD VCR
					//MiniCDI::Log("[ICA%d] vcr $%x", Path+1, inst & 0x003FFFFFu);
					addr = inst & 0x003FFFFFu;
					break;

				case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
				case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // RELOAD VCR + STOP
					//MiniCDI::Log("[ICA%d] vcr_stop $%x", Path+1, inst & 0x003FFFFFu);
					vsr_set<Path>(inst);
					return;

				case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
				case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // INTERRUPT
					IT[Path] = 1;
					if (!DI[Path]) { cpu->interrupt(Path); }
					break;

				case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: // RELOAD DISPLAY PARAMETERS
					CM[Path] = inst >> 4 & 0b01u;
					MF1[Path] = inst >> 3 & 0b01u;
					MF2[Path] = inst >> 2 & 0b01u;
					FT1[Path] = inst >> 1 & 0b01u;
					FT2[Path] = inst & 0b01u;
					vdsc.set_register<Path>(inst);
					break;

				default:
					vdsc.set_register<Path>(inst);
					break;
			}
		}
	}

	template <size_t Path>
	void DCA_execute()
	{
		for (int i = 0; i < (CF ? 16 : 8); i++)
		{
			uint32_t inst = (memory[DCP[Path]] << 24) | (memory[DCP[Path]+1] << 16) | (memory[DCP[Path]+2] << 8) | memory[DCP[Path]+3];
			DCP[Path] += 4;

			switch (inst >> 24 & 0xFF)
			{
				case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
				case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: // STOP
					//MiniCDI::Log("[DCA%d] stop", Path+1);
					return;

				case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
				case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: // NOP
					break;

				case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
				case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: // RELOAD DCP
					break;

				case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
				case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f: // RELOAD DCP + STOP
					//MiniCDI::Log("[DCA%d] dcr_stop $%x", Path+1, inst & 0x003FFFFCu);
					dcp_set<Path>(inst);
					return;

				case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
				case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: // RELOAD VCR
					//MiniCDI::Log("[DCA%d] vcr $%x", Path+1, inst & 0x003FFFFFu);
					vsr_set<Path>(inst);
					break;

				case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
				case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // RELOAD VCR + STOP
					//MiniCDI::Log("[DCA%d] vcr_stop $%x", Path+1, inst & 0x003FFFFFu);
					vsr_set<Path>(inst);
					return;

				case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
				case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // INTERRUPT
					IT[Path] = 1;
					if (!DI[Path]) { cpu->interrupt(Path); }
					break;

				default:
					vdsc.set_register<Path>(inst);
					break;
			}
		}
	}

public:
	MCD212() {}

	MCD212(SCC68070 *cpu, uint8_t *memory) : cpu(cpu), memory(memory)
	{
		reset();
	}

	/**
	 * @brief  Resets the chip.
	 */
	void reset()
	{
		// clear write bits
		DI[0] = DD1 = DD2 = TD = DD = ST = BE[0] = BE[1] = 0;
		DI[1] = 0;
		DE = CF = FD = SM = CM[0] = IC[0] = DC[0] = 0;
		CM[1] = IC[1] = DC[1] = 0;
		MF1[0] = MF2[0] = FT1[0] = FT2[0] = 0;
		MF1[1] = MF2[1] = FT1[1] = FT2[1] = 0;

		// initialization
		CF = FD = MiniCDI::Config::PAL ? 0 : 1;
		SM = /* to-do: interlace */ 0;

		interlace = false;
		linesV = 0;
		line = 0;

		vdsc.reset();
	}

	/**
	 * @brief  Draws a video line.
	 */
	bool tick(bool skip_draw = false)
	{
		if (linesV++ <= MCD212_INACTIVE_VLINES) {
			if (linesV == 1 && DE) {
				if (IC[0]) ICA_execute<0>();
				if (IC[1]) ICA_execute<1>();
			}
			return false;
		}

		DA = 1;
		PA = SM ? (interlace ? 0 : 1) : 1;

		if (line == 0) {
			vdsc.set_mode(VDSC::PAL, CM[0]); // Hardcoded
			if (interlace && SM)
				line = 1;
		}

		if (DE) {
			if (!skip_draw) {
				// render line onto bitmap
				VSR[0] = vdsc.draw_line_to_plane<0>(memory, VSR[0], line);
				VSR[1] = vdsc.draw_line_to_plane<1>(memory, VSR[1], line);
				vdsc.mix_to_frame(line);
			}

			if (DC[0] && IC[0]) DCA_execute<0>();
			if (DC[1] && IC[1]) DCA_execute<1>();
		}

		line += SM ? 2 : 1;

		if (linesV >= MCD212_VSYNC_LINES) {
			DA = 0;
			linesV = 0;
			line = 0;
			interlace = SM ? !interlace : false;

			return true;
		}

		return false;
	}

	uint8_t read8(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return memory[addr];
			case 0x4FFFF0:
			case 0x4FFFF1: // CSR1R
				return (PA << 5) | (DA << 7);
			case 0x4FFFE0:
			case 0x4FFFE1: // CSR2R
				uint8_t value = BE[0] | (IT[1] << 1) | (IT[0] << 2);
				BE[0] = IT[1] = IT[0] = 0;
				return value;
		}
	}

	uint16_t read16(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return (memory[addr] << 8) | memory[addr+1];
			case 0x4FFFF0: // CSR1R
				return 0xFF00 | (PA << 5) | (DA << 7);
			case 0x4FFFE0: // CSR2R
				uint8_t value = BE[0] | (IT[1] << 1) | (IT[0] << 2);
				BE[0] = IT[1] = IT[0] = 0;
				return 0xFF00 | value;
		}
	}

	void write16(uint32_t addr, uint16_t value)
	{
		switch (addr)
		{
			case 0x4FFFF0: // CSR1W
				BE[1] = value & 0b01u;
				ST = value >> 1 & 0b01u;
				DD = value >> 3 & 0b01u;
				TD = value >> 5 & 0b01u;
				DD2 = value >> 8 & 0b01u;
				DD1 = value >> 9 & 0b01u;
				DI[0] = value >> 15 & 0b01u;
				break;
			case 0x4FFFE0: // CSR2W
				DI[1] = value >> 15 & 0b01u;
				break;
			case 0x4FFFF2: // DCR1
				IC[0] = value >> 8 & 0b01u;
				DC[0] = IC[0] ? value >> 7 & 0b01u : 0;
				CM[0] = value >> 10 & 0b01u;
				SM = value >> 12 & 0b01u;
				FD = value >> 13 & 0b01u;
				CF = value >> 14 & 0b01u;
				DE = value >> 15 & 0b01u;

				VSR[0] &= 0x0000FFFFu;
				VSR[0] |= (value & 0x3Fu) << 8;
				break;
			case 0x4FFFE2: // DCR2
				IC[1] = value >> 8 & 0b01u;
				DC[1] = IC[1] ? value >> 7 & 0b01u : 0;
				CM[1] = value >> 10 & 0b01u;

				VSR[1] &= 0x0000FFFFu;
				VSR[1] |= (value & 0x3Fu) << 8;
				break;
			case 0x4FFFF4: // VSR1
				VSR[0] &= 0xFFFF00000u;
				VSR[0] |= value;
				break;
			case 0x4FFFE4: // VSR2
				VSR[1] &= 0xFFFF00000u;
				VSR[1] |= value;
				break;
			case 0x4FFFF8: // DDR1
				FT2[0] = value >> 7 & 0b01u;
				FT1[0] = value >> 8 & 0b01u;
				MF2[0] = value >> 9 & 0b01u;
				MF1[0] = value >> 10 & 0b01u;

				DCP[0] &= 0x0000FFFFu;
				DCP[0] |= (value & 0x3Fu) << 8;
				break;
			case 0x4FFFE8: // DDR2
				FT2[1] = value >> 7 & 0b01u;
				FT1[1] = value >> 8 & 0b01u;
				MF2[1] = value >> 9 & 0b01u;
				MF1[1] = value >> 10 & 0b01u;

				DCP[1] &= 0x0000FFFFu;
				DCP[1] |= (value & 0x3Fu) << 8;
				break;
			case 0x4FFFFA: // DCP1
				DCP[0] &= 0xFFFF00000u;
				DCP[0] |= value;
				break;
			case 0x4FFFEA: // DCP2
				DCP[1] &= 0xFFFF00000u;
				DCP[1] |= value;
				break;
		}
	}

	uint32_t* get_display()
	{
		return vdsc.get_display();
	}

	size_t get_display_width()
	{
		return vdsc.get_display_width();
	}
};

#endif