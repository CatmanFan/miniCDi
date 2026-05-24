#ifndef MINICDI_IMS66490_CDIC
#define MINICDI_IMS66490_CDIC

class CDIC
{
	uint8_t* memory;
	uint16_t DATA[2][0xA00];
	uint16_t ADPCM[2][0xA00];

	uint16_t CMD;
	uint16_t TIME;
	uint16_t FILE;
	uint16_t CHAN;
	uint16_t ACHAN;
	uint16_t DSEL;
	uint16_t ABUF;
	uint16_t XBUF;
	uint16_t DMACTL;
	uint16_t AUDCTL;
	uint16_t IVEC;
	uint16_t DBUF;

public:
	CDIC(uint8_t* memory) : memory(memory)
	{
	}

	uint16_t read16(uint32_t addr)
	{
		switch (addr)
		{
			default:
				if (addr >= 0x300000 && addr <= 0x3009FF) {
					return DATA[0][addr - 0x300000];
				} else if (addr >= 0x300A00 && addr <= 0x3013FF) {
					return DATA[1][addr - 0x300A00];
				} else if (addr >= 0x302800 && addr <= 0x3031FF) {
					return ADPCM[0][addr - 0x302800];
				} else if (addr >= 0x303200 && addr <= 0x303BFF) {
					return ADPCM[1][addr - 0x303200];
				}
				return (memory[addr] << 8) | memory[addr+1];

				case 0x303C00: return CMD;
				case 0x303C02: return TIME;
				case 0x303C06: return FILE;
				case 0x303C08: return CHAN;
				case 0x303C0C: return ACHAN;
				case 0x303C80: return DSEL;
				case 0x303FF4: {
					MiniCDI::Log("[CDIC] ABUF => %04X", ABUF);
					uint16_t value = ABUF;
					ABUF &= 0x7FFF;
					return value;
				}
				case 0x303FF6: {
					MiniCDI::Log("[CDIC] XBUF => %04X", XBUF);
					uint16_t value = XBUF;
					XBUF &= 0x7FFF;
					return value;
				}
				case 0x303FF8: return DMACTL;
				case 0x303FFA: return AUDCTL;
				case 0x303FFC: return IVEC;
				case 0x303FFE: return DBUF;
		}
	}

	void write16(uint32_t addr, uint16_t value, SCC68070* cpu)
	{
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
					memory[addr] = (value >> 8) & 0xFF;
					memory[addr+1] = value & 0xFF;
				}
				break;

				case 0x303C00: MiniCDI::Log("[CDIC] CMD <= %04X", value); CMD = value;
				case 0x303C02: MiniCDI::Log("[CDIC] TIME <= %04X", value); TIME = value; break;
				case 0x303C06: MiniCDI::Log("[CDIC] FILE <= %04X", value); FILE = value; break;
				case 0x303C08: MiniCDI::Log("[CDIC] CHAN <= %04X", value); CHAN = value; break;
				case 0x303C0C: MiniCDI::Log("[CDIC] ACHAN <= %04X", value); ACHAN = value; break;
				case 0x303C80: MiniCDI::Log("[CDIC] DSEL <= %04X", value); DSEL = value; break;
				case 0x303FF4: MiniCDI::Log("[CDIC] ABUF <= %04X", value); ABUF = value; break;
				case 0x303FF6: MiniCDI::Log("[CDIC] XBUF <= %04X", value); XBUF = value; break;
				case 0x303FF8: MiniCDI::Log("[CDIC] DMACTL <= %04X", value); DMACTL = value; break;
				case 0x303FFA: MiniCDI::Log("[CDIC] AUDCTL <= %04X", value); AUDCTL = value; break;
				case 0x303FFC: MiniCDI::Log("[CDIC] IVEC <= %04X", value); IVEC = value; break;
				case 0x303FFE: MiniCDI::Log("[CDIC] DBUF <= %04X", value); DBUF = value;
					if (DBUF & 0x8000)
					{
						switch (CMD)
						{
							case 0x23:
								MiniCDI::Log("[CDIC] reset 1");
								break;
							case 0x24:
								MiniCDI::Log("[CDIC] reset 2");
								break;
							case 0x2B:
								MiniCDI::Log("[CDIC] Stop CDDA");
								break;
							case 0x2E:
								MiniCDI::Log("[CDIC] update");
								break;
							case 0x27:
								MiniCDI::Log("[CDIC] fetch CDDA");
								break;
							case 0x28:
								MiniCDI::Log("[CDIC] begin play CDDA");
								break;
							case 0x29:
								MiniCDI::Log("[CDIC] read 1");
								break;
							case 0x2A:
								MiniCDI::Log("[CDIC] read 2");
								break;
							case 0x2C:
								MiniCDI::Log("[CDIC] seek");
								break;
						}
					}
					break;
		}
	}
};

#endif