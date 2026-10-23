#pragma once
#include <deque>

/// Handles on-chip peripherals for Philips 68070, outside of the CPU
/// Sourced from datasheets unless stated otherwise.

struct SCC68070
{
	// The memory/data bus
	inline static uint8_t* memory = NULL;

	/// **********************
	/// INTERRUPT LEVEL MANAGER
	/// **********************

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

	static inline struct IPL
	{
		uint8_t curr; // Current pending interrupt level
		uint8_t ack; // Acknowledged interrupt level
		bool levels[12];
		uint8_t vectors[12];

		IPL()
		{
			curr = 0;
			ack = 0;

			levels[0] = 0;
			levels[1] = 0;
			levels[2] = 0;
			levels[3] = 0;
			levels[4] = 0;
			levels[5] = 0;
			levels[6] = 0;
			levels[7] = 0;
			levels[8] = 0;
			levels[9] = 0;
			levels[10] = 0;
			levels[11] = 0;

			vectors[0] = 0;
			vectors[1] = 0;
			vectors[2] = 0;
			vectors[3] = 0;
			vectors[4] = 0;
			vectors[5] = 0;
			vectors[6] = 0;
			vectors[7] = 0;
			vectors[8] = 0;
			vectors[9] = 0;
			vectors[10] = 0;
			vectors[11] = 0;
		}
	} Ipl;

	/**
	 * @brief  Sets a peripheral's (or external's) pending interrupt to true or false.
	 *         It gets the level from the corresponding onchip register and then sets the Musashi IRQ through `update_ipl()`.
	 *
	 * @param  assert:  Whether the interrupt is asserted.
	 */
	static void interrupt(size_t index, bool assert);

	/// Called by Musashi's interrupt handler
	int interrupt_ack(int int_level);

	/// **********************
	/// ON-CHIP REGISTERS
	/// **********************
	static inline uint8_t LIR = 0;
	static inline uint8_t PICR[2] = {0,0};

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

				// SCC68070::interrupt(SCC68070::IPL_UART_TX, true);
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
	struct Timer
	{
		uint8_t SR;
		uint8_t CR;
		uint16_t RLR;

		uint16_t T0;
		uint16_t T1; // unused
		uint16_t T2; // unused

		int cycles_to_T0_interrupt;

		void T0_interrupt()
		{
			SR |= 0x80;
			T0 = RLR;
			SCC68070::interrupt(SCC68070::IPL_TIMER, true);

			cycles_to_T0_interrupt = (0xFFFF - RLR) * 96;
		}

		void reset()
		{
			SR = 0;
			CR = 0;
			RLR = 0;

			T0 = 0;	
			cycles_to_T0_interrupt = 0xFFFF * 96;
		}

		Timer()
		{
			reset();
		}
	} T;

	/// DMA
	struct DMA
	{
		bool is_second;

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

		void reset()
		{
			CSR = 0;
			CER = 0;
			DCR = 0;
			OCR = 0;
			SCR = 0;
			CCR = 0;
		}

		void memory_transfer(uint32_t start_address);

		DMA(bool is_second)
		{
			this->is_second = is_second;
			MTC = 0;
			MAC = 0;
			DAC = 0;
			CPR = 0;
			reset();
		}
	};
	static inline struct DMA DMA1 = DMA(false);
	static inline struct DMA DMA2 = DMA(true);

	/// I²C
	uint8_t IDR;
	uint8_t IAR;
	uint8_t ISR;
	uint8_t ICR;
	uint8_t ICCR;
	/// **********************

	uint8_t fc; // used for FC/address space callback

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

	void reset_onchip();
	void reset();

	uint8_t read8(uint32_t addr);
	void write8(uint32_t addr, uint8_t value);

};