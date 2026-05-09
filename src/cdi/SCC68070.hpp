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

#include "m68k/m68k_internal.h"

#ifdef __cplusplus
}
#endif
/*****************************/


class SCC68070
{
	static constexpr int cycleTime = (1.0L / 15'000'000) * 96'000'000'000; // microseconds to nanoseconds
	uint8_t* memory;
	int ns;

public:
	M68kCpu core;

	uint8_t LIR; // 80001001

	struct
	{
		uint8_t IDR;
		uint8_t IAR;
		uint8_t ISR;
		uint8_t ICR;
		uint8_t ICCR;
	} I2C;

	struct
	{
		uint8_t UMR; // 80002011
		uint8_t USR; // 80002013
		uint8_t UCS; // 80002015
		uint8_t UCR; // 80002017
		uint8_t UTH; // 80002019
		uint8_t URH; // 8000201B
	} UART;

	struct
	{
		uint8_t TSR; // 80002020
		uint8_t TCR; // 80002021
		uint16_t RR; // 80002022
		uint16_t T[3]; // 80002024
	} Timer;

	uint8_t PICR[2]; // 80002045, 80002047
	uint8_t DMA[0x6E]; // 80004000

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
		ns = 0;

		m68k_reset(&core);
		core.a_regs[7].l = 0x1500;
		core.pc = 0x4004b8;

