#include "cdi/common.hpp"

/**
 * @brief  Sets a peripheral's (or external's) pending interrupt to true or false.
 *         It gets the level from the corresponding onchip register and then sets the Musashi IRQ through `update_ipl()`.
 *
 * @param  assert:  Whether the interrupt is asserted.
 */
void SCC68070::interrupt(size_t index, bool assert)
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
		Ipl.levels[IPL_DMA1] && (DMA1.CSR & 0x80) && (DMA1.CCR & 0x08) ? DMA1.CCR & 0x07 : 0,
		Ipl.levels[IPL_DMA2] && (DMA2.CSR & 0x80) && (DMA2.CCR & 0x08) ? DMA2.CCR & 0x07 : 0
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
int SCC68070::interrupt_ack(int int_level)
{
	if (int_level != Ipl.ack)
		Ipl.ack = int_level;

	/// *** begin MAME-ported code ***
	// Return manually-set vectors in case of external interrupts
	switch (int_level)
	{
		case 2:
			if (Ipl.vectors[SCC68070::IPL_IN2N])
				return Ipl.vectors[SCC68070::IPL_IN2N];
			break;
		case 4:
			if (Ipl.vectors[SCC68070::IPL_IN4N])
				return Ipl.vectors[SCC68070::IPL_IN4N];
			break;
		case 5:
			if (Ipl.vectors[SCC68070::IPL_IN5N])
				return Ipl.vectors[SCC68070::IPL_IN5N];
			break;
		case 7:
			if (Ipl.vectors[SCC68070::IPL_IN7N])
				return Ipl.vectors[SCC68070::IPL_IN7N];
			break;
	}

	// Otherwise search for the interrupt level that was just raised and deassert it.
	int_level &= 7;
	uint8_t level_int1 = LIR >> 4 & 0x07;
	uint8_t level_int2 = LIR & 0x07;
	uint8_t level_timer = PICR[0] & 0x07;
	uint8_t level_uart_rx = PICR[1] >> 4 & 0x07;
	uint8_t level_uart_tx = PICR[1] & 0x07;
	uint8_t level_i2c = PICR[0] >> 4 & 0x07;

	if (int_level == level_int1) interrupt(SCC68070::IPL_INT1, false);
	else if (int_level == level_int2) interrupt(SCC68070::IPL_INT2, false);
	else if (int_level == level_timer) interrupt(SCC68070::IPL_TIMER, false);
	else if (int_level == level_uart_rx) interrupt(SCC68070::IPL_UART_RX, false);
	else if (int_level == level_uart_tx) interrupt(SCC68070::IPL_UART_TX, false);
	else if (int_level == level_i2c) interrupt(SCC68070::IPL_I2C, false);
	/// *** end MAME-ported code ***

	// If not handled, return autovector.
	return 0xFFFFFFFF;
}

void SCC68070::reset_onchip()
{
	LIR = 0;
	DMA1.reset();
	DMA2.reset();
	Uart.reset();
	T.reset();

	PICR[0] = PICR[1] = 0;

	// I²C
	IDR = IAR = ISR = ICR = ICCR = 0;
}

void SCC68070::reset()
{
	// Clear DRAM banks
	memset(&memory[0x000000], 0, 512*1024*sizeof(char));
	memset(&memory[0x200000], 0, 512*1024*sizeof(char));

	// Copy ROM's starting SSP and PC to DRAM
	memcpy(&memory[0], &memory[0x400000], 8*sizeof(char));

	// Reset internal peripherals
	fc = 0;
	Ipl = SCC68070::IPL();
	reset_onchip();

	// Reset Musashi processor
	m68k_pulse_reset();
	m68k_set_irq(0);

	/// *** begin cdiemu-ported code ***
	// set A0-A7, D0-D6 to 0xffffffff
	for (int i = 0; i < 15; i++) { m68k_set_reg((m68k_register_t)i, 0xffffffff); }
	/// *** end cdiemu-ported code ***
}

