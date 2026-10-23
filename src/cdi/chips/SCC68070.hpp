#pragma once
#include <deque>

/// Handles on-chip peripherals for Philips 68070, outside of the CPU
/// Sourced from datasheets unless stated otherwise.

struct SCC68070
{
	uint8_t* memory;

	/// **********************
	/// ON-CHIP REGISTERS
	/// **********************
	uint8_t LIR;
	uint8_t PICR[2];

	/// UART (officially titled RS232-C)
	struct UART
	{
		uint8_t MR; // Mode Register
		uint8_t CSR; // Clock Select Register
		uint8_t CR; // Command Register
		uint8_t SR; // Status Register

		uint8_t THR; // Transmit Holding Register
		uint8_t RHR; // Receive Holding Register

		/// issued by a RESET instruction or signal
		void reset()
		{
			MR = 0;
			CSR = 0;
			CR = 0;
			SR = 0;
		}

		UART()
		{
			reset();
		}

		std::deque<uint8_t> transmitted;
		void transmit(uint8_t data)
		{
			// If transmitter is enabled
			if ((CR & 0b1100) != 0b0100)
			{
				SR &= ~0x08; // clear TXEMT
				SR |= 0x04; // TXRDY
				THR = data;
				transmitted.push_back(data);
				MiniCDI::Log("[SCC68070:UART] Transmitted byte %02X", data);
			}
		}

		uint8_t receive()
		{
			// If receiver is enabled
			if ((CR & 0b0011) != 0b0001)
			{
				MiniCDI::Log("[SCC68070:UART] Received byte %02X", RHR);
				return RHR;
			}

			return 0;
		}
	} Uart;

	/// Timer
	uint8_t TSR;
	uint8_t TCR;
	uint16_t RR;
	uint16_t T0;

	/// DMA
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

	/// I²C
	uint8_t IDR;
	uint8_t IAR;
	uint8_t ISR;
	uint8_t ICR;
	uint8_t ICCR;
	/// **********************

	uint8_t fc; // used for FC/address space callback

	// Priority order of interrupt signal booleans
	enum
	{
		IPL_IN7N = 0, // External: NMI (Non-Maskable Interrupt)
		IPL_IN5N, // External
		IPL_IN4N, // External: used by CD/audio microcontroller
		IPL_IN2N, // External: used by Slave microcontroller
		IPL_INT1, // On-chip: used by video interrupt (calls Ev$Pulse+Ev$All in OS-9)
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
	void interrupt(size_t index, bool assert);

	/// Called by Musashi's interrupt handler
	int interrupt_ack(int int_level);

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

	void reset_internal();
	void reset();

	uint8_t read8(uint32_t addr);
	void write8(uint32_t addr, uint8_t value);

	int timer0_cycles_to_tick;
	void timer0_tick();
	void dma_call(size_t index, uint32_t start_address);
};