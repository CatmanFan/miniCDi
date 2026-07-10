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
	enum EventType
	{
		SECTOR = 0,
		VPU,
		UART_TX,
		#ifdef MINICDI_PDTICK
		EventCOUNT,
		PD
		#else
		EventCOUNT
		#endif
	};

	int event_rates[EventCOUNT] =
	{
		/* SECTOR */ (MiniCDI::Config::PAL ? 15000000 : 15104900) / 75,
		/* VPU */ (MiniCDI::Config::PAL ? 15000000 : 15104900) / 15625,
		/* UART_TX */ 4915200
		#ifdef MINICDI_PDTICK
		, /* PD */ (MiniCDI::Config::PAL ? 15000000 : 15104900) / 30 // absolute: 30, relative: 40
		#endif
	};

	int event_cycles[EventCOUNT] =
	{
		event_rates[SECTOR],
		event_rates[VPU],
		event_rates[UART_TX]
		#ifdef MINICDI_PDTICK
		, event_rates[PD]
		#endif
	};

public:
	bool init(const std::string &bios, enum BoardType board) override;

	inline void run(bool skip_draw = false) override
	{
		bool VBLANK = false;
		do {
			int cycles = *(std::min_element(event_cycles, event_cycles + (sizeof(event_cycles) / sizeof(event_cycles[0]))));
			cpu.run(cycles);

			for (int i = 0; i < EventCOUNT; i++)
			{
				event_cycles[i] -= cycles;
				if (event_cycles[i] <= 0)
				{
					switch (i)
					{
						case SECTOR:
							if (cdic != NULL) cdic->tick();
							else if (dsp != NULL) dsp->tick();
							else if (ciap != NULL) ciap->tick();
							break;

						case VPU:
							VBLANK = vpu != NULL ? vpu->tick(skip_draw) : true;
							break;

						case UART_TX:
							cpu.uart_tx_tick();
							break;

						#ifdef MINICDI_PDTICK
						case PD:
							pd.send_packet();
							break;
						#endif
					}

					event_cycles[i] += event_rates[i];
				}
			}
		} while (!VBLANK);

		// Print verbose CPU
		cpu.print();
		MiniCDI::OS9::scan_modules(memory);

		#ifndef MINICDI_PDTICK
		// Tick pointing device
		pd.send_packet();
		#endif

		// Update microcontroller
		if (ikat != NULL) ikat->update();
	}

	inline void reset() override {
		MiniCDI::Log("[CDI] reset");
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
		#ifdef MINICDI_PDTICK
		event_cycles[PD] = event_rates[PD];
		#endif
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