#ifndef MINICDI_SCC68070
#define MINICDI_SCC68070

class SCC68070
{
	uint8_t* memory;

	// On-chip peripherals
	uint8_t LIR;
	uint8_t PICR[2];

	/** UART **/
	uint8_t UMR; // Mode Register
	uint8_t USR; // Status Register
	uint8_t UCS; // Clock Select Register
	uint8_t UCR; // Command Register
	uint8_t UTH; // Transmit Holding Register
	uint8_t URH; // Receive Holding Register

	/** Timer **/
	uint8_t TSR;
	uint8_t TCR;
	uint16_t RR;
	uint16_t T[3];

	/** DMA **/
	struct {
		uint8_t CSR;
		uint8_t CER;

		uint8_t DCR;
		uint8_t OCR;
		uint8_t SCR;
		uint8_t CCR;

		uint16_t MTC;
		uint32_t MAC;
		uint32_t DAC;

		uint8_t CPR;
	} DMA[2];

	/** I²C **/
	uint8_t IDR;
	uint8_t IAR;
	uint8_t ISR;
	uint8_t ICR;
	uint8_t ICCR;

	uint32_t romPC, romSP;

public:
	uint8_t fc; // used for FC/address space callback
	friend class PointingDevice;

	SCC68070(uint8_t* memory) : memory(memory)
	{
	}

	void load_rom(const char* romPath, size_t romAddr)
	{
		std::ifstream romStream(romPath);
		std::vector<char> rom(
		 (std::istreambuf_iterator<char>(romStream)),
		 (std::istreambuf_iterator<char>()));
		romStream.close();

		if (rom[4] != 0x00) { // byteswap
			for (size_t i = 0; i < rom.size(); i+=2) {
				uint8_t byte = rom[i];
				rom[i] = rom[i+1];
				rom[i+1] = byte;
			}
		}

		memcpy(&memory[0], &rom[0], 0x8); // contains initial SSP and PC
		memcpy(&memory[romAddr], &rom[0], 512*1024*sizeof(char));
		romSP = READ32(memory, 0);
		romPC = READ32(memory, 4);
	}

	void reset()
	{
		fc = 0;

		m68k_init();
		m68k_set_cpu_type(M68K_CPU_TYPE_SCC68070);
		m68k_pulse_reset();
		m68k_set_reg(M68K_REG_SP, romSP);
		m68k_set_reg(M68K_REG_PC, romPC);

		UMR = 0x20; // unused bit
		USR = 0b0000'0110; // TX ready and unused bit
		UCS = 0x08; // unused bit
		UCR = 0x80; // unused bit
		UTH = URH = 0;

		PICR[0] = PICR[1] = 0;
		TSR = TCR = RR = T[0] = T[1] = T[2] = 0;
		DMA[0].CSR = DMA[0].CER = DMA[0].DCR = DMA[0].OCR = DMA[0].SCR = DMA[0].CCR = 0;
		// IDR = IAR = ISR = ICR = ICCR = 0;
	}

	void interrupt(size_t ch)
	{
		int level_lir = (ch == 1 ? LIR : (LIR >> 4)) & 0x07;
		int level_timer = PICR[0] & 0x07;
		int level_uart_rx = (PICR[1] & 0x70) >> 4;
		int level_uart_tx = PICR[1] & 0x07;
		int level = std::max({level_lir, level_timer, level_uart_rx, level_uart_tx});

		if (level > 0) {
			//MiniCDI::Log("[SCC68070] INT%dN lvl %d", ch, level);

			m68k_set_irq(level + 32);
		}
	}

