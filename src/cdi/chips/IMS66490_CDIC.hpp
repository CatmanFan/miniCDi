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

	/// From MAME CDIC driver
	struct {
		int curr_lba;

		bool reading;
		bool is_mode2;
		bool is_toc;

		int spin_counter;
	} DiscStatus;

	struct {
		bool active;
	} AudioStatus;

	void disc_process_sector()
	{
		/// Synchronous with SS_Play ??

		if (DiscStatus.reading)
		{
			if (DiscStatus.spin_counter > 0) {
				MiniCDI::Log("[CDIC] waiting for disc spin %d", DiscStatus.spin_counter);
				DiscStatus.spin_counter--;
				if (DiscStatus.spin_counter <= 0) XBUF |= 0x8000;
				return;
			}

			disc->read_sector(DiscStatus.curr_lba++);

			// Additional MODE2 processing
			bool selected = true;
			if (DiscStatus.is_mode2 && disc->Sector.Mode == 2)
			{
				if ((disc->Sector.Submode[1] & 0b10000000) /* EOF */
				 || (disc->Sector.Submode[1] & 0b00000001) /* EOR */
				 || (disc->Sector.Submode[1] & 0b00010000) /* Trigger */
				 ) {
					if (disc->Sector.Submode[1] & 0b10000000) {
						DiscStatus.reading = false;
						MiniCDI::Log("[CDIC] MODE2: reached EOF");
					}
					MiniCDI::Log("[CDIC] MODE2 autoread");
					goto copy_sector;
				}

				if ((disc->Sector.FileNum[1] << 8) != FILE) {
					MiniCDI::Log("[CDIC] MODE2 skip: FILE %04X != %02X", FILE, disc->Sector.FileNum[1]);
					selected = false;
					goto copy_sector;
				}
				if (!(CHAN & (1<<disc->Sector.ChNum[1]))) {
					MiniCDI::Log("[CDIC] MODE2 skip: CHAN %08X is not AND (1 << %d)", CHAN, disc->Sector.ChNum[1]);
					selected = false;
					goto copy_sector;
				}

				if (!(disc->Sector.Submode[1] & 0b00001110)) {
					// Either message or empty sector (Green Book II.4.9.1)
					MiniCDI::Log("[CDIC] MODE2 skip: invalid sector");
					selected = false;
					goto copy_sector;
				}
			}

			copy_sector:
			if (selected)
			{
				// Switch DBUF index and reset audio
				DBUF &= 0b01111011; DBUF ^= 0x0001;

				// Decode frame into mainchannel (or ADPCM) data, followed by subchannel data.
				// `use_adpcm` determines whether we should copy to the ADPCM or DATA bufer.

				uint8_t sector_data[0xA00];
				// Copy sector header as normal
				sector_data[0] = disc->Sector.Min;
				sector_data[1] = disc->Sector.Sec;
				sector_data[2] = disc->Sector.Frame;
				sector_data[3] = disc->Sector.Mode;
				sector_data[4] = disc->Sector.FileNum[0];
				sector_data[5] = disc->Sector.ChNum[0];
				sector_data[6] = disc->Sector.Submode[0];
				sector_data[7] = disc->Sector.CodingInfo[0];
				sector_data[8] = disc->Sector.FileNum[1];
				sector_data[9] = disc->Sector.ChNum[1];
				sector_data[10] = disc->Sector.Submode[1];
				sector_data[11] = disc->Sector.CodingInfo[1];
				memcpy(&sector_data[12], &disc->Sector.Data[0], sizeof(disc->Sector.Data));

				// TO-DO: TOC mode - subchannel data ??
				uint8_t subchannel_data[84];
				{
					const uint16_t crc_ccitt_table[256] =
					{
						0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
						0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
						0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
						0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
						0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
						0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
						0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
						0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
						0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
						0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
						0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
						0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
						0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
						0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
						0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
						0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
						0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
						0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
						0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
						0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
						0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
						0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
						0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
						0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
						0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
						0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
						0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
						0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
						0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
						0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
						0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
						0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
					};

					subchannel_data[0] = 0x41;
					subchannel_data[1] = 0x01;
					subchannel_data[2] = 0x01;
					subchannel_data[3] = disc->Sector.Min;
					subchannel_data[4] = disc->Sector.Sec;
					subchannel_data[5] = disc->Sector.Frame;
					subchannel_data[6] = 0x00;
					subchannel_data[7] = disc->Sector.Min;
					subchannel_data[8] = disc->Sector.Sec;
					subchannel_data[9] = disc->Sector.Frame;

					uint16_t crc_accum = 0;
					for (int i = 0; i < 12; i++)
						crc_accum = ((crc_accum << 8) | subchannel_data[i]) ^ crc_ccitt_table[crc_accum >> 8];

					subchannel_data[10] = (crc_accum & 0xFF00) >> 8;
					subchannel_data[11] = crc_accum & 0x00FF;
				}

				memcpy(&sector_data[2340], &subchannel_data[0], sizeof(subchannel_data));

				bool use_adpcm = DiscStatus.is_mode2 && disc->Sector.Mode == 2 && (disc->Sector.Submode[1] & 0x04)
							  && (ACHAN & (1<<disc->Sector.ChNum[1]));

				MiniCDI::Log("[CDIC] %s %02X:%02X:%02X", use_adpcm ? "ADPCM" : "DATA", disc->Sector.Min, disc->Sector.Sec, disc->Sector.Frame);
				if (use_adpcm) {
					// Copy first eight bytes of header first...
					memcpy(&memory[0x300000 + ((DBUF & 0x01)*0xA00)], &sector_data[0], 8*sizeof(char));
					memcpy(&DATA[DBUF & 0x01][0], &sector_data[0], 8*sizeof(char));

					// ...and then switch to ADPCM buffer
					memcpy(&memory[0x302808 + ((DBUF & 0x01)*0xA00)], &sector_data[8], (0xA00-8)*sizeof(char));
					memcpy(&ADPCM[DBUF & 0x01][8], &sector_data[8], (0xA00-8)*sizeof(char));

					DBUF |= 0x0004; // audio index
				} else {
					memcpy(&memory[0x300000 + ((DBUF & 0x01)*0xA00)], &sector_data[0], 0xA00*sizeof(char));
					memcpy(&DATA[DBUF & 0x01][0], &sector_data[0], 0xA00*sizeof(char));

					DBUF |= 0x4000; // send DATA to CPU
					XBUF |= 0x8000; // sector filled for processing
					m68k_set_irq(4);
				}
			}
		}
	}

