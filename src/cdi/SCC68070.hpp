#ifndef MINICDI_SCC68070
#define MINICDI_SCC68070

/*****************************/
/** Wrapper for M68000 core **/
/*****************************/
#ifdef __cplusplus
extern "C" {
#endif

#include "m68k/disasm.h"
#include "m68k/loader.h"
#include "m68k/m68k.h"
#define ROCKET68_VERSION_STR "0.2.1"

#ifdef __cplusplus
}
#endif
/*****************************/


class SCC68070
{
	uint8_t* memory;

public:
	M68kCpu core;

	struct
	{
		uint8_t UMR;
		uint8_t USR;
		uint8_t UCS;
		uint8_t UCR;
		uint8_t UTH;
		uint8_t URH;
	} UART;

	void init(uint8_t* ram, size_t ramSize, const char* rom, size_t romAddr)
	{
		this->memory = &ram[0];
		m68k_init(&core, ram, ramSize);
		m68k_load_bin(&core, rom, romAddr);
		memcpy(ram, rom, 0x8); // contains initial SSP and PC
		this->reset();
	}

	void reset()
	{
		m68k_reset(&core);
		core.a_regs[7].l = 0x1500;
		core.pc = 0x4004b8;

		UART.UMR = 0b00100000;
		UART.USR = 0b00000010;
		UART.UCS = 0b00001000;
		UART.UCR = 0b10000000;
		UART.USR |= 0b00000100; // TX
	}

	uint8_t read8(uint32_t addr, uint8_t def)
	{
		if ((core.sr & M68K_SR_S) != 0)
		{
			if ((addr & 0x00ffffff) == 0x201b)
			{
				if (1) {
					UART.USR &= 0b1111'1110;
					UART.URH = 0;
				} else {
					UART.USR |= 0b0000'0001;
					UART.URH = 1;
				}
				return UART.URH;
			}
			if ((addr & 0x00ffffff) == 0x2013)
			{
				return UART.USR;
			}
		}

		return def;
	}

	void write8(uint32_t addr, uint8_t value)
	{
		if ((core.sr & M68K_SR_S) != 0)
		{
			if ((addr & 0x00ffffff) == 0x2011)
			{
				UART.UMR = value;
				return;
			}
			if ((addr & 0x00ffffff) == 0x2015)
			{
				UART.UCS = value;
				return;
			}
			if ((addr & 0x00ffffff) == 0x2017)
			{
				UART.UCR = value;
				if (UART.UCR & 0b0'010'0000) { UART.URH = 0; }
				if (UART.UCR & 0b0'011'0000) { UART.UTH = 0; }
				if (UART.UCR & 0b0'100'0000) { UART.USR &= 0b0000'1111; }
				return;
			}
			if ((addr & 0x00ffffff) == 0x2019)
			{
				UART.USR |= 0b0000'1000;
				UART.UTH = value;
				return;
			}
		}

		memory[addr & 0x00ffffff] = value;
	}

	void execute()
	{
		m68k_execute(&core, 1900);
		// printf("[CPU] pc: %08x\n", core.pc);
	}
};

#endif
