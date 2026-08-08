#ifndef MINICDI_MC6805_IKAT
#define MINICDI_MC6805_IKAT

#include <deque>
#include "cdi/chips/MCD221_CIAP.hpp"

/*****
  DISCLAIMER:
  Partially sourced from CeDImu emulation code and documentation by cdifan. Some added context from the MC68HC05i8 datasheet is included.
 *****/

class IKAT
{
	SCC68070* _68070;
	uint8_t* memory;

	struct
	{
		uint8_t DR = 0; // Data Register
		uint8_t SR = 0; // Status Register. for channel Status

		std::deque<uint8_t> In;
		std::deque<uint8_t> Out;
		size_t InSize = 0;
		size_t Delay = 0;
	} Ch[4];

	uint8_t ISR;
	uint8_t IMR;
	uint8_t MR; // always 8F (in "set on Receiver Ready" interrupt mode and enable all channels)
				// known as YCTL in cdiemu trace log

	inline void set_ISR(uint8_t flag)
	{
		ISR |= flag;
		if (IMR & flag) _68070->interrupt(SCC68070::IPL_IN2N, true);
		//MiniCDI::Log("[IKAT] ISR %02X |= IMR %02X", ISR, IMR);
	}

	inline void send_packet(size_t c)
	{
		// Per MC68HC05i8 datasheet: under MRH's current mode RRDY when set is supposed to trigger the Rx/Tx bits in ISR.
		// RRDY is not emulated by cdiemu.
		// Ch[c].SR |= 0b01000000; // RRDY ON
		Ch[c].SR &= ~0b00010000; // REMTY OFF

		// set corresponding Rx bit
		set_ISR(1 << (1+2*c));
		//MiniCDI::Log("[IKAT] %sDR packet sent to CPU", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A");
	}

	struct
	{
		bool connected;
		bool enabled;
		bool posChanged;
		int x, y;
		bool absolute;
	} PointerInterface;
	bool Disc = false;
	FTD* ftd;

public:
	friend class FTD;
	friend class PointingDevice;

	IKAT(SCC68070* _68070, uint8_t* memory) : _68070(_68070), memory(memory), PointerInterface({0})
	{
		reset();
	}

	inline void poll_packet(size_t c, int byte1, int byte2 = -1, int byte3 = -1, int byte4 = -1, size_t delay = 0)
	{
		Ch[c].Out.clear();
		Ch[c].Out.push_back(byte1);
		if (byte2 >= 0) { Ch[c].Out.push_back(byte2); }
		if (byte3 >= 0) { Ch[c].Out.push_back(byte3); }
		if (byte4 >= 0) { Ch[c].Out.push_back(byte4); }

		if (delay > 0)
			Ch[c].Delay = delay;
		else
			send_packet(c);
	}

	inline void update()
	{
		for (size_t c = 0; c < 4; c++)
		{
			if (Ch[c].Delay != 0)
			{
				Ch[c].Delay--;
				if (Ch[c].Delay == 0)
					send_packet(c);
			}
		}
	}

	inline void reset()
	{
		Ch[0] = {0};
		Ch[1] = {0};
		Ch[2] = {0};
		Ch[3] = {0};

		// REMTY ON (per MC68HC05i8 datasheet) + TEMTY ON (mimic cdiemu behaviour ?)
		Ch[0].SR |= 0x11;
		Ch[1].SR |= 0x11;
		Ch[2].SR |= 0x11;
		Ch[3].SR |= 0x11;

		ISR = 0;
		IMR = 0;
		MR = 0;
	}

	inline void set_ftd(FTD* ftd)
	{
		this->ftd = ftd;
	}

	inline void send_disc_status(bool value)
	{
		Disc = value;
		if (Disc)
			poll_packet(3, 0xB0, 0x00, 0x02, 0x10, 2); // imitate cdiemu
		else
			poll_packet(3, 0xB0, 0x00, 0x00, 0x00, 2);
	}

