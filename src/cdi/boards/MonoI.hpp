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
	CDIC* cdic = NULL;
	SLAVE* slave = NULL;
	CIAP* ciap = NULL;
	DRVDSP* dsp = NULL;
	IKAT* ikat = NULL;
	MCD212* vpu = NULL;
	PlayerLCD lcd;

	// Scheduler values
	const int sector_tick_rate = (MiniCDI::Config::PAL ? 30000000 : 30209800) / 75;
	const int line_tick_rate = (MiniCDI::Config::PAL ? 30000000 : 30209800) / 15625;
	int cycles_left_sector = sector_tick_rate;
	int cycles_left_vpu = line_tick_rate;

public:
	bool init(const std::string &bios, enum BoardType board) override;

	inline void run(bool skip_draw = false) override
	{
		bool VBLANK = false;
		do {
			int cycles = std::min({cycles_left_sector, cycles_left_vpu});
			cpu.run(cycles);

			cycles_left_sector -= cycles;
			if (cycles_left_sector <= 0) {
				cycles_left_sector += sector_tick_rate;
				if (cdic != NULL) cdic->tick();
				else if (dsp != NULL) dsp->tick();
				else if (ciap != NULL) ciap->tick();
			}

			cycles_left_vpu -= cycles;
			if (cycles_left_vpu <= 0) {
				cycles_left_vpu += line_tick_rate;
				VBLANK = vpu != NULL ? vpu->tick(skip_draw) : true;
			}
		} while (!VBLANK);

		// Print verbose CPU
		cpu.print();
		MiniCDI::OS9::scan_modules(memory);

		// Tick pointing device
		pd.send_packet();
	}

	inline void reset() override {
		MiniCDI::Log("[CDI] reset");
		cpu.reset();
		vpu->reset();
		if (slave != NULL) slave->reset();
		if (ikat != NULL) ikat->reset();
	}
	~MonoI();

	inline uint32_t* get_display() override { return vpu->get_display(); }
	inline size_t get_display_width() override { return vpu->get_display_width(); }

	inline uint32_t* get_lcd() override {
		/*if (slave != NULL) {
			lcd.get_from_slave(slave);
			return &lcd.display[0];
		}*/

		return NULL;
	}
};

#endif