#ifndef MINICDI_MCD221_CIAP
#define MINICDI_MCD221_CIAP

class CIAP
{
	MiniCDIConfig *emuConfig;
	uint8_t* memory;
	uint16_t ADPCM[2][0x8FF];
	uint16_t Main[2][2340];
	uint16_t SubQ[2][0x0A];
	uint16_t SubR[2][0x0A];

public:
	CIAP(uint8_t* memory, MiniCDIConfig *config)
	: emuConfig(config), memory(memory)
	{
	}
};

#endif