#ifndef MINICDI_MC6805_IKAT
#define MINICDI_MC6805_IKAT

#include <deque>
#include "cdi/chips/MCD221_CIAP.hpp"

/// HLE implementation of IKAT as found in Mono-III & Mono-IV.
class IKAT
{
	SCC68070* _68070;
	uint8_t* memory;

	enum Address
	{
		ADRW = 0x310001,
		BDRW = 0x310003,
		CDRW = 0x310005,
		DDRW = 0x310007,
		ADRR = 0x310009,
		BDRR = 0x31000B,
		CDRR = 0x31000D,
		DDRR = 0x31000F,
		ASR = 0x310011,
		BSR = 0x310013,
		CSR = 0x310015,
		DSR = 0x310017,
		Isr = 0x310019,
		Imr = 0x31001B,
		Mr = 0x31001D,
	};

	struct
	{
		uint8_t DR; // Data Register
		uint8_t SR; // Status Register. for channel Status

		std::deque<uint8_t> In;
		std::deque<uint8_t> Out;
		size_t InSize;
	} Ch[4];

	uint8_t ISR;
	uint8_t IMR;
	uint8_t MR;

	struct
	{
		bool connected;
		bool enabled;
		bool posChanged;
		int x, y;
	} PointerInterface;

	uint8_t LCD[16];

public:
	friend class PlayerLCD;
	friend class PointingDevice;

	IKAT(SCC68070* _68070, uint8_t* memory) : _68070(_68070), memory(memory), PointerInterface({0})
	{
		reset();
	}

