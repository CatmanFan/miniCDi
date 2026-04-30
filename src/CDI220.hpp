#ifndef MINICDI_CDI220
#define MINICDI_CDI220

#include "cdi/common.hpp"

#define CDI220_MEM_SIZE 0x800000 /* assume 8 MB */
#define CDI220_SLAVE_START 0x310000
#define CDI220_RAM_BANK1 0x000000
#define CDI220_RAM_BANK2 0x200000
#define CDI220_CIAP_BANK 0x300000
#define CDI220_ROM_BANK 0x400000
#define CDI220_ROM_SIZE 0x0FFBFF
#define CDI220_VDSC_START 0x4FFFE0

/** ******* Mono I memory map *******
	$00000000   512KB.ram    name=planea
	$00200000   512KB.ram    name=planeb
	$00400000   sysrom.rom   size=512KB
	$00300000   cdic.dev     level=4
	$00310000   slave.dev    level=2 vec=26
	$00318000   null.dev     size=32KB
	$00320000   nvr.dev
	$004fffe0   vdsc.dev
	$80000000   68070.dev
**/

class CDI220
{
private:
	uint8_t *memory = nullptr; // contains RAM bank 0, RAM bank 1 and rom
	IKAT slave;
	MCD212 mcd212;
	M68kCpu mpu;
	OS9::System os9;
	int ns;

public:
	MiniCDIConfig config;

	CDI220(const char* bios, MiniCDIConfig *config)
	{
		/** Order of initialization:
		1) Initialising slave processor
		2) Initialising video processor
		3) Clearing system RAM
		4) Building system exception table
		5) Determining the cpu type
		6) Initialising video (blue screen)
		7) Determining and enabling the display
		8) Executing RAM/ROM search
		9) Starting the kernel */

		this->config = *config;
		ns = 0;

		memory = (uint8_t *)memalign(32, CDI220_MEM_SIZE);
		if (memory) {
			// Load system ROM
			m68k_init(&mpu, memory, CDI220_MEM_SIZE);
			m68k_load_bin(&mpu, bios, CDI220_ROM_BANK);
			m68k_reset(&mpu);
			m68k_set_pc(&mpu, 0x4004b8);
			mpu.ssp = 0x1500;

			// Init slave processor (MC68HC)
			slave.init(memory, CDI220_SLAVE_START, &this->config);

			// Init video processor (MCD212)
			mcd212.init(&mpu, memory, CDI220_VDSC_START, &this->config);
			mcd212.reset();

			// Init OS-9
			os9.init(&mpu, &memory[CDI220_ROM_BANK], CDI220_ROM_SIZE);
		}
	}

	~CDI220()
	{
		if (memory) {
			free(memory);
		}
	}

	bool step()
	{
		printf("\x1b[%d;%dH", 3, 0);

		ns = MY_GETTIME;
		m68k_execute(&mpu, 1900);
		ns = MY_GETTIME - ns;

		printf("[CPU viewer]\n");
		printf("pc: %08x\n", mpu.pc);
		/*printf("d0: %08x d1: %08x d2: %08x d3: %08x\n", mpu.d_regs[0].l, mpu.d_regs[1].l, mpu.d_regs[2].l, mpu.d_regs[3].l);
		printf("d4: %08x d5: %08x d6: %08x d7: %08x\n", mpu.d_regs[4].l, mpu.d_regs[5].l, mpu.d_regs[6].l, mpu.d_regs[7].l);
		printf("a0: %08x a1: %08x a2: %08x a3: %08x\n", mpu.a_regs[0].l, mpu.a_regs[1].l, mpu.a_regs[2].l, mpu.a_regs[3].l);
		printf("a4: %08x a5: %08x a6: %08x a7: %08x\n", mpu.a_regs[4].l, mpu.a_regs[5].l, mpu.a_regs[6].l, mpu.a_regs[7].l);*/

		// os9.execute();
		slave.increment_time(ns);
		mcd212.increment_time(ns);

		return mcd212.check_vsync();
	}

	uint32_t* get_display()
	{
		return mcd212.get_display();
	}

	size_t get_display_width()
	{
		return mcd212.get_display_width();
	}
};

#endif