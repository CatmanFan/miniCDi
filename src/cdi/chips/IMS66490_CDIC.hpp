#ifndef MINICDI_IMS66490_CDIC
#define MINICDI_IMS66490_CDIC

/*****
  DISCLAIMER:
  Partially sourced from the MAME CDIC driver and documentation by Slamy.
 *****/

class CDIC
{
	uint8_t* memory;
	uint8_t DATA[2][0xA00];
	uint8_t ADPCM[2][0xA00];

	uint16_t CMD;
	uint32_t TIME;
	uint16_t FILE;
	uint32_t CHAN;
	uint16_t ACHAN;
	uint16_t DSEL;
	uint16_t ABUF;
	uint16_t XBUF;
	uint16_t DMACTL;
	uint16_t AUDCTL;
	uint16_t IVEC;
	uint16_t DBUF;

	CDiDisc *disc;

	struct {
		bool decoding;
		uint16_t decode_addr;
		int sectors;
	} AudioStatus;

	void audio_process()
	{
		if (AudioStatus.decode_addr == 0xFFFF) {
			AudioStatus.sectors = 0;
			AudioStatus.decoding = false;
			return;
		}

		// TO-DO
	}

	/// From MAME CDIC driver
	struct {
		uint8_t cmd;
		uint8_t mode; // mode1, mode2, cdda, toc
		uint8_t spinup_counter;
		int curr_lba;
	} DiscStatus;

	void disc_start_read(uint8_t mode)
	{
		DiscStatus.cmd = CMD;
		DiscStatus.mode = mode;
		DiscStatus.spinup_counter = 6;
		DiscStatus.curr_lba = disc->get_lba_from_time(TIME);
	}

	void disc_stop_read()
	{
		DiscStatus.cmd = 0;
		DiscStatus.mode = 0;
		DiscStatus.spinup_counter = 0;
		DiscStatus.curr_lba = 0;
	}

