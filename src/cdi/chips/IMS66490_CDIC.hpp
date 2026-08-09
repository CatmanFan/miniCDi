#ifndef MINICDI_IMS66490_CDIC
#define MINICDI_IMS66490_CDIC

/*****
  DISCLAIMER:
  Partially sourced from the MAME CDIC driver and documentation by Slamy.
 *****/

class CDIC
{
	SCC68070* _68070;
	uint8_t* memory;

	uint16_t CMD; // 0x3C00
	uint32_t TIME; // 0x3C02
	uint16_t FILE; // 0x3C06
	uint32_t CHAN; // 0x3C08
	uint16_t ACHAN; // 0x3C0C
	uint16_t DSEL; // 0x3C80

	uint16_t ABUF; // 0x3FF4
	uint16_t XBUF; // 0x3FF6
	uint16_t DMACTL; // 0x3FF8
	uint16_t AUDCTL; // 0x3FFA
	uint16_t IVEC; // 0x3FFC
	uint16_t DBUF; // 0x3FFE

	#if MINICDI_AUDIO==1 /* SDL2 */
	uint32_t SDL_audio_id = 0;
	bool SDL_audio_valid = false;
	#endif

	// ****************************
	// AUDIO HANDLING
	// ****************************
	AdpcmDecoder ADPCM;
	struct {
		size_t status;
		bool played[2];
		size_t buffer_index;
		int sector_interval;
	} SoundmapUnit; // Implementation based on Green Book

	bool adpcm_decode_and_play(int buffer, bool soundmap);
	void update_soundmap_unit();

	// ****************************
	// DISC HANDLING
	// ****************************
	CDiDisc *disc;

	struct {
		bool reading; // Whether is actively reading data
		bool seek;
		int delayed_sectors; // Number of sectors to delay for reading (e.g. to simulate discspin)
		int curr_lba; // Taken from TIME register and then incremented
		bool is_mode2; // Determines whether to apply MODE2 filter
	} CdicController;

	bool disc_check_filter();
	void disc_process_sector();

public:
	bool touched_disc = false; // This is so that the emulated machine can reset

	CDIC(SCC68070* _68070, uint8_t* memory, CDiDisc *disc);
	~CDIC();

	inline void reset()
	{
		CdicController = {0};
		SoundmapUnit = {0};
		touched_disc = false;

		AUDCTL = 0;
		ABUF = 0;
		XBUF = 0;
		DBUF = 0;
		_68070->interrupt(SCC68070::IPL_IN4N, false);
	}

	void tick();

	uint16_t read16(uint32_t addr);
	void write16(uint32_t addr, uint16_t value);
	uint32_t read32(uint32_t addr);
	void write32(uint32_t addr, uint32_t value);

	inline bool is_reading() { return CdicController.reading; } // do friend class MonoI?
};

#endif