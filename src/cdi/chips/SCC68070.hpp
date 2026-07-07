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
	uint16_t T[3]; // only Timer 0 is used in practice.

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

	void dma_call(size_t index, uint32_t start_address)
	{
		if (DMA[index].CCR & 0x80) {
			DMA[index].CCR &= ~0x80; // START unset
			DMA[index].CSR &= 0b01001111; // COC, NDT and ERR unset
			DMA[index].CSR |= 0b00001000; // Channel Active set
			DMA[index].CER = 0;

			if (index == 1)
				start_address = DMA[1].DAC;

			MiniCDI::Log("[SCC68070:DMA%d] %s transfer: %d %s %s $%08X %s $%08X", index+1,
						 DMA[index].DCR & 0x80 ? "cycle-steal" : "burst",
						 DMA[index].MTC,
						 DMA[index].OCR & 0x10 ? "words" : "bytes",
						 DMA[index].OCR & 0x80 ? "from" : "to",
						 start_address,
						 DMA[index].OCR & 0x80 ? "to" : "from",
						 DMA[index].MAC);

			while (DMA[index].MTC > 0) {
				if (DMA[index].CCR & 0x10) {
					MiniCDI::Log("[SCC68070:DMA%d] transfer aborted", index+1);
					DMA[index].CSR |= 0b10010000; // COC and ERR set
					DMA[index].CSR &= 0b11110111; // Channel Active unset
					DMA[index].CER = 0b10001u; // Abort Error
					return;
				}

				if (DMA[index].DCR & 0x80) {
					if (DMA[index].OCR & 0x80) {
						memory[DMA[index].MAC] = memory[start_address++];
						if (DMA[index].OCR & 0x10) memory[DMA[index].MAC+1] = memory[start_address++];
					} else {
						memory[start_address++] = memory[DMA[index].MAC];
						if (DMA[index].OCR & 0x10) memory[start_address++] = memory[DMA[index].MAC+1];
					}

					if (index == 1 && (DMA[index].SCR & 0x04)) {
						DMA[index].MAC += DMA[index].OCR & 0x10 ? 2 : 1;
						DMA[index].DAC += DMA[index].OCR & 0x10 ? 2 : 1;
					} else if (index == 0) {
						DMA[index].MAC += DMA[index].OCR & 0x10 ? 2 : 1;
					}

					DMA[index].MTC--;
				} else {
					if (DMA[index].OCR & 0x80) {
						memcpy(&memory[DMA[index].MAC], &memory[start_address], DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC);
					} else {
						memcpy(&memory[start_address], &memory[DMA[index].MAC], DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC);
					}

					if (index == 1 && (DMA[index].SCR & 0x04)) {
						DMA[index].MAC += DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC;
						DMA[index].DAC += DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC;
					} else if (index == 0) {
						DMA[index].MAC += DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC;
					}

					DMA[index].MTC = 0;
				}
			}

			DMA[index].CSR |= 0b10000000; // COC set
			DMA[index].CSR &= 0b11110111; // Channel Active unset
			interrupt(index == 1 ? SCC68070::IPL_DMA2 : SCC68070::IPL_DMA1, true);
		}
	}

	/** I²C **/
	uint8_t IDR;
	uint8_t IAR;
	uint8_t ISR;
	uint8_t ICR;
	uint8_t ICCR;

	void update_ipl()
	{
		Ipl.nxt_index = 0;
		Ipl.nxt_irq = 0;
		for (int i = 11; i >= 0; i--) {
			if (Ipl.levels[i] >= Ipl.nxt_irq) {
				Ipl.nxt_index = i;
				Ipl.nxt_irq = Ipl.levels[i];
			}
		}

		if (Ipl.cur_irq != Ipl.nxt_irq)
		{
			// Mimic MAME behaviour, which is to clear the current interrupt line THEN check for any pending interrupt levels.

			if (Ipl.cur_irq != 0)
			{
				// Log interrupt information
				/*MiniCDI::Log("[SCC68070:IPL] IPL%d(%d) <= reset", Ipl.cur_index, Ipl.cur_irq);*/

				Ipl.cur_index = 0;
				Ipl.cur_irq = 0;

				m68k_set_irq(0);
			}

			if (Ipl.nxt_irq != 0)
			{
				// Log interrupt information
				/*MiniCDI::Log("[SCC68070:IPL] IPL%d(%d) <= IPL%d(%d)  IN7N=%d,IN5N=%d,IN4N=%d,IN2N=%d,INT1=%d,INT2=%d,T=%d,URX=%d,UTX=%d,I2C=%d,DMA1=%d,DMA2=%d",
							 Ipl.cur_index, Ipl.cur_irq,
							 Ipl.nxt_index, Ipl.nxt_irq,
							 Ipl.levels[IPL_IN7N],
							 Ipl.levels[IPL_IN5N],
							 Ipl.levels[IPL_IN4N],
							 Ipl.levels[IPL_IN2N],
							 Ipl.levels[IPL_INT1],
							 Ipl.levels[IPL_INT2],
							 Ipl.levels[IPL_TIMER],
							 Ipl.levels[IPL_UART_RX],
							 Ipl.levels[IPL_UART_TX],
							 Ipl.levels[IPL_I2C],
							 Ipl.levels[IPL_DMA1],
							 Ipl.levels[IPL_DMA2]);*/

				Ipl.cur_index = Ipl.nxt_index;
				Ipl.cur_irq = Ipl.nxt_irq;

				m68k_set_irq(Ipl.cur_irq > 0 && Ipl.cur_index >= IPL_INT1 ? Ipl.cur_irq + 32 : Ipl.cur_irq);
			}
		}
	}

