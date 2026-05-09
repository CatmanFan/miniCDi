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

	uint8_t *DRW[4]; // Data Write Register. receives command bytes for a specific channel.
	uint8_t *DRR[4]; // Data Read Register. writes response bytes for a specific channel.
	uint8_t *SR[4]; // Status Register. for channel Status
	uint8_t *ISR;
	uint8_t *IMR;
	uint8_t *MR;

	uint8_t LCD[16];

	void execute()
	{
		switch (*DRW[0])
		{
			// Get Video Standard
			case 0xF6:
				WRITE32(DRR[2], 0, 0xA5F600FF | (emuConfig->pal ? 0x0200 : 0x0100));
				break;
		}

		switch (*DRW[1])
		{
			// Set Front Panel LCD
			case 0xF0:
				printf("[LCD] ");
				for (int i = 0; i < 16; i++)
				{
					LCD[i] = *(DRW[0]+1+i);
					printf("%02X ", LCD[i]);
				}
				printf("\n");
				break;
		}

		switch (*DRW[2])
		{
			// Set Front Panel LCD
			case 0xF0:
				printf("[LCD] ");
				for (int i = 0; i < 16; i++)
				{
					LCD[i] = *(DRW[1]+1+i);
					printf("%02X ", LCD[i]);
				}
				printf("\n");
				break;

			// Get Boot Mode
			case 0xF4:
				WRITE32(DRR[2], 0, 0xA5F400 | 0x00 /* player shell */);
				break;

			// Get Video Standard
			case 0xF6:
				WRITE32(DRR[2], 0, 0xA5F600FF | (emuConfig->pal ? 0x0200 : 0x0100));
				break;
		}

		switch (*DRW[3])
		{
			default:
				break;
			// Disc Status
			case 0xB0:
				WRITE32(DRR[3], 0, 0xB00210); // value used by cdiemu
				break;
			// Disc Base
			case 0xB1:
				WRITE32(DRR[3], 0, 0xB1000200); // value used by cdiemu
				break;
			// Disc Select
			case 0xB2:
				WRITE32(DRR[3], 0, 0xB2200010); // value used by cdiemu
				break;
			// Get Video Standard
			case 0xF6:
				WRITE16(DRW[2], 0, 0xF600 | (emuConfig->pal ? 0x02 : 0x01)); // this is just a quick hack for now
				break;
		}
	}

public:
	IKAT(uint8_t* memory, size_t start, MiniCDIConfig *config)
	{
		emuConfig = config;
		ns = 0;

		DRW[0] = &memory[start + 0x01];
		DRW[1] = &memory[start + 0x03];
		DRW[2] = &memory[start + 0x05];
		DRW[3] = &memory[start + 0x07];

		DRR[0] = &memory[start + 0x09];
		DRR[1] = &memory[start + 0x0B];
		DRR[2] = &memory[start + 0x0D];
		DRR[3] = &memory[start + 0x0F];

		SR[0] = &memory[start + 0x11];
		SR[1] = &memory[start + 0x13];
		SR[2] = &memory[start + 0x15];
		SR[3] = &memory[start + 0x17];

		ISR = &memory[start + 0x19];
		IMR = &memory[start + 0x1B];
		MR = &memory[start + 0x1D];

		// *SR[0] = 0b00010001u;
		// *SR[1] = 0b00010001u;
		// *SR[2] = 0b00010001u;
		// *SR[3] = 0b00010001u;
	}

	void increment_time(int ns)
	{
		execute();
	}
};

#endif