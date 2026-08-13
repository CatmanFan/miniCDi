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
		bool Available;
	} Ch[4];

	struct
	{
		bool connected;
		bool enabled;
		bool posChanged;
		int x, y;
	} PointerInterface;
	bool Disc = false;
	FTD* ftd;

	uint32_t DR[4]; // addresses to data registers

	// For PointingDevice !!
	inline void assert_irq(size_t c) {
		Ch[c].Available = true;
		_68070->interrupt(SCC68070::IPL_IN2N, true);
	}
	uint8_t revision = 0;

public:
	friend class FTD;
	friend class PointingDevice;

	SLAVE(SCC68070* _68070, uint8_t* memory, uint32_t start) : _68070(_68070), memory(memory), PointerInterface({0})
	{
		DR[0] = start + 0x01; // ADR
		DR[1] = start + 0x03; // BDR
		DR[2] = start + 0x05; // CDR
		DR[3] = start + 0x07; // DDR
		reset();
	}

	inline void reset()
	{
		Ch[0].InSize = 0;
		Ch[1].InSize = 0;
		Ch[2].InSize = 0;
		Ch[3].InSize = 0;
	}

	inline void set_ftd(FTD* ftd)
	{
		this->ftd = ftd;
	}

	inline void send_play_button()
	{
		MiniCDI::Log("[SLAVE] report Play Button status (0xA1 ?)");
		if (revision == 0x20) { Ch[1].Out = { 0x87, 0x20, 0xFF }; }
		else { Ch[1].Out = { 0xA1, 0x87, 0x20, 0xFF }; }
		assert_irq(1);
	}

	inline void send_eject_button()
	{
		MiniCDI::Log("[SLAVE] report Eject Button status (0xA1 ?)");
		if (revision == 0x20) { Ch[1].Out = { 0x87, 0x08, 0xFF }; }
		else { Ch[1].Out = { 0xA1, 0x87, 0x00, 0xFF }; }
		assert_irq(1);
	}

	inline void send_disc_status(bool value)
	{
		if (value)
		{
			if (revision == 0x60) { Ch[3].Out = { 0xB0, 0x00, 0x06, 0x10 }; } // CDI 350 (per cdifan)
			else { Ch[3].Out = { 0xB0, 0x00, 0x02, 0x15 }; }
		}
		else
		{
			Ch[3].Out = { 0xB0, 0x00, 0x00, 0x00 };
		}
		assert_irq(3);

		Disc = value;
	}

	inline uint8_t read8(uint32_t addr)
	{
		if (addr == DR[0] || addr == DR[1] || addr == DR[2] || addr == DR[3])
		{
			size_t c = (addr - DR[0]) / 2;

			Ch[c].Available = false;
			if (!Ch[0].Available && !Ch[1].Available && !Ch[2].Available && !Ch[3].Available)
				_68070->interrupt(SCC68070::IPL_IN2N, false);

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

	inline void write8(uint32_t addr, uint8_t value)
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
								PointerInterface.x = ((Ch[c].In[1] << 3) & 0b1110000000) | (Ch[c].In[2] & 0b01111111);
								PointerInterface.y = ((Ch[c].In[0] << 4) & 0b1111110000) | (Ch[c].In[1] & 0b00001111);
								// PointerInterface.posChanged = true;
								// MiniCDI::Log("[SLAVE] set pointer pos (x=%d,y=%d)", PointerInterface.x, PointerInterface.y);
								Ch[c].In.clear();
								Ch[c].InSize = 0;
								return;
							}

							/** Set Pointer Pos **/
							if (value >= 0xC0 && value <= 0xFF && Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
								Ch[c].InSize = 3;
								return;
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
							if (Ch[c].InSize > 0 && Ch[c].In.size() >= Ch[c].InSize) {
								switch (Ch[c].In[0]) {
									case 0xF0:
									case 0xF2:
									case 0xF6:
										MiniCDI::Log("[SLAVE] set FTD display (0x%02X)", Ch[c].In[0]);
										if (ftd != NULL) ftd->update(Ch[c].In);
										break;
								}
								Ch[c].In.clear();
								Ch[c].InSize = 0;
							}
							break;

						/** Set Front Panel FTD **/
						case 0xF0: // Mono-I?
						case 0xF6: // Mono-II?
							if (Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
								Ch[c].InSize = value == 0xF6 ? 9 : 17;
							}
							break;

						case 0xA1:
							MiniCDI::Log("[SLAVE] disc-related? (0x%02X)", value);
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

						/** Set Front Panel FTD **/
						case 0xF0:
						case 0xF2: // Mono-I? (used at boot)
						case 0xF6:
							// redirects FTD display input to BDR
							Ch[1].In.clear();
							Ch[1].In.push_back(value);
							Ch[1].InSize = value == 0xF6 || value == 0xF2 ? 9 : 17;
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
										send_disc_status(Disc);
										break;
									case 0xB1:
										MiniCDI::Log("[SLAVE] get disc base (0x%02X%02X%02X%02X)", Ch[c].In[0], Ch[c].In[1], Ch[c].In[2], Ch[c].In[3]);
										Ch[c].Out = { 0xB1, 0x00, 0x00, 0x00 }; // use response data from MAME
										assert_irq(c);
										break;
								}
								Ch[c].In.clear();
								Ch[c].InSize = 0;
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
							Ch[2].Out = { 0xF0, 0x32 }; // use response data from cdiemu (SLAVE 3.2?)
							revision = Ch[2].Out[1];
							assert_irq(2);
							break;

						/** Pointer Type **/
						case 0xF3:
							MiniCDI::Log("[SLAVE] get pointer type (0x%02X)", value);
							Ch[2].Out = { 0xF3, 0x01 }; /** cdifan: 1 => CL="c"; 2 => CL="d"; 3 => CL="b"; 4 => CL="a"; 5 => CL="c" + /kb1 **/
							assert_irq(2);
							break;

						/** Boot Mode **/
						case 0xF4:
							MiniCDI::Log("[SLAVE] get test plug status (0x%02X)", value);
							Ch[2].Out = { 0xF4, static_cast<uint8_t>(MiniCDI::Config::TestPlug ? 0x01 : 0x00) };
							assert_irq(2);
							break;

						/** Video Mode **/
						case 0xF6:
							MiniCDI::Log("[SLAVE] get video mode (0x%02X)", value);
							Ch[2].Out = { 0xF6, static_cast<uint8_t>(MiniCDI::Config::PAL ? 0x02 : 0x01) };
							// assert_irq(2); // interrupt not required on MAME ?
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
			}
		}
	}
};

#endif