public:
	uint8_t fc; // used for FC/address space callback
	friend class PointingDevice;
	friend class CDIC;
	friend class CIAP;

	// Priority order of interrupt signal booleans
	enum IPLSignal {
		IPL_IN7N = 0, // NMI, should always come first.
		IPL_IN5N,
		IPL_IN4N, // CD/audio device
		IPL_IN2N, // microcontroller device
		IPL_INT1, // VDSC/video (responsible for Ev$Pulse+Ev$All syscalls)
		IPL_INT2,
		IPL_TIMER,
		IPL_UART_RX,
		IPL_UART_TX,
		IPL_I2C,
		IPL_DMA1,
		IPL_DMA2
	};
	struct {
		int cur_index; // Index of peripheral holding current IRQ
		int nxt_index; // Index of peripheral holding next IRQ
		uint8_t cur_irq; // Current pending interrupt level
		uint8_t nxt_irq; // Next pending interrupt level

		uint8_t levels[12];
		uint8_t vectors[12];
	} Ipl;

	/**
	 * @brief  Sets a peripheral's (or external's) pending interrupt to true or false.
	 *         It gets the level from the corresponding onchip register and then sets the Musashi IRQ through `update_ipl()`.
	 *
	 * @param  assert:  Whether the interrupt is asserted.
	 */
	void interrupt(size_t index, bool assert)
	{
		switch (index)
		{
			default:
				Ipl.levels[index] = assert;
				break;
			case IPL_IN7N:
				Ipl.levels[index] = assert ? 7 : 0;
				break;
			case IPL_IN5N:
				Ipl.levels[index] = assert ? 5 : 0;
				break;
			case IPL_IN4N:
				Ipl.levels[index] = assert ? 4 : 0;
				break;
			case IPL_IN2N:
				Ipl.levels[index] = assert ? 2 : 0;
				break;
			case IPL_INT1:
				Ipl.levels[index] = assert ? LIR >> 4 & 0x07 : 0;
				break;
			case IPL_INT2:
				Ipl.levels[index] = assert ? LIR & 0x07 : 0;
				break;
			case IPL_TIMER:
				Ipl.levels[index] = assert ? PICR[0] & 0x07 : 0;
				break;
			case IPL_UART_RX:
				Ipl.levels[index] = assert ? PICR[1] >> 4 & 0x07 : 0;
				break;
			case IPL_UART_TX:
				Ipl.levels[index] = assert ? PICR[1] & 0x07 : 0;
				break;
			case IPL_I2C:
				Ipl.levels[index] = assert ? PICR[0] >> 4 & 0x07 : 0;
				break;
			case IPL_DMA1:
				Ipl.levels[index] = assert && (DMA[0].CSR & 0x80) && (DMA[0].CCR & 0x08) ? DMA[0].CCR & 0x07 : 0;
				break;
			case IPL_DMA2:
				Ipl.levels[index] = assert && (DMA[1].CSR & 0x80) && (DMA[1].CCR & 0x08) ? DMA[1].CCR & 0x07 : 0;
				break;
		}

		update_ipl();
	}

	SCC68070(uint8_t* memory) : memory(memory)
	{
	}

	void load_rom(std::vector<char> &rom)
	{
		// (Dirty) byteswap method
		if (rom[4] != 0x00) {
			for (size_t i = 0; i < rom.size(); i+=2) {
				std::swap(rom[i], rom[i+1]);
			}
		}

		memcpy(&memory[0x400000], &rom[0], 512*1024*sizeof(char));
	}

	void reset_internal()
	{
		// LIR
		LIR = 0;

		// UART
		UMR = 0x20; // unused bit
		USR = 0x06; // TX ready and unused bit
		UCS = 0x08; // unused bit
		UCR = 0x80; // unused bit
		UTH = URH = 0;

		PICR[0] = PICR[1] = 0;

		// Timer(s)
		TSR = TCR = RR = T[0] = T[1] = T[2] = 0;

		// DMA
		DMA[0].CER = DMA[0].DCR = DMA[0].OCR = DMA[0].SCR = DMA[0].CCR = 0;
		DMA[1].CER = DMA[1].DCR = DMA[1].OCR = DMA[1].SCR = DMA[1].CCR = 0;
		DMA[0].CSR = DMA[1].CSR = 0;
		// DMA[0] = {0};
		// DMA[1] = {0};

		// I²C
		IDR = IAR = ISR = ICR = ICCR = 0;
	}

	void reset()
	{
		// Clear DRAM banks
		memset(&memory[0x000000], 0, 512*1024);
		memset(&memory[0x200000], 0, 512*1024);

		// Copy ROM's starting SSP and PC to DRAM
		memcpy(&memory[0], &memory[0x400000], 8*sizeof(char));

		// Reset internal peripherals
		fc = 0;
		Ipl = {0};
		reset_internal();

		// Reset Musashi processor
		m68k_pulse_reset();
		m68k_set_irq(0);

		// set A0-A7, D0-D6 to 0xffffffff (cdiemu)
		for (int i = 0; i < 15; i++) { m68k_set_reg((m68k_register_t)i, 0xffffffff); }
		/*m68k_set_reg(M68K_REG_A7, (memory[0x400000] << 24) | (memory[0x400001] << 16) | (memory[0x400002] << 8) | memory[0x400003]);
		m68k_set_reg(M68K_REG_PC, (memory[0x400004] << 24) | (memory[0x400005] << 16) | (memory[0x400006] << 8) | memory[0x400007]);*/
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
			#ifdef MINICDI_68070_ACCURACY
			case 0x80002009: return ICCR | 0xE0;
			#else
			case 0x80002009: return ICCR;
			#endif

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
			case 0x80004007: return DMA[0].CCR & 0xEF;
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
			case 0x80004047: return DMA[1].CCR & 0xEF;
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
			case 0x80001001: LIR = value;
				if (value & 0x80) interrupt(SCC68070::IPL_INT1, false);
				if (value & 0x08) interrupt(SCC68070::IPL_INT2, false);
				break;

			/** I²C **/
			#ifdef MINICDI_68070_ACCURACY
			case 0x80002001: IDR = value;
				ISR |= 0x10; // set PIN bit
				ISR &= ~0x0C; // reset AL and AAS
				break;
			#else
			case 0x80002001: IDR = value; break;
			#endif
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
						//MiniCDI::Log("[SCC68070:UART] UCR %02X (reset URH)", value);
						break;
					case 0x30: // reset transmitter
						UTH = 0; USR |= 0x08;
						//MiniCDI::Log("[SCC68070:UART] UCR %02X (reset UTH)", value);
						break;
					case 0x40: // reset error status
						USR &= 0x0F;
						//MiniCDI::Log("[SCC68070:UART] UCR %02X (reset error)", value);
						break;
				}
				break;
			case 0x80002019: UTH = value;
				USR &= ~0x08; // unset TXE
				break;
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
			/// When PIR (4th bit) is set for either peripheral, the pending interrupt is cleared, per datasheet.
			/// It seems to have the same behaviour as PIR for INT1N and INT2N.
			case 0x80002045: PICR[0] = value;
				if (value & 0x80) interrupt(SCC68070::IPL_I2C, false);
				if (value & 0x08) interrupt(SCC68070::IPL_TIMER, false);
				break;
			case 0x80002047: PICR[1] = value;
				if (value & 0x80) interrupt(SCC68070::IPL_UART_RX, false);
				if (value & 0x08) interrupt(SCC68070::IPL_UART_TX, false);
				break;

			/** DMA (ch1) **/
			case 0x80004000: DMA[0].CSR &= 0x08; break;
			case 0x80004004: DMA[0].DCR &= 0x08; DMA[0].DCR |= (value & 0xF7); break;
			case 0x80004005: DMA[0].OCR = value; DMA[0].DCR &= 0xF7; DMA[0].DCR |= ((value >> 1) & 0x08); break;
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
			case 0x80004040: DMA[1].CSR &= 0x08; break;
			case 0x80004044: DMA[1].DCR &= 0x08; DMA[1].DCR |= (value & 0xF7); break;
			case 0x80004045: DMA[1].OCR = value; DMA[1].DCR &= 0xF7; DMA[1].DCR |= ((value >> 1) & 0x08); break;
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

	void print()
	{
		#ifdef MINICDI_DEBUG_CPU
		printf("\x1b[%d;%dH", 4, 0);
		printf("PC: %08X SR: %s%s-%s%s%s%s%s FC: %d\n",
			m68k_get_reg(NULL, M68K_REG_PC),
			(m68k_get_reg(NULL, M68K_REG_SR) >> 15) & 0x01 ? "T" : "-",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 13) & 0x01 ? "S" : "-",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 4) & 0x01 ? "X" : "-",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 3) & 0x01 ? "N" : "-",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 2) & 0x01 ? "Z" : "-",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 1) & 0x01 ? "V" : "-",
			m68k_get_reg(NULL, M68K_REG_SR) & 0x01 ? "C" : "-",
			fc
		);

		for (int i = 0; i < 8; i++)
			printf("D%d: %08X A%d: %08X\n",
			i, m68k_get_reg(NULL, (m68k_register_t)((int)M68K_REG_D0 + i)),
			i, m68k_get_reg(NULL, (m68k_register_t)((int)M68K_REG_A0 + i)));

		/// Timer ticks at an average of 6155 ns. One cycle takes about 64 ns (4 cycles = ~256 ns).
		printf("                                             ");
		// printf("\nUCR: %02X URH: %02X USR: %02X LIR: %02X\n", UCR, URH, USR, LIR);
		printf("\n[DMA1] CSR: %02X MTC: %04X MAC: %08X\n", DMA[0].CSR, DMA[0].MTC, DMA[0].MAC);
		printf("\x1b[%d;%dH", 16, 0);
		#endif
	}

	int run(int cycles)
	{
		int ran = 0;

		/*#ifdef MINICDI_DEBUG_CPU
		if (MiniCDI::Config::LogFile != 0) {
			uint32_t pcLog;
			pcLog = m68k_get_reg(NULL, M68K_REG_PC);
			ran += m68k_execute(cycles);
			if (pcLog != m68k_get_reg(NULL, M68K_REG_PC)) {
				char text[192];
				m68k_disassemble(text, m68k_get_reg(NULL, M68K_REG_PC), M68K_CPU_TYPE_SCC68070);
				fprintf(MiniCDI::Config::LogFile, "[SCC68070:CPU][$%08X] %s\n", m68k_get_reg(NULL, M68K_REG_PC), text);
				// printf("\n$%08X: %s                            \n", m68k_get_reg(NULL, M68K_REG_PC), text);
			}
		} else
			ran += m68k_execute(cycles);
		#else
		ran += m68k_execute(cycles);
		#endif*/

		ran += m68k_execute(cycles);

		// Poll timer.
		for (int i = cycles; i >= 96; i -= 96)
		{
			if (T[0] == 0xFFFF) {
				//MiniCDI::Log("[SCC68070:Timer] T0 overflow");
				TSR |= 0x80; // OV in T0
				T[0] = RR;
				interrupt(SCC68070::IPL_TIMER, true);
			} else {
				T[0]++;
				interrupt(SCC68070::IPL_TIMER, false);
			}
		}

		return ran;
	}
};

#endif