	void poll_packet(size_t c)
	{
		Ch[c].SR &= ~0x10; // REMTY OFF

		// set corresponding Rx bit
		ISR |= (c == 3 ? 0x80 : c == 2 ? 0x20 : c == 1 ? 0x08 : 0x02);
		if (IMR & (c == 3 ? 0x80 : c == 2 ? 0x20 : c == 1 ? 0x08 : 0x02))
			_68070->interrupt(SCC68070::IPL_IN2N, true);
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
		switch (addr)
		{
			case Address::ADRR:
			case Address::BDRR:
			case Address::CDRR:
			case Address::DDRR:
				{
					size_t c = (addr - Address::ADRR) / 2;

					// deassert IRQ
					_68070->interrupt(SCC68070::IPL_IN2N, false);

					if (Ch[c].Out.size() > 0)
					{
						Ch[c].DR = Ch[c].Out[0];
						Ch[c].Out.pop_front();

						// imitate CeDImu behaviour
						if (Ch[c].Out.size() == 0)
						{
							ISR &= ~(c == 3 ? 0x80 : c == 2 ? 0x20 : c == 1 ? 0x08 : 0x02);
							Ch[c].SR |= 0x10; // REMTY ON
							// _68070->interrupt(SCC68070::IPL_IN2N, false); // deassert IRQ
						}
					}
					else
						Ch[c].DR = 0xFF;


					MiniCDI::Log("[IKAT] %sDR => %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].DR);
					return Ch[c].DR;
				}
				break;

			case Address::ASR:
			case Address::BSR:
			case Address::CSR:
			case Address::DSR:
				{
					size_t c = (addr - Address::ASR) / 2;

					//MiniCDI::Log("[IKAT] %sSR => %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].SR);
					return Ch[c].SR | 0x01; // imitate cdiemu
				}
				break;

			case Address::Isr:
				//MiniCDI::Log("[IKAT] ISR => %02X", ISR);
				return ISR;

			case Address::Imr:
				MiniCDI::Log("[IKAT] IMR => %02X", IMR);
				return IMR;

			case Address::Mr:
				//MiniCDI::Log("[IKAT] MR => %02X", MR);
				return MR;
		}

		return memory[addr];
	}

	void write8(uint32_t addr, uint8_t value, CIAP *ciap)
	{
		memory[addr] = value;
		switch (addr)
		{
			case Address::Isr:
				MiniCDI::Log("[IKAT] ISR <= %02X", value);
				ISR = value;
				break;

			case Address::Imr:
				MiniCDI::Log("[IKAT] IMR <= %02X", value);
				IMR = value;
				break;

			case Address::Mr:
				//MiniCDI::Log("[IKAT] MR <= %02X", value);
				MR = value;
				break;

			case Address::ADRW:
			case Address::BDRW:
			case Address::CDRW:
			case Address::DDRW:
			{
				size_t c = (addr - Address::ADRW) / 2;
				Ch[c].In.push_back(value);
				//MiniCDI::Log("[IKAT] %sDR <= %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", value);

				switch (c)
				{
					/** ADR **/
					case 0:
						switch (value)
						{
							/** Set Front Panel LCD **/
							case 0x9A:
								//MiniCDI::Log("[IKAT] set LCD (0x%02X)", value);
								Ch[1].InSize = 6; // redirects LCD display input to BDR
								break;
						}
						break;

					/** BDR **/
					case 1:
						switch (value)
						{
							default:
								switch (Ch[c].InSize) {
									case 6:
										LCD[Ch[c].In.size() - 1] = value;
										if (Ch[c].In.size() >= Ch[c].InSize) {
											Ch[c].In.clear();
											Ch[c].InSize = 0;
										}
										return;
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
								if (_68070 != NULL) _68070->reset();
								break;

							/** Pointing Device **/
							case 0xF3:
								MiniCDI::Log("[IKAT] report pointing device type (0x%02X)", value);
								PointerInterface.connected = true;
								Ch[c].Out = { 0xA5, 0xF3, 'T', 'T' }; // imitate cdiemu
								poll_packet(c);
								break;

							/** Boot Mode **/
							case 0xF4:
								MiniCDI::Log("[IKAT] report boot status (0x%02X)", value);
								Ch[c].Out = { 0xA5, 0xF4, (uint8_t)(MiniCDI::Config::TestPlug ? 0x01 : 0x00) };
								poll_packet(c);
								break;

							/** Video Mode **/
							case 0xF6:
								MiniCDI::Log("[IKAT] report video mode (0x%02X)", value);
								Ch[c].Out = { 0xA5, 0xF6, (uint8_t)(MiniCDI::Config::PAL ? 0x02 : 0x01), 0xFF };
								poll_packet(c);
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
											MiniCDI::Log("[IKAT] report disc status (0x%02X)", Ch[c].In[0]);
											Ch[c].Out = { 0xB0, 0x00, 0x02, 0x10 }; // cdifan: $00060E for SLAVE 5.0 (CD-i rev 450), $000210 for IKAT 6.x-9.x
											poll_packet(c);
											break;

										case 0xB1:
											MiniCDI::Log("[IKAT] report disc base (0x%02X)", Ch[c].In[0]);
											Ch[c].Out = { 0xB1, 0x00, 0x02, 0x00 }; // imitate cdiemu
											poll_packet(c);
											break;

										case 0xB2:
											MiniCDI::Log("[IKAT] report disc select (0x%02X)", Ch[c].In[0]);
											Ch[c].Out = { 0xB2, 0x20, 0x00, 0x10 }; // imitate cdiemu
											poll_packet(c);
											break;

										case 0xE0:
											MiniCDI::Log("[SLAVE] start CD-DA playback (0x%02X%02X%02X%02X)", Ch[c].In[0], Ch[c].In[1], Ch[c].In[2], Ch[c].In[3]);
											break;

										case 0xE1:
											MiniCDI::Log("[SLAVE] start CD sector read (0x%02X%02X%02X%02X)", Ch[c].In[0], Ch[c].In[1], Ch[c].In[2], Ch[c].In[3]);
											if (ciap) { ciap->disc_set_lba(Ch[c].In[1], Ch[c].In[2], Ch[c].In[3]); }
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

							case 0xB0: /** Disc Status **/
							case 0xB1: /** Disc Base **/
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