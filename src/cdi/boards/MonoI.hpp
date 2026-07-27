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
	MCD212* vpu = NULL;
	SLAVE* slave = NULL;
	IKAT* ikat = NULL;
	CDIC* cdic = NULL;
	DRVDSP* dsp = NULL;
	CIAP* ciap = NULL;

	FPD* fpd;

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
	~MonoI();

	inline void run(int frames = 1) override
	{
		#ifndef MINICDI_RAW_68K_MODE
		pd.send_packet();
		#endif

		// Benchmark
		const auto t1 = std::chrono::steady_clock::now();

		for (int frames_left = frames; frames_left > 0; frames_left--)
		{
			MiniCDI::OS9::scan_modules(memory);

			// Update microcontroller
			if (ikat != NULL) ikat->update();

			#ifdef MINICDI_RAW_68K_MODE
			for (int total_cycles = 0; total_cycles < event_rates[VPU] * (MiniCDI::Config::PAL ? 312 : 262);)
			#else
			for (bool vblank = false; !vblank;)
			#endif
			{
				// Find a less-memory intensive method?
				// int cycles = 480;
				int cycles = *(std::min_element(event_cycles, event_cycles + (sizeof(event_cycles) / sizeof(event_cycles[0]))));
				cpu.run(cycles);

				#ifdef MINICDI_RAW_68K_MODE
				total_cycles += cycles;
				#else
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
								if (vpu != NULL) vblank = vpu->tick(frames_left != frames);
								break;

							case UART_TX:
								cpu.uart_tx_tick();
								break;
						}

						event_cycles[i] += event_rates[i];
					}
				}
				#endif
			}
		}

		// Benchmark end
		// integral duration: requires duration_cast
		const auto t2 = std::chrono::steady_clock::now();
		const std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
		#ifdef MINICDI_BENCHMARKING
		MiniCDI::Log("[CDI] Executed %d %s in %.2f ms", frames, frames == 1 ? "frame" : "frames", fp_ms.count());
		#endif

		#ifdef MINICDI_NO_THROTTLING
		return;
		#endif

		// Throttling
		#if defined(_WIN32) || defined(__APPLE__)
		if (fp_ms.count() < (1000.0f/60.0f)) {
			const int wait_ms = (int)((1000.0f/60.0f) - fp_ms.count());
			std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
		}
		#endif
	}

	inline void reset() override {
		MiniCDI::Log("[CDI] reset");
		MiniCDI::OS9::clear_modules();

		nvram_save();
		cpu.reset();
		vpu->reset();
		if (fpd != NULL) fpd->reset();

		// Microcontroller
		if (slave != NULL) slave->reset();
		if (ikat != NULL) ikat->reset();

		// CD-Audio
		if (cdic != NULL) cdic->reset();
		if (dsp != NULL) dsp->reset();
		if (ciap != NULL) ciap->reset();

		event_cycles[SECTOR] = event_rates[SECTOR];
		event_cycles[VPU] = event_rates[VPU];
		event_cycles[UART_TX] = event_rates[UART_TX];
	}

	inline void play_disc() {
		if (slave != NULL) slave->send_play_button();
	}

	inline void swap_disc(const std::string &path) {
		if (cdic != NULL) cdic->reset();

		disc.eject();
		if (!disc.open(path)) return;

		if (slave != NULL) slave->send_disc_status(true);
		if (ikat != NULL) ikat->send_disc_status(true);
	}

	inline uint32_t* get_display() override { return vpu->get_display(); }
	inline size_t get_display_width() override { return vpu->get_display_width(); }

	inline uint8_t* get_fpd() { return fpd != NULL ? fpd->get_display() : NULL; }
	inline size_t get_fpd_width() { return fpd != NULL ? fpd->get_display_width() : 0; }
	inline size_t get_fpd_height() { return fpd != NULL ? fpd->get_display_height() : 0; }

	inline bool get_cd_read_status() override {
		if (cdic != NULL) return cdic->is_reading();
		if (ciap != NULL) return ciap->is_reading();
		return false;
	}
};

#endif