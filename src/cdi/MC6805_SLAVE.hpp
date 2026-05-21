#ifndef MINICDI_MC6805_SLAVE
#define MINICDI_MC6805_SLAVE

// HLE implementation of SLAVE as found in MiniMMC & Mono-I.
class SLAVE
{
	uint8_t* memory;

	struct
	{
		uint8_t DR; // Data Register

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

	uint32_t addresses[4];

	uint8_t LCD[16];

	void send_pointer_msg()
	{
		Ch[0].Out = { 0x83, (uint8_t)((Pointer.Button[1] << 5) | (Pointer.Button[0] << 4) | (Pointer.Active << 3)
								   | ((Pointer.X & 0b111'0000000) >> 7)),
							(uint8_t)(Pointer.X & 0b000'1111111),
							(uint8_t)((Pointer.Y & 0b111'0000000) >> 7),
							(uint8_t)(Pointer.Y & 0b000'1111111) };
	}

public:
	SLAVE(uint8_t* memory, uint32_t start) : memory(memory)
	{
		addresses[0] = start + 0x01; // ADR
		addresses[1] = start + 0x03; // BDR
		addresses[2] = start + 0x05; // CDR
		addresses[3] = start + 0x07; // DDR
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
		if (addr == addresses[0] || addr == addresses[1] || addr == addresses[2] || addr == addresses[3])
		{
			size_t c = (addr - addresses[0]) / 2;

			if (Ch[c].Out.size() > 0)
			{
				Ch[c].DR = Ch[c].Out[0];
				Ch[c].Out.erase(Ch[c].Out.begin());
			} else {
				Ch[c].DR = 0xFF;
			}

			MiniCDI::Log("[SLAVE] %sDR => %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].DR);
			return Ch[c].DR;
		}

		return memory[addr];
	}

	void write8(uint32_t addr, uint8_t value)
	{
		if (addr == addresses[0] || addr == addresses[1] || addr == addresses[2] || addr == addresses[3])
		{
			size_t c = (addr - addresses[0]) / 2;
			Ch[c].In.push_back(value);
			MiniCDI::Log("[SLAVE] %sDR <= %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", value);

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
							MiniCDI::Log("[SLAVE] enable pointer input");
							Pointer.Active = true;
							send_pointer_msg();
							Ch[c].In.clear();
							break;

						/** Disable Pointer Input **/
						case 0x84:
							MiniCDI::Log("[SLAVE] disable pointer input");
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
							MiniCDI::Log("[SLAVE] get disc status");
							Ch[c].Out = { 0xB0, 0x00, 0x02, 0x15 }; // use response data from MAME
							Ch[c].In.clear();
							break;

						/** SLAVE rev **/
						case 0xF0:
							MiniCDI::Log("[SLAVE] get SLAVE revision");
							Ch[2].Out = { 0xF0, 0x32 }; // use response data from MAME
							Ch[c].In.clear();
							break;

						/** Pointer Type **/
						case 0xF3:
							MiniCDI::Log("[SLAVE] get pointer type");
							Ch[2].Out = { 0xF3, 0x01 }; // use response data from MAME
							Ch[c].In.clear();
							break;

						/** Boot Mode **/
						case 0xF4:
							MiniCDI::Log("[SLAVE] get test plug status");
							Ch[2].Out = { 0xF4, 0x00 }; // use response data from MAME
							Ch[c].In.clear();
							break;

						/** Video Mode **/
						case 0xF6:
							MiniCDI::Log("[SLAVE] get video mode");
							{
								Ch[2].Out = { 0xF6, 0x02 };
							}
							Ch[c].In.clear();
							break;

						/** Enable X-Bus **/
						case 0xFA:
							MiniCDI::Log("[SLAVE] enable X-Bus");
							// TO-DO
							Ch[c].In.clear();
							break;
					}
					break;
			}

			if (Ch[c].InSize == 0)
				Ch[c].In.clear();
		}
	}

	uint8_t *get_lcd()
	{
		return &LCD[0];
	}
};

#endif