uint8_t SCC68070::read8(uint32_t addr)
{
	switch (addr & 0xFFFF)
	{
		case 0x1001: return LIR & 0x77;

		/** UART **/
		// TO-DO !!!!
		case 0x2011: return Uart.MR | 0x20; // 1<<5 is unused
		case 0x2013: return Uart.SR | 0x02; // 1<<1 is unused
		case 0x2015: return Uart.CSR | 0x08; // 1<<3 is unused
		case 0x2017: return Uart.CR | 0x80; // 1<<7 is unused
		case 0x2019: return Uart.THR;
		case 0x201B: return Uart.receive();

		/** I²C **/
		case 0x2001: return IDR;
		case 0x2003: return IAR;
		case 0x2005: return ISR;
		case 0x2007: return ICR;
		case 0x2009: return ICCR;

		/** Timer **/
		case 0x2020: return T.SR;
		case 0x2021: return T.CR;
		case 0x2022: return (T.RLR >> 8) & 0x00FF;
		case 0x2023: return T.RLR & 0x00FF;
		case 0x2024: return (T.T0 >> 8) & 0x00FF;
		case 0x2025: return T.T0 & 0x00FF;

		/** PICR **/
		case 0x2045: return PICR[0] & 0x77;
		case 0x2047: return PICR[1] & 0x77;

		/** DMA (ch1) **/
		case 0x4000: return DMA1.CSR;
		case 0x4001: return DMA1.CER;
		case 0x4004: return DMA1.DCR;
		case 0x4005: return DMA1.OCR;
		case 0x4006: return DMA1.SCR;
		case 0x4007: return DMA1.CCR & 0xEF;
		case 0x400a: return (DMA1.MTC >> 8) & 0x00FF;
		case 0x400b: return DMA1.MTC & 0x00FF;
		case 0x400c: return (DMA1.MAC >> 24) & 0x000000FF;
		case 0x400d: return (DMA1.MAC >> 16) & 0x000000FF;
		case 0x400e: return (DMA1.MAC >> 8) & 0x000000FF;
		case 0x400f: return DMA1.MAC & 0x000000FF;
		case 0x4014: return (DMA1.DAC >> 24) & 0x000000FF;
		case 0x4015: return (DMA1.DAC >> 16) & 0x000000FF;
		case 0x4016: return (DMA1.DAC >> 8) & 0x000000FF;
		case 0x4017: return DMA1.DAC & 0x000000FF;

		/** DMA (ch2) **/
		case 0x4040: return DMA2.CSR;
		case 0x4041: return DMA2.CER;
		case 0x4044: return DMA2.DCR;
		case 0x4045: return DMA2.OCR;
		case 0x4046: return DMA2.SCR;
		case 0x4047: return DMA2.CCR & 0xEF;
		case 0x404a: return (DMA2.MTC >> 8) & 0x00FF;
		case 0x404b: return DMA2.MTC & 0x00FF;
		case 0x404c: return (DMA2.MAC >> 24) & 0x000000FF;
		case 0x404d: return (DMA2.MAC >> 16) & 0x000000FF;
		case 0x404e: return (DMA2.MAC >> 8) & 0x000000FF;
		case 0x404f: return DMA2.MAC & 0x000000FF;
		case 0x4054: return (DMA2.DAC >> 24) & 0x000000FF;
		case 0x4055: return (DMA2.DAC >> 16) & 0x000000FF;
		case 0x4056: return (DMA2.DAC >> 8) & 0x000000FF;
		case 0x4057: return DMA2.DAC & 0x000000FF;
	}

	return 0;
}

