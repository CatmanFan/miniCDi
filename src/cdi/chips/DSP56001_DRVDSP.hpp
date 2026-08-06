#ifndef MINICDI_DSP56001_DRVDSP
#define MINICDI_DSP56001_DRVDSP

/** [@00409416(kernel)][SLAVE] report Play Button status (0xA1 ?)
[@00428536(cdivolset)][DSP] TX <= 7FFFFF
[@0042853E(cdivolset)][DSP] CVR => 15
[@0042854E(cdivolset)][DSP] set volume (0x9F : 7FFFFF)
[@0043818A(dspdriv)][DSP] TX <= 000005
[@00438190(dspdriv)][DSP] CVR => 1F
[@0043819C(dspdriv)][DSP] set read mode (0x8E : 000005)
[@004381A4(dspdriv)][DSP] CVR => 0E
[@004381B0(dspdriv)][DSP] start sector (0x96 : 000005)
[@00434E4E(dspdriv)][DSP] TX <= 000000
[@00434E54(dspdriv)][DSP] CVR => 16
[@00434E60(dspdriv)][DSP] set read mode (0x8E : 000000)
[@00435ED0(dspdriv)][DSP] TX <= 000000
[@00435ED6(dspdriv)][DSP] CVR => 0E
[@00435EE2(dspdriv)][DSP] set read mode (0x8E : 000000)
[@00436124(dspdriv)][DSP] CVR => 0E
[@00436130(dspdriv)][DSP] select sectors (0x9D : 000000)
[@00436150(dspdriv)][DSP] TX <= 000000
[@0043616E(dspdriv)][DSP] TX <= 000000
[@00436186(dspdriv)][DSP] TX <= 000000
[@0043619C(dspdriv)][DSP] TX <= 000000
[@004361A6(dspdriv)][DSP] CVR => 1D
[@004361B2(dspdriv)][DSP] get unknown status (0x9C : 000000)
[@00400636][SLAVE] reset CPU (0x8A) **/

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

	// ****************************
	// DISC HANDLING
	// ****************************
	CDiDisc *disc;

	struct {
		bool active;
		int delayed_sectors; // Number of sectors to delay for reading (e.g. to simulate discspin)
		int curr_lba; // Taken from TIME register and then incremented
		int dma_mode; // 0 - inactive, 1 - read, 2 - write
		int read_mode;
	} CdStatus;

public:
	DRVDSP(SCC68070* _68070, uint8_t* memory, CDiDisc *disc) : _68070(_68070), memory(memory), disc(disc), CdStatus({0})
	{
	}

	inline void reset()
	{
		ICR = 0;
		CVR = 0;
		ISR = 0x02;
		IVR = 0;
		RTX = 0;
		CdStatus = {0};
	}

	inline void tick()
	{
		if (!CdStatus.active)
			return;

		if (CdStatus.delayed_sectors > 0) {
			CdStatus.delayed_sectors--;
			return;
		}

		// TO-DO
		MiniCDI::Log("[DRVDSP] Disc reading not implemented!! resetting");
		reset();
		_68070->reset();
		/*if (!(disc->Sector.Mode == 2 && (disc->Sector.Submode[1] & 0x04))) // not audio ?
		{
			uint32_t targetAddr = 0x300010;

			memory[targetAddr++] = disc->Sector.Min;
			memory[targetAddr++] = disc->Sector.Sec;
			memory[targetAddr++] = disc->Sector.Frame;
			memory[targetAddr++] = disc->Sector.Mode;
			memory[targetAddr++] = disc->Sector.FileNum[0];
			memory[targetAddr++] = disc->Sector.ChNum[0];
			memory[targetAddr++] = disc->Sector.Submode[0];
			memory[targetAddr++] = disc->Sector.CodingInfo[0];

			memory[targetAddr++] = disc->Sector.FileNum[1];
			memory[targetAddr++] = disc->Sector.ChNum[1];
			memory[targetAddr++] = disc->Sector.Submode[1];
			memory[targetAddr++] = disc->Sector.CodingInfo[1];
			memcpy(&memory[targetAddr], &disc->Sector.Data[0], 2328*sizeof(char));
			targetAddr += 2328;
		}*/

		// Update LBA
		if (CdStatus.active) {
			CdStatus.curr_lba++;
			disc->read_sector(CdStatus.curr_lba);
		} else {
			CdStatus = {0};
		}
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
							MiniCDI::Log("[DSP] unknown command (0x%02X : %06X)", value, RTX);
							break;

						case 0x00:
							MiniCDI::Log("[DSP] run program (0x%02X : %06X)", value, RTX);
							assert(0 && "[DRVDSP] Command is not implemented.");
							return;

						case 0x08:
							MiniCDI::Log("[DSP] submit buffer 4 (0x%02X : %06X)", value, RTX);
							assert(0 && "[DRVDSP] Command is not implemented.");
							return;

						case 0x09:
							MiniCDI::Log("[DSP] submit buffer 5 (0x%02X : %06X)", value, RTX);
							assert(0 && "[DRVDSP] Command is not implemented.");
							return;

						case 0x0E:
							MiniCDI::Log("[DSP] set read mode (0x%02X : %06X)", value, RTX);
							CdStatus.read_mode = RTX;
							break;

						case 0x12:
							MiniCDI::Log("[DSP] read audio status (0x%02X : %06X)", value, RTX);
							assert(0 && "[DRVDSP] Command is not implemented.");
							return;

						case 0x13:
							MiniCDI::Log("[DSP] read status (0x%02X : %06X)", value, RTX);
							// this command writes and reads two pairs of 24-bit values ? (0C0000 0C0000)
							RX = {0x00,0x00,0x00, 0x00,0x00,0x00};
							RTX = (RX[0] << 16) | (RX[1] << 8) | RX[2];

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
							CdStatus.active = true;
							CdStatus.delayed_sectors = 6;
							CdStatus.curr_lba = RTX;
							disc->read_sector(CdStatus.curr_lba);
							break;

						case 0x17:
							MiniCDI::Log("[DSP] start DMA read (0x%02X : %06X)", value, RTX);
							CdStatus.dma_mode = 1;
							break;

						case 0x18:
							MiniCDI::Log("[DSP] start DMA write (0x%02X : %06X)", value, RTX);
							CdStatus.dma_mode = 2;
							break;

						case 0x19:
							MiniCDI::Log("[DSP] stop DMA (0x%02X : %06X)", value, RTX);
							CdStatus.dma_mode = 0;
							break;

						case 0x1A:
							MiniCDI::Log("[DSP] read memory (0x%02X : %06X)", value, RTX);
							assert(0 && "[DRVDSP] Command is not implemented.");
							return;

						case 0x1B:
							MiniCDI::Log("[DSP] write memory (0x%02X : %06X)", value, RTX);
							assert(0 && "[DRVDSP] Command is not implemented.");
							return;

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
					CVR &= 0x1F;
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
				// ISR &= ~0x02; // deassert TX empty (not emulated)
				// update_irq();
				// no need to send anything to the vector.
				MiniCDI::Log("[DSP] TX <= %06X", RTX);
				break;
		}
	}
};

#endif