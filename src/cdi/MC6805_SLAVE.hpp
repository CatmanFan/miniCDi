#ifndef MINICDI_MC6805_SLAVE
#define MINICDI_MC6805_SLAVE

#include "cdi/common.hpp"

// HLE implementation of SLAVE as found in CD-i Mono-I board.
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
		size_t InSize;
	} Ch[4];

	struct
	{
		bool Active;
		int X;
		int Y;
		bool Button[2];
	} Pointer;

	enum {
		Addr_ADR = 0x310001, // 0x200000 on Mini-MMC, Maxi-MMC, 0x310000 on Mono-I and Mono-II
		Addr_BDR = 0x310003,
		Addr_CDR = 0x310005,
		Addr_DDR = 0x310007
	} SlaveAddr;

	uint8_t LCD[16];

public:
	SLAVE(uint8_t* memory, MiniCDIConfig *config)
	: emuConfig(config), memory(memory)
	{
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
				printf("[SLAVE] %sDR => %02X\n", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].DR);
				#endif
				return Ch[c].DR;
		}
	}

	void write8(uint32_t addr, uint8_t value, SCC68070* cpu)
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
				printf("[SLAVE] %sDR <= %02X\n", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", value);
				#endif

				switch (c)
				{
					/** ADR **/
					case 0:
						switch (value)
						{
							default:
								if (!Pointer.Active) {
									Ch[c].In.clear();
								} else if (value >= 0xC0) {
									Ch[c].InSize = 3;
									if (Ch[c].In.size() >= Ch[c].InSize) {
										Pointer.X = ((Ch[c].In[1] & 0x70) << 3) | (Ch[c].In[2] & 0x7F);
										Pointer.Y = ((Ch[c].In[0] & 0x3F) << 4) | (Ch[c].In[1] & 0x0F);
										Ch[c].In.clear();
										Ch[c].InSize = 0;
									}
								}
								break;

							/** Enable Pointer Input **/
							case 0x83:
								Ch[c].Out = { 0x83, (uint8_t)((Pointer.Button[1] << 5) | (Pointer.Button[0] << 4) | (Pointer.Active << 3)
														   | ((Pointer.X & 0b111'0000000) >> 7)),
													(uint8_t)(Pointer.X & 0b000'1111111),
													(uint8_t)((Pointer.Y & 0b111'0000000) >> 7),
													(uint8_t)(Pointer.Y & 0b000'1111111) };
								Ch[c].SR &= ~(0b00010000u);
								cpu->interrupt(c);
								Pointer.Active = true;

								#ifdef MINICDI_DEBUG
								printf("[SLAVE] enable pointer input\n");
								#endif
								Ch[c].In.clear();
								break;

							/** Disable Pointer Input **/
							case 0x84:
								Pointer.Active = false;
								Ch[c].In.clear();
								break;
						}
						break;

					/** BDR **/
					case 1:
						switch (value)
						{
							default:
								switch (Ch[c].InSize) {
									case 16:
									case 17:
										LCD[Ch[c].In.size() - (Ch[c].InSize == 16 ? 1 : 2)] = value;
										if (Ch[c].In.size() >= Ch[c].InSize) {
											Ch[c].In.clear();
											Ch[c].InSize = 0;

											#ifdef MINICDI_DEBUG
											printf("[SLAVE] set LCD: ");
											for (int i = 0; i < 16; i++) printf("%02X ", LCD[i]);
											printf("\n");
											#endif
										}
										break;
								}
								break;

							/** Set Front Panel LCD **/
							case 0xF0:
								if (Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
									Ch[c].InSize = 17;
								}
								break;
						}
						break;

					/** CDR **/
					case 2:
						switch (value)
						{
							/** Set Front Panel LCD **/
							case 0xF0:
								Ch[1].InSize = 16; // redirects LCD display input to BDR
								Ch[c].In.clear();
								break;
						}
						break;

					/** DDR **/
					case 3:
						switch (value)
						{
							default:
								break;

							/** Disc Status **/
							case 0xB0:
								Ch[c].Out = { 0xB0, 0x00, 0x02, 0x15 }; // use response data from MAME
								Ch[c].SR &= ~(0b00010000u);
								cpu->interrupt(c);

								#ifdef MINICDI_DEBUG
								printf("[SLAVE] received disc status\n");
								#endif
								Ch[c].In.clear();
								break;

							/** SLAVE rev **/
							case 0xF0:
								Ch[2].Out = { 0xF0, 0x32 }; // use response data from MAME
								Ch[c].SR &= ~(0b00010000u);
								cpu->interrupt(c);

								#ifdef MINICDI_DEBUG
								printf("[SLAVE] received SLAVE revision\n");
								#endif
								Ch[c].In.clear();
								break;

							/** Pointer Type **/
							case 0xF3:
								Ch[2].Out = { 0xF3, 0x01 }; // use response data from MAME
								Ch[c].SR &= ~(0b00010000u);
								cpu->interrupt(c);

								#ifdef MINICDI_DEBUG
								printf("[SLAVE] received pointer type\n");
								#endif
								Ch[c].In.clear();
								break;

							/** Test Plug Status (enables service menu) **/
							case 0xF4:
								Ch[2].Out = { 0xF4, 0x00 }; // use response data from MAME
								Ch[c].SR &= ~(0b00010000u);
								cpu->interrupt(c);

								#ifdef MINICDI_DEBUG
								printf("[SLAVE] received test plug status\n");
								#endif
								Ch[c].In.clear();
								break;

							/** Video Mode **/
							case 0xF6:
								Ch[2].Out = { 0xF6, (uint8_t)(emuConfig->pal == true ? 0x01 : 0x02) };
								Ch[c].SR &= ~(0b00010000u);
								cpu->interrupt(c);

								#ifdef MINICDI_DEBUG
								printf("[SLAVE] received video mode\n");
								#endif
								Ch[c].In.clear();
								break;

							/** Enable X-Bus **/
							case 0xFA:
								Ch[c].In.clear();
								break;
						}
						break;
				}

				if (Ch[c].InSize == 0)
					Ch[c].In.clear();
				break;
		}
	}

	uint8_t *get_lcd()
	{
		return &LCD[0];
	}
};

#endif