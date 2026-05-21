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

public:
	CIAP(uint8_t* memory) : memory(memory)
	{
	}

	uint16_t read16(uint32_t addr)
	{
		switch (addr)
		{
			default:
				if (addr >= 0x300000 && addr <= 0x3008FE) {
					return ADPCM[0][addr - 0x300000];
				} else if (addr >= 0x300900 && addr <= 0x3011FE) {
					return ADPCM[1][addr - 0x300900];
				}
				return (memory[addr] << 8) | memory[addr+1];

			case 0x302584: return IER;
			case 0x302586: { uint16_t n = ISR; ISR = 0; return n; }
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
			case 0x3025C4: return 0xCD02;
			case 0x3025FE: return DLOAD;
		}
	}

	void write16(uint32_t addr, uint16_t value, SCC68070* cpu)
	{
		switch (addr)
		{
			default:
				if (addr >= 0x300000 && addr <= 0x3008FE) {
					ADPCM[0][addr - 0x300000] = value;
				} else if (addr >= 0x300900 && addr <= 0x3011FE) {
					ADPCM[1][addr - 0x300900] = value;
				} else {
					memory[addr] = value;
				}
				break;

			case 0x302584: IER = value; break;
			case 0x302586: ISR = value;
				if ((ISR & 0x01) && (IER & 0x01)) { cpu->interrupt(0); }
				if ((ISR & 0x04) && (IER & 0x04)) { cpu->interrupt(0); }
				if ((ISR & 0x08) && (IER & 0x08)) { cpu->interrupt(0); }
				if ((ISR & 0x0800) && (IER & 0x0800)) { cpu->interrupt(0); }
				break;
			case 0x302588: TACS = value; break;
			case 0x30258A: AACS = value; break;
			case 0x30258C: TCM1 = value; break;
			case 0x30258E: ACM1 = value; break;
			case 0x302590: ACM2 = value; break;
			case 0x302592: FILE = value; break;
			case 0x302594: BMAN = value; break;
			case 0x302596: CCR = value; break;
			case 0x30259A: A_SHDW = value; break;
			case 0x3025A0: AP_Left = value; break;
			case 0x3025A2: AP_Right = value; break;
			case 0x3025A4: AP_Vol = value; break;
			case 0x3025A6: APCR = value; break;
			case 0x3025A8: ACONF = value; break;
			case 0x3025AA: ASTAT = value; break;
			case 0x3025C0: ICR = value; break;
			case 0x3025C2: DMACTL = value; break;
			case 0x3025FE: DLOAD = value; break;
		}
	}
};

#endif