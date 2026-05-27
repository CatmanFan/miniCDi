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
	uint8_t* memory;
	uint16_t ADPCM[2][0x8FF];
	uint16_t Main[2][2340];
	uint16_t SubQ[2][0x0A];
	uint16_t SubR[2][0x0A];

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

	CDiDisc *disc;

	void go_to_LBN(int lbn, int offset)
	{
		;
	}

public:
	CIAP(CDiDisc *disc, uint8_t* memory) : memory(memory), disc(disc)
	{
	}

	uint16_t read16(uint32_t addr)
	{
		// MiniCDI::Log("[CIAP] read");
		switch (addr)
		{
			default:
				if (addr >= 0x300000 && addr <= 0x3008FE) {
					return ADPCM[0][addr - 0x300000];
				}
				if (addr >= 0x300900 && addr <= 0x3011FE) {
					return ADPCM[1][addr - 0x300900];
				}
				if (addr >= 0x301200 && addr <= 0x301B22) {
					return Main[0][addr - 0x301200];
				}
				if (addr >= 0x301BC2 && addr <= 0x3024E4) {
					return Main[1][addr - 0x301BC2];
				}
				return (memory[addr] << 8) | memory[addr+1];

			case 0x302584: return IER;
			case 0x302586: {
				uint16_t value = ISR;
				ISR = 0;
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
				if (addr >= 0x300000 && addr <= 0x3008FE) {
					ADPCM[0][addr - 0x300000] = value;
				} else if (addr >= 0x300900 && addr <= 0x3011FE) {
					ADPCM[1][addr - 0x300900] = value;
				} else if (addr >= 0x301200 && addr <= 0x301B22) {
					Main[0][addr - 0x301200] = value;
				} else if (addr >= 0x301BC2 && addr <= 0x3024E4) {
					Main[1][addr - 0x301BC2] = value;
				} else {
					memory[addr] = (value >> 8) & 0xFF;
					memory[addr+1] = value & 0xFF;
				}
				break;

			case 0x302584:
				MiniCDI::Log("[CIAP] IER <= QERROR=%d, AUDIO=%d, SUBCODE=%d, DATA=%d",
							 value & 0x800 ? 1 : 0, value & 0x08 ? 1 : 0, value & 0x04 ? 1 : 0, value & 0x01 ? 1 : 0);
				IER = value;
				break;
			case 0x302586: MiniCDI::Log("[CIAP] ISR <= %04X", value); set_isr(value);
				break;
			case 0x302588: MiniCDI::Log("[CIAP] TACS <= %04X", value); TACS = value; break;
			case 0x30258A: MiniCDI::Log("[CIAP] AACS <= %04X", value); AACS = value; break;
			case 0x30258C: MiniCDI::Log("[CIAP] TCM1 <= %04X", value); TCM1 = value; break;
			case 0x30258E: MiniCDI::Log("[CIAP] ACM1 <= %04X", value); ACM1 = value; break;
			case 0x302590: MiniCDI::Log("[CIAP] ACM2 <= %04X", value); ACM2 = value; break;
			case 0x302592: MiniCDI::Log("[CIAP] FILE <= %04X", value); FILE = value; break;
			case 0x302594: MiniCDI::Log("[CIAP] BMAN <= %04X", value); BMAN = value; break;
			case 0x302596: {
					CCR = value;
					switch (CCR)
					{
						default:
							MiniCDI::Log("[CIAP] CCR <= %04X", value);
							break;
						case 0x0100:
							MiniCDI::Log("[CIAP] RESET (0x%04X)", value);
							break;
						case 0x3000:
							MiniCDI::Log("[CIAP] PREPA (0x%04X)", value);
							break;
						case 0x7000:
							MiniCDI::Log("[CIAP] PREPD (0x%04X)", value);
							if (disc && disc->disc.is_open())
							{
								disc->disc.clear();
								disc->disc.seekg(16 * 2352 + 24 + 148);
								for (int i = 0; i < 2340; i++)
								{
									uint16_t var = 0;
									char c;
									disc->disc.get(c); var |= (uint8_t)c << 8;
									disc->disc.get(c); var |= (uint8_t)c;
									memory[0x301200 + i] = Main[0][i] = var;
								}
								MiniCDI::Log("[CIAP] data: %02X %02X %02X %02X", Main[0][0], Main[0][1], Main[0][2], Main[0][3]);
								set_isr(0x01);
							}
							break;
						case 0x0094: // STARTA
						case 0x00C4: // STARTD
							MiniCDI::Log("[CIAP] START read (0x%04X)", value);
							break;
					}
				}
				break;
			case 0x30259A: MiniCDI::Log("[CIAP] A_SHDW <= %04X", value); A_SHDW = value; break;
			case 0x3025A0: MiniCDI::Log("[CIAP] AP_Left <= %04X", value); AP_Left = value; break;
			case 0x3025A2: MiniCDI::Log("[CIAP] AP_Right <= %04X", value); AP_Right = value; break;
			case 0x3025A4: MiniCDI::Log("[CIAP] AP_Vol <= %04X", value); AP_Vol = value; break;
			case 0x3025A6: MiniCDI::Log("[CIAP] APCR <= %04X", value); APCR = value; break;
			case 0x3025A8: ACONF = value; break;
			case 0x3025AA: MiniCDI::Log("[CIAP] ASTAT <= %04X", value); ASTAT = value; break;
			case 0x3025C0: MiniCDI::Log("[CIAP] ICR <= v=%d,l=%d", value >> 3 & 0xFF, value & 0x07); ICR = value; break;
			case 0x3025C2: MiniCDI::Log("[CIAP] DMACTL <= %04X", value); DMACTL = value; break;
			case 0x3025FE: DLOAD = value; break;
		}
	}

	void set_isr(uint16_t value)
	{
		ISR = value;
		if (((ISR & 0x01) && (IER & 0x01))
		|| ((ISR & 0x04) && (IER & 0x04))
		|| ((ISR & 0x08) && (IER & 0x08))
		|| ((ISR & 0x0800) && (IER & 0x0800)))
			m68k_set_irq(ICR & 0x07);
	}
};

#endif