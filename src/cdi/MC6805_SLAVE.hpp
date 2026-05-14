#ifndef MINICDI_MC6805_SLAVE
#define MINICDI_MC6805_SLAVE

#include "cdi/common.hpp"

// SLAVE as implemented in CD-i Mono-I board.
class SLAVE
{
	MiniCDIConfig *emuConfig;
	uint8_t* memory;

	struct
	{
		uint8_t DR; // Data Register
		uint8_t SR; // Status Register. for channel Status

		std::vector<uint8_t> In;
		std::vector<uint8_t> Out;
	} Ch[4];

	enum {
		Addr_ADR = 0x310001, // 0x200000 on Mini-MMC, Maxi-MMC, 0x310000 on Mono-I and Mono-II
		Addr_BDR = 0x310003,
		Addr_CDR = 0x310005,
		Addr_DDR = 0x310007
	} SlaveAddr;

	uint8_t LCD[16];

	/*void interrupt(int ch)
	{
		switch (ch)
		{
			case 0:
				ISR |= 0b00000010u;
				if (IMR & 0b00000010u)
					cpu->INT2();
				break;
			case 1:
				ISR |= 0b00001000u;
				if (IMR & 0b00001000u)
					cpu->INT2();
				break;
			case 2:
				ISR |= 0b00100000u;
				if (IMR & 0b00100000u)
					cpu->INT2();
				break;
			case 3:
				ISR |= 0b10000000u;
				if (IMR & 0b10000000u)
					cpu->INT2();
				break;
		}
	}*/

public:
	SLAVE(uint8_t* memory, MiniCDIConfig *config)
	: emuConfig(config), memory(memory)
	{
		/*// Mirror cdiemu emulation, set REMTY to 1 and TEMTY to always 1.
		Ch[0].SR = 0b00010001u;
		Ch[1].SR = 0b00010001u;
		Ch[2].SR = 0b00010001u;
		Ch[3].SR = 0b00010001u;*/
	}

	uint8_t read8(uint32_t addr)
	{
		switch (addr)
		{
			default: return memory[addr];

			case Addr_ADR:
			case Addr_BDR:
			case Addr_CDR:
			case Addr_DDR:
				size_t c = (addr - Addr_ADR) / 2;

				if (Ch[c].Out.size() > 0)
				{
					Ch[c].DR = Ch[c].Out[0];
					Ch[c].Out.erase(Ch[c].Out.begin());
				} else {
					Ch[c].DR = 0xFF;
				}

				#ifdef MINICDI_DEBUG
					// printf("[SLAVE] %sDR => %02X\n", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].DR);
				#endif
				return Ch[c].DR;
		}
	}

	void write8(uint32_t addr, uint8_t value)
	{
		switch (addr)
		{
			default: memory[addr] = value; break;

			case Addr_ADR:
			case Addr_BDR:
			case Addr_CDR:
			case Addr_DDR:
				size_t c = (addr - Addr_ADR) / 2;
				Ch[c].In.push_back(value);
				#ifdef MINICDI_DEBUG
					// printf("[SLAVE] %sDR <= %02X\n", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", value);
				#endif

				switch (c)
				{
					case 1:
						switch (value)
						{
							default:
								Ch[c].In.clear();
								break;

							/** Set Front Panel LCD **/
							case 0xF0:
								if (Ch[c].In.size() >= 17)
								{
									for (int i = 0; i < 16; i++)
										LCD[i] = Ch[c].In[1+i];

									Ch[c].In.clear();
								}
								break;
						}
						break;

					case 3:
						switch (value)
						{
							/** Video Mode **/
							case 0xF6:
								Ch[2].Out = { 0xF6, (uint8_t)(emuConfig->pal ? 0x02 : 0x01) };
								// Ch[c].SR &= ~(0b00010000u);
								// interrupt(c);
								break;
						}
						break;
				}
				break;
		}
	}

	uint8_t *get_lcd()
	{
		return &LCD[0];
	}
};

#endif