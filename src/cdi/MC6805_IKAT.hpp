#ifndef MINICDI_MC6805_IKAT
#define MINICDI_MC6805_IKAT

#include <deque>

// HLE implementation of IKAT as found in Mono-III & Mono-IV.
class IKAT
{
	uint8_t* memory;
	SCC68070* cpu;

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

	uint8_t LCD[16];

public:
	friend class PlayerLCD;
	friend class PointingDevice;

	IKAT(SCC68070* cpu) : cpu(cpu)
	{
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
		switch (addr)
		{
			default:
				assert(0);
				return 0;

			case Address::ADRR:
			case Address::BDRR:
			case Address::CDRR:
			case Address::DDRR:
				{
					size_t c = (addr - Address::ADRR) / 2;

					if (Ch[c].Out.size() > 0)
					{
						Ch[c].SR &= ~(0x10); // REMTY OFF
						Ch[c].DR = Ch[c].Out[0];
						Ch[c].Out.pop_front();

						// set corresponding Rx bit
						uint8_t INT = c == 3 ? 0b10000000
									: c == 2 ? 0b00100000
									: c == 1 ? 0b00001000
									: 0b00000010;
						ISR |= INT;
						if ((IMR & INT) && cpu) {
							MiniCDI::Log("[IKAT] sending interrupt via %d", c);
							cpu->interrupt(1);
						}
					}
					else {
						Ch[c].SR |= 0x10; // REMTY ON
						Ch[c].DR = 0xFF;
					}

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

					MiniCDI::Log("[IKAT] %sSR %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", Ch[c].SR);
					return Ch[c].SR | 0x01; // imitate cdiemu
				}
				break;

			case Address::Isr:
				// MiniCDI::Log("[IKAT] ISR %02X", ISR);
				return ISR;

			case Address::Imr:
				// MiniCDI::Log("[IKAT] IMR %02X", IMR);
				return IMR;

			case Address::Mr:
				// MiniCDI::Log("[IKAT] MR %02X", MR);
				return MR;
		}
	}

	void write8(uint32_t addr, uint8_t value)
	{
		switch (addr)
		{
			case Address::ADRW:
			case Address::BDRW:
			case Address::CDRW:
			case Address::DDRW:
			{
				size_t c = (addr - Address::ADRW) / 2;
				Ch[c].In.push_back(value);
				MiniCDI::Log("[IKAT] %sDR <= %02X", c == 3 ? "D" : c == 2 ? "C" : c == 1 ? "B" : "A", value);

				switch (c)
				{
					/** ADR **/
					case 0:
						switch (value)
						{
							/** Set Front Panel LCD **/
							case 0x9A:
								Ch[1].InSize = 6; // redirects LCD display input to BDR
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
									case 6:
										LCD[Ch[c].In.size() - 1] = value;
										if (Ch[c].In.size() >= Ch[c].InSize) {
											MiniCDI::Log("[IKAT] $9A : set LCD");
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
							/** Pointing Device **/
							case 0xF3:
								MiniCDI::Log("[IKAT] $F3 : report pointing device type");
								Ch[c].Out = { 0xA5, 0xF3, 'K', 'M' }; // imitate cdiemu
								Ch[c].In.clear();
								break;

							/** Boot Mode **/
							case 0xF4:
								MiniCDI::Log("[IKAT] $F4 : report boot status");
								Ch[c].Out = { 0xA5, 0xF4, 0x01 };
								Ch[c].In.clear();
								break;

							/** Video Mode **/
							case 0xF6:
								MiniCDI::Log("[IKAT] $F6 : report video mode");
								Ch[c].Out = { 0xA5, 0xF6, 0x02, 0xFF };
								Ch[c].In.clear();
								break;
						}
						break;

					/** DDR **/
					case 3:
						switch (value)
						{
							default:
								switch (Ch[c].InSize) {
									case 4:
										if (Ch[c].In.size() >= Ch[c].InSize) {
											switch (Ch[c].In[0]) {
												case 0xE0:
													MiniCDI::Log("[SLAVE] $E0 : start CDDA");
													// TO-DO
													break;

												case 0xE1:
													MiniCDI::Log("[SLAVE] $E1 : start READ");
													// TO-DO
													break;
											}
											Ch[c].In.clear();
											Ch[c].InSize = 0;
										}
										break;
								}
								break;

							/** Disc Status **/
							case 0xB0:
								MiniCDI::Log("[IKAT] $B0 : report disc status");
								Ch[c].Out = { 0xB0, 0x02, 0x10 }; // imitate cdiemu
								Ch[c].In.clear();
								break;

							/** Disc Base **/
							case 0xB1:
								MiniCDI::Log("[IKAT] $B1 : report disc base");
								Ch[c].Out = { 0xB1, 0x00, 0x02, 0x00 }; // imitate cdiemu
								Ch[c].In.clear();
								break;

							/** Disc Select **/
							case 0xB2:
								MiniCDI::Log("[IKAT] $B2 : report disc select");
								Ch[c].Out = { 0xB2, 0x20, 0x00, 0x10 }; // imitate cdiemu
								Ch[c].In.clear();
								break;

							/** Start TOC lead-in read **/
							case 0xC0:
								MiniCDI::Log("[IKAT] $C0 : start TOC");
								// TO-DO
								Ch[c].In.clear();
								break;

							/** Start CDDA playback **/
							case 0xE0:
								if (Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
									Ch[c].InSize = 4;
								}
								break;

							/** Start CDDA sector read **/
							case 0xE1:
								if (Ch[c].In.size() == 1 && Ch[c].InSize == 0) {
									Ch[c].InSize = 4;
								}
								break;
						}
						break;
				}

				if (Ch[c].InSize == 0)
					Ch[c].In.clear();
			}
			return;

			case Address::Isr: MiniCDI::Log("[IKAT] ISR %02X", value); ISR = value; return;
			case Address::Imr: MiniCDI::Log("[IKAT] IMR %02X", value); IMR = value; return;
			case Address::Mr: MiniCDI::Log("[IKAT] MR %02X", value); MR = value; return;
		}
	}
};

#endif