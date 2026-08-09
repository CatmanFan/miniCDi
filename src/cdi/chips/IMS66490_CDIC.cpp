#include "cdi/common.hpp"

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

		/*targetAddr += 2332;
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
		memory[targetAddr++] = 0xFF; // CRC*/

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

	#ifdef MINICDI_AUDIO_SDL2
	if (SDL_audio_valid) SDL_QueueAudio(SDL_audio_id, &ADPCM.output[0], ADPCM.output_size * sizeof(int16_t));
	#endif

	return true;
}

void CDIC::tick()
{
	disc_process_sector();
	update_soundmap_unit();
}