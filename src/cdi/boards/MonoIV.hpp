#ifndef MINICDI_BOARDS_MONO4
#define MINICDI_BOARDS_MONO4

/** ******* Mono-IV memory map *******
	$00000000   512KB.ram    name=planea
	$00200000   512KB.ram    name=planeb
	$00300000   ciap.dev     level=4
	$00310000   ikat.dev     level=2 vec=26
	$00320000   nvr.dev
	$00400000   sysrom.rom   size=512KB
	$004fffe0   vdsc.dev
	$80000000   68070.dev
**/

class MonoIV : public CDi
{
private:
	CIAP* ciap;
	IKAT* ikat;
	MCD212* vpu;
	PlayerLCD lcd;

public:
	bool init(const std::string &bios) override;

	inline void run(bool skip_draw = false) override {
		// Tick pointing device
		pd.send();

		// Timer normally ticks at 96 cycles, line polling at 960 ? Should verify
		int loops = 0;
		bool VBLANK = false;
		do {
			cpu.run(96);
			cpu.tick_timer();

			loops++;

			// 1035 is arbitrary number, should check how many cycles is equal to a sector tick
			// The speed MUST be at approximately 75 sectors per sec, otherwise it will not work!!
			if (loops % /*(MiniCDI::Config::PAL ? 1035 : 830)*/1035 == 0) { ciap->tick(); }

			// 15 MHz (not accurate) / 15625 Hz (line frequency) = 960 cycles
			if (loops % 10 == 0) { VBLANK = vpu->tick(skip_draw); }
		} while (!VBLANK);

		// Update LCD display
		// lcd.update_IKAT(ikat);

		// Print verbose CPU
		cpu.print();
		MiniCDI::OS9::scan_modules(memory);
	}

	inline void reset() override {
		cpu.reset();
		ikat->reset();
		vpu->reset();
	}

	inline uint32_t* get_display() override { return vpu->get_display(); }
	inline size_t get_display_width() override { return vpu->get_display_width(); }

	inline uint32_t* get_lcd() override { return nullptr; } // Not implemented
};

#endif