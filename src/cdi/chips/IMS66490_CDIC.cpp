#include "cdi/common.hpp"

#if MINICDI_AUDIO==1 /* SDL2 */
#include <SDL2/SDL.h>
#endif

#define SAMPLE_COUNT 448

CDIC::CDIC(SCC68070* _68070, uint8_t* memory, CDiDisc *disc) : _68070(_68070), memory(memory), XBUF(0), SoundmapUnit({0}), disc(disc), CdicController({0})
{
	// INIT AUDIO DRIVER
	#if MINICDI_AUDIO==1 /* SDL2 */
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
		input.samples = SAMPLE_COUNT;

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

CDIC::~CDIC()
{
	// CLOSE AUDIO DRIVER
	#if MINICDI_AUDIO==1 /* SDL2 */
	if (SDL_audio_valid)
	{
		SDL_CloseAudioDevice(SDL_audio_id);
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		SDL_audio_valid = false;
	}
	#endif
}

bool CDIC::disc_check_filter()
{
	if (disc->Sector[CDiDisc::H_MODE] == 2 && CdicController.is_mode2)
	{
		// Use order from MAME
		if ((disc->Sector[CDiDisc::SH_FILE2] << 8) != FILE) {
			//MiniCDI::Log("[CDIC] MODE2 skip: FILE %04X != %02X", FILE, disc->Sector[CDiDisc::SH_FILE2]);
			return false;
		}

		if ((disc->Sector[CDiDisc::SH_SUBMODE2] & 0b10000000) // EOF
		 || (disc->Sector[CDiDisc::SH_SUBMODE2] & 0b00000001) // EOR
		 || (disc->Sector[CDiDisc::SH_SUBMODE2] & 0b00010000) // Trigger
		 ) {
			if (disc->Sector[CDiDisc::SH_SUBMODE2] & 0b10000000) {
				MiniCDI::Log("[CDIC] MODE2: reached EOF");
				CdicController.reading = false;
			}
			//MiniCDI::Log("[CDIC] MODE2 autoread");
			return true;
		}

		if (!(disc->Sector[CDiDisc::SH_SUBMODE2] & 0b00001110)) {
			// Either message or empty sector (Green Book II.4.9.1)
			//MiniCDI::Log("[CDIC] MODE2 skip: invalid sector");
			return false;
		}

		if (!(CHAN & (1<<disc->Sector[CDiDisc::SH_CHAN2]))) {
			//MiniCDI::Log("[CDIC] MODE2 skip: CHAN %08X is not AND (1 << %d)", CHAN, disc->Sector[CDiDisc::SH_CHAN2]);
			return false;
		}
	}

	return true;
}

void CDIC::disc_process_sector()
{
	if (!CdicController.reading)
		return;

	if (CdicController.delayed_sectors > 0) {
		CdicController.delayed_sectors--;
		return;
	}

	if (disc_check_filter()) // This skips if MODE2 is not satisfied
	{
		touched_disc = true;

		DBUF &= ~0x0004; // Reset audio (per MAME)
		DBUF ^= 0x0001; // Update buffer index (per MAME). This has to be done BEFORE targetAddr is defined otherwise disc will not init.
		uint32_t targetAddr = 0x300000 + ((DBUF & 0x01)*0xA00);

		// Fetch mainchannel data from frame, followed by subchannel data.
		memcpy(&memory[targetAddr], &disc->Sector[0], 8);
		targetAddr += 8;

		// At this point the CDIC should switch to the ADPCM buffer if the fetched sector is of audio type.
		// Submode filter (Form=1, Data=0, Audio=1, Video=0) is defined in Figure IV.3 of the Green Book
		bool is_adpcm = !(CdicController.is_mode2 && disc->Sector[CDiDisc::H_MODE] == 2) ? false
					  : (disc->Sector[CDiDisc::SH_SUBMODE2] & 0b00101110) == 0b00100100;
		if (SoundmapUnit.status == 0) is_adpcm = is_adpcm && (ACHAN & (1 << disc->Sector[CDiDisc::SH_CHAN2]));
		if (SoundmapUnit.status == 1) is_adpcm = false;

		if (is_adpcm)
		{
			targetAddr += 0x2800; // switch to ADPCM buffer

			// Set audio bit in DBUF and AUDCTL playback bit.
			DBUF |= 0x0004;
			if ((AUDCTL & 0x0800) == 0x0000)
			{
				MiniCDI::Log("[CDIC] start audio playback from disc");
				AUDCTL |= 0x0800;
			}
		}

		// These bytes should be separated if the sector is passed as audio
		memcpy(&memory[targetAddr], &disc->Sector[8], 2332);
		//MiniCDI::Log("[CDIC] read %s sector %02X:%02X:%02X", is_adpcm ? "audio" : "data", disc->Sector[CDiDisc::H_MIN], disc->Sector[CDiDisc::H_SEC], disc->Sector[CDiDisc::H_FRAME]);

		// Decode and play the fetched ADPCM sector.
		if (is_adpcm) adpcm_decode_and_play(DBUF & 0x01, false);

		targetAddr += 2332;
		// switch (mode) default:
		memory[targetAddr++] = 0x41; // Control
		memory[targetAddr++] = 0x01; // Track
		memory[targetAddr++] = 0x01; // Index
		memory[targetAddr++] = disc->Sector[CDiDisc::H_MIN];
		memory[targetAddr++] = disc->Sector[CDiDisc::H_SEC];
		memory[targetAddr++] = disc->Sector[CDiDisc::H_FRAME];
		memory[targetAddr++] = 0x00;
		memory[targetAddr++] = disc->Sector[CDiDisc::H_MIN];
		memory[targetAddr++] = disc->Sector[CDiDisc::H_SEC];
		memory[targetAddr++] = disc->Sector[CDiDisc::H_FRAME];
		memory[targetAddr++] = 0xFF; // CRC
		memory[targetAddr++] = 0xFF; // CRC

		DBUF |= 0x4000; // send DATA to CPU
		XBUF |= 0x8000; // sector filled for processing (causes IRQ)
		_68070->interrupt(SCC68070::IPL_IN4N, true);

	}

	// Update LBA
	if (CdicController.reading && !CdicController.seek) {
		CdicController.curr_lba++;
		disc->read_sector(CdicController.curr_lba);
	} else {
		MiniCDI::Log("[CDIC] Stopped automatically");
		CdicController = {0};
	}
}

void CDIC::update_soundmap_unit()
{
	if (SoundmapUnit.status != 1) return;

	if (SoundmapUnit.sector_interval > 0) {
		SoundmapUnit.sector_interval--;
		return;
	}

	uint8_t coding = memory[(SoundmapUnit.buffer_index ? 0x303200 : 0x302800) + 11];
	if (coding == 0xFF) // coding byte
	{
		MiniCDI::Log("[CDIC:Soundmap] Encountered $FF coding, yield to XA");

		// For whatever reason, ACHAN is not written at this point, which affects subsequent audio tracks that
		// should be read and played back from the disc but because the audio channel filter is null, they are
		// passed as data sectors.
		// Slamy's analyzer: https://github.com/Slamy/CDIC_BlackBoxAnalyzer/blob/main/src/test_audiomap_to_xa_play.c
		SoundmapUnit.status = 2;

		AUDCTL &= ~0x0800; // reset Playback Start bit
		AUDCTL |= 0x0001; // set Playback End bit (causes IRQ?)
		if (AUDCTL & 0x2000) _68070->interrupt(SCC68070::IPL_IN4N, true);
		return;
	}

	if (!SoundmapUnit.played[SoundmapUnit.buffer_index])
	{
		if (adpcm_decode_and_play(SoundmapUnit.buffer_index, true))
		{
			// Select sector interval for ADPCM (per Slamy documentation).
			// CDDA has sample data on every sector so can be ignored.
			SoundmapUnit.sector_interval = 2;
			if (coding & 0b000100) { SoundmapUnit.sector_interval *= 2; } // XA 18.9 kHz
			if (!(coding & 0b010000)) { SoundmapUnit.sector_interval *= 2; } // XA 4bps
			if (!(coding & 0b000001)) { SoundmapUnit.sector_interval *= 2; } // XA Mono
			SoundmapUnit.sector_interval--;

			SoundmapUnit.played[SoundmapUnit.buffer_index] = true;
			SoundmapUnit.buffer_index = !SoundmapUnit.buffer_index;

			// If for any reason this interrupt fails to pass, it will break Hotel Mario with its "dirty disc" error.
			if (AUDCTL & 0x2000) {
				ABUF |= 0x8000; // finished playback of single ADPCM buffer (causes IRQ)
				_68070->interrupt(SCC68070::IPL_IN4N, true);
			}
		}
	}
}

bool CDIC::adpcm_decode_and_play(int buffer, bool soundmap)
{
	if (!(AUDCTL & 0x0800)) return false;

	if (!ADPCM.decode_sector(&memory[buffer ? 0x303200 : 0x302800], soundmap))
		return false;

	#if MINICDI_AUDIO==1 /* SDL2 */
	if (SDL_audio_valid && SDL_GetQueuedAudioSize(SDL_audio_id) < 50000)
		SDL_QueueAudio(SDL_audio_id, &ADPCM.output[0], ADPCM.output_size * sizeof(int16_t));
	#endif

	return true;
}

void CDIC::tick()
{
	disc_process_sector();
	update_soundmap_unit();
}

uint16_t CDIC::read16(uint32_t addr)
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

void CDIC::write16(uint32_t addr, uint16_t value)
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
							case 0x24: MiniCDI::Log("[CDIC] Stop reading? (0x%02X)", CMD);
									   touched_disc = false; // This command seems to be issued only at boot so would be safe to reset this bool?
									   break;
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
						assert(0 && "[CDIC] Command 0x27 is currently unimplemented.");
						break;

					case 0x28:
						MiniCDI::Log("[CDIC] Start CDDA playback (0x%02X)", CMD);
						assert(0 && "[CDIC] Command 0x28 is currently unimplemented.");
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

uint32_t CDIC::read32(uint32_t addr)
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

void CDIC::write32(uint32_t addr, uint32_t value)
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