	inline uint8_t read8(uint32_t addr)
	{
		switch (addr)
		{
			case 0x310009:
			case 0x31000B:
			case 0x31000D:
			case 0x31000F:
				{
					size_t c = (addr - 0x310009) / 2;

					if (Ch[c].Out.size() > 0)
					{
						Ch[c].DR = Ch[c].Out[0];
						Ch[c].Out.pop_front();

						if (Ch[c].Out.size() == 0)
						{
							ISR &= ~(1 << (1+2*c)); // imitate CeDImu
							if ((ISR & IMR) == 0) _68070->interrupt(SCC68070::IPL_IN2N, false);
							// Ch[c].SR &= ~0b01000000; // RRDY OFF
							Ch[c].SR |= 0b00010000; // REMTY ON
							//MiniCDI::Log("[IKAT] %sDR read completed", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A");
						}
						else
							Ch[c].SR &= ~0b00010000; // REMTY OFF
					}
					else
						Ch[c].DR = 0xFF;

					//MiniCDI::Log("[IKAT] %sDRR => %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].DR);
					return Ch[c].DR;
				}
				break;

			case 0x310011:
			case 0x310013:
			case 0x310015:
			case 0x310017:
				{
					size_t c = (addr - 0x310011) / 2;

					//MiniCDI::Log("[IKAT] %sSR => %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].SR);
					return Ch[c].SR;
				}
				break;

			case 0x310019:
				//MiniCDI::Log("[IKAT] ISR => %02X", ISR);
				return ISR;

			case 0x31001B:
				//MiniCDI::Log("[IKAT] IMR => %02X", IMR);
				return IMR;

			case 0x31001D:
				//MiniCDI::Log("[IKAT] MR => %02X", MR);
				return MR;
		}

		return memory[addr];
	}

