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

void SCC68070::reset_internal()
{
	LIR = 0;
	Uart.reset();

	PICR[0] = PICR[1] = 0;

	// Timer(s)
	TSR = TCR = RR = T0 = 0;
	timer0_cycles_to_tick = 0xFFFF * 192;

	// DMA
	DMA[0].CER = DMA[0].DCR = DMA[0].OCR = DMA[0].SCR = DMA[0].CCR = 0;
	DMA[1].CER = DMA[1].DCR = DMA[1].OCR = DMA[1].SCR = DMA[1].CCR = 0;
	DMA[0].CSR = DMA[1].CSR = 0;

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
	Ipl = {0};
	reset_internal();

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

		/** Timer (status) **/
		case 0x2020: return TSR;
		case 0x2021: return TCR;
		case 0x2022: return (RR >> 8) & 0x00FF;
		case 0x2023: return RR & 0x00FF;
		/** Timer (0) **/
		case 0x2024: return (T0 >> 8) & 0x00FF;
		case 0x2025: return T0 & 0x00FF;

		/** PICR **/
		case 0x2045: return PICR[0] & 0x77;
		case 0x2047: return PICR[1] & 0x77;

		/** DMA (ch1) **/
		case 0x4000: return DMA[0].CSR;
		case 0x4001: return DMA[0].CER;
		case 0x4004: return DMA[0].DCR;
		case 0x4005: return DMA[0].OCR;
		case 0x4006: return DMA[0].SCR;
		case 0x4007: return DMA[0].CCR & 0xEF;
		case 0x400a: return (DMA[0].MTC >> 8) & 0x00FF;
		case 0x400b: return DMA[0].MTC & 0x00FF;
		case 0x400c: return (DMA[0].MAC >> 24) & 0x000000FF;
		case 0x400d: return (DMA[0].MAC >> 16) & 0x000000FF;
		case 0x400e: return (DMA[0].MAC >> 8) & 0x000000FF;
		case 0x400f: return DMA[0].MAC & 0x000000FF;
		case 0x4014: return (DMA[0].DAC >> 24) & 0x000000FF;
		case 0x4015: return (DMA[0].DAC >> 16) & 0x000000FF;
		case 0x4016: return (DMA[0].DAC >> 8) & 0x000000FF;
		case 0x4017: return DMA[0].DAC & 0x000000FF;

		/** DMA (ch2) **/
		case 0x4040: return DMA[1].CSR;
		case 0x4041: return DMA[1].CER;
		case 0x4044: return DMA[1].DCR;
		case 0x4045: return DMA[1].OCR;
		case 0x4046: return DMA[1].SCR;
		case 0x4047: return DMA[1].CCR & 0xEF;
		case 0x404a: return (DMA[1].MTC >> 8) & 0x00FF;
		case 0x404b: return DMA[1].MTC & 0x00FF;
		case 0x404c: return (DMA[1].MAC >> 24) & 0x000000FF;
		case 0x404d: return (DMA[1].MAC >> 16) & 0x000000FF;
		case 0x404e: return (DMA[1].MAC >> 8) & 0x000000FF;
		case 0x404f: return DMA[1].MAC & 0x000000FF;
		case 0x4054: return (DMA[1].DAC >> 24) & 0x000000FF;
		case 0x4055: return (DMA[1].DAC >> 16) & 0x000000FF;
		case 0x4056: return (DMA[1].DAC >> 8) & 0x000000FF;
		case 0x4057: return DMA[1].DAC & 0x000000FF;
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

		/** Timer (status) **/
		case 0x2020: TSR &= ~value; break;
		case 0x2021: TCR = value; break;
		case 0x2022: RR &= 0x00FF; RR |= (value << 8); break;
		case 0x2023: RR &= 0xFF00; RR |= value; break;
		/** Timer (0) **/
		case 0x2024: T0 &= 0x00FF; T0 |= (value << 8); break;
		case 0x2025: T0 &= 0xFF00; T0 |= value; break;

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
		case 0x4000: DMA[0].CSR &= 0x08; break;
		case 0x4004: DMA[0].DCR &= 0x08; DMA[0].DCR |= (value & 0xF7); break;
		case 0x4005: DMA[0].OCR = value; DMA[0].DCR &= 0xF7; DMA[0].DCR |= ((value >> 1) & 0x08); break;
		case 0x4006: DMA[0].SCR = value; break;
		case 0x4007: DMA[0].CCR = value; break;
		case 0x400a: DMA[0].MTC &= 0x00FF; DMA[0].MTC |= (value << 8); break;
		case 0x400b: DMA[0].MTC &= 0xFF00; DMA[0].MTC |= value; break;
		case 0x400c: DMA[0].MAC &= 0x00FFFFFF; DMA[0].MAC |= (value << 24); break;
		case 0x400d: DMA[0].MAC &= 0xFF00FFFF; DMA[0].MAC |= (value << 16); break;
		case 0x400e: DMA[0].MAC &= 0xFFFF00FF; DMA[0].MAC |= (value << 8); break;
		case 0x400f: DMA[0].MAC &= 0xFFFFFF00; DMA[0].MAC |= value; break;
		case 0x4014: DMA[0].DAC &= 0x00FFFFFF; DMA[0].DAC |= (value << 24); break;
		case 0x4015: DMA[0].DAC &= 0xFF00FFFF; DMA[0].DAC |= (value << 16); break;
		case 0x4016: DMA[0].DAC &= 0xFFFF00FF; DMA[0].DAC |= (value << 8); break;
		case 0x4017: DMA[0].DAC &= 0xFFFFFF00; DMA[0].DAC |= value; break;

		/** DMA (ch2) **/
		case 0x4040: DMA[1].CSR &= 0x08; break;
		case 0x4044: DMA[1].DCR &= 0x08; DMA[1].DCR |= (value & 0xF7); break;
		case 0x4045: DMA[1].OCR = value; DMA[1].DCR &= 0xF7; DMA[1].DCR |= ((value >> 1) & 0x08); break;
		case 0x4046: DMA[1].SCR = value; break;
		case 0x4047: DMA[1].CCR = value; break;
		case 0x404a: DMA[1].MTC &= 0x00FF; DMA[1].MTC |= (value << 8); break;
		case 0x404b: DMA[1].MTC &= 0xFF00; DMA[1].MTC |= value; break;
		case 0x404c: DMA[1].MAC &= 0x00FFFFFF; DMA[1].MAC |= (value << 24); break;
		case 0x404d: DMA[1].MAC &= 0xFF00FFFF; DMA[1].MAC |= (value << 16); break;
		case 0x404e: DMA[1].MAC &= 0xFFFF00FF; DMA[1].MAC |= (value << 8); break;
		case 0x404f: DMA[1].MAC &= 0xFFFFFF00; DMA[1].MAC |= value; break;
		case 0x4054: DMA[1].DAC &= 0x00FFFFFF; DMA[1].DAC |= (value << 24); break;
		case 0x4055: DMA[1].DAC &= 0xFF00FFFF; DMA[1].DAC |= (value << 16); break;
		case 0x4056: DMA[1].DAC &= 0xFFFF00FF; DMA[1].DAC |= (value << 8); break;
		case 0x4057: DMA[1].DAC &= 0xFFFFFF00; DMA[1].DAC |= value; break;
	}
}

