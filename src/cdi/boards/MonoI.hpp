#ifndef MINICDI_BOARDS_MONO1
#define MINICDI_BOARDS_MONO1

/** ******* Mono-I memory map *******
	$00000000   512KB.ram    name=planea
	$00200000   512KB.ram    name=planeb
	$00300000   cdic.dev     level=4
	$00310000   slave.dev    level=2 vec=26
	$00320000   nvr.dev
	$00400000   sysrom.rom   size=512KB
	$004fffe0   vdsc.dev
	$80000000   68070.dev
**/

class MonoI : public CDi
{
private:
	const int ramBank1	= 0x000000;
	const int ramBank2	= 0x200000;
	const int romAddr	= 0x400000;
	const int romSize	= 0x0FFBFF;

	const int vdscAddr	= 0x4FFFE0;
	const int ciapAddr	= 0x300000;
	const int slaveAddr	= 0x310000;

	SCC68070 cpu;
	SLAVE* slave;
	MCD212* vpu;
	PlayerLCD lcd;

public:
	MonoI() : CDi(), cpu(memory) {}

	bool Init(const char* bios) override;

	inline void do_frame(bool draw = true) override {
		// Timer normally ticks at 96 cycles, line polling at 960 ? Should verify
		while (1)
		{
			for (uint8_t cycles = 0; cycles < 10; cycles++) {
				cpu.run(96);
				cpu.tick_timer();
			}
			if (vpu->tick(!draw)) break;
		}

		// Update LCD display
		lcd.update(slave);
	}

	inline void reset() override {
		cpu.reset();
		slave->reset();
		vpu->reset();
	}

	inline uint32_t* get_display() override { return vpu->get_display(); }
	inline size_t get_display_width() override { return vpu->get_display_width(); }

	inline uint32_t* get_lcd() override { return &lcd.display[0]; }

	inline void set_pointer_x(int x, bool increment) override { slave->set_pointer_x(x, increment); }
	inline void set_pointer_y(int y, bool increment) override { slave->set_pointer_y(y, increment); }
	inline void set_pointer_button(int b, bool value) override { slave->set_pointer_button(b, value); }
};

#endif