#ifndef MINICDI_MC6805_SLAVE
#define MINICDI_MC6805_SLAVE

#include <deque>

/// HLE implementation of SLAVE as found in MiniMMC & Mono-I.
class SLAVE
{
	SCC68070* _68070;
	uint8_t* memory;

	struct
	{
		std::deque<uint8_t> In;
		std::deque<uint8_t> Out;
		size_t InSize;
		size_t ReadSize;
	} Ch[4];

	struct
	{
		bool connected;
		bool enabled;
		bool posChanged;
		int x, y;
	} PointerInterface;

	uint32_t DR[4]; // addresses to data registers

	uint8_t LCD[16];

	// For PointingDevice !!
	void assert_irq() { _68070->interrupt(SCC68070::IPL_IN2N, true); }

public:
	friend class PlayerLCD;
	friend class PointingDevice;

	SLAVE(SCC68070* _68070, uint8_t* memory, uint32_t start) : _68070(_68070), memory(memory), PointerInterface({0})
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

			// deassert IRQ
			_68070->interrupt(SCC68070::IPL_IN2N, false);

			if (Ch[c].Out.size() > 0)
			{
				//MiniCDI::Log("[SLAVE] %sDR => %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].Out[0]);

				memory[DR[c]] = Ch[c].Out[0];
				Ch[c].Out.pop_front();
				Ch[c].ReadSize++;
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
							if (Ch[c].InSize > 0 && Ch[c].In.size() >= Ch[c].InSize) {
								PointerInterface.x = ((Ch[c].In[1] & 0x70) << 3) | (Ch[c].In[2] & 0x7F);
								PointerInterface.y = ((Ch[c].In[0] & 0x3F) << 4) | (Ch[c].In[1] & 0x0F);
								PointerInterface.posChanged = true;
								//MiniCDI::Log("[SLAVE] pointer x=%d,y=%d", PointerInterface.x, PointerInterface.y);
								Ch[c].In.clear();
								Ch[c].InSize = 0;
								Ch[c].ReadSize = 0;
								return;
							} else {
								/** Set Pointer Pos **/
								if (PointerInterface.enabled && value >= 0xC0 && value <= 0xFF
								 && Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
									Ch[c].InSize = 3;
									return;
								}
							}
							break;

						/** Enable Pointer Input **/
						case 0x83:
							MiniCDI::Log("[SLAVE] enable pointer input (0x%02X)", value);
							PointerInterface.enabled = true;
							break;

						/** Disable Pointer Input **/
						case 0x84:
							MiniCDI::Log("[SLAVE] disable pointer input (0x%02X)", value);
							PointerInterface.enabled = false;
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
										Ch[c].ReadSize = 0;
									}
									return;
							}
							break;

						/** Set Front Panel LCD **/
						case 0xF0:
							if (Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
								//MiniCDI::Log("[SLAVE] set LCD (0x%02X)", value);
								Ch[c].InSize = 17;
							}
							break;
					}
					break;

				/** CDR **/
				case 2:
					switch (value)
					{
						/** Reset CPU **/
						case 0x8A:
							MiniCDI::Log("[SLAVE] reset CPU (0x%02X)", value);
							if (_68070 != NULL) _68070->reset();
							break;

						/** Set Front Panel LCD **/
						case 0xF0:
							//MiniCDI::Log("[SLAVE] set LCD (0x%02X)", value);
							Ch[1].InSize = 16; // redirects LCD display input to BDR
							break;
					}
					break;

				/** DDR **/
				case 3:
					switch (value)
					{
						default:
							if (Ch[c].InSize > 0 && Ch[c].In.size() >= Ch[c].InSize) {
								switch (Ch[c].In[0]) {
									case 0xB0:
										MiniCDI::Log("[SLAVE] get disc status (0x%02X%02X%02X%02X)", Ch[c].In[0], Ch[c].In[1], Ch[c].In[2], Ch[c].In[3]);
										if (MiniCDI::Config::HasDisc) Ch[c].Out = { 0xB0, 0x00, 0x02, 0x15 }; // cdifan: $000215 for SLAVE 1.x-4.x, $000610 for SLAVE 6.0 (CD-i rev 350)
										else Ch[c].Out = { 0xB0, 0x00, 0x00, 0x00 };
										assert_irq();
										break;
									case 0xB1:
										MiniCDI::Log("[SLAVE] get disc base (0x%02X%02X%02X%02X)", Ch[c].In[0], Ch[c].In[1], Ch[c].In[2], Ch[c].In[3]);
										Ch[c].Out = { 0xB1, 0x00, 0x00, 0x00 }; // use response data from MAME
										assert_irq();
										break;
								}
								Ch[c].In.clear();
								Ch[c].InSize = 0;
								Ch[c].ReadSize = 0;
							}
							break;

						/** Disc Status **/
						case 0xB0:
							if (Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
								Ch[c].InSize = 4;
							}
							break;

						/** Disc Base **/
						case 0xB1:
							if (Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
								Ch[c].InSize = 4;
							}
							break;

						/** SLAVE rev **/
						case 0xF0:
							MiniCDI::Log("[SLAVE] get SLAVE revision (0x%02X)", value);
							Ch[2].Out = { 0xF0, 0x32 }; // use response data from MAME
							assert_irq();
							break;

						/** Pointer Type **/
						case 0xF3:
							MiniCDI::Log("[SLAVE] get pointer type (0x%02X)", value);
							Ch[2].Out = { 0xF3, 0x01 }; /** cdifan: 1 => CL="c"; 2 => CL="d"; 3 => CL="b"; 4 => CL="a"; 5 => CL="c" + /kb1 **/
							assert_irq();
							break;

						/** Boot Mode **/
						case 0xF4:
							MiniCDI::Log("[SLAVE] get test plug status (0x%02X)", value);
							Ch[2].Out = { 0xF4, (uint8_t)(MiniCDI::Config::TestPlug ? 0x01 : 0x00) };
							assert_irq();
							break;

						/** Video Mode **/
						case 0xF6:
							MiniCDI::Log("[SLAVE] get video mode (0x%02X)", value);
							Ch[2].Out = { 0xF6, (uint8_t)(MiniCDI::Config::PAL ? 0x02 : 0x01) };
							assert_irq(); // interrupt not required on MAME ?
							break;

						/** Enable Polling **/
						case 0xF7:
							MiniCDI::Log("[SLAVE] enable polling data (0x%02X)", value);
							break;

						/** Enable X-Bus **/
						case 0xFA:
							MiniCDI::Log("[SLAVE] enable X-Bus (0x%02X)", value);
							// TO-DO
							break;
					}
					break;
			}

			if (Ch[c].InSize == 0) {
				Ch[c].In.clear();
				Ch[c].ReadSize = 0;
			}
		}
	}
};

#endif