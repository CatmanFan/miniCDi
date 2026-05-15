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
	MiniCDIConfig *emuConfig;
	uint8_t* memory;

	// On-chip peripherals
	uint8_t LIR;
	uint8_t PICR[2];

	/** UART **/
	uint8_t UMR; // Mode Register
	uint8_t USR; // Status Register
	uint8_t UCS; // Clock Select Register
	uint8_t UCR; // Command Register
	uint8_t UTH; // Transmit Holding Register
	uint8_t URH; // Receive Holding Register

	/** Timer **/
	uint8_t TSR;
	uint8_t TCR;
	uint16_t RR;
	uint16_t T[3];

public:
#ifndef MINICDI_MUSASHI
	M68kCpu context;
#endif

	SCC68070(uint8_t* memory, size_t mem_size, MiniCDIConfig *config)
	: emuConfig(config), memory(memory)
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
		for (int i = 0; i < 15; i++)
			m68k_set_reg((m68k_register_t)i, 0xffffffff);
	#else
		m68k_reset(&context);
		for (int i = 0; i < 7; i++)
			context.a_regs[i].l = 0xffffffff;
		for (int i = 0; i < 8; i++)
			context.d_regs[i].l = 0xffffffff;
	#endif

		UMR = 0x20; // unused bit
		USR = 0b0000'0110; // TX ready and unused bit
		UCS = 0x08; // unused bit
		UCR = 0x80; // unused bit
		UTH = URH = 0;

		PICR[0] = PICR[1] = 0;
	}

	void interrupt(size_t ch, int level = 0)
	{
		if (level <= 0 || level > 7) {
			int level_lir = (ch == 1 ? LIR : (LIR >> 4)) & 0x07;
			int level_timer = PICR[0] & 0x07;
			int level_uart_rx = PICR[1] & 0x70;
			int level_uart_tx = PICR[1] & 0x07;
			level = std::max({level_lir, level_timer, level_uart_rx, level_uart_tx});
		}

		if (level > 0) {
		#ifdef MINICDI_MUSASHI
			m68k_set_irq(level + 56);
		#else
			m68k_set_irq(&context, level + 56 - 24);
		#endif
		}
	}

	uint8_t read8(uint32_t addr)
	{
		switch (addr)
		{
			case 0x80001001: return LIR & 0x77;

			/** UART **/
			case 0x80002011: return UMR | 0x20;
			case 0x80002013: USR |= (1<<1); return USR | 0x08;
			case 0x80002015: return UCS | 0x08;
			case 0x80002017: return UCR | 0x80;
			case 0x80002019: return UTH;
			case 0x8000201B: if (URH) USR |= 0x01; else USR &= ~(0x01); return URH;

			/** Timer **/
			case 0x80002020: return TSR;
			case 0x80002021: return TCR;
			case 0x80002022: return (RR >> 8) & 0x00FF;
			case 0x80002023: return RR & 0x00FF;
			case 0x80002024: return (T[0] >> 8) & 0x00FF;
			case 0x80002025: return T[0] & 0x00FF;
			case 0x80002026: return (T[1] >> 8) & 0x00FF;
			case 0x80002027: return T[1] & 0x00FF;
			case 0x80002028: return (T[2] >> 8) & 0x00FF;
			case 0x80002029: return T[2] & 0x00FF;

			/** PICR **/
			case 0x80002045: return PICR[0] & 0x77;
			case 0x80002047: return PICR[1] & 0x77;
		}

		return 0;
	}

	void write8(uint32_t addr, uint8_t value)
	{
		switch (addr)
		{
			/** LIR **/
			case 0x80001001:
				if ((value & 0x88) && (LIR & 0x88)) LIR &= 0x77;
				else if ((value & 0x80) && (LIR & 0x80)) LIR &= 0x7F;
				else if ((value & 0x08) && (LIR & 0x08)) LIR &= 0xF7;
				LIR = (LIR & 0x88) | (value & 0x77);
				break;

			/** UART **/
			case 0x80002011: UMR = value; break;
			case 0x80002013: USR = value; break;
			case 0x80002015: UCS = value; break;
			case 0x80002017: UCR = value;
				if (UCR & 0x0'011'0000) { UTH = 0; USR |= 0x08; } // reset transmitter
				else if (UCR & 0x0'010'0000) URH = 0; // reset receiver
				else if (UCR & 0x0'100'0000) USR &= 0x0F; // reset error status
				break;
			case 0x80002019: UTH = value; USR &= ~(0x08); break;
			case 0x8000201B: URH = value; break;

			/** Timer **/
			case 0x80002020: TSR = value; break;
			case 0x80002021: TCR = value; break;
			case 0x80002022: RR &= 0x00FF; RR |= (value << 8); break;
			case 0x80002023: RR &= 0xFF00; RR |= value; break;
			case 0x80002024: T[0] &= 0x00FF; T[0] |= (value << 8); break;
			case 0x80002025: T[0] &= 0xFF00; T[0] |= value; break;
			case 0x80002026: T[1] &= 0x00FF; T[1] |= (value << 8); break;
			case 0x80002027: T[1] &= 0xFF00; T[1] |= value; break;
			case 0x80002028: T[2] &= 0x00FF; T[2] |= (value << 8); break;
			case 0x80002029: T[2] &= 0xFF00; T[2] |= value; break;

			/** PICR **/
			case 0x80002045: PICR[0] = value;
				if (PICR[0] & 0x80) PICR[0] &= 0x7F;
				if (PICR[0] & 0x08) PICR[0] &= 0xF7;
				break;
			case 0x80002047: PICR[1] = value;
				if (PICR[1] & 0x80) PICR[1] &= 0x7F;
				if (PICR[1] & 0x08) PICR[1] &= 0xF7;
				break;
		}
	}

	int run(int cycles = 2000)
	{
		int ran = 0;

		for (ran = 0; ran < cycles;) {
			if (emuConfig && emuConfig->log != 0) {
				uint32_t pcLog;
			#ifdef MINICDI_MUSASHI
				pcLog = m68k_get_reg(NULL, M68K_REG_PC);
				ran += m68k_execute(500);
			#else
				pcLog = context.pc;
				ran += m68k_execute(&context, 500);
			#endif
				#ifdef MINICDI_MUSASHI
					if (pcLog != m68k_get_reg(NULL, M68K_REG_PC)) {
						char text[192];
						m68k_disassemble(text, m68k_get_reg(NULL, M68K_REG_PC), M68K_CPU_TYPE_SCC68070);
						fprintf(emuConfig->log, "[CPU][$%08X] %s\n", m68k_get_reg(NULL, M68K_REG_PC), text);
						// printf("\n$%08X: %s                            \n", m68k_get_reg(NULL, M68K_REG_PC), text);
					}
				#else
					if (pcLog != context.pc) {
						char text[128];
						m68k_disasm(&context, context.pc, text, (int)sizeof(text));
						fprintf(emuConfig->log, "[CPU][$%08X] %s\n", (uint32_t)context.pc, text);
						// printf("\n$%08X: %s                            \n", context.pc, text);
					}
				#endif
			} else {
			#ifdef MINICDI_MUSASHI
				ran += m68k_execute(cycles);
			#else
				ran += m68k_execute(&context, cycles);
			#endif
			}
		}

		for (int i = 0; i < ran; i+=96)
		{
			if (T[0] == 0xFFFF)
			{
				TSR |= 0x80; // OV in T0
				T[0] = RR;
				interrupt(0);
			}
			T[0]++;
		}

	#ifdef MINICDI_DEBUG
			printf("PC: %08X SR: %s%s %s%s%s%s%s\n",
		#ifdef MINICDI_MUSASHI
			m68k_get_reg(NULL, M68K_REG_PC),
			(m68k_get_reg(NULL, M68K_REG_SR) >> 15) & 0x01 ? "T" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 13) & 0x01 ? "S" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 4) & 0x01 ? "X" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 3) & 0x01 ? "N" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 2) & 0x01 ? "Z" : " ",
			(m68k_get_reg(NULL, M68K_REG_SR) >> 1) & 0x01 ? "V" : " ",
			m68k_get_reg(NULL, M68K_REG_SR) & 0x01 ? "C" : " "
		#else
			(uint32_t)context.pc,
			(context.sr >> 15) & 0x01 ? "T" : " ",
			(context.sr >> 13) & 0x01 ? "S" : " ",
			(context.sr >> 4) & 0x01 ? "X" : " ",
			(context.sr >> 3) & 0x01 ? "N" : " ",
			(context.sr >> 2) & 0x01 ? "Z" : " ",
			(context.sr >> 1) & 0x01 ? "V" : " ",
			context.sr & 0x01 ? "C" : " "
		#endif
			);

			for (int i = 0; i < 8; i++)
				printf("D%d: %08X A%d: %08X\n",
			#ifdef MINICDI_MUSASHI
				i, m68k_get_reg(NULL, (m68k_register_t)((int)M68K_REG_D0 + i)),
				i, m68k_get_reg(NULL, (m68k_register_t)((int)M68K_REG_A0 + i)));
			#else
				i, (uint32_t)context.d_regs[i].l,
				i, (uint32_t)context.a_regs[i].l);
			#endif

		printf("\nUCR: %02X URH: %02X USR: %02X LIR: %02X T0: %04X\n", UCR, URH, USR, LIR, T[0]);
	#endif

		return ran;
	}
};

#endif
