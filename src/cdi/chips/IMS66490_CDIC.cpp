#include "cdi/common.hpp"

#define CLEAR_AUDIOCONTROLLER	/*AudioController.sector_interval = 0;*/ \
								AudioController.cpu = false; \
								AudioController.running = false; \
								AudioController.second_buffer = false; \
								AudioController.finish_scheduled = false;

CDIC::CDIC(SCC68070* _68070, uint8_t* memory, CDiDisc *disc) : _68070(_68070), memory(memory), XBUF(0), AudioController({0}), disc(disc), CdicController({0})
{
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
		DBUF &= ~0x0004; // Reset audio (per MAME)
		DBUF ^= 0x0001; // Update buffer index (per MAME). This has to be done BEFORE targetAddr is defined otherwise disc will not init.
		//MiniCDI::Log("[CDIC] Reading sector %02X:%02X:%02X to $%08X", disc->Sector[CDiDisc::H_MIN], disc->Sector[CDiDisc::H_SEC], disc->Sector[CDiDisc::H_FRAME]);

		uint32_t targetAddr = 0x300000 + ((DBUF & 0x01)*0xA00);

		// Fetch mainchannel data from frame, followed by subchannel data.
		// These bytes should be separated if the sector is passed as audio
		memcpy(&memory[targetAddr], &disc->Sector[0], 8);
		targetAddr += 8;

		// At this point the CDIC should switch to the ADPCM buffer if the fetched sector is of audio type.
		// Submode filter (Form=1, Data=0, Audio=1, Video=0) is defined in Figure IV.3 of the Green Book
		bool is_adpcm = disc->Sector[CDiDisc::H_MODE] == 2 && CdicController.is_mode2
					 && (disc->Sector[CDiDisc::SH_SUBMODE2] & 0b00101110) == 0b00100100
					 && (ACHAN & (1 << disc->Sector[CDiDisc::SH_CHAN2]));

		if (is_adpcm)
		{
			DBUF |= 0x0004; // Set audio bit in DBUF to declare reception into ADPCM buffer
			targetAddr += 0x2800; // Jump to ADPCM buffer address
			// ADPCM buffer is automatically decoded and played when AUDCTL is set to enable playback.
		}

		memcpy(&memory[targetAddr], &disc->Sector[8], 2332);

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
	if (AudioController.sector_interval > 0)
		AudioController.sector_interval--;

	if (AudioController.running && AudioController.sector_interval == 0)
	{
		const uint8_t coding = memory[(AudioController.second_buffer ? 0x303200 : 0x302800) + 11];

		if (coding == 0xFF) // coding byte
		{
			MiniCDI::Log("[CDIC:DSP] Encountered $FF coding, reset");
			CLEAR_AUDIOCONTROLLER

			AUDCTL &= ~0x0800; // reset Playback Start bit (cdiemu does not unset this bit?)
			AUDCTL |= 0x0001; // set Playback End bit

			// Trigger ABUF interrupt bit regardless (cdiemu behaviour)
			ABUF |= 0x8000;
			if (AUDCTL & 0x2000) _68070->interrupt(SCC68070::IPL_IN4N, true);
		}

		else if (ADPCM.decode_sector(&memory[AudioController.second_buffer ? 0x303200 : 0x302800]))
		{
			/*if (!AudioController.muted)*/ ADPCM.play();
			AudioController.second_buffer = !AudioController.second_buffer;

			// Update audio controller struct
			if (AudioController.finish_scheduled)
			{
				CLEAR_AUDIOCONTROLLER
			}
			else
			{
				// Select sector interval for ADPCM (per Slamy documentation).
				// CDDA has sample data on every sector so can be ignored.
				AudioController.sector_interval = 2;
				if (coding & 0b000100) { AudioController.sector_interval *= 2; } // XA 18.9 kHz
				if (!(coding & 0b010000)) { AudioController.sector_interval *= 2; } // XA 4bps
				if (!(coding & 0b000001)) { AudioController.sector_interval *= 2; } // XA Mono
			}

			// finished playback of single ADPCM buffer. This automatically triggers an IRQ.
			// If for any reason this interrupt fails to pass, it will break Hotel Mario with its "dirty disc" error.
			ABUF |= 0x8000;
			if (AUDCTL & 0x2000)
			{
				MiniCDI::Log("[CDIC:DSP] Audio sector playback finished (IRQ). Sector interval: %d", AudioController.sector_interval);
				_68070->interrupt(SCC68070::IPL_IN4N, true);
			}
		}
	}
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
			}
			break;

		case 0x303FFA: case 0x303FFB:
			MiniCDI::Log("[CDIC] AUDCTL <= %04X", value);
			AUDCTL = value;
			if (value & 0x0800)
			{
				AudioController.sector_interval = 0;
				AudioController.cpu = value & 0x2000;
				AudioController.second_buffer = false;
				AudioController.running = true;

				MiniCDI::Log("[CDIC:DSP] Starting playback from %s", AudioController.cpu ? "CPU" : "disc");
			}
			else
			{
				if (ACHAN != 0)
				{
					MiniCDI::Log("[CDIC:DSP] Aborting playback");
					CLEAR_AUDIOCONTROLLER
				}
				else
				{
					MiniCDI::Log("[CDIC:DSP] Finishing playback");
					AudioController.finish_scheduled = true;
				}
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
							case 0x24: MiniCDI::Log("[CDIC] End disc reading? (0x%02X)", CMD); break;
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
						if (ACHAN != 0 && (AUDCTL & 0x0800) == 0)
						{
							MiniCDI::Log("[CDIC] Update MODE2 parameters & abort ADPCM (0x%02X)", CMD);
							CLEAR_AUDIOCONTROLLER
						}
						else
						{
							MiniCDI::Log("[CDIC] Update MODE2 parameters (0x%02X)", CMD);
						}
						break;
				}

				// Acknowledge the command
				DBUF &= ~0x8000;
			}

			if (!(value & 0x4000))
			{
				MiniCDI::Log("[CDIC] Disc read aborted by DBUF");

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