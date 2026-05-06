#ifndef MINICDI_PLAYERS
#define MINICDI_PLAYERS

#include "cdi/common.hpp"

class CDIPlayer
{
protected:
	uint8_t *memory		= nullptr; // Contains full memory map
	const int memSize	= 0x680000; // cdifan: max possible CD-i memory size is roughly 6.5 MB (CD-i 605 with DVC and expansion card)

	SCC68070 cpu;
	OS9::System os9;
	int ns;

public:
	MiniCDIConfig config;

	virtual bool Init(const char* bios, MiniCDIConfig *config)
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

		memory = (uint8_t *)memalign(32, memSize);
		if (memory) {
			memset(memory, 0, memSize);
			return true;
		}

		return false;
	}

	virtual ~CDIPlayer()
	{
		if (memory) {
			free(memory);
		}
	}

	virtual bool step()
	{
		ns = MY_GETTIME;
		cpu.execute();
		os9.execute();
		ns = MY_GETTIME - ns;

		return true;
	}

	virtual uint32_t* get_display() = 0;
	virtual size_t get_display_width() = 0;
};

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

class MonoIPlayer : public CDIPlayer
{
private:
	const int ramBank1	= 0x000000;
	const int ramBank2	= 0x200000;
	const int romAddr	= 0x400000;
	const int romSize	= 0x0FFBFF;

	const int vdscAddr	= 0x4FFFE0;
	const int ciapAddr	= 0x300000;
	const int slaveAddr	= 0x310000;

	IKAT* slave;
	MCD212* vpu;

public:
	bool Init(const char* bios, MiniCDIConfig *config) override;

	inline bool step() override {
		CDIPlayer::step();
		slave->increment_time(ns);
		vpu->increment_time(ns);

		return vpu->check_vsync();
	}

	inline uint32_t* get_display() override {
		return vpu->get_display();
	}

	inline size_t get_display_width() override {
		return vpu->get_display_width();
	}
};

#endif