#ifndef MINICDI_SCC68070
#define MINICDI_SCC68070

#ifndef MINICDI_MUSASHI
	/*****************************/
	/** Wrapper for M68000 core **/
	/*****************************/
	#ifdef __cplusplus
	extern "C" {
	#endif

	#include "cdi/rocket68/raw/disasm.h"
	#include "cdi/rocket68/raw/loader.h"
	#include "cdi/rocket68/raw/m68k.h"
	#define ROCKET68_VERSION_STR "0.2.1"

	#ifdef __cplusplus
	}
	#endif
	/*****************************/
#endif

class SCC68070
{
	uint8_t* memory;
	int cycles;

	// On-chip peripherals
	uint8_t LIR;

public:
#ifndef MINICDI_MUSASHI
	M68kCpu context;
#endif

	SCC68070(uint8_t* memory, size_t mem_size) : memory(memory), cycles(0)
	{
	#ifdef MINICDI_MUSASHI
		m68k_init();
		m68k_set_cpu_type(M68K_CPU_TYPE_SCC68070);
	#else
		m68k_init(&context, memory, mem_size);
	#endif
	}

	void set_bios(const char* romPath, size_t romAddr)
	{
		std::ifstream romStream(romPath);
		std::vector<char> rom(
		 (std::istreambuf_iterator<char>(romStream)),
		 (std::istreambuf_iterator<char>()));
		romStream.close();

		memcpy(&memory[0], &rom[0], 0x8); // contains initial SSP and PC
		memcpy(&memory[romAddr], &rom[0], 512*1024*sizeof(char));
	}

	void reset()
	{
	#ifdef MINICDI_MUSASHI
		m68k_pulse_reset();

		m68k_set_reg(M68K_REG_D0, 0xffffffff);
		m68k_set_reg(M68K_REG_D1, 0xffffffff);
		m68k_set_reg(M68K_REG_D2, 0xffffffff);
		m68k_set_reg(M68K_REG_D3, 0xffffffff);
		m68k_set_reg(M68K_REG_D4, 0xffffffff);
		m68k_set_reg(M68K_REG_D5, 0xffffffff);
		m68k_set_reg(M68K_REG_D6, 0xffffffff);
		m68k_set_reg(M68K_REG_D7, 0xffffffff);

		m68k_set_reg(M68K_REG_A0, 0xffffffff);
		m68k_set_reg(M68K_REG_A1, 0xffffffff);
		m68k_set_reg(M68K_REG_A2, 0xffffffff);
		m68k_set_reg(M68K_REG_A3, 0xffffffff);
		m68k_set_reg(M68K_REG_A4, 0xffffffff);
		m68k_set_reg(M68K_REG_A5, 0xffffffff);
		m68k_set_reg(M68K_REG_A6, 0xffffffff);
	#else
		m68k_reset(&context);
		for (int i = 0; i < 7; i++)
			context.a_regs[i].l = 0xffffffff;
		for (int i = 0; i < 8; i++)
			context.d_regs[i].l = 0xffffffff;
	#endif
	}

	uint8_t read8(uint32_t addr)
	{
	#ifdef MINICDI_MUSASHI
		if ((m68k_get_reg(NULL, M68K_REG_SR) & 0x2000) != 0)
	#else
		if ((context.sr & M68K_SR_S) != 0)
	#endif
		{
			switch (addr & 0x0fffffff) {
				case 0x1001: return LIR;

				/*case 0x2001: return I2C.IDR;
				case 0x2003: return I2C.IAR;
				case 0x2005: return I2C.ISR;
				case 0x2007: return I2C.ICR;
				case 0x2009: return I2C.ICCR;

				case 0x2013: return UART.USR;
				case 0x201b:
					if (1) {
						UART.USR &= 0b1111'1110; // RX
						UART.URH = 1;
					} else {
						UART.USR |= 0b0000'0001; // RX
						UART.URH = 0;
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
					break;*/
			}
		}

		return 0;
	}

	void write8(uint32_t addr, uint8_t value)
	{
	#ifdef MINICDI_MUSASHI
		if ((m68k_get_reg(NULL, M68K_REG_SR) & 0x2000) != 0)
	#else
		if ((context.sr & M68K_SR_S) != 0)
	#endif
		{
			switch (addr & 0x0fffffff) {
				case 0x1001:
					LIR = value & 0x77;
					break;

				/*case 0x2001:
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
					if (UART.UCR & 0b0'011'0000)		UART.UTH = 0; // reset transmitter
					else if (UART.UCR & 0b0'010'0000)	UART.URH = 0; // reset receiver
					else if (UART.UCR & 0b0'100'0000)	UART.USR &= 0b0000'1111; // reset errors
					break;
				case 0x2019:
					UART.UTH = value;
					UART.USR &= ~(0b0000'1000); // is not empty
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
					if (PICR[0] & 0x80)		 PICR[0] &= 0x0F; // PIR for UART receiver
					else if (PICR[0] & 0x08) PICR[0] &= 0xF0; // PIR for UART transmitter
					break;

				case 0x2047:
					PICR[1] = value;
					if (PICR[1] & 0x80)		 PICR[1] &= 0x0F; // PIR for UART receiver
					else if (PICR[1] & 0x08) PICR[1] &= 0xF0; // PIR for UART transmitter
					break;

				default:
					if ((addr & 0x0fffffff) >= 0x4000 && (addr & 0x0fffffff) <= 0x406D) {
						DMA[(addr & 0x0fffffff) - 0x4000] = value;
					}
					break;*/
			}
		}
	}