void SCC68070::write8(uint32_t addr, uint8_t value)
{
	switch (addr & 0xFFFF)
	{
		/// LIR
		case 0x1001: LIR = value;
			// Check if LIR has clear bits
			if (value & 0x80) interrupt(SCC68070::IPL_INT1, false);
			if (value & 0x08) interrupt(SCC68070::IPL_INT2, false);
			break;

		/// I²C
		// Not even trying with these
		case 0x2001: IDR = value; break;
		case 0x2003: IAR = value; break;
		case 0x2005: ISR = value; break;
		case 0x2007: ICR = value; break;
		case 0x2009: ICCR = value; break;

		/// UART
		// TO-DO !!!!
		case 0x2011: Uart.MR = value; break;
		case 0x2013: Uart.SR = value; break;
		case 0x2015: Uart.CSR = value; break;
		case 0x2017:
			Uart.CR = value;
			switch (value & 0x70)
			{
				default:
					break;

				case 0x20: // Reset Receiver
					Uart.SR |= 0b11; // RXRDY and unused bit
					Uart.CR = (Uart.CR & ~0b11) | 0b01; // Reset Receiver Control
					break;

				case 0x30: // Reset Transmitter
					// Reset Transmitter Control
					Uart.CR = (Uart.CR & ~0b1100) | 0b0100;

					// Reset TXEMT and set TXRDY
					// Player shell expects TXRDY to be set, otherwise it will hang!
					Uart.SR &= ~0b1000;
					Uart.SR |= 0b0100;

					Uart.transmitted.clear();
					Uart.THR = 0;
					break;

				case 0x40: // Reset Error Status
					Uart.SR &= ~0xF0; // all error bits
					break;

				case 0x60: // Start break
					// TO-DO
					break;

				case 0x70: // Stop break
					// TO-DO
					break;
			}
			break;
		case 0x2019: Uart.transmit(value); break; // CPU writes to this register(?)
		// case 0x201B: printf("RHR <= %02X\n", value); Uart.RHR = value; break; // Bus should normally NOT write here

		/** Timer **/
		case 0x2020: T.SR &= ~value; break;
		case 0x2021: T.CR = value; break;
		case 0x2022: T.RLR &= 0x00FF; T.RLR |= (value << 8); break;
		case 0x2023: T.RLR &= 0xFF00; T.RLR |= value; break;
		case 0x2024: T.T0 = 0x00FF; T.T0 |= (value << 8); break;
		case 0x2025: T.T0 = 0xFF00; T.T0 |= value; break;

		/** PICR **/
		/// When PIR (4th bit) is set for either peripheral, the pending interrupt is cleared, per datasheet.
		/// It seems to have the same behaviour as PIR for INT1N and INT2N.
		case 0x2045: PICR[0] = value;
			if (value & 0x80) interrupt(SCC68070::IPL_I2C, false);
			if (value & 0x08) interrupt(SCC68070::IPL_TIMER, false);
			break;
		case 0x2047: PICR[1] = value;
			if (value & 0x80) interrupt(SCC68070::IPL_UART_RX, false);
			if (value & 0x08) interrupt(SCC68070::IPL_UART_TX, false);
			break;

		/** DMA (ch1) **/
		case 0x4000: DMA1.CSR &= 0x08; break;
		case 0x4004: DMA1.DCR &= 0x08; DMA1.DCR |= (value & 0xF7); break;
		case 0x4005: DMA1.OCR = value; DMA1.DCR &= 0xF7; DMA1.DCR |= ((value >> 1) & 0x08); break;
		case 0x4006: DMA1.SCR = value; break;
		case 0x4007: DMA1.CCR = value; break;
		case 0x400a: DMA1.MTC &= 0x00FF; DMA1.MTC |= (value << 8); break;
		case 0x400b: DMA1.MTC &= 0xFF00; DMA1.MTC |= value; break;
		case 0x400c: DMA1.MAC &= 0x00FFFFFF; DMA1.MAC |= (value << 24); break;
		case 0x400d: DMA1.MAC &= 0xFF00FFFF; DMA1.MAC |= (value << 16); break;
		case 0x400e: DMA1.MAC &= 0xFFFF00FF; DMA1.MAC |= (value << 8); break;
		case 0x400f: DMA1.MAC &= 0xFFFFFF00; DMA1.MAC |= value; break;
		case 0x4014: DMA1.DAC &= 0x00FFFFFF; DMA1.DAC |= (value << 24); break;
		case 0x4015: DMA1.DAC &= 0xFF00FFFF; DMA1.DAC |= (value << 16); break;
		case 0x4016: DMA1.DAC &= 0xFFFF00FF; DMA1.DAC |= (value << 8); break;
		case 0x4017: DMA1.DAC &= 0xFFFFFF00; DMA1.DAC |= value; break;

		/** DMA (ch2) **/
		case 0x4040: DMA2.CSR &= 0x08; break;
		case 0x4044: DMA2.DCR &= 0x08; DMA2.DCR |= (value & 0xF7); break;
		case 0x4045: DMA2.OCR = value; DMA2.DCR &= 0xF7; DMA2.DCR |= ((value >> 1) & 0x08); break;
		case 0x4046: DMA2.SCR = value; break;
		case 0x4047: DMA2.CCR = value; break;
		case 0x404a: DMA2.MTC &= 0x00FF; DMA2.MTC |= (value << 8); break;
		case 0x404b: DMA2.MTC &= 0xFF00; DMA2.MTC |= value; break;
		case 0x404c: DMA2.MAC &= 0x00FFFFFF; DMA2.MAC |= (value << 24); break;
		case 0x404d: DMA2.MAC &= 0xFF00FFFF; DMA2.MAC |= (value << 16); break;
		case 0x404e: DMA2.MAC &= 0xFFFF00FF; DMA2.MAC |= (value << 8); break;
		case 0x404f: DMA2.MAC &= 0xFFFFFF00; DMA2.MAC |= value; break;
		case 0x4054: DMA2.DAC &= 0x00FFFFFF; DMA2.DAC |= (value << 24); break;
		case 0x4055: DMA2.DAC &= 0xFF00FFFF; DMA2.DAC |= (value << 16); break;
		case 0x4056: DMA2.DAC &= 0xFFFF00FF; DMA2.DAC |= (value << 8); break;
		case 0x4057: DMA2.DAC &= 0xFFFFFF00; DMA2.DAC |= value; break;
	}
}

