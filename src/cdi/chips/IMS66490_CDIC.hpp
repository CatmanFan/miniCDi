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

	CDiDisc *disc;

	struct {
		bool active; // Whether is actively reading data
		int skip_read; // Number of sectors to skip
		int curr_lba; // Taken from TIME register and then incremented
		bool is_mode2; // Determines whether to apply MODE2 filter
	} CdReader;

	bool disc_check_filter()
	{
		if (disc->Sector.Mode == 2 && CdReader.is_mode2)
		{
			if ((disc->Sector.Submode[1] & 0b10000000) // EOF
			 || (disc->Sector.Submode[1] & 0b00000001) // EOR
			 || (disc->Sector.Submode[1] & 0b00010000) // Trigger
			 ) {
				if (disc->Sector.Submode[1] & 0b10000000) {
					MiniCDI::Log("[CDIC] MODE2: reached EOF");
					CdReader.active = false;
				}
				MiniCDI::Log("[CDIC] MODE2 autoread");
				return true;
			}

			if ((disc->Sector.FileNum[1] << 8) != FILE) {
				MiniCDI::Log("[CDIC] MODE2 skip: FILE %04X != %02X", FILE, disc->Sector.FileNum[1]);
				return false;
			}

			if (!(CHAN & (1<<disc->Sector.ChNum[1]))) {
				MiniCDI::Log("[CDIC] MODE2 skip: CHAN %08X is not AND (1 << %d)", CHAN, disc->Sector.ChNum[1]);
				return false;
			}

			if (!(disc->Sector.Submode[1] & 0b00001110)) {
				// Either message or empty sector (Green Book II.4.9.1)
				MiniCDI::Log("[CDIC] MODE2 skip: invalid sector");
				return false;
			}
		}

		return true;
	}

	void disc_process_sector()
	{
		if (CdReader.active)
		{
			if (CdReader.skip_read > 0) {
				CdReader.skip_read--;
				return;
			}

			// Additional MODE2 processing
			if (disc_check_filter())
			{
				MiniCDI::Log("[CDIC] read sector %02X:%02X:%02X", disc->Sector.Min, disc->Sector.Sec, disc->Sector.Frame);

				// Switch DBUF index and reset audio
				DBUF &= 0b01110001; DBUF ^= 0x0001;
				uint32_t targetAddr = 0x300000 + ((DBUF & 0x01)*0xA00);

				// Copy sector header as normal

				memory[targetAddr++] = disc->Sector.Min;
				memory[targetAddr++] = disc->Sector.Sec;
				memory[targetAddr++] = disc->Sector.Frame;
				memory[targetAddr++] = disc->Sector.Mode;
				memory[targetAddr++] = disc->Sector.FileNum[0];
				memory[targetAddr++] = disc->Sector.ChNum[0];
				memory[targetAddr++] = disc->Sector.Submode[0];
				memory[targetAddr++] = disc->Sector.CodingInfo[0];

				// Decode frame into mainchannel (or ADPCM) data, followed by subchannel data.
				// `use_adpcm` determines whether we should copy to the ADPCM or DATA bufer.

				bool use_adpcm = disc->Sector.Mode == 2 && CdReader.is_mode2
							  && (disc->Sector.Submode[1] & 0x04) && (ACHAN & (1 << disc->Sector.ChNum[1]));

				if (use_adpcm) {
					DBUF |= 0x0004; // audio index
					if (disc->Sector.CodingInfo[1] == 0xFF) { AUDCTL |= 0x0001; }
					targetAddr = 0x302808 + ((DBUF & 0x01)*0xA00); // ADPCM + 8
				}

				memory[targetAddr++] = disc->Sector.FileNum[1];
				memory[targetAddr++] = disc->Sector.ChNum[1];
				memory[targetAddr++] = disc->Sector.Submode[1];
				memory[targetAddr++] = disc->Sector.CodingInfo[1];

				memcpy(&memory[targetAddr], &disc->Sector.Data[0], 2328*sizeof(char));
				targetAddr += 2328;

				// TO-DO: TOC subchannel data ??

				memory[targetAddr++] = 0x41; // Control
				memory[targetAddr++] = 0x01; // Track
				memory[targetAddr++] = 0x01; // Index
				memory[targetAddr++] = disc->Sector.Min;
				memory[targetAddr++] = disc->Sector.Sec;
				memory[targetAddr++] = disc->Sector.Frame;
				memory[targetAddr++] = 0x00;
				memory[targetAddr++] = disc->Sector.Min;
				memory[targetAddr++] = disc->Sector.Sec;
				memory[targetAddr++] = disc->Sector.Frame;
				memory[targetAddr++] = 0xFF; // CRC
				memory[targetAddr++] = 0xFF; // CRC

				XBUF |= 0x8000; // sector filled for processing
				DBUF |= 0x4000; // send DATA to CPU
				_68070->generate_irq(4, true);
			}

			// Continue to next sector
			if (CdReader.active)
				disc->read_sector(CdReader.curr_lba++);
		}
	}

