#ifndef MINICDI_MCD221_CIAP
#define MINICDI_MCD221_CIAP

/** CIAP/CD–DRIVE ARCHITECTURE
In conjunction with a suitable microcontroller, MC68HC05
(IKAT), the MCD221 provides the functionality to connect an
MC68xxx host processor to a CD–Drive. The MCD221 de-
codes both main and subchannel CD data and plays both
ADPCM and CDDA audio. External I2S format audio (e.g.,
MPEG1) may be input and mixed with the CIAP audio. CIAP
audio output can be in either I2S or Sony formats. **/

class CIAP
{
	SCC68070* _68070;
	uint8_t* memory;
	// uint8_t ADPCM[2][0x8FF];
	// uint8_t Main[2][2340];
	// uint8_t SubQ[2][10];
	// uint8_t SubR[2][12];
	// uint8_t SubS[2][12];
	// uint8_t SubT[2][12];
	// uint8_t SubU[2][12];
	// uint8_t SubV[2][12];
	// uint8_t SubW[2][12];

	uint16_t IER;
	uint16_t ISR;
	uint16_t TACS;
	uint16_t AACS;
	uint16_t TCM1;
	uint16_t ACM1;
	uint16_t ACM2;
	uint16_t FILE;
	uint16_t BMAN;
	uint16_t CCR;
	uint16_t A_SHDW;
	uint16_t AP_Left;
	uint16_t AP_Right;
	uint16_t AP_Vol;
	uint16_t APCR;
	uint16_t ACONF;
	uint16_t ASTAT;
	uint16_t ICR;
	uint16_t DMACTL;
	uint16_t DLOAD;

	void assert_irq()
	{
		if (((ISR & 0x01) && (IER & 0x01)) || ((ISR & 0x04) && (IER & 0x04))
		 || ((ISR & 0x08) && (IER & 0x08)) || ((ISR & 0x0800) && (IER & 0x0800)))
		{
			MiniCDI::Log("[CIAP] INT %s", (ISR & 0x01) && (IER & 0x01) ? "data"
										: (ISR & 0x04) && (IER & 0x04) ? "subcode"
										: (ISR & 0x08) && (IER & 0x08) ? "audio"
										: (ISR & 0x0800) && (IER & 0x0800) ? "qerror"
										: "unknown");
			_68070->interrupt(SCC68070::IPL_IN4N, true);
		}
	}

	CDiDisc *disc;

	struct {
		bool reading; // Whether is actively reading data
		bool audio; // Controls MODE2 filter(?)
		int delayed_sectors; // Number of sectors to delay for reading (e.g. to simulate discspin)
		int curr_lba; // Taken from TIME register and then incremented
	} CdStatus;

	// struct {
	// } DAC;

	bool disc_check_filter()
	{
		if (disc->Sector.Mode == 2 && CdStatus.audio)
		{
			// Use order from MAME
			if (disc->Sector.FileNum[1] != (FILE & 0xFF)) {
				MiniCDI::Log("[CIAP] MODE2 skip: FILE %02X != %02X", FILE & 0xFF, disc->Sector.FileNum[1]);
				return false;
			}

			if ((disc->Sector.Submode[1] & 0b10000000) // EOF
			 || (disc->Sector.Submode[1] & 0b00000001) // EOR
			 || (disc->Sector.Submode[1] & 0b00010000) // Trigger
			 ) {
				if (disc->Sector.Submode[1] & 0b10000000) {
					MiniCDI::Log("[CIAP] MODE2: reached EOF");
					CdStatus.reading = false;
				}
				MiniCDI::Log("[CIAP] MODE2 autoread");
				return true;
			}

			if (!(disc->Sector.Submode[1] & 0b00001110)) {
				// Either message or empty sector (Green Book II.4.9.1)
				MiniCDI::Log("[CIAP] MODE2 skip: invalid sector");
				return false;
			}

			if (!(TCM1 & (1<<disc->Sector.ChNum[1])) && !(ACM2 & (1<<disc->Sector.ChNum[1]))) {
				MiniCDI::Log("[CIAP] MODE2 skip: channel does not match TCM1 ($%04X) or ACM2 ($%04X)", TCM1, ACM2);
				return false;
			}
		}

		return true;
	}