	inline void write8(uint32_t addr, uint8_t value, CIAP *ciap)
	{
		memory[addr] = value;
		switch (addr)
		{
			case 0x310019:
				//MiniCDI::Log("[IKAT] ISR <= %02X", value);
				ISR = value;
				break;

			case 0x31001B:
				//MiniCDI::Log("[IKAT] IMR <= %02X", value);
				IMR = value;
				break;

			case 0x31001D:
				//MiniCDI::Log("[IKAT] MR <= %02X", value);
				MR = value;
				break;

			case 0x310001:
			case 0x310003:
			case 0x310005:
			case 0x310007:
			{
				size_t c = (addr - 0x310001) / 2;
				Ch[c].In.push_back(value);
				//MiniCDI::Log("[IKAT] %sDRW <= %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", value);

				switch (c)
				{
					/** ADR **/
					case 0:
						switch (value)
						{
							/** Set Front Panel FTD **/
							case 0x9A:
								// redirects FTD display input to BDR
								Ch[1].In.clear();
								Ch[1].In.push_back(value);
								Ch[1].InSize = 7;
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
										case 0x9A:
											MiniCDI::Log("[IKAT] set FTD display (0x%02X)", Ch[c].In[0]);
											if (ftd != NULL) ftd->update(Ch[c].In);
											break;
									}
									Ch[c].In.clear();
									Ch[c].InSize = 0;
								}
								break;
						}
						break;

					/** CDR **/
					case 2:
						switch (value)
						{
							/** Reset CPU **/
							case 0x88:
								MiniCDI::Log("[IKAT] reset CPU (0x%02X)", value);
								_68070->reset();
								break;

							/** Player Shell Startup Animation **/
							case 0xF1:
								MiniCDI::Log("[IKAT] player shell startup animation ? (0x%02X)", value);
								poll_packet(c, 0xA5, 0xF1, 0x00);
								break;

							/** Player Shell Branding **/
							case 0xF2:
								MiniCDI::Log("[IKAT] player shell branding ? (0x%02X)", value);
								poll_packet(c, 0xA5, 0xF2, 0x00); // 0x00 returns Philips
								break;

							/** Pointing Device **/
							case 0xF3:
								MiniCDI::Log("[IKAT] report pointing device type (0x%02X)", value);
								PointerInterface.connected = true;
								poll_packet(c, 0xA5, 0xF3, (uint8_t)(0x80 | 'T'), 0);
								PointerInterface.absolute = (Ch[c].Out[2] & 0x7F) == 'T' || (Ch[c].Out[2] & 0x7F) == 'S';
								break;

							/** Boot Mode **/
							case 0xF4:
								MiniCDI::Log("[IKAT] report boot status (0x%02X)", value);
								poll_packet(c, 0xA5, 0xF4, MiniCDI::Config::TestPlug ? 0x01 : 0x00);
								break;

							/** Video Mode **/
							case 0xF6:
								MiniCDI::Log("[IKAT] report video mode (0x%02X)", value);
								poll_packet(c, 0xA5, 0xF6, MiniCDI::Config::PAL ? 0x02 : 0x01, 0xFF);
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
										case 0xA1:
											MiniCDI::Log("[IKAT] SS_Enable? (0x%02X)", Ch[c].In[0]);
											break;

										case 0xA6:
											MiniCDI::Log("[IKAT] SS_Eject (0x%02X)", Ch[c].In[0]);
											send_disc_status(false);
											break;

										case 0xA7:
											MiniCDI::Log("[IKAT] close disc tray? (0x%02X)", Ch[c].In[0]);
											break;

										case 0xB0:
											MiniCDI::Log("[IKAT] report disc status (0x%02X)", Ch[c].In[0]);
											if (Disc)
												poll_packet(c, 0xB0, 0x00, 0x02, 0x10, 2); // imitate cdiemu
											else
												poll_packet(c, 0xB0, 0x00, 0x00, 0x00, 2);
											break;

										case 0xB1:
											MiniCDI::Log("[IKAT] report disc base (0x%02X)", Ch[c].In[0]);
											poll_packet(c, 0xB1, 0x00, 0x02, 0x00); // imitate cdiemu
											break;

										case 0xB2:
											MiniCDI::Log("[IKAT] report disc select (0x%02X)", Ch[c].In[0]);
											poll_packet(c, 0xB2, 0x20, 0x00, 0x01, 2); // imitate cdiemu
											break;

										case 0xE0:
											MiniCDI::Log("[IKAT] init CIAP CD-DA playback (0x%02X)", Ch[c].In[0]);
											break;

										case 0xE1:
											MiniCDI::Log("[IKAT] init CIAP sector read (0x%02X)", Ch[c].In[0]);
											if (ciap) {
												ciap->disc_set_lba(Ch[c].In[1], Ch[c].In[2], Ch[c].In[3]);
											}
											break;
									}
									Ch[c].In.clear();
									Ch[c].InSize = 0;
								}
								break;

							/** Start TOC lead-in read **/
							case 0xC0:
								MiniCDI::Log("[IKAT] start TOC (0x%02X)", value);
								// TO-DO
								break;

							case 0xB1: /** Disc Base **/
								MiniCDI::Log("[IKAT] report disc base (0x%02X)", Ch[c].In[0]);
								poll_packet(c, 0xB1, 0x00, 0x02, 0x00); // imitate cdiemu
								break;

							// IKAT PORT C => $A08200 ???
							case 0xA1: /** SS_Enable? **/
							case 0xA6: /** SS_Eject? **/
							case 0xA7: /** Close Disc Tray? **/
							case 0xB0: /** Disc Status **/
							case 0xB2: /** Disc Select **/
							case 0xE0: /** Start CDDA playback **/
							case 0xE1: /** Start CDDA sector read **/
								if (Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
									Ch[c].InSize = 4;
								}
								break;
						}
						break;
				}

				if (Ch[c].InSize == 0)
					Ch[c].In.clear();
				break;
			}
		}
	}
};

#endif