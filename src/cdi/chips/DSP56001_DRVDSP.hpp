#ifndef MINICDI_DSP56001_DRVDSP
#define MINICDI_DSP56001_DRVDSP

/**
	@00434990(dspdriv) DSP <= 0C0000
	@00434990(dspdriv) WR.B 0030000F <= 00 [S] .LSB
	@00434984(dspdriv) WR.B 0030000B <= 0C [S] .HSB
	@0043498A(dspdriv) WR.B 0030000D <= 00 [S] .MSB
	@00434990(dspdriv) DSP <= 0C0000
	@00434990(dspdriv) WR.B 0030000F <= 00 [S] .LSB
	@004346D2(dspdriv) WR.B 00300001 <= 80 [S] .ICR
	@00434736(dspdriv) RD.B 00300003 => 00 [S] .CVR
	@00434744(dspdriv) DSP ==> 000000 000000
	@00434744(dspdriv) WR.B 00300003 <= 93 [S] .CVR
	@0043474A(dspdriv) RD.B 00300005 => 03 [S] .ISR
	@00434754(dspdriv) RD.B 0030000B => 00 [S] .HSB
	@0043475A(dspdriv) RD.B 0030000D => 00 [S] .MSB
	@00434760(dspdriv) DSP => 000000
	@00434760(dspdriv) RD.B 0030000F => 00 [S] .LSB
	@00434764(dspdriv) RD.B 00300005 => 03 [S] .ISR
	@00434770(dspdriv) RD.B 0030000D => 00 [S] .MSB
	@00434776(dspdriv) DSP => 000000
	@00434776(dspdriv) RD.B 0030000F => 00 [S] .LSB
	@00434786(dspdriv) RD.B 00300005 => 02 [S] .ISR
	@00434794(dspdriv) WR.B 0030000B <= 00 [S] .HSB
	@0043479E(dspdriv) WR.B 0030000D <= 13 [S] .MSB
	@004347A2(dspdriv) DSP <= 001301
	@004347A2(dspdriv) WR.B 0030000F <= 01 [S] .LSB
	@004347A6(dspdriv) RD.B 00300003 => 13 [S] .CVR
	@004347B2(dspdriv) WR.B 00300003 <= 95 [S] .CVR
**/

class DRVDSP
{
	SCC68070* _68070;
	uint8_t* memory;

	uint8_t ICR = 0;
	uint8_t CVR = 0;
	uint8_t ISR = 0x02;
	uint8_t IVR = 0;
	uint32_t RTX = 0; // byte receive (RX) when read, byte transmit (TX) when written

	std::deque<uint8_t> RX; // to be read by CPU

	inline void update_irq()
	{
		if (((ISR & 0x01) && (ICR & 0x01))/* || ((ISR & 0x02) && (ICR & 0x02))*/)
		{
			_68070->interrupt(SCC68070::IPL_IN4N, true);
		}
		else
		{
			_68070->interrupt(SCC68070::IPL_IN4N, false);
		}
	}

	CDiDisc *disc;

public:
	DRVDSP(SCC68070* _68070, uint8_t* memory, CDiDisc *disc) : _68070(_68070), memory(memory), disc(disc)
	{
	}

	inline void tick()
	{
		// TO-DO
	}

	inline uint8_t read8(uint32_t addr)
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
				// MiniCDI::Log("[DSP] IVR => %02X", IVR);
				return IVR;
			case 0x30000B:
				return RTX >> 16 & 0xFF;
			case 0x30000D:
				return RTX >> 8 & 0xFF;
			case 0x30000F:
				MiniCDI::Log("[DSP] RX => %06X", RTX);
				if (RX.size() % 3 == 0) {
					uint8_t value = RTX & 0xFF;
					RTX = (RX[0] << 16) | (RX[1] << 8) | RX[2];
					RX.pop_front();
					RX.pop_front();
					RX.pop_front();
					return value;
				}

				ISR &= ~0x01; // RXDF = receiver empty
				update_irq();
				return RTX & 0xFF;
		}
	}

	inline void write8(uint32_t addr, uint8_t value)
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
					// Commands usually write A NUMBER OF argument values to RTX and THEN read result values from the same register. Prefix parameter is optional.
					switch (CVR & 0x1F) {
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
							RX = {0x00,0x00,0x00, 0x00,0x00,0x00};
							RTX = (RX[0] << 16) | (RX[1] << 8) | RX[2];

							CVR &= 0x1F;
							ISR |= 0x01; // RXDF = receiver full
							update_irq();
							break;

						case 0x14:
							MiniCDI::Log("[DSP] audio-related? 1 (0x%02X : %06X)", value, RTX);
							break;
						case 0x15:
							MiniCDI::Log("[DSP] audio-related? 2 (0x%02X : %06X)", value, RTX);
							break;
						case 0x16:
							MiniCDI::Log("[DSP] start sector (0x%02X : %06X)", value, RTX);
							assert(0); // Not implemented
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
							CVR &= 0x1F;
							break;
					}
				}
				break;
			case 0x300005: ISR = value;
				update_irq();
				MiniCDI::Log("[DSP] ISR <= %02X", value);
				break;
			case 0x300007: IVR = value;
				_68070->Ipl.vectors[SCC68070::IPL_IN4N] = value;
				MiniCDI::Log("[DSP] IVR <= %02X", value);
				break;
			case 0x30000B: RTX &= 0x0000FFFF; RTX |= value << 16;
				break;
			case 0x30000D: RTX &= 0x00FF00FF; RTX |= value << 8;
				break;
			case 0x30000F: RTX &= 0x00FFFF00; RTX |= value;
				// ISR &= ~0x02; // deassert TX empty
				// update_irq();
				// no need to send anything to the vector.
				MiniCDI::Log("[DSP] TX <= %06X", RTX);
				break;
		}
	}
};

#endif