	uint8_t read8(uint32_t addr)
	{
		switch (addr)
		{
			case 0x80001001: return LIR & 0x77;

			/** UART **/
			case 0x80002011: return UMR;
			case 0x80002013: USR |= (1<<1); return USR | 0x08;
			case 0x80002015: return UCS | 0x08;
			case 0x80002017: return UCR | 0x80;
			case 0x80002019: return UTH;
			case 0x8000201B: if (URH) USR |= 0x01; else USR &= ~(0x01); return URH;

			/** I²C **/
			case 0x80002001: return IDR;
			case 0x80002003: return IAR;
			case 0x80002005: return ISR;
			case 0x80002007: return ICR;
			case 0x80002009: return ICCR;

			/** Timer **/
			case 0x80002020: return TSR;
			case 0x80002021: return TCR;
			case 0x80002022: return (RR >> 8) & 0x00FF;
			case 0x80002023: return RR & 0x00FF;
			case 0x80002024: return (T[0] >> 8) & 0x00FF;
			case 0x80002025: return T[0] & 0x00FF;
			case 0x80002026: return (T[1] >> 8) & 0x00FF;
			case 0x80002027: return T[1] & 0x00FF;
			case 0x80002028: return (T[2] >> 8) & 0x00FF;
			case 0x80002029: return T[2] & 0x00FF;

			/** PICR **/
			case 0x80002045: return PICR[0] & 0x77;
			case 0x80002047: return PICR[1] & 0x77;

			/** DMA (ch1) **/
			case 0x80004000: return DMA[0].CSR;
			case 0x80004001: return DMA[0].CER;
			case 0x80004004: return DMA[0].DCR;
			case 0x80004005: return DMA[0].OCR;
			case 0x80004006: return DMA[0].SCR;
			case 0x80004007: return DMA[0].CCR;
			case 0x8000400a: return (DMA[0].MTC >> 8) & 0x00FF;
			case 0x8000400b: return DMA[0].MTC & 0x00FF;
			case 0x8000400c: return (DMA[0].MAC >> 24) & 0x000000FF;
			case 0x8000400d: return (DMA[0].MAC >> 16) & 0x000000FF;
			case 0x8000400e: return (DMA[0].MAC >> 8) & 0x000000FF;
			case 0x8000400f: return DMA[0].MAC & 0x000000FF;
			case 0x80004014: return (DMA[0].DAC >> 24) & 0x000000FF;
			case 0x80004015: return (DMA[0].DAC >> 16) & 0x000000FF;
			case 0x80004016: return (DMA[0].DAC >> 8) & 0x000000FF;
			case 0x80004017: return DMA[0].DAC & 0x000000FF;

			/** DMA (ch2) **/
			case 0x80004040: return DMA[1].CSR;
			case 0x80004041: return DMA[1].CER;
			case 0x80004044: return DMA[1].DCR;
			case 0x80004045: return DMA[1].OCR;
			case 0x80004046: return DMA[1].SCR;
			case 0x80004047: return DMA[1].CCR;
			case 0x8000404a: return (DMA[1].MTC >> 8) & 0x00FF;
			case 0x8000404b: return DMA[1].MTC & 0x00FF;
			case 0x8000404c: return (DMA[1].MAC >> 24) & 0x000000FF;
			case 0x8000404d: return (DMA[1].MAC >> 16) & 0x000000FF;
			case 0x8000404e: return (DMA[1].MAC >> 8) & 0x000000FF;
			case 0x8000404f: return DMA[1].MAC & 0x000000FF;
			case 0x80004054: return (DMA[1].DAC >> 24) & 0x000000FF;
			case 0x80004055: return (DMA[1].DAC >> 16) & 0x000000FF;
			case 0x80004056: return (DMA[1].DAC >> 8) & 0x000000FF;
			case 0x80004057: return DMA[1].DAC & 0x000000FF;
		}

		return memory[addr & 0x00FFFFFF];
	}