	void disc_process_sector()
	{
		if (DiscStatus.cmd == 0)
			return;

		if (DiscStatus.spinup_counter > 0) {
			DiscStatus.spinup_counter--;
			return;
		}

		disc->read_sector(DiscStatus.curr_lba++);

		// Additional MODE2 processing
		bool selected = true;
		if (disc->Sector.Mode == 2 && DiscStatus.mode == 2)
		{
			if ((FILE >> 8 & 0x00FF) != disc->Sector.FileNum[1]) {
				//MiniCDI::Log("[CDIC] MODE2: sector file num does not match, skipping");
				selected = false;
				goto copy_sector;
			}
			if (disc->Sector.Submode[1] & 0x80) { // EOF
				MiniCDI::Log("[CDIC] MODE2: reached EOF");
				DiscStatus.cmd = 0;
			}
			if (disc->Sector.Submode[1] & (0x80 | 0x10 | 0x01)) { // EOF, TRIGGER, EOR
				//MiniCDI::Log("[CDIC] MODE2: sector automatically satisfied");
				goto copy_sector;
			}
			if (!(disc->Sector.Submode[1] & (0x08 | 0x04 | 0x02))) { // DATA, VIDEO, AUDIO
				//MiniCDI::Log("[CDIC] MODE2: sector is message, skipping");
				selected = false;
				goto copy_sector;
			}
			if (!(CHAN >> disc->Sector.ChNum[1] & 0b01)) {
				//MiniCDI::Log("[CDIC] MODE2: sector channel not satisfied, skipping");
				selected = false;
				goto copy_sector;
			}
		}

		copy_sector:
		if (selected)
		{
			MiniCDI::Log("[CDIC] read sector %02X:%02X:%02X", disc->Sector.Min, disc->Sector.Sec, disc->Sector.Frame);

			// Switch DBUF index and reset audio
			DBUF &= ~0x0004; DBUF ^= 0x0001;
			uint32_t targetAddr = 0x300000 + ((DBUF & 0x01)*0xA00);
			uint8_t* targetBuf = &DATA[DBUF & 0x01][0];

			// Copy sector header as normal

			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Min;
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Sec;
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Frame;
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Mode;
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.FileNum[0];
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.ChNum[0];
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Submode[0];
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.CodingInfo[0];

			// Decode frame into mainchannel (or ADPCM) data, followed by subchannel data.
			// `use_adpcm` determines whether we should copy to the ADPCM or DATA bufer.

			bool use_adpcm = disc->Sector.Mode == 2 && DiscStatus.mode == 2
						  && (disc->Sector.Submode[1] & 0x04) && (ACHAN >> disc->Sector.ChNum[1] & 0b01);

			if (use_adpcm) {
				AudioStatus.decoding = false;

				DBUF |= 0x0004; // audio index
				if (disc->Sector.CodingInfo[1] == 0xFF) { AUDCTL |= 0x0001; }
				targetAddr = 0x302808 + ((DBUF & 0x01)*0xA00); // ADPCM + 8
				targetBuf = &ADPCM[DBUF & 0x01][8];
			}
			if (DiscStatus.mode == 3) { // CDDA
				AudioStatus.decoding = false;
			}

			memory[targetAddr++] = *(targetBuf++) = disc->Sector.FileNum[1];
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.ChNum[1];
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Submode[1];
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.CodingInfo[1];

			memcpy(&memory[targetAddr], &disc->Sector.Data[0], 2328*sizeof(char));
			memcpy(targetBuf, &disc->Sector.Data[0], 2328*sizeof(char));
			targetAddr += 2328;
			targetBuf += 2328;

			// TO-DO: TOC subchannel data ??

			memory[targetAddr++] = *(targetBuf++) = DiscStatus.mode == 3 ? 0x01 : 0x41; // Control
			memory[targetAddr++] = *(targetBuf++) = 0x01; // Track
			memory[targetAddr++] = *(targetBuf++) = 0x01; // Index
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Min;
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Sec;
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Frame;
			memory[targetAddr++] = *(targetBuf++) = 0x00;
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Min;
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Sec;
			memory[targetAddr++] = *(targetBuf++) = disc->Sector.Frame;
			memory[targetAddr++] = *(targetBuf++) = 0xFF; // CRC
			memory[targetAddr++] = *(targetBuf++) = 0xFF; // CRC

			XBUF |= 0x8000; // sector filled for processing
			DBUF |= 0x4000; // send DATA to CPU
			m68k_set_irq(4);
		}

		if (DiscStatus.cmd == 0) {
			disc_stop_read();
		}
	}

public:
	CDIC(CDiDisc *disc, uint8_t* memory) : memory(memory), disc(disc), AudioStatus({0}), DiscStatus({0})
	{
	}

	void tick()
	{
		if (AudioStatus.sectors > 0) AudioStatus.sectors--;
		if (AudioStatus.decoding) audio_process();

		disc_process_sector();
	}

