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

	void audio_process()
	{
		// TO-DO
		/*if (AUDCTL & 0x2000) {
			assert_irq();
		}*/
	}

	CDiDisc *disc;

	struct {
		bool reading; // Whether is actively reading data
		bool seek;
		int delayed_sectors; // Number of sectors to delay for reading (e.g. to simulate discspin)
		int curr_lba; // Taken from TIME register and then incremented
		bool is_mode2; // Determines whether to apply MODE2 filter
	} CdicController;

	bool disc_check_filter()
	{
		if (disc->Sector.Mode == 2 && CdicController.is_mode2)
		{
			// Use order from MAME
			if ((disc->Sector.FileNum[1] << 8) != FILE) {
				MiniCDI::Log("[CDIC] MODE2 skip: FILE %04X != %02X", FILE, disc->Sector.FileNum[1]);
				return false;
			}

			if ((disc->Sector.Submode[1] & 0b10000000) // EOF
			 || (disc->Sector.Submode[1] & 0b00000001) // EOR
			 || (disc->Sector.Submode[1] & 0b00010000) // Trigger
			 ) {
				if (disc->Sector.Submode[1] & 0b10000000) {
					MiniCDI::Log("[CDIC] MODE2: reached EOF");
					CdicController.reading = false;
				}
				MiniCDI::Log("[CDIC] MODE2 autoread");
				return true;
			}

			if (!(disc->Sector.Submode[1] & 0b00001110)) {
				// Either message or empty sector (Green Book II.4.9.1)
				MiniCDI::Log("[CDIC] MODE2 skip: invalid sector");
				return false;
			}

			if (!(CHAN & (1<<disc->Sector.ChNum[1]))) {
				MiniCDI::Log("[CDIC] MODE2 skip: CHAN %08X is not AND (1 << %d)", CHAN, disc->Sector.ChNum[1]);
				return false;
			}
		}

		return true;
	}

	void disc_process_sector()
	{
		if (!CdicController.reading)
			return;

		if (CdicController.delayed_sectors > 0) {
			CdicController.delayed_sectors--;
			return;
		}

		// Skip if MODE2 not satisfied
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
			bool use_adpcm = disc->Sector.Mode == 2 && CdicController.is_mode2
						  && (disc->Sector.Submode[1] & 0x04) && (ACHAN & (1 << disc->Sector.ChNum[1]));
			if (use_adpcm) {
				DBUF |= 0x0004; // audio index
				if ((AUDCTL & 0x0800) == 0) AUDCTL |= 0x0800; // start playback
				targetAddr += 0x2800;
			}

			memory[targetAddr++] = disc->Sector.FileNum[1];
			memory[targetAddr++] = disc->Sector.ChNum[1];
			memory[targetAddr++] = disc->Sector.Submode[1];
			memory[targetAddr++] = disc->Sector.CodingInfo[1];
			memcpy(&memory[targetAddr], &disc->Sector.Data[0], 2328*sizeof(char));
			targetAddr += 2328;

			// switch (mode) default:
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
			assert_irq();
		}

		if (CdicController.reading && !CdicController.seek) {
			CdicController.curr_lba++;
			disc->read_sector(CdicController.curr_lba);
		} else {
			CdicController.reading = false;
			CdicController.seek = false;
			CdicController.delayed_sectors = 0;
			CdicController.curr_lba = 0;
			CdicController.is_mode2 = false;
		}
	}

	void assert_irq()
	{
		bool xbuf_raised = XBUF & 0x8000 ? true : false;
		bool abuf_raised = ABUF & 0x8000 ? true : false;
		_68070->interrupt(SCC68070::IPL_IN4N, xbuf_raised || abuf_raised);
	}

public:
	CDIC(SCC68070* _68070, uint8_t* memory, CDiDisc *disc) : _68070(_68070), memory(memory), XBUF(0), disc(disc), CdicController({0})
	{
	}

	void tick()
	{
		disc_process_sector();
		audio_process();
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
				MiniCDI::Log("[CDIC] ABUF => %04X", ABUF);
				const uint16_t value = ABUF;
				if (ABUF & 0x8000) {
					ABUF &= 0x7FFF;
					assert_irq();
				}
				return value;
			}

			case 0x303FF6: case 0x303FF7:
			{
				MiniCDI::Log("[CDIC] XBUF => %04X", XBUF);
				const uint16_t value = XBUF;
				if (XBUF & 0x8000) {
					XBUF &= 0x7FFF;
					assert_irq();
				}
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
				IVEC = value;
				_68070->InterruptManager.vectors[SCC68070::IPL_IN4N] = value & 0x00FF;
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
							MiniCDI::Log("[CDIC] stop disc rotation (0x%02X)", CMD);

							// Set to MODE2 and stop
							CdicController.reading = false;
							CdicController.seek = false;
							CdicController.delayed_sectors = 0;
							CdicController.curr_lba = 0;
							CdicController.is_mode2 = false;
							break;

						case 0x24:
							MiniCDI::Log("[CDIC] stop data read (0x%02X)", CMD);

							// Set to MODE2 and stop
							CdicController.reading = false;
							CdicController.seek = false;
							CdicController.delayed_sectors = 0;
							CdicController.curr_lba = 0;
							CdicController.is_mode2 = true;
							break;

						case 0x27:
							MiniCDI::Log("[CDIC] fetch TOC (0x%02X)", CMD);
							break;

						case 0x28:
							MiniCDI::Log("[CDIC] play CDDA (0x%02X)", CMD);
							break;

						case 0x2B:
							MiniCDI::Log("[CDIC] stop CDDA ? (0x%02X)", CMD);
							CdicController.reading = false;
							break;

						case 0x29:
						case 0x2A:
						case 0x2C:
							switch (CMD) {
								case 0x29: MiniCDI::Log("[CDIC] read $%08X (MODE1) (0x%02X)", TIME, CMD); break;
								case 0x2A: MiniCDI::Log("[CDIC] read $%08X (MODE2) (0x%02X)", TIME, CMD); break;
								case 0x2C: MiniCDI::Log("[CDIC] seek ? (0x%02X)", CMD); break;
							}

							// Start reading
							CdicController.reading = true;
							CdicController.seek = CMD == 0x2C;
							CdicController.delayed_sectors = 6;
							CdicController.curr_lba = disc->get_lba_from_time(TIME);
							CdicController.is_mode2 = CMD == 0x2A ? true : false;
							disc->read_sector(CdicController.curr_lba);
							break;

						case 0x2E:
							MiniCDI::Log("[CDIC] update MODE2 filter (0x%02X)", CMD);
							break;
					}
				}

				if (!(value & 0x4000))
				{
					MiniCDI::Log("[CDIC] abort");
					CdicController.reading = false;
					CdicController.seek = false;
					CdicController.delayed_sectors = 0;
					CdicController.curr_lba = 0;
					CdicController.is_mode2 = false;
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