	void write8(uint32_t addr, uint8_t value)
	{
		switch (addr)
		{
			/** LIR **/
			case 0x80001001:
				if ((value & 0x88) && (LIR & 0x88)) LIR &= 0x77;
				else if ((value & 0x80) && (LIR & 0x80)) LIR &= 0x7F;
				else if ((value & 0x08) && (LIR & 0x08)) LIR &= 0xF7;
				LIR = (LIR & 0x88) | (value & 0x77);
				break;

			/** I²C **/
			case 0x80002001: IDR = value; break;
			case 0x80002003: IAR = value; break;
			case 0x80002005: ISR = value; break;
			case 0x80002007: ICR = value; break;
			case 0x80002009: ICCR = value; break;

			/** UART **/
			case 0x80002011: UMR = value | 0x20; break;
			case 0x80002013: USR = value; break;
			case 0x80002015: UCS = value; break;
			case 0x80002017: UCR = value;
				switch (UCR & 0x70)
				{
					case 0x20: // reset receiver
						URH = 0;
						MiniCDI::Log("[UART] UCR %02X (reset URH)", value);
						break;
					case 0x30: // reset transmitter
						UTH = 0; USR |= 0x08;
						MiniCDI::Log("[UART] UCR %02X (reset UTH)", value);
						break;
					case 0x40: // reset error status
						USR &= 0x0F;
						MiniCDI::Log("[UART] UCR %02X (reset error)", value);
						break;
				}
				break;
			case 0x80002019: UTH = value; USR &= ~(0x08); break;
			case 0x8000201B: URH = value; break;

			/** Timer **/
			case 0x80002020: TSR = value; break;
			case 0x80002021: TCR = value; break;
			case 0x80002022: RR &= 0x00FF; RR |= (value << 8); break;
			case 0x80002023: RR &= 0xFF00; RR |= value; break;
			case 0x80002024: T[0] &= 0x00FF; T[0] |= (value << 8); break;
			case 0x80002025: T[0] &= 0xFF00; T[0] |= value; break;
			case 0x80002026: T[1] &= 0x00FF; T[1] |= (value << 8); break;
			case 0x80002027: T[1] &= 0xFF00; T[1] |= value; break;
			case 0x80002028: T[2] &= 0x00FF; T[2] |= (value << 8); break;
			case 0x80002029: T[2] &= 0xFF00; T[2] |= value; break;

			/** PICR **/
			case 0x80002045: PICR[0] = value;
				if (PICR[0] & 0x80) PICR[0] &= 0x7F;
				if (PICR[0] & 0x08) PICR[0] &= 0xF7;
				break;
			case 0x80002047: PICR[1] = value;
				if (PICR[1] & 0x80) PICR[1] &= 0x7F;
				if (PICR[1] & 0x08) PICR[1] &= 0xF7;
				break;

			/** DMA (ch1) **/
			case 0x80004000: DMA[0].CSR = value; break;
			case 0x80004001: DMA[0].CER = value; break; // cannot be written per datasheet
			case 0x80004004: DMA[0].DCR = value; break;
			case 0x80004005: DMA[0].OCR = value; break;
			case 0x80004006: DMA[0].SCR = value; break;
			case 0x80004007: DMA[0].CCR = value; break;
			case 0x8000400a: DMA[0].MTC &= 0x00FF; DMA[0].MTC |= (value << 8); break;
			case 0x8000400b: DMA[0].MTC &= 0xFF00; DMA[0].MTC |= value; break;
			case 0x8000400c: DMA[0].MAC &= 0x00FFFFFF; DMA[0].MAC |= (value << 24); break;
			case 0x8000400d: DMA[0].MAC &= 0xFF00FFFF; DMA[0].MAC |= (value << 16); break;
			case 0x8000400e: DMA[0].MAC &= 0xFFFF00FF; DMA[0].MAC |= (value << 8); break;
			case 0x8000400f: DMA[0].MAC &= 0xFFFFFF00; DMA[0].MAC |= value; break;
			case 0x80004014: DMA[0].DAC &= 0x00FFFFFF; DMA[0].DAC |= (value << 24); break;
			case 0x80004015: DMA[0].DAC &= 0xFF00FFFF; DMA[0].DAC |= (value << 16); break;
			case 0x80004016: DMA[0].DAC &= 0xFFFF00FF; DMA[0].DAC |= (value << 8); break;
			case 0x80004017: DMA[0].DAC &= 0xFFFFFF00; DMA[0].DAC |= value; break;

			/** DMA (ch2) **/
			case 0x80004040: DMA[1].CSR = value; break;
			case 0x80004041: DMA[1].CER = value; break; // cannot be written per datasheet
			case 0x80004044: DMA[1].DCR = value; break;
			case 0x80004045: DMA[1].OCR = value; break;
			case 0x80004046: DMA[1].SCR = value; break;
			case 0x80004047: DMA[1].CCR = value; break;
			case 0x8000404a: DMA[1].MTC &= 0x00FF; DMA[1].MTC |= (value << 8); break;
			case 0x8000404b: DMA[1].MTC &= 0xFF00; DMA[1].MTC |= value; break;
			case 0x8000404c: DMA[1].MAC &= 0x00FFFFFF; DMA[1].MAC |= (value << 24); break;
			case 0x8000404d: DMA[1].MAC &= 0xFF00FFFF; DMA[1].MAC |= (value << 16); break;
			case 0x8000404e: DMA[1].MAC &= 0xFFFF00FF; DMA[1].MAC |= (value << 8); break;
			case 0x8000404f: DMA[1].MAC &= 0xFFFFFF00; DMA[1].MAC |= value; break;
			case 0x80004054: DMA[1].DAC &= 0x00FFFFFF; DMA[1].DAC |= (value << 24); break;
			case 0x80004055: DMA[1].DAC &= 0xFF00FFFF; DMA[1].DAC |= (value << 16); break;
			case 0x80004056: DMA[1].DAC &= 0xFFFF00FF; DMA[1].DAC |= (value << 8); break;
			case 0x80004057: DMA[1].DAC &= 0xFFFFFF00; DMA[1].DAC |= value; break;
		}
	}

