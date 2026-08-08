#ifndef MINICDI_IMS66490_CDIC
#define MINICDI_IMS66490_CDIC

#ifdef MINICDI_AUDIO_SDL2
#include <SDL2/SDL.h>
#endif

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

	#ifdef MINICDI_AUDIO_SDL2
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

	CDIC(SCC68070* _68070, uint8_t* memory, CDiDisc *disc) : _68070(_68070), memory(memory), XBUF(0), SoundmapUnit({0}), disc(disc), CdicController({0})
	{
		// INIT AUDIO DRIVER
		#ifdef MINICDI_AUDIO_SDL2
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
		{
			MiniCDI::Log("[Audio:SDL2] failed to init audio subsystem");
			SDL_audio_valid = false;
		}
		else
		{
			SDL_AudioSpec input, output;
			SDL_zero(input);
			input.freq = 37800;
			input.format = AUDIO_S16SYS;
			input.channels = 2;
			input.samples = 448;

			SDL_audio_id = SDL_OpenAudioDevice(NULL, 0, &input, &output, 0);
			SDL_audio_valid = SDL_audio_id > 0;
			if (SDL_audio_valid)
			{
				MiniCDI::Log("[Audio:SDL2] initialized audio device #%d", SDL_audio_id);
				// MiniCDI::Log("[Audio:SDL2] audiospec frequency: %d", output.freq);
				// MiniCDI::Log("[Audio:SDL2] audiospec format: %d", output.format);
				// MiniCDI::Log("[Audio:SDL2] audiospec channels: %d", output.channels);
				// MiniCDI::Log("[Audio:SDL2] audiospec samples: %d", output.samples);
				SDL_PauseAudioDevice(SDL_audio_id, 0);
			}
		}
		#endif
	}

	~CDIC()
	{
		// CLOSE AUDIO DRIVER
		#ifdef MINICDI_AUDIO_SDL2
		if (SDL_audio_valid)
		{
			SDL_CloseAudioDevice(SDL_audio_id);
			SDL_QuitSubSystem(SDL_INIT_AUDIO);
			SDL_audio_valid = false;
		}
		#endif
	}

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

	inline uint16_t read16(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return (memory[addr] << 8) | memory[addr+1];

			case 0x303C00: case 0x303C01:
				//MiniCDI::Log("[CDIC] CMD => %04X", CMD);
				return CMD;

			case 0x303C06: case 0x303C07:
				//MiniCDI::Log("[CDIC] FILE => %04X", FILE);
				return FILE;

			case 0x303C0C: case 0x303C0D:
				//MiniCDI::Log("[CDIC] ACHAN => %04X", ACHAN);
				return ACHAN;

			case 0x303C80: case 0x303C81:
				//MiniCDI::Log("[CDIC] DSEL => %04X", DSEL);
				return DSEL;

			case 0x303FF4: case 0x303FF5:
			{
				//MiniCDI::Log("[CDIC] ABUF => %04X", ABUF);
				const uint16_t value = ABUF;
				if (ABUF & 0x8000) {
					ABUF &= ~0x8000;
					_68070->interrupt(SCC68070::IPL_IN4N, false);
				}
				return value;
			}

			case 0x303FF6: case 0x303FF7:
			{
				//MiniCDI::Log("[CDIC] XBUF => %04X", XBUF);
				const uint16_t value = XBUF;
				if (XBUF & 0x8000) {
					XBUF &= ~0x8000;
					_68070->interrupt(SCC68070::IPL_IN4N, false);
				}
				return value;
			}

			case 0x303FF8: case 0x303FF9:
				//MiniCDI::Log("[CDIC] DMACTL => %04X", DMACTL);
				return DMACTL;

			case 0x303FFA: case 0x303FFB:
			{
				//MiniCDI::Log("[CDIC] AUDCTL => %04X", AUDCTL);
				const uint16_t value = AUDCTL;
				if (AUDCTL & 0x0001) {
					AUDCTL &= ~0x0001;
					// _68070->interrupt(SCC68070::IPL_IN4N, false);
				}
				return value;
			}

			case 0x303FFC: case 0x303FFD:
				//MiniCDI::Log("[CDIC] IVEC => %04X", IVEC);
				return IVEC;

			case 0x303FFE: case 0x303FFF:
				//MiniCDI::Log("[CDIC] DBUF => %04X", DBUF);
				return DBUF;
		}
	}

	inline void write16(uint32_t addr, uint16_t value)
	{
		switch (addr)
		{
			default:
				memory[addr] = value >> 8 & 0xFF;
				memory[addr+1] = value & 0xFF;
				break;

			case 0x303C00: case 0x303C01:
				//MiniCDI::Log("[CDIC] CMD <= %04X", value);
				CMD = value;
				break;

			case 0x303C06: case 0x303C07:
				//MiniCDI::Log("[CDIC] FILE <= %04X", value);
				FILE = value;
				break;

			case 0x303C0C: case 0x303C0D:
				//MiniCDI::Log("[CDIC] ACHAN <= %04X", value);
				ACHAN = value;
				if (SoundmapUnit.status == 2) SoundmapUnit.status = 0;
				break;

			case 0x303C80: case 0x303C81:
				//MiniCDI::Log("[CDIC] DSEL <= %04X", value);
				DSEL = value;
				break;

			case 0x303FF4: case 0x303FF5:
				//MiniCDI::Log("[CDIC] ABUF <= %04X", value);
				ABUF = value;
				break;

			case 0x303FF6: case 0x303FF7:
				//MiniCDI::Log("[CDIC] XBUF <= %04X", value);
				XBUF = value;
				break;

			case 0x303FF8: case 0x303FF9:
				//MiniCDI::Log("[CDIC] DMACTL <= %04X", value);
				DMACTL = value;
				if (value & 0x8000) {
					_68070->dma_call(0, 0x300000 + (value & 0x3FFF));
					SoundmapUnit.played[(value & 0x3FFF) >= 0x3200] = false;
				}
				break;

			case 0x303FFA: case 0x303FFB:
				MiniCDI::Log("[CDIC] AUDCTL <= %04X", value);
				AUDCTL = value;

				if (value == 0x2800 && SoundmapUnit.status != 1)
				{
					MiniCDI::Log("[CDIC:Soundmap] Started playback");
					SoundmapUnit.status = 1;
					SoundmapUnit.buffer_index = 0;
				}
				else if (value != 0x2800 && SoundmapUnit.status == 1)
				{
					MiniCDI::Log("[CDIC:Soundmap] Stopped playback");
					SoundmapUnit.status = 0;
					SoundmapUnit.buffer_index = 0;
				}
				break;

			case 0x303FFC: case 0x303FFD:
				//MiniCDI::Log("[CDIC] IVEC <= %04X", value);
				IVEC = value;
				_68070->Ipl.vectors[SCC68070::IPL_IN4N] = value & 0x00FF;
				break;

			case 0x303FFE: case 0x303FFF:
				//MiniCDI::Log("[CDIC] DBUF <= %04X", value);
				DBUF = value;
				if (value & 0x8000)
				{
					switch (CMD)
					{
						case 0x23: // Known as "Reset Mode 1" in MAME
						case 0x24: // Known as "Reset Mode 2" in MAME
							switch (CMD) {
								case 0x23: MiniCDI::Log("[CDIC] End disc rotation (0x%02X)", CMD); break;
								case 0x24: MiniCDI::Log("[CDIC] Stop reading? (0x%02X)", CMD); break;
							}

							// Set disc status to inactive
							CdicController.reading = false;
							CdicController.seek = false;
							CdicController.delayed_sectors = 6;
							CdicController.curr_lba = 0;
							CdicController.is_mode2 = CMD == 0x24;
							break;

						case 0x27:
							MiniCDI::Log("[CDIC] Fetch Table of Contents (0x%02X)", CMD);
							assert(0 && "[CDIC] Command not implemented.");
							break;

						case 0x28:
							MiniCDI::Log("[CDIC] Start CDDA playback (0x%02X)", CMD);
							assert(0 && "[CDIC] Command not implemented.");
							break;

						case 0x2B:
							MiniCDI::Log("[CDIC] Stop CDDA playback? (0x%02X)", CMD);
							CdicController.reading = false; // Replicate MAME behaviour
							break;

						case 0x29:
						case 0x2A:
						case 0x2C:
							switch (CMD) {
								case 0x29: MiniCDI::Log("[CDIC] Start read MODE1 at $%08X (0x%02X)", TIME, CMD); break;
								case 0x2A: MiniCDI::Log("[CDIC] Start read MODE2 at $%08X (0x%02X)", TIME, CMD); break;
								case 0x2C: MiniCDI::Log("[CDIC] Seek MODE1 at $%08X? (0x%02X)", TIME, CMD); break;
							}

							// Set disc status to active. This will cause it to start reading each sector at 75Hz.
							CdicController.reading = true;
							CdicController.seek = CMD == 0x2C;
							CdicController.curr_lba = disc->get_lba_from_time(TIME);
							CdicController.is_mode2 = CMD == 0x2A ? true : false;
							disc->read_sector(CdicController.curr_lba);
							break;

						case 0x2E:
							MiniCDI::Log("[CDIC] Update MODE2 filter (0x%02X)", CMD);
							break;
					}

					// Acknowledge the command
					DBUF &= ~0x8000;
				}

				if (!(value & 0x4000))
				{
					MiniCDI::Log("[CDIC] Stop reading by DBUF");

					// Reset only the disc read status
					CdicController = {0};
				}
				break;
		}
	}

	inline uint32_t read32(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return (read16(addr) << 16) | read16(addr+2);

				case 0x303C02:
					//MiniCDI::Log("[CDIC] TIME => %08X", TIME);
					return TIME;

				case 0x303C08:
					//MiniCDI::Log("[CDIC] CHAN => %08X", CHAN);
					return CHAN;
		}
	}

	inline void write32(uint32_t addr, uint32_t value)
	{
		switch (addr)
		{
			default:
				write16(addr, value >> 16 & 0xFFFF);
				write16(addr+2, value & 0xFFFF);
				break;

			case 0x303C02:
				//MiniCDI::Log("[CDIC] TIME <= %08X", value);
				TIME = value;
				break;

			case 0x303C08:
				//MiniCDI::Log("[CDIC] CHAN <= %08X", value);
				CHAN = value;
				break;
		}
	}

	inline bool is_reading() { return CdicController.reading; } // do friend class MonoI?
};

#endif