		LIR = 0;
		UART.UMR = 0b00100000;
		UART.USR = 0b00000010;
		UART.UCS = 0b00001000;
		UART.UCR = 0b10000000;
		UART.USR |= 0b00000100; // TX
		Timer = {0};
		PICR[0] = PICR[1] = 0;
	}

	uint8_t read8(uint32_t addr)
	{
		if ((core.sr & M68K_SR_S) != 0)
		{
			switch (addr & 0x0fffffff) {
				case 0x1001: return LIR;

				case 0x2001: return I2C.IDR;
				case 0x2003: return I2C.IAR;
				case 0x2005: return I2C.ISR;
				case 0x2007: return I2C.ICR;
				case 0x2009: return I2C.ICCR;

				case 0x2013: return UART.USR;
				case 0x201b:
					if (1) {
						UART.USR &= 0b1111'1110;
						UART.URH = 0;
					} else {
						UART.USR |= 0b0000'0001;
						UART.URH = 1;
					}
					return UART.URH;

				case 0x2020: return Timer.TSR;
				case 0x2021: return Timer.TCR;
				case 0x2022: return (Timer.RR & 0xFF00) >> 8;
				case 0x2023: return Timer.RR & 0x00FF;
				case 0x2024: return (Timer.T[0] & 0xFF00) >> 8;
				case 0x2025: return Timer.T[0] & 0x00FF;
				case 0x2026: return (Timer.T[1] & 0xFF00) >> 8;
				case 0x2027: return Timer.T[1] & 0x00FF;
				case 0x2028: return (Timer.T[2] & 0xFF00) >> 8;
				case 0x2029: return Timer.T[2] & 0x00FF;

				case 0x2045: return PICR[0];
				case 0x2047: return PICR[1];

				default:
					if ((addr & 0x0fffffff) >= 0x4000 && (addr & 0x0fffffff) <= 0x406D) {
						return DMA[(addr & 0x0fffffff) - 0x4000];
					}
					break;
			}
		}

		return 0;
	}

	void write8(uint32_t addr, uint8_t value)
	{
		if ((core.sr & M68K_SR_S) != 0)
		{
			switch (addr & 0x0fffffff) {
				case 0x1001:
					LIR = value & 0x77;
					// if (LIR & 0x80) LIR &= 0x8F;
					// if (LIR & 0x08) LIR &= 0xF8;
					break;

				case 0x2001:
					I2C.IDR = value;
					break;
				case 0x2003:
					I2C.IAR = value;
					break;
				case 0x2005:
					I2C.ISR = value;
					break;
				case 0x2007:
					I2C.ICR = value;
					break;
				case 0x2009:
					I2C.ICCR = value;
					break;

				case 0x2011:
					UART.UMR = value;
					break;
				case 0x2015:
					UART.UCS = value;
					break;
				case 0x2017:
					UART.UCR = value;
					if (UART.UCR & 0b0'010'0000)
						UART.URH = 0;
					if (UART.UCR & 0b0'011'0000)
						UART.UTH = 0;
					if (UART.UCR & 0b0'100'0000)
						UART.USR &= 0b0000'1111;
					break;
				case 0x2019:
					UART.USR |= 0b0000'1000;
					UART.UTH = value;
					break;

				case 0x2020:
					Timer.TSR = value;
					break;
				case 0x2021:
					Timer.TCR = value;
					break;
				case 0x2022:
					Timer.RR &= 0x00FF;
					Timer.RR |= (value << 8);
					break;
				case 0x2023:
					Timer.RR &= 0xFF00;
					Timer.RR |= value;
					break;
				case 0x2024:
					Timer.T[0] &= 0x00FF;
					Timer.T[0] |= (value << 8);
					break;
				case 0x2025:
					Timer.T[0] &= 0xFF00;
					Timer.T[0] |= value;
					break;
				case 0x2026:
					Timer.T[1] &= 0x00FF;
					Timer.T[1] |= (value << 8);
					break;
				case 0x2027:
					Timer.T[1] &= 0xFF00;
					Timer.T[1] |= value;
					break;
				case 0x2028:
					Timer.T[2] &= 0x00FF;
					Timer.T[2] |= (value << 8);
					break;
				case 0x2029:
					Timer.T[2] &= 0xFF00;
					Timer.T[2] |= value;
					break;

				case 0x2045:
					PICR[0] = value;
					if (PICR[0] & 0x80)
						PICR[0] &= 0x0F; // PIR for UART receiver
					else if (PICR[0] & 0x08)
						PICR[0] &= 0xF0; // PIR for UART transmitter
					break;

				case 0x2047:
					PICR[1] = value;
					if (PICR[1] & 0x80)
						PICR[1] &= 0x0F; // PIR for UART receiver
					else if (PICR[1] & 0x08)
						PICR[1] &= 0xF0; // PIR for UART transmitter
					break;

				default:
					if ((addr & 0x0fffffff) >= 0x4000 && (addr & 0x0fffffff) <= 0x406D) {
						DMA[(addr & 0x0fffffff) - 0x4000] = value;
					}
					break;
			}
		}
	}

	void execute()
	{
		m68k_execute(&core, 1900);
		printf("\x1b[%d;%dH", 4, 0);
		printf("[SCC68070] core.pc: %06X\n", (uint32_t)core.pc);

		/*char text[128];
		m68k_disasm(&core, core.pc, text, (int)sizeof(text));
		printf("[SCC68070] %s\n", text);*/

	}

	void increment_time(int ns)
	{
		// this->ns += ns;
		// if (this->ns >= cycleTime)
		{
			// this->ns -= cycleTime;

			printf("[SCC68070] INT1N:  %d    INT2N:  %d\n", (LIR >> 4) & 0x07, LIR & 0x07);
			printf("           PICR1:  %02X\n", PICR[0]);
			printf("           TSR:    %02X   TCR:    %02X\n", Timer.TSR, Timer.TCR);
			printf("           RR:     %04X Timer0: %04X\n", Timer.RR, Timer.T[0]);

			if (Timer.T[0] >= 0xFFFF)
			{
				Timer.TSR |= 0x80; // overflow0 flag
				Timer.T[0] = Timer.RR;

				if ((PICR[0] & 0x07) != 0)
				{
					m68k_exception(&core, 62);
					core.sr &= ~0x0700;
					core.sr |= (6 << 8);
					core.irq_level = 0;
					core.stopped = false;
				}
			}
			Timer.T[0]++;

			if (Timer.T[0] == Timer.T[1] && (Timer.TCR & 0b00110000) == 0b00010000)
				Timer.TSR |= 0x40; // match1 flag

			if (Timer.T[0] == Timer.T[2] && (Timer.TCR & 0b00000011) == 0b00000001)
				Timer.TSR |= 0x08; // match2 flag
		}
	}
};

#endif
