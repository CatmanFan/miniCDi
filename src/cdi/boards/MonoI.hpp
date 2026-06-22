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
	CDIC* cdic;
	SLAVE* slave;
	CIAP* ciap;
	IKAT* ikat;
	MCD212* vpu;
	PlayerLCD lcd;

public:
	bool init(const std::string &bios) override;

	inline void run(bool skip_draw = false) override {
		// Tick pointing device
		pd.send();

		const int disc_tick_rate = 15'000'000 / 75;
		const int line_tick_rate = 15'000'000 / 15625;

		int cycles_left_sector = disc_tick_rate;
		int cycles_left_vpu = line_tick_rate;

		bool VBLANK = false;
		do {
			int cycles = std::min({cycles_left_sector, cycles_left_vpu});
			for (int i = 0; i < cycles; i += 96) {
				m68k_execute(96);
				cpu.tick_timer();
			}

			cycles_left_sector -= cycles;
			cycles_left_vpu -= cycles;

			if (cycles_left_sector <= 0) {
				cycles_left_sector += disc_tick_rate;
				if (cdic) cdic->tick();
				else if (ciap) ciap->tick();
			}

			if (cycles_left_vpu <= 0) {
				cycles_left_vpu += line_tick_rate;
				VBLANK = vpu->tick(skip_draw);
			}
		} while (!VBLANK);

		// Update LCD display
		if (slave) lcd.get_from_slave(slave);

		// Print verbose CPU
		cpu.print();
		MiniCDI::OS9::scan_modules(memory);
	}

	inline void reset() override {
		cpu.reset();
		vpu->reset();
		if (slave) slave->reset();
	}

	inline uint32_t* get_display() override { return vpu->get_display(); }
	inline size_t get_display_width() override { return vpu->get_display_width(); }

	inline uint32_t* get_lcd() override { return slave ? &lcd.display[0] : nullptr; }
};

#endif