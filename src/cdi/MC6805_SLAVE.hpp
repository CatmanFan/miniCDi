#ifndef MINICDI_MC6805_SLAVE
#define MINICDI_MC6805_SLAVE

#include <deque>

// HLE implementation of SLAVE as found in MiniMMC & Mono-I.
class SLAVE
{
	uint8_t* memory;
	SCC68070* cpu;

	struct
	{
		std::deque<uint8_t> In;
		std::deque<uint8_t> Out;
		size_t InSize;
	} Ch[4];

	bool pointer_used;
	bool polling;
	bool pointer;
	int pointer_x, pointer_y;

	uint32_t DR[4]; // addresses to data registers

	uint8_t LCD[16];

public:
	friend class PlayerLCD;
	friend class PointingDevice;

	SLAVE(SCC68070* cpu, uint8_t* memory, uint32_t start) : memory(memory), cpu(cpu), pointer_used(false), polling(false), pointer(false)
	{
		DR[0] = start + 0x01; // ADR
		DR[1] = start + 0x03; // BDR
		DR[2] = start + 0x05; // CDR
		DR[3] = start + 0x07; // DDR
		reset();
	}

	void reset()
	{
		memset(&LCD[0], 0, 16);
		Ch[0].InSize = 0;
		Ch[1].InSize = 0;
		Ch[2].InSize = 0;
		Ch[3].InSize = 0;
	}

	uint8_t read8(uint32_t addr)
	{
		if (addr == DR[0] || addr == DR[1] || addr == DR[2] || addr == DR[3])
		{
			size_t c = (addr - DR[0]) / 2;

			if (Ch[c].Out.size() > 0)
			{
				//MiniCDI::Log("[SLAVE] %sDR => %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].Out[0]);
				memory[DR[c]] = Ch[c].Out[0];
				Ch[c].Out.pop_front();
			}
			else
				memory[DR[c]] = 0xFF;
		}

		return memory[addr];
	}

	void write8(uint32_t addr, uint8_t value)
	{
		memory[addr] = value;
		if (addr == DR[0] || addr == DR[1] || addr == DR[2] || addr == DR[3])
		{
			size_t c = (addr - DR[0]) / 2;
			Ch[c].In.push_back(value);
			//MiniCDI::Log("[SLAVE] %sDR <= %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", value);

			switch (c)
			{
				/** ADR **/
				case 0:
					switch (value)
					{
						default:
							switch (Ch[c].InSize) {
								default:
									/** Set Pointer Pos **/
									if (pointer_used && value >= 0xC0 && value <= 0xFF
									 && Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
										Ch[c].InSize = 3;
									}
									break;
								case 3:
									if (Ch[c].In.size() >= Ch[c].InSize) {
										pointer_x = ((Ch[c].In[1] & 0x70) << 3) | (Ch[c].In[2] & 0x7F);
										pointer_y = ((Ch[c].In[0] & 0x3F) << 4) | (Ch[c].In[1] & 0x0F);
										MiniCDI::Log("[SLAVE] pointer x=%d,y=%d", pointer_x, pointer_y);
										Ch[c].In.clear();
										Ch[c].InSize = 0;
									}
									break;
							}
							break;

						/** Enable Pointer Input **/
						case 0x83:
							MiniCDI::Log("[SLAVE] enable pointer input");
							pointer_used = true;
							break;

						/** Disable Pointer Input **/
						case 0x84:
							MiniCDI::Log("[SLAVE] disable pointer input");
							pointer_used = false;
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
										MiniCDI::Log("[SLAVE] set LCD");
										Ch[c].In.clear();
										Ch[c].InSize = 0;
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
							MiniCDI::Log("[SLAVE] get disc status");
							Ch[c].Out = { 0xB0, 0x00, 0x02, 0x15 }; // use response data from MAME
							break;

						/** SLAVE rev **/
						case 0xF0:
							MiniCDI::Log("[SLAVE] get SLAVE revision");
							Ch[2].Out = { 0xF0, 0x32 }; // use response data from MAME
							break;

						/** Pointer Type **/
						case 0xF3:
							MiniCDI::Log("[SLAVE] get pointer type");
							Ch[2].Out = { 0xF3, 0x01 };
							/** cdifan: 1 => CL="c"; 2 => CL="d"; 3 => CL="b"; 4 => CL="a"; 5 => CL="c" + /kb1 **/
							break;

						/** Boot Mode **/
						case 0xF4:
							MiniCDI::Log("[SLAVE] get test plug status");
							Ch[2].Out = { 0xF4, 0x01 };
							break;

						/** Video Mode **/
						case 0xF6:
							MiniCDI::Log("[SLAVE] get video mode");
							{
								Ch[2].Out = { 0xF6, 0x02 };
							}
							break;

						/** Enable Polling **/
						case 0xF7:
							MiniCDI::Log("[SLAVE] enable polling data");
							polling = true;
							break;

						/** Enable X-Bus **/
						case 0xFA:
							MiniCDI::Log("[SLAVE] enable X-Bus");
							// TO-DO
							break;
					}
					break;
			}

			if (Ch[c].InSize == 0)
				Ch[c].In.clear();
		}
	}
};

#endif