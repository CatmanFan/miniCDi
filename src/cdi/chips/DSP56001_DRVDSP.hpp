#ifndef MINICDI_DSP56001_DRVDSP
#define MINICDI_DSP56001_DRVDSP

// @004346AA(dspdriv) DSP ICR 88
// @004346D2(dspdriv) DSP ICR 80
// @00434744(dspdriv) DSP CMD 93
// @00434744(dspdriv) DSP STAT 0000 0000
// @004347B2(dspdriv) DSP CMD 95[001301]
// @004347B2(dspdriv) DSP AUDIO2 1301
// @00434830(dspdriv) SLAVE PORT D <= $FA
// @00434848(dspdriv) SLAVE PORT D <= $B0000000
// SLAVE PORT D => $B0000000

class DRVDSP
{
	SCC68070* _68070;
	uint8_t* memory;

	uint8_t ICR;
	uint8_t CVR;
	uint8_t ISR;
	uint8_t IVR;
	uint32_t RTX; // byte receive (RX) when read, byte transmit (TX) when written

	void check_isr()
	{
		if (IVR > 0)
			_68070->interrupt(SCC68070::IPL_IN4N, ((ISR & 0b00000001) && (ICR & 0b00000001)) || ((ISR & 0b00000010) && (ICR & 0b00000010)));
	}

	CDiDisc *disc;

public:
	DRVDSP(SCC68070* _68070, uint8_t* memory, CDiDisc *disc) : _68070(_68070), memory(memory), disc(disc)
	{
	}

	void tick()
	{
		// TO-DO
	}

	uint8_t read8(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return memory[addr];
			case 0x300001:
				MiniCDI::Log("[DSP] ICR => %02X", ICR);
				return ICR;
			case 0x300003:
				MiniCDI::Log("[DSP] CVR => %02X", CVR);
				return CVR;
			case 0x300005:
				// MiniCDI::Log("[DSP] ISR => %02X", ISR);
				return ISR;
			case 0x300007:
				MiniCDI::Log("[DSP] IVR => %02X", IVR);
				return IVR;
			case 0x30000B:
				return RTX >> 16 & 0xFF;
			case 0x30000D:
				return RTX >> 8 & 0xFF;
			case 0x30000F:
				MiniCDI::Log("[DSP] send %06X to host", RTX);
				ISR &= ~0x01; // deassert RX full
				check_isr();
				return RTX & 0xFF;
		}
	}

	void write8(uint32_t addr, uint8_t value)
	{
		switch (addr)
		{
			default:
				memory[addr] = value;
				break;
			case 0x300001: ICR = value;
				MiniCDI::Log("[DSP] ICR <= %02X", value);
				break;
			case 0x300003: CVR = value;
				if (CVR & 0x80) {
					CVR &= 0x1F;
					// Commands usually write A NUMBER OF argument values to RTX and THEN read result values from the same register. Prefix parameter is optional.
					switch (CVR) {
						default:
							break;
						case 0x00:
							MiniCDI::Log("[DSP] run program (0x%02X : %06X)", value, RTX);
							break;
						case 0x08:
							MiniCDI::Log("[DSP] submit buffer 4 (0x%02X : %06X)", value, RTX);
							break;
						case 0x09:
							MiniCDI::Log("[DSP] submit buffer 5 (0x%02X : %06X)", value, RTX);
							break;
						case 0x0E:
							MiniCDI::Log("[DSP] set read mode (0x%02X : %06X)", value, RTX);
							break;
						case 0x12:
							MiniCDI::Log("[DSP] read audio status (0x%02X : %06X)", value, RTX);
							break;
						case 0x13:
							MiniCDI::Log("[DSP] read status (0x%02X : %06X)", value, RTX);
							// this command writes and reads two pairs of 24-bit values ? (0C0000 0C0000)
							break;
						case 0x14:
							MiniCDI::Log("[DSP] audio-related? 1 (0x%02X : %06X)", value, RTX);
							break;
						case 0x15:
							MiniCDI::Log("[DSP] audio-related? 2 (0x%02X : %06X)", value, RTX);
							break;
						case 0x16:
							MiniCDI::Log("[DSP] start sector (0x%02X : %06X)", value, RTX);
							break;
						case 0x17:
							MiniCDI::Log("[DSP] start DMA read (0x%02X : %06X)", value, RTX);
							break;
						case 0x18:
							MiniCDI::Log("[DSP] start DMA write (0x%02X : %06X)", value, RTX);
							break;
						case 0x19:
							MiniCDI::Log("[DSP] stop DMA (0x%02X : %06X)", value, RTX);
							break;
						case 0x1A:
							MiniCDI::Log("[DSP] read memory (0x%02X : %06X)", value, RTX);
							break;
						case 0x1B:
							MiniCDI::Log("[DSP] write memory (0x%02X : %06X)", value, RTX);
							break;
						case 0x1C:
							MiniCDI::Log("[DSP] get unknown status (0x%02X : %06X)", value, RTX);
							break;
						case 0x1D:
							MiniCDI::Log("[DSP] select sectors (0x%02X : %06X)", value, RTX);
							break;
						case 0x1F:
							MiniCDI::Log("[DSP] set volume (0x%02X : %06X)", value, RTX);
							break;
					}
				}
				break;
			case 0x300005: ISR = value;
				check_isr();
				MiniCDI::Log("[DSP] ISR <= %02X", value);
				break;
			case 0x300007: IVR = value;
				_68070->InterruptManager.vectors[SCC68070::IPL_IN4N] = value;
				MiniCDI::Log("[DSP] IVR <= %02X", value);
				break;
			case 0x30000B: RTX &= 0x0000FFFF; RTX |= value << 16;
				break;
			case 0x30000D: RTX &= 0x00FF00FF; RTX |= value << 8;
				break;
			case 0x30000F: RTX &= 0x00FFFF00; RTX |= value;
				ISR &= ~0x02; // deassert TX empty
				check_isr();
				// MiniCDI::Log("[DSP] received %06X from host", value);
				break;
		}
	}
};

#endif