	void INT1()
	{
		int level = (LIR >> 4) & 0x07;
		if (level > 0) {
		#ifdef MINICDI_MUSASHI
			m68k_set_irq(level + 56);
		#else
			m68k_set_irq(&context, level + 56 - 24);
		#endif
		}
	}

	void INT2()
	{
		int level = LIR & 0x07;
		if (level > 0) {
		#ifdef MINICDI_MUSASHI
			m68k_set_irq(level + 56);
		#else
			m68k_set_irq(&context, level + 56 - 24);
		#endif
		}
	}

	int run(int cycles = 2400)
	{
	#ifdef MINICDI_MUSASHI
		int ran = m68k_execute(cycles);
	#else
		int ran = m68k_execute(&context, cycles);
	#endif

	#ifdef MINICDI_DEBUG
		#ifdef MINICDI_MUSASHI
			printf("PC: %08X SR: %08X\n", m68k_get_reg(NULL, M68K_REG_PC), m68k_get_reg(NULL, M68K_REG_SR));

			for (int i = 0; i < 8; i++) {
				printf("D%d: %08X ", i, m68k_get_reg(NULL, (m68k_register_t)((int)M68K_REG_D0 + i)));
				if (i == 3 || i == 7)
					printf("\n");
			}

			for (int i = 0; i < 8; i++) {
				printf("A%d: %08X ", i, m68k_get_reg(NULL, (m68k_register_t)((int)M68K_REG_A0 + i)));
				if (i == 3 || i == 7)
					printf("\n");
			}

			char text[192];
			m68k_disassemble(text, m68k_get_reg(NULL, M68K_REG_PC), M68K_CPU_TYPE_SCC68070);
			printf("\n$%08X: %s                            \n", m68k_get_reg(NULL, M68K_REG_PC), text);
		#else
			printf("PC: %06X SR: %06X\n", context.pc, context.sr);

			for (int i = 0; i < 8; i++) {
				printf("D%d: %08X ", i, context.d_regs[i].l);
				if (i == 3 || i == 7)
					printf("\n");
			}

			for (int i = 0; i < 8; i++) {
				printf("A%d: %08X ", i, context.a_regs[i].l);
				if (i == 3 || i == 7)
					printf("\n");
			}

			char text[128];
			m68k_disasm(&context, context.pc, text, (int)sizeof(text));
			printf("\n$%08X: %s                            \n", context.pc, text);
		#endif
	#endif

		return ran;
	}

	/*void increment_timer(int cycles)
	{
		this->cycles += cycles;
		while (this->cycles >= 96) {
			this->cycles -= 96;
			if (Timer.T[0] >= 0xFFFF)
			{
				Timer.TSR |= 0x80; // overflow0 flag
				Timer.T[0] = Timer.RR;

				if ((PICR[0] & 0x07) != 0) {
					#ifdef MINICDI_MUSASHI
						m68k_set_irq(62);
					#else
						m68k_set_irq(&context, 62 - 24);
					#endif
					// INT1();
				}
			}
			Timer.T[0]++;

			if (Timer.T[0] == Timer.T[1] && (Timer.TCR & 0b00110000) == 0b00010000)
				Timer.TSR |= 0x40; // match1 flag

			if (Timer.T[0] == Timer.T[2] && (Timer.TCR & 0b00000011) == 0b00000001)
				Timer.TSR |= 0x08; // match2 flag
		}

	#ifdef MINICDI_DEBUG
		// printf("\n[CPU viewer]\n");
		// printf("UART >  UMR: %02X  USR: %02X  UCS: %02X  UCR: %02X  UTH: %02X  URH: %02X\n", UART.UMR, UART.USR, UART.UCS, UART.UCR, UART.UTH, UART.URH);
		// printf("Timer > TSR: %02X  TCR: %02X\n", Timer.TSR, Timer.TCR);
		// printf("        RR:  %04X\n", Timer.RR);
		// printf("        T0:  %04X\n", Timer.T[0]);

		// printf("INT1N:  %d    INT2N:  %d\n", (LIR >> 4) & 0x07, LIR & 0x07);
		// printf("PICR1:  %02X\n", PICR[0]);
	#endif
	}*/
};

#endif