void SCC68070::DMA::memory_transfer(uint32_t start_address)
{
	if (CCR & 0x80)
	{
		// Indicate that operation is in progress.
		CCR &= ~0x80; // START unset
		CSR &= 0b01001111; // COC, NDT and ERR unset
		CSR |= 0b00001000; // Channel Active set
		CER = 0;

		if (is_second) start_address = DAC;

		MiniCDI::Log("[SCC68070:DMA%d] $%08X %s $%08X (%d %s)", is_second ? 2 : 1,
					 start_address,
					 OCR & 0x80 ? "=>" : "<=",
					 MAC,
					 MTC,
					 OCR & 0x10 ? "words" : "bytes"/*,
					 DCR & 0x80 ? "cycle-steal" : "burst"*/);

		// Avoid segmentation fault
		if (MAC < 8388608 && start_address < 8388608)
		{
			while (MTC > 0)
			{
				if (CCR & 0x10)
				{
					MiniCDI::Log("[SCC68070:DMA%d] transfer aborted", is_second ? 2 : 1);
					CSR |= 0b10010000; // COC and ERR set
					CSR &= 0b11110111; // Channel Active unset
					CER = 0b10001u; // Abort Error
					return;
				}

				// Set operation based on external request mode (DCR bit 15)
				// Cycle-steal mode (transfer one byte at a time and continue looping to end)
				if (DCR & 0x80)
				{
					// Determine direction (OCR bit 7)
					// If bit set, device to memory
					if (OCR & 0x80)
					{
						SCC68070::memory[MAC] = SCC68070::memory[start_address++];
						if (OCR & 0x10)
							SCC68070::memory[MAC+1] = SCC68070::memory[start_address++];
					}
					// Otherwise, memory to device.
					else
					{
						SCC68070::memory[start_address++] = SCC68070::memory[MAC];
						if (OCR & 0x10)
							SCC68070::memory[start_address++] = SCC68070::memory[MAC+1];
					}

					// Increment MAC/DAC registers accordingly and decrement MTC.
					if (is_second && (SCR & 0x04))
					{
						MAC += OCR & 0x10 ? 2 : 1;
						DAC += OCR & 0x10 ? 2 : 1;
					}
					else if (!is_second)
						MAC += OCR & 0x10 ? 2 : 1;

					MTC--;
				}

				// Burst mode (transfer the memory block using memcpy)
				else
				{
					// Determine direction (OCR bit 7), same as before
					if (OCR & 0x80)
						memcpy(&SCC68070::memory[MAC], &SCC68070::memory[start_address], OCR & 0x10 ? MTC*2 : MTC);
					else
						memcpy(&SCC68070::memory[start_address], &SCC68070::memory[MAC], OCR & 0x10 ? MTC*2 : MTC);

					// Increment MAC/DAC registers accordingly and set MTC to 0.
					if (is_second && (SCR & 0x04))
					{
						MAC += OCR & 0x10 ? MTC*2 : MTC;
						DAC += OCR & 0x10 ? MTC*2 : MTC;
					}
					else if (!is_second)
						MAC += OCR & 0x10 ? MTC*2 : MTC;

					MTC = 0;
				}
			}
		}

		// Indicate that operation is completed.
		CSR |= 0b10000000; // COC set
		CSR &= 0b11110111; // Channel Active unset
		interrupt(is_second ? SCC68070::IPL_DMA2 : SCC68070::IPL_DMA1, true);
	}
}