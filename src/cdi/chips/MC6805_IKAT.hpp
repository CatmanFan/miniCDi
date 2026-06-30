#ifndef MINICDI_MC6805_IKAT
#define MINICDI_MC6805_IKAT

#include <deque>
#include "cdi/chips/MCD221_CIAP.hpp"

/*****
  DISCLAIMER:
  Partially sourced from CeDImu emulation code. Some added context from the MC68HC05i8 datasheet is included.
 *****/

class IKAT
{
	SCC68070* _68070;
	uint8_t* memory;

	struct
	{
		uint8_t DR; // Data Register
		uint8_t SR; // Status Register. for channel Status

		std::deque<uint8_t> In;
		std::deque<uint8_t> Out;
		size_t InSize;
		size_t Delay;
	} Ch[4];

	uint8_t ISR;
	uint8_t IMR;
	uint8_t MR; // always 8F (in "set on Receiver Ready" interrupt mode and enable all channels)

	void set_ISR(uint8_t flag)
	{
		ISR |= flag;
		if (IMR & flag)
			_68070->interrupt(SCC68070::IPL_IN2N, true);
	}

	void unset_ISR(uint8_t flag)
	{
		ISR &= ~flag;
		if (IMR & flag)
			_68070->interrupt(SCC68070::IPL_IN2N, false);
	}

	struct
	{
		bool connected;
		bool enabled;
		bool posChanged;
		int x, y;
		bool absolute;
	} PointerInterface;

	uint8_t LCD[16];

public:
	friend class PlayerLCD;
	friend class PointingDevice;

	IKAT(SCC68070* _68070, uint8_t* memory) : _68070(_68070), memory(memory), PointerInterface({0})
	{
		reset();
	}

	void poll_packet(size_t c, size_t delay = 0)
	{
		if (delay > 0)
		{
			Ch[c].Delay = delay;
			return;
		}

		// Per MC68HC05i8 datasheet: under MRH's current mode Receiver Ready is supposed to trigger the Rx/Tx bits in ISR.
		Ch[c].SR |= 0b01000000; // RRDY ON
		Ch[c].SR &= ~0b00010000; // REMTY OFF

		// set corresponding Rx bit
		set_ISR(c == 3 ? 0b10'00'00'00 : c == 2 ? 0b00'10'00'00 : c == 1 ? 0b00'00'10'00 : 0b00'00'00'10);
		MiniCDI::Log("[IKAT] %sDR packet sent to CPU", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A");
	}

	void update()
	{
		for (size_t c = 0; c < 4; c++)
		{
			if (Ch[c].Delay != 0)
			{
				Ch[c].Delay--;
				if (Ch[c].Delay == 0)
				{
					Ch[c].SR |= 0b01000000; // RRDY ON
					Ch[c].SR &= ~0b00010000; // REMTY OFF

					// set corresponding Rx bit
					set_ISR(c == 3 ? 0b10'00'00'00 : c == 2 ? 0b00'10'00'00 : c == 1 ? 0b00'00'10'00 : 0b00'00'00'10);
					MiniCDI::Log("[IKAT] %sDR packet sent to CPU", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A");
				}
			}
		}
	}

	void reset()
	{
		memset(&LCD[0], 0, 16);

		// REMTY ON (per MC68HC05i8 datasheet)
		Ch[0].SR |= 0b00010000;
		Ch[1].SR |= 0b00010000;
		Ch[2].SR |= 0b00010000;
		Ch[3].SR |= 0b00010000;

		// TEMTY ON (mimic cdiemu behaviour ?)
		Ch[0].SR |= 0b00000001;
		Ch[1].SR |= 0b00000001;
		Ch[2].SR |= 0b00000001;
		Ch[3].SR |= 0b00000001;

		Ch[0].InSize = 0;
		Ch[1].InSize = 0;
		Ch[2].InSize = 0;
		Ch[3].InSize = 0;
	}

	uint8_t read8(uint32_t addr)
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
							unset_ISR(c == 3 ? 0b10'00'00'00 : c == 2 ? 0b00'10'00'00 : c == 1 ? 0b00'00'10'00 : 0b00'00'00'10);
							Ch[c].SR &= ~0b01000000; // RRDY OFF
							Ch[c].SR |= 0b00010000; // REMTY ON
							MiniCDI::Log("[IKAT] %dDR read completed", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A");
						}
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
				MiniCDI::Log("[IKAT] ISR => %02X", ISR);
				return ISR;

			case 0x31001B:
				MiniCDI::Log("[IKAT] IMR => %02X", IMR);
				return IMR;

			case 0x31001D:
				MiniCDI::Log("[IKAT] MR => %02X", MR);
				return MR;
		}

		return memory[addr];
	}

	void write8(uint32_t addr, uint8_t value, CIAP *ciap)
	{
		memory[addr] = value;
		switch (addr)
		{
			case 0x310019:
				MiniCDI::Log("[IKAT] ISR <= %02X", value);
				ISR = value;
				break;

			case 0x31001B:
				MiniCDI::Log("[IKAT] IMR <= %02X", value);
				IMR = value;
				break;

			case 0x31001D:
				MiniCDI::Log("[IKAT] MR <= %02X", MR);
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
							/** Set Front Panel LCD **/
							case 0x9A:
								MiniCDI::Log("[IKAT] set LCD (0x%02X)", value);
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
										break;
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

							/** Pointing Device **/
							case 0xF3:
								MiniCDI::Log("[IKAT] report pointing device type (0x%02X)", value);
								PointerInterface.connected = true;
								Ch[c].Out = { 0xA5, 0xF3, 'T', 'T' };
								PointerInterface.absolute = Ch[c].Out[2] == 'T';
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
										case 0xA1:
										case 0xB0:
											MiniCDI::Log("[IKAT] report disc status (0x%02X)", Ch[c].In[0]);
											if (MiniCDI::Config::HasDisc) Ch[c].Out = { 0xB0, 0x00, 0x02, 0x10 }; // cdifan: $00060E for SLAVE 5.0 (CD-i rev 450), $000210 for IKAT 6.x-9.x
											else Ch[c].Out = { 0xB0, 0x00, 0x00, 0x00 };
											poll_packet(c, 2);
											break;

										case 0xB1:
											MiniCDI::Log("[IKAT] report disc base (0x%02X)", Ch[c].In[0]);
											Ch[c].Out = { 0xB1, 0x00, 0x02, 0x00 }; // imitate cdiemu
											poll_packet(c);
											break;

										case 0xB2:
											MiniCDI::Log("[IKAT] report disc select (0x%02X)", Ch[c].In[0]);
											Ch[c].Out = { 0xB2, 0x20, 0x00, 0x10 }; // imitate cdiemu
											poll_packet(c, 2);
											break;

										case 0xE0:
											MiniCDI::Log("[IKAT] init CIAP CD-DA playback (0x%02X)", Ch[c].In[0]);
											break;

										case 0xE1:
											MiniCDI::Log("[IKAT] init CIAP sector read (0x%02X)", Ch[c].In[0]);
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

							case 0xB1: /** Disc Base **/
								MiniCDI::Log("[IKAT] report disc base (0x%02X)", Ch[c].In[0]);
								Ch[c].Out = { 0xB1, 0x00, 0x02, 0x00 }; // imitate cdiemu
								poll_packet(c);
								break;

							case 0xA1: /** Disc Status (alt) **/
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