	void tick_timer()
	{
		if (T[0] == 0xFFFF)
		{
			//MiniCDI::Log("[Timer] T0 overflow");
			TSR |= 0x80; // OV in T0
			T[0] = RR;
			interrupt(0);
		}
		T[0]++;
	}

	int run(int cycles)
	{
		int ran = 0;

		#ifdef MINICDI_DEBUG_CPU
		if (MiniCDI::Config::LogFile != 0) {
			uint32_t pcLog;
			pcLog = m68k_get_reg(NULL, M68K_REG_PC);
			ran += m68k_execute(cycles);
			if (pcLog != m68k_get_reg(NULL, M68K_REG_PC)) {
				char text[192];
				m68k_disassemble(text, m68k_get_reg(NULL, M68K_REG_PC), M68K_CPU_TYPE_SCC68070);
				fprintf(MiniCDI::Config::LogFile, "[CPU][$%08X] %s\n", m68k_get_reg(NULL, M68K_REG_PC), text);
				// printf("\n$%08X: %s                            \n", m68k_get_reg(NULL, M68K_REG_PC), text);
			}
		} else {
			ran += m68k_execute(cycles);
		}
		#else
		ran += m68k_execute(cycles);
		#endif

		#ifdef MINICDI_DEBUG_CPU
		printf("\x1b[%d;%dH", 4, 0);
		printf("PC: %08X SR: %s%s %s%s%s%s%s FC: %d\n",
			m68k_get_reg(NULL, M68K_REG_PC),
			(m68k_get_reg(NULL, M68K_REG_SR) >> 15) & 0x01 ? "T" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 13) & 0x01 ? "S" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 4) & 0x01 ? "X" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 3) & 0x01 ? "N" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 2) & 0x01 ? "Z" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 1) & 0x01 ? "V" : " ",
			m68k_get_reg(NULL, M68K_REG_SR) & 0x01 ? "C" : " ",
			fc
		);

		for (int i = 0; i < 8; i++)
			printf("D%d: %08X A%d: %08X\n",
			i, m68k_get_reg(NULL, (m68k_register_t)((int)M68K_REG_D0 + i)),
			i, m68k_get_reg(NULL, (m68k_register_t)((int)M68K_REG_A0 + i)));

		printf("\nUCR: %02X URH: %02X USR: %02X LIR: %02X\n", UCR, URH, USR, LIR);
		#endif

		return ran;
	}
};

#endif