public:
	CDIC(SCC68070* _68070, uint8_t* memory, CDiDisc *disc) : _68070(_68070), memory(memory), XBUF(0), disc(disc), CdReader({0})
	{
	}

	void tick()
	{
		disc_process_sector();
	}

	uint16_t read16(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return (memory[addr] << 8) | memory[addr+1];

			case 0x303C00: case 0x303C01:
				MiniCDI::Log("[CDIC] CMD => %04X", CMD);
				return CMD;

			case 0x303C06: case 0x303C07:
				MiniCDI::Log("[CDIC] FILE => %04X", FILE);
				return FILE;

			case 0x303C0C: case 0x303C0D:
				MiniCDI::Log("[CDIC] ACHAN => %04X", ACHAN);
				return ACHAN;

			case 0x303C80: case 0x303C81:
				MiniCDI::Log("[CDIC] DSEL => %04X", DSEL);
				return DSEL;

			case 0x303FF4: case 0x303FF5:
			{
				uint16_t value = ABUF;
				if (ABUF & 0x8000) {
					ABUF &= 0x7FFF;
					if (AUDCTL & 0x2000) _68070->generate_irq(4, true);
				}
				MiniCDI::Log("[CDIC] ABUF => %04X", value);
				return value;
			}

			case 0x303FF6: case 0x303FF7:
			{
				uint16_t value = XBUF;
				if (XBUF & 0x8000) {
					XBUF &= 0x7FFF;
					if (DBUF & 0x4000) _68070->generate_irq(4, true);
				}
				MiniCDI::Log("[CDIC] XBUF => %04X", value);
				return value;
			}

			case 0x303FF8: case 0x303FF9:
				MiniCDI::Log("[CDIC] DMACTL => %04X", DMACTL);
				return DMACTL;

			case 0x303FFA: case 0x303FFB:
				AUDCTL ^= 0x0001; // reset ADPCM playback stopped bit
				MiniCDI::Log("[CDIC] AUDCTL => %04X", AUDCTL);
				return AUDCTL;

			case 0x303FFC: case 0x303FFD:
				MiniCDI::Log("[CDIC] IVEC => %04X", IVEC);
				return IVEC;

			case 0x303FFE: case 0x303FFF:
				MiniCDI::Log("[CDIC] DBUF => %04X", DBUF);
				return DBUF;
		}
	}

	void write16(uint32_t addr, uint16_t value)
	{
		switch (addr)
		{
			default:
				memory[addr] = value >> 8 & 0xFF;
				memory[addr+1] = value & 0xFF;
				break;

			case 0x303C00: case 0x303C01:
				MiniCDI::Log("[CDIC] CMD <= %04X", value);
				CMD = value;
				break;

			case 0x303C06: case 0x303C07:
				MiniCDI::Log("[CDIC] FILE <= %04X", value);
				FILE = value;
				break;

			case 0x303C0C: case 0x303C0D:
				MiniCDI::Log("[CDIC] ACHAN <= %04X", value);
				ACHAN = value;
				break;

			case 0x303C80: case 0x303C81:
				MiniCDI::Log("[CDIC] DSEL <= %04X", value);
				DSEL = value;
				break;

			case 0x303FF4: case 0x303FF5:
				MiniCDI::Log("[CDIC] ABUF <= %04X", value);
				ABUF = value;
				break;

			case 0x303FF6: case 0x303FF7:
				MiniCDI::Log("[CDIC] XBUF <= %04X", value);
				XBUF = value;
				break;

			case 0x303FF8: case 0x303FF9:
				MiniCDI::Log("[CDIC] DMACTL <= %04X", value);
				DMACTL = value;
				if (value & 0x8000) _68070->dma_call(0, 0x300000 + (value & 0x3FFF));
				break;

			case 0x303FFA: case 0x303FFB:
				MiniCDI::Log("[CDIC] AUDCTL <= %04X", value);
				AUDCTL = value;
				break;

			case 0x303FFC: case 0x303FFD:
				MiniCDI::Log("[CDIC] IVEC <= %04X", value);
				memory[addr] = IVEC = value;
				break;

			case 0x303FFE: case 0x303FFF:
				MiniCDI::Log("[CDIC] DBUF <= %04X", value);
				DBUF = value;
				if (value & 0x8000)
				{
					DBUF &= ~0x8000;
					switch (CMD)
					{
						case 0x23:
							MiniCDI::Log("[CDIC] stop disc, reset MODE1 (0x%02X)", CMD);

							// Set to MODE2 and stop
							CdReader.is_mode2 = false;
							CdReader.active = false;
							CdReader.curr_lba = 0;
							break;

						case 0x24:
							MiniCDI::Log("[CDIC] stop read, reset MODE2 (0x%02X)", CMD);

							// Set to MODE2 and stop
							CdReader.is_mode2 = true;
							CdReader.active = false;
							CdReader.curr_lba = 0;
							break;

						case 0x27:
							MiniCDI::Log("[CDIC] fetch TOC (0x%02X)", CMD);
							break;

						case 0x28:
							MiniCDI::Log("[CDIC] play CDDA (0x%02X)", CMD);
							break;

						case 0x2B:
							MiniCDI::Log("[CDIC] stop CDDA ? (0x%02X)", CMD);
							break;

						case 0x29:
						case 0x2A:
						case 0x2C:
							switch (CMD) {
								case 0x29: MiniCDI::Log("[CDIC] start CD-i dataread from $%08X (MODE1) (0x%02X)", TIME, CMD); break;
								case 0x2A: MiniCDI::Log("[CDIC] start CD-i dataread from $%08X (MODE2) (0x%02X)", TIME, CMD); break;
								case 0x2C: MiniCDI::Log("[CDIC] seek ? (0x%02X)", CMD); break;
							}
							CdReader.is_mode2 = CMD == 0x2A ? true : false;

							// Start reading
							CdReader.active = true;
							CdReader.skip_read = 6;
							CdReader.curr_lba = disc->get_lba_from_time(TIME);
							disc->read_sector(CdReader.curr_lba++);
							break;

						case 0x2E:
							MiniCDI::Log("[CDIC] update MODE2 filter (0x%02X)", CMD);
							break;
					}
				}

				if (!(value & 0x4000))
				{
					MiniCDI::Log("[CDIC] abort");
					CdReader.active = false;
					CdReader.curr_lba = 0;
				}
				break;
		}
	}

	uint32_t read32(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return (read16(addr) << 16) | read16(addr+2);

				case 0x303C02:
					MiniCDI::Log("[CDIC] TIME => %08X", TIME);
					return TIME;

				case 0x303C08:
					MiniCDI::Log("[CDIC] CHAN => %08X", CHAN);
					return CHAN;
		}
	}

	void write32(uint32_t addr, uint32_t value)
	{
		switch (addr)
		{
			default:
				write16(addr, value >> 16 & 0xFFFF);
				write16(addr+2, value & 0xFFFF);
				break;

			case 0x303C02:
				MiniCDI::Log("[CDIC] TIME <= %08X", value);
				TIME = value;
				break;

			case 0x303C08:
				MiniCDI::Log("[CDIC] CHAN <= %08X", value);
				CHAN = value;
				break;
		}
	}
};

#endif