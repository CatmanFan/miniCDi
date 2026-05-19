#ifndef MINICDI_MC6805_IKAT
#define MINICDI_MC6805_IKAT

#include "cdi/common.hpp"

// HLE implementation of IKAT as found in Mono-III & Mono-IV.
class IKAT
{
	MiniCDIConfig *emuConfig;
	uint8_t* memory;

	struct
	{
		uint8_t DR; // Data Register
		uint8_t SR; // Status Register. for channel Status

		std::vector<uint8_t> In;
		std::vector<uint8_t> Out;
		size_t InSize;
	} Ch[4];

	uint8_t ISR;
	uint8_t IMR;
	uint8_t MR;

	uint32_t addresses[15];

	uint8_t LCD[16];

public:
	IKAT(uint8_t* memory, uint32_t start, MiniCDIConfig *config)
	: emuConfig(config), memory(memory)
	{
		addresses[0] = start + 0x01; // ADRW
		addresses[1] = start + 0x03; // BDRW
		addresses[2] = start + 0x05; // CDRW
		addresses[3] = start + 0x07; // DDRW
		addresses[4] = start + 0x09; // ADRR
		addresses[5] = start + 0x0B; // BDRR
		addresses[6] = start + 0x0D; // CDRR
		addresses[7] = start + 0x0F; // DDRR
		addresses[8] = start + 0x11; // ASR
		addresses[9] = start + 0x13; // BSR
		addresses[10] = start + 0x15; // CSR
		addresses[11] = start + 0x17; // DSR
		addresses[12] = start + 0x19; // ISR
		addresses[13] = start + 0x1B; // IMR
		addresses[14] = start + 0x1D; // MR

		memset(&LCD[0], 0, 16);
		Ch[0].InSize = 0;
		Ch[1].InSize = 0;
		Ch[2].InSize = 0;
		Ch[3].InSize = 0;
	}

	void set_pointer_x(int value, bool increment)
	{
		;
	}

	void set_pointer_y(int value, bool increment)
	{
		;
	}

	void set_pointer_button(int button, bool value)
	{
		;
	}

	uint8_t read8(uint32_t addr, SCC68070* cpu)
	{
		if (addr == addresses[4] || addr == addresses[5] || addr == addresses[6] || addr == addresses[7])
		{
			size_t c = (addr - addresses[4]) / 2;

			if (Ch[c].Out.size() > 0)
			{
				Ch[c].SR &= ~(0x10); // REMTY OFF
				Ch[c].DR = Ch[c].Out[0];
				Ch[c].Out.erase(Ch[c].Out.begin());

				// set corresponding Rx bit
				uint8_t INT = 1 << (c+4);
				ISR |= INT;
				if (IMR & INT) {
					cpu->interrupt(0);
				}
			} else {
				Ch[c].SR |= 0x10; // REMTY ON
				Ch[c].DR = 0xFF;
			}

			MiniCDI::Log("[IKAT] %sDRR %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].DR);
			return Ch[c].DR;
		}

		if (addr == addresses[8] || addr == addresses[9] || addr == addresses[10] || addr == addresses[11])
		{
			size_t c = (addr - addresses[4]) / 2;

			MiniCDI::Log("[IKAT] %sSR %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].SR);
			return Ch[c].SR | 0x01; // imitate cdiemu
		}

		if (addr == addresses[12]) { MiniCDI::Log("[IKAT] ISR %02X", ISR); return ISR; }
		if (addr == addresses[13]) { MiniCDI::Log("[IKAT] IMR %02X", IMR); return IMR; }
		if (addr == addresses[14]) { MiniCDI::Log("[IKAT] MR %02X", MR); return MR; }

		return memory[addr];
	}

	void write8(uint32_t addr, uint8_t value)
	{
		if (addr == addresses[0] || addr == addresses[1] || addr == addresses[2] || addr == addresses[3])
		{
			size_t c = (addr - addresses[0]) / 2;
			Ch[c].In.push_back(value);
			MiniCDI::Log("[IKAT] %sDRW %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", value);

			switch (c)
			{
				/** CDR **/
				case 2:
					switch (value)
					{
						/** Boot Mode **/
						case 0xF4:
							Ch[c].Out = { 0xA5, 0xF4, 0x00 };
							Ch[c].In.clear();
							break;

						/** Video Mode **/
						case 0xF6:
							Ch[c].Out = { 0xA5, 0xF6, 0x02, 0xFF };
							Ch[c].In.clear();
							break;
					}
					break;
			}

			if (Ch[c].InSize == 0)
				Ch[c].In.clear();
		}

		if (addr == addresses[12]) ISR = value;
		if (addr == addresses[13]) IMR = value;
		if (addr == addresses[14]) MR = value;
	}

	uint8_t *get_lcd()
	{
		return &LCD[0];
	}
};

#endif