	uint16_t read16(uint32_t addr)
	{
		switch (addr)
		{
			default:
				if (addr >= 0x300000 && addr <= 0x3009FF) {
					return (DATA[0][addr - 0x300000] << 8) | DATA[0][addr - 0x300000 + 1];
				} else if (addr >= 0x300A00 && addr <= 0x3013FF) {
					return (DATA[1][addr - 0x300A00] << 8) | DATA[1][addr - 0x300A00 + 1];
				} else if (addr >= 0x302800 && addr <= 0x3031FF) {
					return (ADPCM[0][addr - 0x302800] << 8) | ADPCM[0][addr - 0x302800 + 1];
				} else if (addr >= 0x303200 && addr <= 0x303BFF) {
					return (ADPCM[1][addr - 0x303200] << 8) | ADPCM[1][addr - 0x303200 + 1];
				}
				return (memory[addr] << 8) | memory[addr+1];

			case 0x303C00: {
				//MiniCDI::Log("[CDIC] CMD => %04X", CMD);
				return CMD;
			}
			case 0x303C02: MiniCDI::Log("[CDIC] TIME (upper) => %04X", TIME >> 16 & 0xFF); return TIME >> 16 & 0xFF;
			case 0x303C04: MiniCDI::Log("[CDIC] TIME (lower) => %04X", TIME & 0xFF); return TIME & 0xFF;
			case 0x303C06: MiniCDI::Log("[CDIC] FILE => %04X", FILE); return FILE;
			case 0x303C08: MiniCDI::Log("[CDIC] CHAN (upper) => %04X", CHAN >> 16 & 0xFF); return CHAN >> 16 & 0xFF;
			case 0x303C0A: MiniCDI::Log("[CDIC] CHAN (lower) => %04X", CHAN & 0xFF); return CHAN & 0xFF;
			case 0x303C0C: MiniCDI::Log("[CDIC] ACHAN => %04X", ACHAN); return ACHAN;
			case 0x303C80: MiniCDI::Log("[CDIC] DSEL => %04X", DSEL); return DSEL;
			case 0x303FF4: {
				MiniCDI::Log("[CDIC] ABUF => %04X", ABUF);
				uint16_t value = ABUF;
				if (ABUF & 0x8000) {
					ABUF &= 0x7FFF;
					if (AUDCTL & 0x2000) { MiniCDI::Log("[CDIC] audio IRQ"); m68k_set_irq(4); }
				}
				return value;
			}
			case 0x303FF6: {
				//MiniCDI::Log("[CDIC] XBUF => %04X", XBUF);
				uint16_t value = XBUF;
				if (XBUF & 0x8000) {
					XBUF &= 0x7FFF;
					if (DBUF & 0x4000) { MiniCDI::Log("[CDIC] sector read IRQ"); m68k_set_irq(4); }
				}
				return value;
			}
			case 0x303FF8: {
				MiniCDI::Log("[CDIC] DMACTL => %04X", DMACTL);
				return DMACTL;
			}
			case 0x303FFA: {
				if (!AudioStatus.decoding) { AUDCTL ^= 0x0001; } // reset ADPCM playback stopped bit
				MiniCDI::Log("[CDIC] AUDCTL => %04X", AUDCTL);
				return AUDCTL;
			}
			case 0x303FFC: MiniCDI::Log("[CDIC] IVEC => %04X", IVEC); return IVEC;
			case 0x303FFE: {
				//MiniCDI::Log("[CDIC] DBUF => %04X", DBUF);
				uint16_t value = DBUF;
				if (DBUF & 0x0080) { // reset subQ CRC error bit
					DBUF &= 0xFF7F;
				}
				return value;
			}
		}
	}