void SCC68070::timer0_tick()
{
	if (T0 == 0xFFFF)
	{
		TSR |= 0x80;
		T0 = RR;
		interrupt(SCC68070::IPL_TIMER, true);
	}
	else
		T0++;
}

void SCC68070::dma_call(size_t index, uint32_t start_address)
{
	if (DMA[index].CCR & 0x80)
	{
		// Indicate that operation is in progress.
		DMA[index].CCR &= ~0x80; // START unset
		DMA[index].CSR &= 0b01001111; // COC, NDT and ERR unset
		DMA[index].CSR |= 0b00001000; // Channel Active set
		DMA[index].CER = 0;

		// DMA1
		if (index == 1)
			start_address = DMA[1].DAC;

		MiniCDI::Log("[SCC68070:DMA%d] $%08X %s $%08X (%d %s)", index+1,
					 start_address,
					 DMA[index].OCR & 0x80 ? "=>" : "<=",
					 DMA[index].MAC,
					 DMA[index].MTC,
					 DMA[index].OCR & 0x10 ? "words" : "bytes"/*,
					 DMA[index].DCR & 0x80 ? "cycle-steal" : "burst"*/);

		// Avoid segmentation fault
		if (DMA[index].MAC < 8388608 && start_address < 8388608)
		{
			while (DMA[index].MTC > 0)
			{
				if (DMA[index].CCR & 0x10)
				{
					MiniCDI::Log("[SCC68070:DMA%d] transfer aborted", index+1);
					DMA[index].CSR |= 0b10010000; // COC and ERR set
					DMA[index].CSR &= 0b11110111; // Channel Active unset
					DMA[index].CER = 0b10001u; // Abort Error
					return;
				}

				// Set operation based on external request mode (DCR bit 15)
				// Cycle-steal mode (transfer one byte at a time and continue looping to end)
				if (DMA[index].DCR & 0x80)
				{
					// Determine direction (OCR bit 7)
					// If bit set, device to memory
					if (DMA[index].OCR & 0x80)
					{
						memory[DMA[index].MAC] = memory[start_address++];
						if (DMA[index].OCR & 0x10) memory[DMA[index].MAC+1] = memory[start_address++];
					}
					// Otherwise, memory to device.
					else
					{
						memory[start_address++] = memory[DMA[index].MAC];
						if (DMA[index].OCR & 0x10) memory[start_address++] = memory[DMA[index].MAC+1];
					}

					// Increment MAC/DAC registers accordingly and decrement MTC.
					if (index == 1 && (DMA[index].SCR & 0x04)) {
						DMA[index].MAC += DMA[index].OCR & 0x10 ? 2 : 1;
						DMA[index].DAC += DMA[index].OCR & 0x10 ? 2 : 1;
					} else if (index == 0) {
						DMA[index].MAC += DMA[index].OCR & 0x10 ? 2 : 1;
					}

					DMA[index].MTC--;
				}

				// Burst mode (transfer the memory block using memcpy)
				else
				{
					// Determine direction (OCR bit 7), same as before
					if (DMA[index].OCR & 0x80)
						memcpy(&memory[DMA[index].MAC], &memory[start_address], DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC);
					else
						memcpy(&memory[start_address], &memory[DMA[index].MAC], DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC);

					// Increment MAC/DAC registers accordingly and set MTC to 0.
					if (index == 1 && (DMA[index].SCR & 0x04)) {
						DMA[index].MAC += DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC;
						DMA[index].DAC += DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC;
					} else if (index == 0) {
						DMA[index].MAC += DMA[index].OCR & 0x10 ? DMA[index].MTC*2 : DMA[index].MTC;
					}

					DMA[index].MTC = 0;
				}
			}
		}

		// Indicate that operation is completed.
		DMA[index].CSR |= 0b10000000; // COC set
		DMA[index].CSR &= 0b11110111; // Channel Active unset
		interrupt(index == 1 ? SCC68070::IPL_DMA2 : SCC68070::IPL_DMA1, true);
	}
}