public:
	CDIC(CDiDisc *disc, uint8_t* memory) : memory(memory), disc(disc), DiscStatus({0}), AudioStatus({0})
	{
	}

	void tick()
	{
		// if (AudioStatus.sectors > 0) AudioStatus.sectors--;
		// if (AudioStatus.decoding) audio_process();

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

			case 0x303C02:
				//MiniCDI::Log("[CDIC] TIME (upper) => %04X", TIME >> 16 & 0xFF);
				return (TIME & 0xFFFF0000) >> 16;

			case 0x303C04:
				//MiniCDI::Log("[CDIC] TIME (lower) => %04X", TIME & 0xFF);
				return TIME & 0x0000FFFF;

			case 0x303C06:
				MiniCDI::Log("[CDIC] FILE => %04X", FILE);
				return FILE;

			case 0x303C08:
				//MiniCDI::Log("[CDIC] CHAN (upper) => %04X", CHAN >> 16 & 0xFF);
				return (CHAN & 0xFFFF0000) >> 16;

			case 0x303C0A:
				//MiniCDI::Log("[CDIC] CHAN (lower) => %04X", CHAN & 0xFF);
				return CHAN & 0x0000FFFF;

			case 0x303C0C:
				//MiniCDI::Log("[CDIC] ACHAN => %04X", ACHAN);
				return ACHAN;

			case 0x303C80:
				MiniCDI::Log("[CDIC] DSEL => %04X", DSEL);
				return DSEL;

			case 0x303FF4: {
				MiniCDI::Log("[CDIC] ABUF => %04X", ABUF);
				uint16_t value = ABUF;
				if (ABUF & 0x8000) {
					ABUF &= 0x7FFF;
					if (AUDCTL & 0x2000)
						m68k_set_irq(4);
				}
				return value;
			}

			case 0x303FF6: {
				//MiniCDI::Log("[CDIC] XBUF => %04X", XBUF);
				uint16_t value = XBUF;
				if (XBUF & 0x8000) {
					XBUF &= 0x7FFF;
					if (DBUF & 0x4000)
						m68k_set_irq(4);
				}
				return value;
			}

			case 0x303FF8: {
				//MiniCDI::Log("[CDIC] DMACTL => %04X", DMACTL);
				return DMACTL;
			}

			case 0x303FFA: {
				AUDCTL ^= 0x0001; // reset ADPCM playback stopped bit
				MiniCDI::Log("[CDIC] AUDCTL => %04X", AUDCTL);
				uint16_t value = AUDCTL;
				if (AUDCTL & 0x0800) {
					AUDCTL &= ~0x0800;
				}
				return value;
			}

			case 0x303FFC:
				//MiniCDI::Log("[CDIC] IVEC => %04X", IVEC);
				return IVEC;

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
			default: return (read16(addr) << 16) | read16(addr+2);
			case 0x303C02: return TIME;
			case 0x303C08: return CHAN;
		}
	}

	void write16(uint32_t addr, uint16_t value, SCC68070* cpu)
	{
		memory[addr] = value;
		switch (addr)
		{
			default:
				if (addr >= 0x300000 && addr <= 0x3009FF) {
					MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x300000, value);
					DATA[0][addr - 0x300000] = value;
				} else if (addr >= 0x300A00 && addr <= 0x3013FF) {
					MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x300A00, value);
					DATA[1][addr - 0x300A00] = value;
				} else if (addr >= 0x302800 && addr <= 0x3031FF) {
					MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x302800, value);
					ADPCM[0][addr - 0x302800] = value;
				} else if (addr >= 0x303200 && addr <= 0x303BFF) {
					MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x303200, value);
					ADPCM[1][addr - 0x303200] = value;
				} else {
					// memory[addr] = value >> 8 & 0xFF;
					// memory[addr+1] = value & 0xFF;
				}
				return;

			case 0x303C00:
				CMD = value;
				return;

			case 0x303C02:
				TIME &= 0x0000FFFF;
				TIME |= (value << 16);
				return;

			case 0x303C04:
				TIME &= 0xFFFF0000;
				TIME |= value;
				return;

			case 0x303C06:
				MiniCDI::Log("[CDIC] FILE <= %04X", value);
				FILE = value;
				return;

			case 0x303C08:
				MiniCDI::Log("[CDIC] CHAN (upper) <= %04X", value);
				CHAN &= 0x0000FFFF;
				CHAN |= (value << 16);
				return;

			case 0x303C0A:
				MiniCDI::Log("[CDIC] CHAN (lower) <= %04X", value);
				CHAN &= 0xFFFF0000;
				CHAN |= value;
				return;

			case 0x303C0C:
				MiniCDI::Log("[CDIC] ACHAN <= %04X", value);
				ACHAN = value;
				return;

			case 0x303C80:
				MiniCDI::Log("[CDIC] DSEL <= %04X", value);
				DSEL = value;
				return;

			case 0x303FF4:
				MiniCDI::Log("[CDIC] ABUF <= %04X", value);
				ABUF = value;
				return;

			case 0x303FF6:
				MiniCDI::Log("[CDIC] XBUF <= %04X", value);
				XBUF = value;
				return;

			case 0x303FF8:
				MiniCDI::Log("[CDIC] DMACTL <= %04X", value);
				DMACTL = value;
				if (value & 0x8000) cpu->dma_call(0, 0x300000 + (value & 0x3FFF));
				return;

			case 0x303FFA: MiniCDI::Log("[CDIC] AUDCTL <= %04X", value); AUDCTL = value;
				if (value & 0x0800) {
					if (!AudioStatus.active) {
						// start playback : TO-DO
						AudioStatus.active = true;
					}
				} else {
					if (AudioStatus.active) {
						AudioStatus.active = false;
					}
				}
				/*if (!(value & 0x2000)) {
					AudioStatus.decode_addr = 0xFFFF;
				} else if (!AudioStatus.decoding) {
					AudioStatus.decode_addr = value & 0x3A00;
					AudioStatus.sectors = 1;
					AudioStatus.decoding = true;
				}*/
				return;

			case 0x303FFC:
				MiniCDI::Log("[CDIC] IVEC <= %04X", value);
				IVEC = value;
				return;

			case 0x303FFE:
				// MiniCDI::Log("[CDIC] DBUF <= %04X", value);
				DBUF = value;
				if (DBUF & 0x8000)
				{
					switch (CMD)
					{
						default:
							assert(0);
							break;

						case 0x23:
							DBUF &= 0x7FFF;
							MiniCDI::Log("[CDIC] stop disc (0x%02X)", CMD);
							DiscStatus.spin_counter = 6;
							DiscStatus.reading = false;
							DiscStatus.curr_lba = disc->get_lba_from_time(TIME);
							DiscStatus.is_mode2 = false;
							DiscStatus.is_toc = false;
							break;

						case 0x24:
							DBUF &= 0x7FFF;
							MiniCDI::Log("[CDIC] reset (MODE2) (0x%02X)", CMD);
							DiscStatus.reading = false;
							DiscStatus.curr_lba = disc->get_lba_from_time(TIME);
							DiscStatus.is_mode2 = true;
							DiscStatus.is_toc = false;
							break;

						case 0x27:
							DBUF &= 0x7FFF;
							MiniCDI::Log("[CDIC] fetch TOC (0x%02X)", CMD);
							DiscStatus.reading = true;
							DiscStatus.curr_lba = 0xFFFF0000;
							DiscStatus.is_mode2 = false;
							DiscStatus.is_toc = true;
							exit(0);
							break;

						case 0x28:
							DBUF &= 0x7FFF;
							MiniCDI::Log("[CDIC] play CDDA (0x%02X)", CMD);
							DiscStatus.reading = true;
							DiscStatus.curr_lba = disc->get_lba_from_time(TIME);
							DiscStatus.is_mode2 = false;
							DiscStatus.is_toc = false;
							exit(0);
							break;

						case 0x29:
						case 0x2A:
							DBUF &= 0x7FFF;
							MiniCDI::Log("[CDIC] start CD-i dataread from $%08X (MODE%d) (0x%02X)", TIME, CMD - 0x28, CMD);
							DiscStatus.spin_counter = 6;
							DiscStatus.reading = true;
							DiscStatus.curr_lba = disc->get_lba_from_time(TIME);
							DiscStatus.is_mode2 = CMD == 0x2A;
							DiscStatus.is_toc = false;
							break;

						case 0x2B:
							DBUF &= 0x7FFF;
							MiniCDI::Log("[CDIC] stop CDDA ? (0x%02X)", CMD);
							break;

						case 0x2C:
							DBUF &= 0x7FFF;
							MiniCDI::Log("[CDIC] seek ? (0x%02X)", CMD);
							DiscStatus.reading = true;
							DiscStatus.curr_lba = disc->get_lba_from_time(TIME);
							DiscStatus.is_mode2 = false;
							DiscStatus.is_toc = false;
							break;

						case 0x2E:
							DBUF &= 0x7FFF;
							MiniCDI::Log("[CDIC] continue / update Mode2 filter (0x%02X)", CMD);
							DiscStatus.reading = true;
							break;
					}
				}

				if (!(DBUF & 0x4000))
				{
					MiniCDI::Log("[CDIC] abort");
					DiscStatus.spin_counter = 0;
					DiscStatus.reading = false;
					DiscStatus.curr_lba = 0;
					DiscStatus.is_mode2 = false;
					DiscStatus.is_toc = false;
				}
				return;
		}
	}

	void write32(uint32_t addr, uint32_t value, SCC68070* cpu)
	{
		switch (addr)
		{
			case 0x303C02: MiniCDI::Log("[CDIC] TIME <= %08X", value); TIME = value; break;
			case 0x303C08: MiniCDI::Log("[CDIC] CHAN <= %08X", value); CHAN = value; break;
		}
	}
};

#endif