	uint32_t read32(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return (read16(addr) << 16) | read16(addr+2);
				case 0x303C02: return TIME;
				case 0x303C08: return CHAN;
		}
	}

	void write16(uint32_t addr, uint16_t value, SCC68070* cpu)
	{
		switch (addr)
		{
			default:
				if (addr >= 0x300000 && addr <= 0x3009FF) {
					// MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x300000, value);
					// DATA[0][addr - 0x300000] = value;
				} else if (addr >= 0x300A00 && addr <= 0x3013FF) {
					// MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x300A00, value);
					// DATA[1][addr - 0x300A00] = value;
				} else if (addr >= 0x302800 && addr <= 0x3031FF) {
					// MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x302800, value);
					// ADPCM[0][addr - 0x302800] = value;
				} else if (addr >= 0x303200 && addr <= 0x303BFF) {
					// MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x303200, value);
					// ADPCM[1][addr - 0x303200] = value;
				} else {
					memory[addr] = value >> 8 & 0xFF;
					memory[addr+1] = value & 0xFF;
				}
				break;

			case 0x303C00: CMD = value; break;
			case 0x303C02: TIME &= 0x00FF; TIME |= (value << 16); break;
			case 0x303C04: TIME &= 0xFF00; TIME |= value; break;
			case 0x303C06: MiniCDI::Log("[CDIC] FILE <= %04X", value); FILE = value; break;
			case 0x303C08: MiniCDI::Log("[CDIC] CHAN (upper) <= %04X", value); CHAN &= 0x00FF; CHAN |= (value << 16); break;
			case 0x303C0A: MiniCDI::Log("[CDIC] CHAN (lower) <= %04X", value); CHAN &= 0xFF00; CHAN |= value; break;
			case 0x303C0C: MiniCDI::Log("[CDIC] ACHAN <= %04X", value); ACHAN = value; break;
			case 0x303C80: MiniCDI::Log("[CDIC] DSEL <= %04X", value); DSEL = value; break;
			case 0x303FF4: MiniCDI::Log("[CDIC] ABUF <= %04X", value); ABUF = value; break;
			case 0x303FF6: MiniCDI::Log("[CDIC] XBUF <= %04X", value); XBUF = value; break;
			case 0x303FF8: MiniCDI::Log("[CDIC] DMACTL <= %04X", value); DMACTL = value;
				if (value & 0x8000) cpu->dma_call(0, 0x300000 + (value & 0x3FFF));
				break;
			case 0x303FFA: MiniCDI::Log("[CDIC] AUDCTL <= %04X", value); AUDCTL = value;
				if (!(value & 0x2000)) {
					AudioStatus.decode_addr = 0xFFFF;
				} else if (!AudioStatus.decoding) {
					AudioStatus.decode_addr = value & 0x3A00;
					AudioStatus.sectors = 1;
					AudioStatus.decoding = true;
				}
				break;
			case 0x303FFC: MiniCDI::Log("[CDIC] IVEC <= %04X", value); memory[addr] = IVEC = value; break;
			case 0x303FFE: /*MiniCDI::Log("[CDIC] DBUF <= %04X", value);*/ DBUF = value;
				if (DBUF & 0x8000)
				{
					switch (CMD)
					{
						case 0x23:
							MiniCDI::Log("[CDIC] stop disc rotation (0x%02X)", CMD);
							disc_stop_read();
							break;
						case 0x24:
							MiniCDI::Log("[CDIC] stop reading (0x%02X)", CMD);
							disc_stop_read();
							break;
						case 0x27:
							MiniCDI::Log("[CDIC] fetch TOC (0x%02X)", CMD);
							disc_start_read(4);
							break;
						case 0x28:
							MiniCDI::Log("[CDIC] play CDDA (0x%02X)", CMD);
							disc_start_read(3);
							break;
						case 0x29:
							MiniCDI::Log("[CDIC] start CD-i dataread from $%08X (mode1) (0x%02X)", TIME, CMD);
							disc_start_read(1);
							break;
						case 0x2A:
							MiniCDI::Log("[CDIC] start CD-i dataread from $%08X (mode2) (0x%02X)", TIME, CMD);
							disc_start_read(2);
							break;
						case 0x2B:
							MiniCDI::Log("[CDIC] stop CDDA ? (0x%02X)", CMD);
							disc_stop_read();
							break;
						case 0x2C:
							MiniCDI::Log("[CDIC] seek ? (0x%02X)", CMD);
							disc_start_read(1);
							break;
						case 0x2E:
							MiniCDI::Log("[CDIC] update Mode2 filter (0x%02X)", CMD);
							break;
					}
					DBUF &= 0x7FFF;
				}
				break;
		}
	}

	void write32(uint32_t addr, uint32_t value, SCC68070* cpu)
	{
		switch (addr)
		{
			default:
				write16(addr, value >> 16 & 0xFFFF, cpu);
				write16(addr+2, value & 0xFFFF, cpu);
				break;

			case 0x303C02: TIME = value; break;
			case 0x303C08: MiniCDI::Log("[CDIC] CHAN <= %08X", value); CHAN = value; break;
		}
	}
};

#endif