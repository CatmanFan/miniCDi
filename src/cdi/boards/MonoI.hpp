#ifndef MINICDI_BOARDS_MONO1
#define MINICDI_BOARDS_MONO1

#include <chrono>
#include <thread>

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
	// PlayerLCD lcd;

	// Scheduler values
	enum EventType
	{
		SECTOR = 0,
		VPU,
		UART_TX,
		EVTNUM
	};

	int event_rates[EVTNUM] =
	{
		/* SECTOR */ (MiniCDI::Config::PAL ? 15000000 : 15104900) / 75,
		/* VPU */ (MiniCDI::Config::PAL ? 15000000 : 15104900) / 15625,
		/* UART_TX */ 4915200
	};

	int event_cycles[EVTNUM] =
	{
		event_rates[SECTOR],
		event_rates[VPU],
		event_rates[UART_TX]
	};

public:
	bool init(const std::string &bios, enum BoardType board) override;

	inline void run(int frames = 1) override
	{
		const auto t1 = std::chrono::steady_clock::now();
		for (int total_frames = 0; total_frames < frames; total_frames++)
		{
			for (int total_cycles = 0; total_cycles < event_rates[VPU] * (MiniCDI::Config::PAL ? 312 : 262);)
			{
				int cycles = *(std::min_element(event_cycles, event_cycles + (sizeof(event_cycles) / sizeof(event_cycles[0]))));
				cpu.run(cycles);

				for (int i = 0; i < EVTNUM; i++)
				{
					event_cycles[i] -= cycles;
					while (event_cycles[i] <= 0)
					{
						switch (i)
						{
							case SECTOR:
								if (cdic != NULL) cdic->tick();
								else if (dsp != NULL) dsp->tick();
								else if (ciap != NULL) ciap->tick();
								break;

							case VPU:
								#ifndef MINICDI_RAW_68K_MODE
								if (vpu != NULL) vpu->tick(total_frames > 0);
								#endif
								break;

							case UART_TX:
								cpu.uart_tx_tick();
								break;
						}

						event_cycles[i] += event_rates[i];
					}
				}

				total_cycles += cycles;
			}
		}

		// Print verbose CPU
		cpu.print();
		MiniCDI::OS9::scan_modules(memory);

		#ifndef MINICDI_PDTICK
		// Tick pointing device
		pd.send_packet();
		#endif

		// Update microcontroller
		if (ikat != NULL) ikat->update();

		// integral duration: requires duration_cast
		const auto t2 = std::chrono::steady_clock::now();
		const auto fp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
		if (fp_ms.count() < (MiniCDI::Config::PAL ? 20.0 : 16.67)) {
			const int wait_ms = (int)((MiniCDI::Config::PAL ? 20.0 : 16.67) - fp_ms.count());
			std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
		}
	}

	inline void reset() override {
		MiniCDI::Log("[CDI] reset");
		MiniCDI::OS9::clear_modules();

		nvram_save();
		cpu.reset();
		vpu->reset();

		// Microcontroller
		if (slave != NULL) slave->reset();
		if (ikat != NULL) ikat->reset();

		// CD-Audio
		if (cdic != NULL) cdic->reset();

		event_cycles[SECTOR] = event_rates[SECTOR];
		event_cycles[VPU] = event_rates[VPU];
		event_cycles[UART_TX] = event_rates[UART_TX];
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
	inline bool get_cd_read_status() override {
		if (cdic != NULL) return cdic->is_reading();
		if (ciap != NULL) return ciap->is_reading();
		return false;
	}
};

#endif