	void disc_process_sector()
	{
		if (!CdStatus.reading)
			return;

		if (CdStatus.delayed_sectors > 0) {
			CdStatus.delayed_sectors--;
			return;
		}

		// Skip if MODE2 not satisfied
		if (disc_check_filter())
		{
			MiniCDI::Log("[CIAP] read sector %02X:%02X:%02X", disc->Sector.Min, disc->Sector.Sec, disc->Sector.Frame);

			// Select target DATA buffer
			uint32_t targetAddr = 0x300000 + ((BMAN & 0b000100) ? 0x1BC2 : 0x1200);

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
			// TACS contains audio channel.
			/// TO-DO

			memory[targetAddr++] = disc->Sector.FileNum[1];
			memory[targetAddr++] = disc->Sector.ChNum[1];
			memory[targetAddr++] = disc->Sector.Submode[1];
			memory[targetAddr++] = disc->Sector.CodingInfo[1];
			memcpy(&memory[targetAddr], &disc->Sector.Data[0], 2328*sizeof(char));

			// Select DATA buffer bit
			if (BMAN & 0b001100) BMAN ^= 0b001100;
			else BMAN |= 0b000100;
			MiniCDI::Log("[CIAP] read BMAN : %04X", BMAN);

			ISR |= 0b0001; // Mainchannel DATA buffer is full
			assert_irq();
		}

		if (CdStatus.reading) {
			CdStatus.curr_lba++;
			disc->read_sector(CdStatus.curr_lba);
		}
	}

public:
	CIAP(SCC68070* _68070, uint8_t* memory, CDiDisc *disc) : _68070(_68070), memory(memory), disc(disc), CdStatus({0})
	{
	}

	void tick()
	{
		disc_process_sector();
	}

	void disc_set_lba(uint8_t min, uint8_t sec, uint8_t frame)
	{
		MiniCDI::Log("[CIAP] load LBA <= %02X:%02X:%02X", min, sec, frame);

		CdStatus.curr_lba = disc->get_lba_from_time((min << 24) | (sec << 16) | (frame << 8));
		disc->read_sector(CdStatus.curr_lba);
	}

