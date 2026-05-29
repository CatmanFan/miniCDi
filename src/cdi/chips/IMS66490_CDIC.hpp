#ifndef MINICDI_IMS66490_CDIC
#define MINICDI_IMS66490_CDIC

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

public:
	CDIC(CDiDisc *disc, uint8_t* memory) : memory(memory), disc(disc)
	{
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

				case 0x303C00: return CMD;
				case 0x303C02: return TIME >> 16 & 0xFF;
				case 0x303C04: return TIME;
				case 0x303C06: return FILE;
				case 0x303C08: return CHAN >> 16 & 0xFF;
				case 0x303C0A: return CHAN;
				case 0x303C0C: return ACHAN;
				case 0x303C80: return DSEL;
				case 0x303FF4: {
					MiniCDI::Log("[CDIC] ABUF => %04X", ABUF);
					uint16_t value = ABUF;
					if ((ABUF & 0x8000) && (AUDCTL & 0x2000)) {
						MiniCDI::Log("[CDIC] audio IRQ");
						m68k_set_irq(4);
					}
					ABUF &= 0x7FFF;
					return value;
				}
				case 0x303FF6: {
					MiniCDI::Log("[CDIC] XBUF => %04X", XBUF);
					uint16_t value = XBUF;
					if ((XBUF & 0x8000) && (DBUF & 0x4000)) {
						MiniCDI::Log("[CDIC] sector IRQ");
						m68k_set_irq(4);
					}
					XBUF &= 0x7FFF;
					return value;
				}
				case 0x303FF8: return DMACTL;
				case 0x303FFA: return AUDCTL;
				case 0x303FFC: return IVEC;
				case 0x303FFE: return DBUF;
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
					MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x300000, value);
					// DATA[0][addr - 0x300000] = value;
				} else if (addr >= 0x300A00 && addr <= 0x3013FF) {
					MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x300A00, value);
					// DATA[1][addr - 0x300A00] = value;
				} else if (addr >= 0x302800 && addr <= 0x3031FF) {
					MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x302800, value);
					// ADPCM[0][addr - 0x302800] = value;
				} else if (addr >= 0x303200 && addr <= 0x303BFF) {
					MiniCDI::Log("[CDIC] data %02X <= %04X", addr-0x303200, value);
					// ADPCM[1][addr - 0x303200] = value;
				} else {
					memory[addr] = value >> 8 & 0xFF;
					memory[addr+1] = value & 0xFF;
				}
				break;

			case 0x303C00: CMD = value;
			case 0x303C02: MiniCDI::Log("[CDIC] TIME (upper) <= %04X", value); TIME &= 0x00FF; TIME |= (value << 16); break;
			case 0x303C04: MiniCDI::Log("[CDIC] TIME (lower) <= %04X", value); TIME &= 0xFF00; TIME |= value; break;
			case 0x303C06: MiniCDI::Log("[CDIC] FILE <= %04X", value); FILE = value; break;
			case 0x303C08: MiniCDI::Log("[CDIC] CHAN (upper) <= %04X", value); CHAN &= 0x00FF; CHAN |= (value << 16); break;
			case 0x303C0A: MiniCDI::Log("[CDIC] CHAN (lower) <= %04X", value); CHAN &= 0xFF00; CHAN |= value; break;
			case 0x303C0C: MiniCDI::Log("[CDIC] ACHAN <= %04X", value); ACHAN = value; break;
			case 0x303C80: MiniCDI::Log("[CDIC] DSEL <= %04X", value); DSEL = value; break;
			case 0x303FF4: MiniCDI::Log("[CDIC] ABUF <= %04X", value); ABUF = value; break;
			case 0x303FF6: MiniCDI::Log("[CDIC] XBUF <= %04X", value); XBUF = value; break;
			case 0x303FF8: MiniCDI::Log("[CDIC] DMACTL <= %04X", value);
				{
					DMACTL = value;
				}
				break;
			case 0x303FFA: MiniCDI::Log("[CDIC] AUDCTL <= %04X", value); AUDCTL = value; break;
			case 0x303FFC: MiniCDI::Log("[CDIC] IVEC <= %04X", value); memory[addr] = IVEC = value; break;
			case 0x303FFE: MiniCDI::Log("[CDIC] DBUF <= %04X", value); DBUF = value;
				if (DBUF & 0x8000)
				{
					switch (CMD)
					{
						case 0x23:
							MiniCDI::Log("[CDIC] stop disc rotation (0x%02X)", CMD);
							DBUF &= 0x7FFF;
							break;
						case 0x24:
							MiniCDI::Log("[CDIC] stop reading (0x%02X)", CMD);
							disc->read_sector();
							{
								memory[0x300000] = DATA[0][0] = disc->Sector.Min;
								memory[0x300001] = DATA[0][1] = disc->Sector.Sec;
								memory[0x300002] = DATA[0][2] = disc->Sector.Frame;
								memory[0x300003] = DATA[0][3] = disc->Sector.Mode;
								memory[0x300004] = DATA[0][4] = disc->Sector.FileNum;
								memory[0x300005] = DATA[0][5] = disc->Sector.ChNum;
								memory[0x300006] = DATA[0][6] = disc->Sector.Submode;
								memory[0x300007] = DATA[0][7] = disc->Sector.CodingInfo;
								memory[0x300008] = DATA[0][8] = disc->Sector.FileNum;
								memory[0x300009] = DATA[0][9] = disc->Sector.ChNum;
								memory[0x30000A] = DATA[0][10] = disc->Sector.Submode;
								memory[0x30000B] = DATA[0][11] = disc->Sector.CodingInfo;
								for (int i = 0; i < 2328; i++)
								{
									memory[0x30000C+i] = DATA[0][12+i] = disc->Sector.Data[i];
								}
								MiniCDI::Log("[CDIC] read CD-i data (mode1) (0x%02X)", CMD);
							}
							DBUF &= 0x7FFF;
							break;
						case 0x27:
							MiniCDI::Log("[CDIC] fetch TOC (0x%02X)", CMD);
							DBUF &= 0x7FFF;
							break;
						case 0x28:
							MiniCDI::Log("[CDIC] play CDDA (0x%02X)", CMD);
							DBUF &= 0x7FFF;
							break;
						case 0x29:
							disc->get_lba_from_time(TIME);
							disc->read_sector();
							{
								memory[0x300000] = DATA[0][0] = disc->Sector.Min;
								memory[0x300001] = DATA[0][1] = disc->Sector.Sec;
								memory[0x300002] = DATA[0][2] = disc->Sector.Frame;
								memory[0x300003] = DATA[0][3] = disc->Sector.Mode;
								memory[0x300004] = DATA[0][4] = disc->Sector.FileNum;
								memory[0x300005] = DATA[0][5] = disc->Sector.ChNum;
								memory[0x300006] = DATA[0][6] = disc->Sector.Submode;
								memory[0x300007] = DATA[0][7] = disc->Sector.CodingInfo;
								memory[0x300008] = DATA[0][8] = disc->Sector.FileNum;
								memory[0x300009] = DATA[0][9] = disc->Sector.ChNum;
								memory[0x30000A] = DATA[0][10] = disc->Sector.Submode;
								memory[0x30000B] = DATA[0][11] = disc->Sector.CodingInfo;
								for (int i = 0; i < 2328; i++)
								{
									memory[0x30000C+i] = DATA[0][12+i] = disc->Sector.Data[i];
								}
								MiniCDI::Log("[CDIC] read CD-i data (mode1) (0x%02X)", CMD);
							}
							XBUF |= 0x8000; // sector filled
							DBUF &= 0x7FFF;
							break;
						case 0x2A:
							disc->get_lba_from_time(TIME);
							disc->read_sector();
							{
								memory[0x300000] = DATA[0][0] = disc->Sector.Min;
								memory[0x300001] = DATA[0][1] = disc->Sector.Sec;
								memory[0x300002] = DATA[0][2] = disc->Sector.Frame;
								memory[0x300003] = DATA[0][3] = disc->Sector.Mode;
								memory[0x300004] = DATA[0][4] = disc->Sector.FileNum;
								memory[0x300005] = DATA[0][5] = disc->Sector.ChNum;
								memory[0x300006] = DATA[0][6] = disc->Sector.Submode;
								memory[0x300007] = DATA[0][7] = disc->Sector.CodingInfo;
								memory[0x300008] = DATA[0][8] = disc->Sector.FileNum;
								memory[0x300009] = DATA[0][9] = disc->Sector.ChNum;
								memory[0x30000A] = DATA[0][10] = disc->Sector.Submode;
								memory[0x30000B] = DATA[0][11] = disc->Sector.CodingInfo;
								for (int i = 0; i < 2328; i++)
								{
									memory[0x30000C+i] = DATA[0][12+i] = disc->Sector.Data[i];
								}
								MiniCDI::Log("[CDIC] read CD-i data (mode2) (0x%02X)", CMD);
							}
							XBUF |= 0x8000; // sector filled
							DBUF &= 0x7FFF;
							break;
						case 0x2B:
							MiniCDI::Log("[CDIC] stop CDDA ? (0x%02X)", CMD);
							DBUF &= 0x7FFF;
							break;
						case 0x2E:
							MiniCDI::Log("[CDIC] update (0x%02X)", CMD);
							DBUF &= 0x7FFF;
							break;
						case 0x2C:
							MiniCDI::Log("[CDIC] seek ? (0x%02X)", CMD);
							DBUF &= 0x7FFF;
							break;
					}
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

			case 0x303C02: MiniCDI::Log("[CDIC] TIME <= %08X", value); TIME = value; break;
			case 0x303C08: MiniCDI::Log("[CDIC] CHAN <= %08X", value); CHAN = value; break;
		}
	}
};

#endif