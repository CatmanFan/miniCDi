#ifndef MINICDI_MC68HC
#define MINICDI_MC68HC

#include "cdi/common.hpp"

// The SLAVE and IKAT are two different variants: SLAVE is C8 and IKAT is i8 (credit to cdifan).
// Currently only IKAT is implemented in order to ensure that the system ROM can handle the current emulation.

// MC68HC05i8
class IKAT
{
	MiniCDIConfig *emuConfig;
	int ns;

	uint8_t *ADRW, *BDRW, *CDRW, *DDRW; // Data Write Register. receives command bytes for a specific channel.
	uint8_t *ADRR, *BDRR, *CDRR, *DDRR; // Data Read Register. writes response bytes for a specific channel.
	uint8_t *ASR, *BSR, *CSR, *DSR; // Status Register. for channel Status
	uint8_t *ISR;
	uint8_t *IMR;
	uint8_t *MR;

	void execute()
	{
		switch (*DDRW)
		{
			default:
				break;
			case 0xF4:
				WRITE32(CDRR, 0, 0xA5F600FF | (emuConfig->pal ? 0x0200 : 0x0100));
				break;
			// this is just a quick hack for now
			case 0xF6:
				WRITE16(CDRW, 0, 0xF600 | (emuConfig->pal ? 0x02 : 0x01));
				WRITE32(CDRR, 0, 0xA5F600FF | (emuConfig->pal ? 0x0200 : 0x0100));
				break;
		}
	}

public:
	void init(uint8_t* memory, size_t start, MiniCDIConfig *config)
	{
		emuConfig = config;
		ns = 0;

		ADRW = &memory[start + 0x01];
		BDRW = &memory[start + 0x03];
		CDRW = &memory[start + 0x05];
		DDRW = &memory[start + 0x07];

		ADRR = &memory[start + 0x09];
		BDRR = &memory[start + 0x0B];
		CDRR = &memory[start + 0x0D];
		DDRR = &memory[start + 0x0F];

		ASR = &memory[start + 0x11];
		BSR = &memory[start + 0x13];
		CSR = &memory[start + 0x15];
		DSR = &memory[start + 0x17];

		ISR = &memory[start + 0x19];
		IMR = &memory[start + 0x1B];
		MR = &memory[start + 0x1D];

		// *SR[0] = 0b00010001u;
		// *SR[1] = 0b00010001u;
		// *SR[2] = 0b00010001u;
		// *DSR = 0b00010001u;
	}

	void increment_time(int ns)
	{
		execute();
	}
};

#endif