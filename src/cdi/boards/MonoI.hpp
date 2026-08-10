#ifndef MINICDI_BOARDS_MONO1
#define MINICDI_BOARDS_MONO1

#include <chrono>
#include <thread>
#include <numeric>

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

	FTD* ftd;

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

	inline bool check_for_unmapped(int address)
	{
		if ((address >= 0x080000 && address < 0x200000) // in-between DRAM banks
		 || (address >= 0x328000 && address < 0x400000) // between NVRAM and ROM
		 || (address >= 0x500000 && address < 0xD00000) // between MCD212 and VMPEG
		 || (address >= 0xD00000 && address < 0xF00000)) // VMPEG is not connected
		{
			m68k_pulse_bus_error();
			return false;
		}
		return true;
	}

public:
	bool init(const std::string &bios, enum BoardType board) override;
	~MonoI();

	inline void run(bool no_draw = false) override
	{
		#ifndef MINICDI_RAW_68K_MODE
		pd.send_packet();
		#endif

		// Benchmark
		const auto t1 = std::chrono::steady_clock::now();

		#ifdef MINICDI_DEBUG_OS9
		MiniCDI::OS9::scan_modules(memory);
		#endif

		while (1)
		{
			// A cycle rate of 240 is large enough that it doesn't break CDi_BadApple, but small enough that it also doesn't break the 2nd player shell.
			// On embedded consoles this also affects the speed of the emulator.
			const int cycles = std::min({240, event_rates[0], event_rates[1], event_rates[2]});
			cpu.run(cycles);

			for (int i = 0; i < EVTNUM; i++)
			{
				event_cycles[i] -= cycles;
				while (event_cycles[i] <= 0)
				{
					event_cycles[i] += event_rates[i];
					switch (i)
					{
						case SECTOR:
							if (!disc.is_open()) continue;
							if (cdic != NULL) cdic->tick();
							else if (dsp != NULL) dsp->tick();
							else if (ciap != NULL) ciap->tick();
							break;

						case VPU:
							if (vpu == NULL) continue;
							vpu->skip_draw = no_draw;
							if (vpu->tick()) goto frame_end;
							break;

						case UART_TX:
							cpu.uart_tx_tick();
							break;
					}
				}
			}
		}

		frame_end:
		// Update microcontroller
		if (ikat != NULL) ikat->update();

		// Benchmark end
		// integral duration: requires duration_cast
		const auto t2 = std::chrono::steady_clock::now();
		const std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
		#ifdef MINICDI_BENCHMARKING
		MiniCDI::Log("[CDI] %s frame in %.2f ms", no_draw ? "Executed" : "Executed and drawn", fp_ms.count());
		#endif

		#ifdef MINICDI_NO_THROTTLING
		return;
		#endif

		// Throttling
		#if defined(_WIN32) || defined(__APPLE__)
		if (fp_ms.count() < (1000.0f/60.0f) && !MiniCDI::Config::NoFrameLimit) {
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
		if (ftd != NULL) ftd->reset();

		// Microcontroller
		if (slave != NULL) slave->reset();
		if (ikat != NULL) ikat->reset();

		// CD-Audio
		if (cdic != NULL) cdic->reset();
		if (dsp != NULL) dsp->reset();
		if (ciap != NULL) ciap->reset();

		for (int i = 0; i < EVTNUM; i++) { event_cycles[i] = event_rates[i]; }
	}

	inline uint8_t read8(int address) override
	{
		if (!check_for_unmapped(address)) return 0;
		return (address & 0xC0000000) == 0x80000000 ? cpu.read8(address)
			 : (address & 0x00FFFF00) == 0x00310000 ? (ikat ? ikat->read8(address) : slave->read8(address))
			 : dsp && address >= 0x00300000 && address < 0x00303FFF ? dsp->read8(address)
			 : vpu && (address & 0x00FFFF00) == 0x004FFF00 ? vpu->read8(address)
			 : address < memsize ? memory[address]
			 : 0;
	}

	inline uint16_t read16(int address) override
	{
		if (!check_for_unmapped(address)) return 0;
		return vpu && (address & 0x00FFFF00) == 0x004FFF00 ? vpu->read16(address)
			 : address >= 0x00300000 && address < 0x00303FFF ? (cdic ? cdic->read16(address) : ciap ? ciap->read16(address) : 0)
			 : (uint16_t)(read8(address) << 8 | read8(address+1));
	}

	inline uint32_t read32(int address) override
	{
		if (!check_for_unmapped(address)) return 0;
		return cdic && address >= 0x00300000 && address < 0x00303FFF ? cdic->read32(address)
			 : (uint32_t)(read16(address) << 16 | read16(address+2));
	}

	inline void write8(int address, uint8_t value) override
	{
		if (!check_for_unmapped(address))
			return;
		#ifdef MINICDI_DEADNVRAM
		// Dead Timekeeper/NVRAM will not allow writing
		if ((address & 0x00FFFF00) == 0x00320000)
			return;
		#endif
		else if ((address & 0xC0000000) == 0x80000000)
			cpu.write8(address, value);
		else if (slave && (address & 0x00FFFF00) == 0x00310000)
			slave->write8(address, value);
		else if (ikat && (address & 0x00FFFF00) == 0x00310000)
			ikat->write8(address, value, ciap);
		else if (dsp && address >= 0x00300000 && address < 0x00303FFF)
			dsp->write8(address, value);
		else if (address < memsize)
			memory[address] = value;
	}

	inline void write16(int address, uint16_t value) override
	{
		if (!check_for_unmapped(address))
			return;
		if (vpu && (address & 0x00FFFF00) == 0x004FFF00)
			vpu->write16(address, value);
		else if (cdic && address >= 0x00300000 && address < 0x00303FFF)
			cdic->write16(address, value);
		else if (ciap && address >= 0x00300000 && address < 0x00303FFF)
			ciap->write16(address, value);
		else
		{
			write8(address, value >> 8 & 0xFF);
			write8(address+1, value & 0xFF);
		}
	}

	inline void write32(int address, uint32_t value) override
	{
		if (!check_for_unmapped(address))
			return;
		if (cdic && address >= 0x00300000 && address < 0x00303FFF)
			return cdic->write32(address, value);
		else
		{
			write16(address, value >> 16 & 0xFFFF);
			write16(address+2, value & 0xFFFF);
		}
	}

	inline void play_disc() {
		if (slave != NULL) slave->send_play_button();
	}

	inline void swap_disc(const std::string &path) {
		if (cdic != NULL) {
			if (cdic->touched_disc) reset();
			else cdic->reset();
		}

		disc.eject();
		if (!disc.open(path)) return;

		if (slave != NULL) slave->send_disc_status(true);
		if (ikat != NULL) ikat->send_disc_status(true);
	}

	inline uint32_t* get_display() override { return vpu->get_display(); }
	inline size_t get_display_width() override { return vpu->get_display_width(); }

	inline uint8_t* get_ftd() { return ftd != NULL ? ftd->get_display() : NULL; }
	inline size_t get_ftd_width() { return ftd != NULL ? ftd->get_display_width() : 0; }
	inline size_t get_ftd_height() { return ftd != NULL ? ftd->get_display_height() : 0; }

	inline bool get_cd_read_status() override {
		if (cdic != NULL) return cdic->is_reading();
		if (ciap != NULL) return ciap->is_reading();
		return false;
	}
};

#endif