	uint16_t read16(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return (memory[addr] << 8) | memory[addr+1];

			case 0x302584: return IER;
			case 0x302586: {
				const uint16_t value = ISR;
				ISR = 0;
				if (_68070 != nullptr) _68070->interrupt(SCC68070::IPL_IN4N, false);
				return value;
			}
			case 0x302588: return TACS;
			case 0x30258A: return AACS;
			case 0x30258C: return TCM1;
			case 0x30258E: return ACM1;
			case 0x302590: return ACM2;
			case 0x302592: return FILE;
			case 0x302594: return BMAN;
			case 0x302596: return CCR;
			case 0x30259A: return A_SHDW;
			case 0x3025A0: return AP_Left;
			case 0x3025A2: return AP_Right;
			case 0x3025A4: return AP_Vol;
			case 0x3025A6: return APCR;
			case 0x3025A8: return ACONF;
			case 0x3025AA: return ASTAT;
			case 0x3025C0: return ICR;
			case 0x3025C2: return DMACTL;
			case 0x3025C4: return 0xCD02; // ID*
			case 0x3025FE: return DLOAD;
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

			case 0x302584:
				MiniCDI::Log("[CIAP] IER <= QERROR=%d, AUDIO=%d, SUBCODE=%d, DATA=%d",
							 value & 0x800 ? 1 : 0, value & 0x08 ? 1 : 0, value & 0x04 ? 1 : 0, value & 0x01 ? 1 : 0);
				IER = value;
				assert_irq();
				break;
			case 0x302586:
				MiniCDI::Log("[CIAP] ISR <= QERROR=%d, AUDIO=%d, SUBCODE=%d, DATA=%d",
							 value & 0x800 ? 1 : 0, value & 0x08 ? 1 : 0, value & 0x04 ? 1 : 0, value & 0x01 ? 1 : 0);
				ISR = value;
				assert_irq();
				break;
			case 0x302588: MiniCDI::Log("[CIAP] TACS <= %04X", value); TACS = value; break;
			case 0x30258A: MiniCDI::Log("[CIAP] AACS <= %04X", value); AACS = value; break;
			case 0x30258C: MiniCDI::Log("[CIAP] TCM1 <= %04X", value); TCM1 = value; break;
			case 0x30258E: MiniCDI::Log("[CIAP] ACM1 <= %04X", value); ACM1 = value; break;
			case 0x302590: MiniCDI::Log("[CIAP] ACM2 <= %04X", value); ACM2 = value; break;
			case 0x302592: MiniCDI::Log("[CIAP] FILE <= %04X", value); FILE = value; break;
			case 0x302594: MiniCDI::Log("[CIAP] BMAN <= %04X", value); BMAN ^= value; break;
			case 0x302596: {
					CCR = value;
					switch (CCR)
					{
						default:
							MiniCDI::Log("[CIAP] CCR <= %04X", value);
							break;
						case 0x0100:
							MiniCDI::Log("[CIAP] RESET (0x%04X)", value);

							// Stop reading
							CdStatus.reading = false;
							CdStatus.audio = false;
							CdStatus.delayed_sectors = 6;
							break;
						case 0x3000:
							MiniCDI::Log("[CIAP] PREPA (0x%04X)", value);
							break;
						case 0x7000:
							MiniCDI::Log("[CIAP] PREPD (0x%04X)", value);
							break;
						case 0x0094: // STARTA
						case 0x00C4: // STARTD
							MiniCDI::Log("[CIAP] START read (0x%04X)", value);

							// Start reading
							CdStatus.reading = true;
							CdStatus.audio = CCR == 0x0094;
							CdStatus.delayed_sectors = 6;
							break;
					}
				}
				break;
			case 0x30259A: MiniCDI::Log("[CIAP] A_SHDW <= %04X", value); A_SHDW = value; break;
			case 0x3025A0: MiniCDI::Log("[CIAP] AP_Left <= %04X", value); AP_Left = value; break;
			case 0x3025A2: MiniCDI::Log("[CIAP] AP_Right <= %04X", value); AP_Right = value; break;
			case 0x3025A4: MiniCDI::Log("[CIAP] AP_Vol <= %04X", value); AP_Vol = value; break;
			case 0x3025A6: {
					APCR = value;
					switch (value)
					{
						default:
							MiniCDI::Log("[CIAP] APCR <= %04X", value);
							break;
						case 0x0020:
							MiniCDI::Log("[CIAP] audio interrupt - wait (%04X)", value);
							ISR |= 0x0008;
							assert_irq();
							break;
						case 0x00A0:
							MiniCDI::Log("[CIAP] audio interrupt - now (%04X)", value);
							ISR |= 0x0008;
							assert_irq();
							break;
						case 0x0140:
							MiniCDI::Log("[CIAP] audio playback ADPCM0 (%04X)", value);
							break;
					}
				}
				break;
			case 0x3025A8: MiniCDI::Log("[CIAP] ACONF <= %04X", value); ACONF = value; break;
			case 0x3025AA: MiniCDI::Log("[CIAP] ASTAT <= %04X", value); ASTAT = value; break;
			case 0x3025C0: MiniCDI::Log("[CIAP] ICR <= v=%d,l=%d", value >> 3 & 0xFF, value & 0x07); ICR = value;
				if (_68070 != nullptr) _68070->Ipl.vectors[SCC68070::IPL_IN4N] = value >> 3 & 0xFF;
				break;
			case 0x3025C2: MiniCDI::Log("[CIAP] DMACTL <= %04X", value); DMACTL = value;
				if (value & 0x4000) _68070->dma_call(0, 0x300000 + (value & 0x1FFF));
				break;
			case 0x3025FE: DLOAD = value; break;
		}
	}
};

#endif