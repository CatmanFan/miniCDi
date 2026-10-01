#pragma once

struct SCC68070
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
	struct {
		uint8_t HR; // Transmit Holding Register
		std::vector<uint8_t> chars;
		int clock;
	} UART_T;
	uint8_t URH; // Receive Holding Register
	bool URH_reset = false;

	/** Timer **/
	uint8_t TSR;
	uint8_t TCR;
	uint16_t RR;
	uint16_t T0;

	/** DMA **/
	struct {
		uint8_t CSR = 0;
		uint8_t CER = 0;

		uint8_t DCR = 0;
		uint8_t OCR = 0;
		uint8_t SCR = 0;
		uint8_t CCR = 0;

		uint16_t MTC = 0;
		uint32_t MAC = 0;
		uint32_t DAC = 0;

		uint8_t CPR = 0;
	} DMA[2];

	void uart_tx_log_line();
	void dma_call(size_t index, uint32_t start_address);

	/** I²C **/
	uint8_t IDR;
	uint8_t IAR;
	uint8_t ISR;
	uint8_t ICR;
	uint8_t ICCR;

	uint8_t fc; // used for FC/address space callback

	// Priority order of interrupt signal booleans
	enum IPLSignal
	{
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
	struct
	{
		uint8_t curr = 0; // Current pending interrupt level
		uint8_t ack = 0; // Acknowledged interrupt level
		bool levels[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		uint8_t vectors[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
	} Ipl;

	/**
	 * @brief  Sets a peripheral's (or external's) pending interrupt to true or false.
	 *         It gets the level from the corresponding onchip register and then sets the Musashi IRQ through `update_ipl()`.
	 *
	 * @param  assert:  Whether the interrupt is asserted.
	 */
	void interrupt(size_t index, bool assert)
	{
		Ipl.levels[index] = assert;

		// Update the IPL.
		// **************************
		int all_levels[] = {
			Ipl.levels[IPL_IN7N] ? 7 : 0,
			Ipl.levels[IPL_IN5N] ? 5 : 0,
			Ipl.levels[IPL_IN4N] ? 4 : 0,
			Ipl.levels[IPL_IN2N] ? 2 : 0,
			Ipl.levels[IPL_INT1] ? LIR >> 4 & 0x07 : 0,
			Ipl.levels[IPL_INT2] ? LIR & 0x07 : 0,
			Ipl.levels[IPL_TIMER] ? PICR[0] & 0x07 : 0,
			Ipl.levels[IPL_UART_RX] ? PICR[1] >> 4 & 0x07 : 0,
			Ipl.levels[IPL_UART_TX] ? PICR[1] & 0x07 : 0,
			Ipl.levels[IPL_I2C] ? PICR[0] >> 4 & 0x07 : 0,
			Ipl.levels[IPL_DMA1] && (DMA[0].CSR & 0x80) && (DMA[0].CCR & 0x08) ? DMA[0].CCR & 0x07 : 0,
			Ipl.levels[IPL_DMA2] && (DMA[1].CSR & 0x80) && (DMA[1].CCR & 0x08) ? DMA[1].CCR & 0x07 : 0
		};
		for (int i = IPL_INT1; i <= IPL_DMA2; i++) if (all_levels[i] > 0) all_levels[i] += 32;
		uint8_t new_irq = *(std::max_element(all_levels, all_levels + (sizeof(all_levels) / sizeof(all_levels[0]))));

		// *******************************************
		// This method is more unstable
		/*uint8_t new_irq = 0, new_irq_index = 0;
		for (int i = IPL_DMA2; i >= 0; i--) {
			if (new_irq <= all_levels[i]) {
				new_irq = all_levels[i];
				new_irq_index = i;
			}
		}
		if (new_irq_index >= IPL_INT1 && new_irq > 0) new_irq += 32;*/
		// *******************************************

		if (Ipl.curr != new_irq)
		{
			if (Ipl.curr != 0) {
				Ipl.curr = 0;
				m68k_set_irq(0);
			}

			if (new_irq != 0) {
				Ipl.curr = new_irq;
				m68k_set_irq(new_irq);
			}
		}
	}

	/// Called by Musashi's interrupt handler
	int interrupt_ack(int int_level)
	{
		if (int_level != Ipl.ack) {
			Ipl.ack = int_level;
			// MiniCDI::Log("[SCC68070:CPU] irq lvl%d ack", int_level);
		}

		// Return manually-set vectors in case of external interrupts (per MAME)
		switch (int_level)
		{
			case 2:
				if (Ipl.vectors[SCC68070::IPL_IN2N] > 0) { return Ipl.vectors[SCC68070::IPL_IN2N]; }
				break;
			case 4:
				if (Ipl.vectors[SCC68070::IPL_IN4N] > 0) { return Ipl.vectors[SCC68070::IPL_IN4N]; }
				break;
			case 5:
				if (Ipl.vectors[SCC68070::IPL_IN5N] > 0) { return Ipl.vectors[SCC68070::IPL_IN5N]; }
				break;
			case 7:
				if (Ipl.vectors[SCC68070::IPL_IN7N] > 0) { return Ipl.vectors[SCC68070::IPL_IN7N]; }
				break;
		}

		// Automatically deassert onchip timers (per MAME)
		if ((int_level % 8) == (LIR >> 4 & 0x07)) { interrupt(IPL_INT1, false); }
		if ((int_level % 8) == (LIR & 0x07)) { interrupt(IPL_INT2, false); }
		if ((int_level % 8) == (PICR[0] & 0x07)) { interrupt(IPL_TIMER, false); }
		if ((int_level % 8) == (PICR[1] >> 4 & 0x07)) { interrupt(IPL_UART_RX, false); }
		if ((int_level % 8) == (PICR[1] & 0x07)) { interrupt(IPL_UART_TX, false); }
		if ((int_level % 8) == (PICR[0] >> 4 & 0x07)) { interrupt(IPL_I2C, false); }

		// If not handled, return autovector.
		return 0xffffffff;
	}

	SCC68070(uint8_t* memory) : memory(memory) { }

	void load_rom(std::vector<char> &rom)
	{
		// (Dirty) byteswap method
		if (rom[4] != 0x00) {
			for (size_t i = 0; i < rom.size(); i+=2) {
				std::swap(rom[i], rom[i+1]);
			}
		}

		memcpy(&memory[0x400000], &rom[0], rom.size()*sizeof(char));
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
		UART_T.HR = UART_T.clock = 0;
		URH_reset = true;
		uart_tx_log_line();
		URH = 0;

		PICR[0] = PICR[1] = 0;

		// Timer(s)
		TSR = TCR = RR = T0 = 0;

		// DMA
		DMA[0].CER = DMA[0].DCR = DMA[0].OCR = DMA[0].SCR = DMA[0].CCR = 0;
		DMA[1].CER = DMA[1].DCR = DMA[1].OCR = DMA[1].SCR = DMA[1].CCR = 0;
		DMA[0].CSR = DMA[1].CSR = 0;

		// I²C
		IDR = IAR = ISR = ICR = ICCR = 0;
	}

	void reset()
	{
		// Clear DRAM banks
		memset(&memory[0x000000], 0, 512*1024*sizeof(char));
		memset(&memory[0x200000], 0, 512*1024*sizeof(char));

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
			case 0x80002019: return UART_T.HR;
			case 0x8000201B: if (URH_reset && MiniCDI::Config.PCB_LLTest) { URH = 0x06; URH_reset = false; }
							 MiniCDI::Log("[SCC68070:UART_RX] read %02X", URH);
							 if (URH) USR |= 0x01; else USR &= ~(0x01); return URH;

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
			case 0x80002024: return (T0 >> 8) & 0x00FF; // Timer0 HiByte
			case 0x80002025: return T0 & 0x00FF; // Timer0 LoByte
			case 0x80002026:
			case 0x80002027:
			case 0x80002028:
			case 0x80002029: return 0; // Timer1 and Timer2, given that these are unused, they will just return a dummy 0.

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

			/** I²C **/ // Not even trying with these
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
						MiniCDI::Log("[SCC68070:UART] UCR %02X (reset URH)", value);
						URH = 0;
						UCR &= 0xF0; // reset TxD control + RxD control
						break;
					case 0x30: // reset transmitter
						MiniCDI::Log("[SCC68070:UART] UCR %02X (reset UTH)", value);
						UART_T.HR = 0;
						UART_T.chars.clear();
						USR |= 0x0C; // set TXE+TXRDY
						UCR &= 0xF0; // reset TxD control + RxD control
						break;
					case 0x40: // reset error status
						MiniCDI::Log("[SCC68070:UART] UCR %02X (reset error)", value);
						USR &= 0x0F; // reset error bits in USR 7:4
						UCR &= 0xF0; // reset TxD control + RxD control
						break;
				}
				break;
			case 0x80002019: UART_T.HR = value;
				USR &= ~0x08; // unset TXE
				USR &= ~0x04; // unset TXRDY
				if (value == '\n' || value == '\0')
					uart_tx_log_line();
				else
					UART_T.chars.push_back(value);
				break;
			case 0x8000201B: URH = value; break;

			/** Timer **/
			case 0x80002020: TSR &= ~value; break;
			case 0x80002021: TCR = value; break;
			case 0x80002022: RR &= 0x00FF; RR |= (value << 8); break;
			case 0x80002023: RR &= 0xFF00; RR |= value; break;
			case 0x80002024: T0 &= 0x00FF; T0 |= (value << 8); break;
			case 0x80002025: T0 &= 0xFF00; T0 |= value; break;

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

	void run(int cycles)
	{
		/*#ifdef MINICDI_DEBUG_CPU
		// Print disassembly to log
		if (MiniCDI::Config.LogFile != 0) {
			char text[192];
			m68k_disassemble(text, m68k_get_reg(NULL, M68K_REG_PC), M68K_CPU_TYPE_SCC68070);
			MiniCDI::Log("[SCC68070:CPU][$%08X] %s\n", m68k_get_reg(NULL, M68K_REG_PC), text);
		}
		#endif*/

		// Run M68000 core via Musashi for specified number of cycles
		m68k_execute(cycles);
	}

	void timer0_tick()
	{
		if (T0 == 0xFFFF) {
			//MiniCDI::Log("[SCC68070:Timer0] Overflow");
			TSR |= 0x80; // OV in T0
			T0 = RR;
			interrupt(SCC68070::IPL_TIMER, true);
		} else {
			T0++;
		}
	}

	void uart_tx_tick()
	{
		if ((UCR & 0b1100) == 0b0100)
		{
			if (UART_T.chars.size() > 0)
			{
				//MiniCDI::Log("[SCC68070:UART] transferring %02X", UART_T.chars[0]);
				UART_T.HR = UART_T.chars[0];
				USR |= 0x04; // set TXRDY
				interrupt(SCC68070::IPL_UART_TX, true);
			}

			if (UART_T.chars.size() == 0) USR |= 0x0C; // set TXE+TXRDY
		}
	}
};