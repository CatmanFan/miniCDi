#include "cdi/common.hpp"

/********************************************
               BEGIN VDSC CODE
*********************************************/

template <size_t Path>
uint8_t MCD212::VDSC::getCLUTindex(uint8_t* src, bool second)
{
	switch (reg.Icm[Path])
	{
		default:
			assert(0 && "[VDSC] Attempted to retrieve CLUT index but Image Coding Method was not of CLUT type.");
			return 0;

		case CLUT7:
			return Path ? std::clamp(*src & 0x7F, 0, 127) + 128 : *src & 0x7F;

		case CLUT77:
			return !Path && reg.IcmCS ? std::clamp(*src & 0x7F, 0, 127) + 128 : *src & 0x7F;

		case CLUT8:
			return *src;

		case CLUT4:
			const uint8_t data = second ? (*src >> 4) : *src;
			return Path ? std::clamp(data & (reg.FT[Path] == Bitmap ? 0x0F : 0x07), 0, 127) + 128
						: data & (reg.FT[Path] == Bitmap ? 0x0F : 0x07);
	}
}

template <size_t Path>
bool MCD212::VDSC::isTransparent(uint8_t* src, bool second)
{
	// Color key boolean (CLUT only)
	// formula adapted from MAME
	const bool UseColorKey = !(reg.Icm[0] == Off && reg.Icm[1] == RGB555) && (reg.Icm[Path] == CLUT4 || reg.Icm[Path] == CLUT7 || reg.Icm[Path] == CLUT77);
	const bool ColorKey = UseColorKey ? ((reg.TransparentCol[Path] & 0x00FCFCFCu) & (~(reg.MaskCol[Path]) & 0x00FCFCFCu))
									 == ((reg.ColorCLUT[getCLUTindex<Path>(src, second)] & 0x00FCFCFCu) & (~(reg.MaskCol[Path]) & 0x00FCFCFCu))
									  : true;

	switch (reg.TransparencyCtrl[Path])
	{
		default:
			assert(0 && "[VDSC] Transparency Control data is invalid.");
			return false;
		case 0b0000:
			return true;
		case 0b0001:
			return ColorKey;
		case 0b0010:
			return reg.Icm[0] == Off && reg.Icm[1] == RGB555 && Path && (*src & 0x8000);
		case 0b0011:
			return Matte[0];
		case 0b0100:
			return Matte[1];
		case 0b0101:
			return Matte[0] || ColorKey;
		case 0b0110:
			return Matte[1] || ColorKey;
		case 0b1000:
			return false;
		case 0b1001:
			return !ColorKey;
		case 0b1010:
			return !(reg.Icm[0] == Off && reg.Icm[1] == RGB555 && Path && (*src & 0x8000));
		case 0b1011:
			return !Matte[0];
		case 0b1100:
			return !Matte[1];
		case 0b1101:
			return !Matte[0] || !ColorKey;
		case 0b1110:
			return !Matte[1] || !ColorKey;
	}
}

template <size_t Path>
uint32_t MCD212::VDSC::decodeDYUV(uint8_t* src, uint32_t *dst)
{
	uint8_t Y[2], U[2], V[2];

	// Decode DYUV pair of bytes
	U[1] = DYUVDecoder.U + DYUVDecoder.LUT_deq[src[0] >> 4 & 0x0F];
	Y[0] = DYUVDecoder.Y + DYUVDecoder.LUT_deq[src[0] & 0x0F];
	V[1] = DYUVDecoder.V + DYUVDecoder.LUT_deq[src[1] >> 4 & 0x0F];
	Y[1] = Y[0] + DYUVDecoder.LUT_deq[src[1] & 0x0F];

	// Interpolation for U0 and V0
	U[0] = (DYUVDecoder.U + U[1]) >> 1;
	V[0] = (DYUVDecoder.V + V[1]) >> 1;

	DYUVDecoder.Y = Y[1];
	DYUVDecoder.U = U[1];
	DYUVDecoder.V = V[1];

	// Convert to RGB values
	for (size_t i = 0; i < 2; i++)
	{
		const int U_adj = U[i] - 128;
		const int V_adj = V[i] - 128;

		/// Green Book V.4.4.2.2
		int B = std::clamp(((Y[i] << 10) + U_adj * 1774) >> 10, 0, 255);
		int R = std::clamp(((Y[i] << 10) + V_adj * 1404) >> 10, 0, 255);
		int G = std::clamp(((Y[i]*1000) - 299 * R - 114 * B) / 587, 0, 255);

		dst[i] = (R << 24) | (G << 16) | (B << 8) | 0xFF;
	}

	return 2;
}

template <size_t Path>
uint32_t MCD212::VDSC::decodeRGB555(uint8_t* src, uint32_t *dst)
{
	if (!isTransparent<Path>(src))
	{
		uint16_t rgb555 = (*src << 8) | *(src+1);
		uint8_t r = std::clamp(static_cast<int>(static_cast<float>((rgb555 & 0x1F) >> 10) / 32.0f * 255.0f), 0, 255);
		uint8_t g = std::clamp(static_cast<int>(static_cast<float>((rgb555 & 0x1F) >> 5) / 32.0f * 255.0f), 0, 255);
		uint8_t b = std::clamp(static_cast<int>(static_cast<float>(rgb555 & 0x1F) / 32.0f * 255.0f), 0, 255);

		*dst = (r << 24) | (g << 16) | (b << 8) | 0xFF;
	}

	return 1;
}

template <size_t Path>
uint32_t MCD212::VDSC::decodeCLUT(uint8_t* src, uint32_t *dst)
{
	switch (reg.Icm[Path])
	{
		default:
			if (!isTransparent<Path>(src)) *dst = (reg.ColorCLUT[getCLUTindex<Path>(src)] << 8) | 0xFF;
			return 1;

		case CLUT4:
			if (!isTransparent<Path>(src)) dst[0] = (reg.ColorCLUT[getCLUTindex<Path>(src)] << 8) | 0xFF;
			if (!isTransparent<Path>(src, true)) dst[1] = (reg.ColorCLUT[getCLUTindex<Path>(src, true)] << 8) | 0xFF;
			return 2;
	}
}

template <size_t Path>
uint32_t MCD212::VDSC::draw_line_to_plane(uint8_t* memory, uint32_t vsr, int y)
{
	// Reset matte flags and count
	Matte[0] = Matte[1] = false;
	MCR.current = 0;

	// reset DYUV to initial values
	DYUVDecoder.Y = reg.ColorDYUV[Path] >> 16 & 0xFF;
	DYUVDecoder.U = reg.ColorDYUV[Path] >> 8 & 0xFF;
	DYUVDecoder.V = reg.ColorDYUV[Path] & 0xFF;

	if (reg.Icm[Path] == Off)
	{
		if (!this->skip_draw) memset(&FG[Path].decoded[(y * 768)], 0, FG[Path].width * sizeof(uint32_t));
		return vsr;
	}

	for (int x = 0; x < FG[Path].width;)
	{
		matte_set_flag<Path>(FG[0].width < 400 ? x*2 : x);
		uint8_t* src = &memory[vsr];
		uint32_t* dst = &FG[Path].decoded[(y * 768) + x];

		switch (reg.FT[Path])
		{
			default:
			case Bitmap:
				if (reg.Icm[0] == Off && reg.Icm[1] == RGB555)
				{
					x += this->skip_draw ? 1 : decodeRGB555<Path>(src, dst);
					vsr++;
				}
				else
				{
					switch (reg.Icm[Path])
					{
						default:
							assert(0 && "[VDSC] Image Coding Method is invalid.");
							vsr++;
							continue;

						case DYUV:
							x += this->skip_draw ? 2 : decodeDYUV<Path>(src, dst);
							vsr += 2;
							continue;

						case CLUT4:
						case CLUT7:
						case CLUT77:
						case CLUT8:
							x += this->skip_draw ? (reg.Icm[Path] == CLUT4 ? 2 : 1) : decodeCLUT<Path>(src, dst);
							vsr++;
							continue;
					}
				}
				continue;

			case RunLength:
				switch (reg.Icm[Path])
				{
					default:
					case CLUT4: // RL3
					case CLUT7: // RL7
						int length = (*src & 0x80 ? memory[vsr+1] : 1) * (reg.Icm[Path] == CLUT4 ? 2 : 1);
						int endX = length == 0 ? FG[Path].width : std::min({x+length, FG[Path].width});
						while (x < endX) {
							x += this->skip_draw ? (reg.Icm[Path] == CLUT4 ? 2 : 1) : decodeCLUT<Path>(src, &FG[Path].decoded[(y * 768) + x]);
						}
						vsr += (*src & 0x80 ? 2 : 1);
						continue;
				}

			case Mosaic:
				// TO-DO
				// reg.ICF[Path] /= 2;
				assert(0 && "[VDSC] Mosaic video decoding is not implemented.");
				vsr++;
				continue;
		}
	}

	return vsr;
}

void MCD212::VDSC::mix_to_frame(int y)
{
	const int p1 = reg.PlaneOrder ? 0 : 1;
	const int p2 = reg.PlaneOrder ? 1 : 0;

	/// per Green Book:
	/// "C = ICF * (C'-16) + 16
	/// where ICF = Image Contribution Factor (between 0 and 1)
	/// 	C = One of the color components, R G or B.
	/// 	C' = The corresponding component after decoding."
	/// MCD212 datasheet states that the color has to be first combined with the black level of 16 (MUX: BL_A or BL_B).
	#define ICF_APPLY(C, ICF) if (C < 16) C = 16; \
							  C -= 16; \
							  C *= ICF; \
							  C >>= 6; \
							  C += 16;

	// Reset matte ICF and count
	MCR.current = 0;

	// Output screen coords.
	int fb_xy = (FG[0].height == 240 ? y + 20 : y) * 768 + (FG[0].width % 360 == 0 ? 24 : 0);
	for (int x = 0; x < FG[0].width; x++)
	{
		const int pos = (y*768)+x;
		matte_set_icf(FG[0].width < 400 ? x*2 : x);

		uint_fast16_t rA = FG[p1].decoded[pos] >> 24 & 0xFF;
		uint_fast16_t gA = FG[p1].decoded[pos] >> 16 & 0xFF;
		uint_fast16_t bA = FG[p1].decoded[pos] >> 8 & 0xFF;
		uint_fast8_t aA = FG[p1].decoded[pos] & 0xFF;
		uint_fast16_t rB = FG[p2].decoded[pos] >> 24 & 0xFF;
		uint_fast16_t gB = FG[p2].decoded[pos] >> 16 & 0xFF;
		uint_fast16_t bB = FG[p2].decoded[pos] >> 8 & 0xFF;
		uint_fast8_t aB = FG[p2].decoded[pos] & 0xFF;

		ICF_APPLY(rA, reg.ICF[p1]);
		ICF_APPLY(gA, reg.ICF[p1]);
		ICF_APPLY(bA, reg.ICF[p1]);
		ICF_APPLY(rB, reg.ICF[p2]);
		ICF_APPLY(gB, reg.ICF[p2]);
		ICF_APPLY(bB, reg.ICF[p2]);

		if (reg.Icm[0] == Off && reg.Icm[1] == Off)
			framebuffer[fb_xy] = 0x101010ff;
		else if (reg.Mixing && aA && aB)
		{
			int rM = rA + rB - 16;
			int gM = gA + gB - 16;
			int bM = bA + bB - 16;
			if (rM > 255) rM = 255; else if (rM < 0) rM = 0;
			if (gM > 255) gM = 255; else if (gM < 0) gM = 0;
			if (bM > 255) bM = 255; else if (bM < 0) bM = 0;
			framebuffer[fb_xy] = (rM << 24) | (gM << 16) | (bM << 8) | 0xFF;
		}
		else if (aB)
			framebuffer[fb_xy] = (rB << 24) | (gB << 16) | (bB << 8) | 0xFF;
		else if (aA)
			framebuffer[fb_xy] = (rA << 24) | (gA << 16) | (bA << 8) | 0xFF;
		else {
			// Transparent, draw backdrop.
			switch (reg.BackdropColor & 0x07)
			{
				default: framebuffer[fb_xy] = 0x101010ff; break;
				case 0x01: framebuffer[fb_xy] = reg.BackdropColor & 0x08 ? 0x1010FFff : 0x101090ff; break;
				case 0x02: framebuffer[fb_xy] = reg.BackdropColor & 0x08 ? 0x10FF10ff : 0x109010ff; break;
				case 0x03: framebuffer[fb_xy] = reg.BackdropColor & 0x08 ? 0x10FFFFff : 0x109090ff; break;
				case 0x04: framebuffer[fb_xy] = reg.BackdropColor & 0x08 ? 0xFF1010ff : 0x901010ff; break;
				case 0x05: framebuffer[fb_xy] = reg.BackdropColor & 0x08 ? 0xFF10FFff : 0x901090ff; break;
				case 0x06: framebuffer[fb_xy] = reg.BackdropColor & 0x08 ? 0xFFFF10ff : 0x909010ff; break;
				case 0x07: framebuffer[fb_xy] = reg.BackdropColor & 0x08 ? 0xFFFFFFff : 0x909090ff; break;
			}
		}

		if (x >= reg.CursorPosition[0] && x < reg.CursorPosition[0]+16
		 && y >= reg.CursorPosition[1] && y < reg.CursorPosition[1]+16
		 && cursor[(y-reg.CursorPosition[1])*16 + (x-reg.CursorPosition[0])] != 0
		 && reg.CursorEnable)
		{
			switch (reg.CursorColor & 0x07)
			{
				default: framebuffer[fb_xy] = 0x101010ff; break;
				case 0x01: framebuffer[fb_xy] = reg.CursorColor & 0x08 ? 0x1010FFff : 0x101090ff; break;
				case 0x02: framebuffer[fb_xy] = reg.CursorColor & 0x08 ? 0x10FF10ff : 0x109010ff; break;
				case 0x03: framebuffer[fb_xy] = reg.CursorColor & 0x08 ? 0x10FFFFff : 0x109090ff; break;
				case 0x04: framebuffer[fb_xy] = reg.CursorColor & 0x08 ? 0xFF1010ff : 0x901010ff; break;
				case 0x05: framebuffer[fb_xy] = reg.CursorColor & 0x08 ? 0xFF10FFff : 0x901090ff; break;
				case 0x06: framebuffer[fb_xy] = reg.CursorColor & 0x08 ? 0xFFFF10ff : 0x909010ff; break;
				case 0x07: framebuffer[fb_xy] = reg.CursorColor & 0x08 ? 0xFFFFFFff : 0x909090ff; break;
			}
		}

		if (MiniCDI::Config::AnalogColors)
		{
			/// Subtract to get the analog output (per Green Book 4.4.1.2).
			int rM = framebuffer[fb_xy] >> 24 & 0xFF;
			int gM = framebuffer[fb_xy] >> 16 & 0xFF;
			int bM = framebuffer[fb_xy] >> 8 & 0xFF;

			// Actual formula is (C - 16) / 219 but it has been optimized.
			rM -= 16;
			gM -= 16;
			bM -= 16;

			rM *= 255;
			gM *= 255;
			bM *= 255;

			rM /= 219;
			gM /= 219;
			bM /= 219;

			if (rM > 255) rM = 255; else if (rM < 0) rM = 0;
			if (gM > 255) gM = 255; else if (gM < 0) gM = 0;
			if (bM > 255) bM = 255; else if (bM < 0) bM = 0;

			framebuffer[fb_xy] = (rM << 24) | (gM << 16) | (bM << 8) | 0xFF;
		}

		if (FG[0].width < 400) {
			framebuffer[fb_xy+1] = framebuffer[fb_xy];
			fb_xy += 2;
		} else {
			fb_xy++;
		}
	}
}

template <size_t Path>
void MCD212::VDSC::set_register(uint32_t inst)
{
	switch (inst >> 24 & 0xFF)
	{
		default:
			if ((inst >> 24 & 0xFF) >= 0x80u && (inst >> 24 & 0xFF) < 0xC0u) {
				reg.ColorCLUT[(inst >> 24 & 0xFF) - 0x80u + (reg.BankCLUT * 64)] = inst & 0x00FFFFFFu;
				//MiniCDI::Log("[VDSC] P%d color $%06x", Path, reg.ColorCLUT[(inst >> 24 & 0xFF) - 0x80u + (reg.BankCLUT * 64)]);
			}
			break;

		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7E:
		case 0x7F:
			reg.FT[Path] = (inst & 0b0011u) == 0b11 ? Mosaic : (inst & 0b0011u) == 0b10 ? RunLength : Bitmap;
			reg.MF[Path] = (enum MosaicFactor)(inst >> 2 & 0x03u);
			reg.CM[Path] = (enum ColorMode)(inst >> 8 & 0x01u);
			/*MiniCDI::Log("[VDSC] P%d dprm cm=%s,mf=%s,ft=%s", Path, reg.CM[Path] == Double4 ? "p4" : "p8",
															reg.MF[Path] == x16 ? "x16" : reg.MF[Path] == x8 ? "x8"
														  : reg.MF[Path] == x4 ? "x4" : "x2",
															reg.FT[Path] == Mosaic ? "m" : reg.FT[Path] == RunLength ? "rl"
														  : "bmp");*/
			break;

		case 0xC0: // channel 1
			if (!Path)
			{
				reg.IcmCS = inst >> 22 & 0x01u;
				reg.MatteCount = inst >> 19 & 0x01u;
				reg.ExternalVideo = inst >> 18 & 0x01u;
				reg.Icm[1] = (enum Icm)(inst >> 8 & 0x0Fu);
				reg.Icm[0] = (enum Icm)(inst & 0x0Fu);
				/*MiniCDI::Log("[VDSC] P%d icm cma=%s,cmb=%s,nr=%d,ev=%d,cs=%d", Path,
						  reg.Icm[0] == CLUT8 ? "clut8" : reg.Icm[0] == CLUT7 ? "clut7"
						: reg.Icm[0] == CLUT77 ? "clut7+7" : reg.Icm[0] == DYUV ? "dyuv"
						: reg.Icm[0] == CLUT4 ? "clut4" : "off",
						  reg.Icm[1] == RGB555 ? "rgb555" : reg.Icm[1] == DYUV ? "dyuv"
						: reg.Icm[1] == CLUT4 ? "clut4" : "off",
						  reg.MatteCount, reg.ExternalVideo, reg.IcmCS);*/
			}
			break;

		case 0xC1: // channel 1
			if (!Path) 
			{
				reg.TransparencyCtrl[0] = inst & 0x0Fu;
				reg.TransparencyCtrl[1] = inst >> 8 & 0x0Fu;
				reg.Mixing = !(inst >> 23 & 0x01u);
				//MiniCDI::Log("[VDSC] P%d tctl mx=%d,tca=%02d,tcb=%02d", Path, reg.Mixing, reg.TransparencyCtrl[0], reg.TransparencyCtrl[1]);
			}
			break;

		case 0xC2: // channel 1
			if (!Path)
			{
				reg.PlaneOrder = inst & 0x0Fu;
				//MiniCDI::Log("[VDSC] P%d po %s", Path, reg.PlaneOrder ? "a,b" : "b,a");
			}
			break;

		case 0xC3:
			reg.BankCLUT = Path ? (inst & 0x01u) + 2 : inst & 0x0Fu;
			//MiniCDI::Log("[VDSC] P%d cbnk %d", Path, reg.BankCLUT);
			break;

		case 0xC4:
			if (!Path) reg.TransparentCol[0] = inst & 0x00FCFCFCu;
			break;
		case 0xC6:
			if (Path) reg.TransparentCol[1] = inst & 0x00FCFCFCu;
			break;

		case 0xC7:
			if (!Path) reg.MaskCol[0] = inst & 0x00FCFCFCu;
			break;
		case 0xC9:
			if (Path) reg.MaskCol[1] = inst & 0x00FCFCFCu;
			break;

		case 0xCA:
			if (!Path)
			{
				reg.ColorDYUV[0] = inst & 0x00FFFFFFu;
				//MiniCDI::Log("[VDSC] P0 yuv_b y=$%02x,u=$%02x,v=$%02x", inst >> 16 & 0xFFu, inst >> 8 & 0xFFu, inst & 0xFFu);
			}
			break;
		case 0xCB:
			if (Path)
			{
				reg.ColorDYUV[1] = inst & 0x00FFFFFFu;
				//MiniCDI::Log("[VDSC] P1 yuv_b y=$%02x,u=$%02x,v=$%02x", inst >> 16 & 0xFFu, inst >> 8 & 0xFFu, inst & 0xFFu);
			}
			break;

		case 0xCD: // channel 1
			if (!Path)
			{
				reg.CursorPosition[0] = (inst & 0x00000FFFu) / (FG[0].width < 400 ? 2 : 1); // double-resolution
				reg.CursorPosition[1] = inst >> 12 & 0x0FFFu;
				//MiniCDI::Log("[VDSC] P%d cpos x=%d,y=%d", Path, reg.CursorPosition[0], reg.CursorPosition[1]);
			}
			break;

		case 0xCE: // channel 1
			if (!Path)
			{
				reg.CursorColor = inst & 0x0FFu;
				reg.CursorRes = inst >> 15 & 0x01u;
				reg.CursorOffTime = inst >> 16 & 0x07u;
				reg.CursorOnTime = inst >> 19 & 0x07u;
				reg.CursorBlink = inst >> 22 & 0x01u;
				reg.CursorEnable = inst >> 23 & 0x01u;
				/*MiniCDI::Log("[VDSC] P%d cctl en=%d,blkc=%d,con=%d,fon=%d,cuw=%d,y=%d,r=%d,g=%d,b=%d",
								Path, reg.CursorEnable, reg.CursorBlink, reg.CursorOnTime, reg.CursorOffTime, reg.CursorRes,
								reg.CursorColor & 0b1000u ? 1 : 0,
								reg.CursorColor & 0b0100u ? 1 : 0,
								reg.CursorColor & 0b0010u ? 1 : 0,
								reg.CursorColor & 0b0001u ? 1 : 0);*/
			}
			break;

		case 0xCF: // channel 1
			if (!Path)
			{
				reg.CursorPatternX = inst & 0x0000FFFFu;
				reg.CursorPatternY = inst >> 16 & 0x0Fu;
				for (uint8_t x = 0; x < 16; x++) {
					cursor[(reg.CursorPatternY*16)+x] = (reg.CursorPatternX >> (15-x) & 0x01) != 0 ? 0xFF : 0x00;
				}
				//MiniCDI::Log("[VDSC] P%d cpat %d,p=$%04X", Path, reg.CursorPatternY, reg.CursorPatternX);
			}
			break;

		case 0xD0:
		case 0xD1:
		case 0xD2:
		case 0xD3:
		case 0xD4:
		case 0xD5:
		case 0xD6:
		case 0xD7:
			{
				uint8_t mcr = (inst >> 24 & 0xFF) - 0xD0;
				MCR.opcode[mcr] = inst >> 20 & 0x0F;
				MCR.mf[mcr] = reg.MatteCount ? (mcr < 4 ? 0 : 1) : inst >> 16 & 0x01;
				MCR.icf[mcr] = inst >> 10 & 0x3F;
				MCR.x[mcr] = inst & 0x3FF;
				//MiniCDI::Log("[VDSC] P%d rctl %d,op=%X,rf=%d,wf=%d,x=%d", Path, mcr, MCR.opcode[mcr], MCR.mf[mcr], MCR.icf[mcr], MCR.x[mcr]);
			}
			break;

		case 0xD8: // channel 1
			if (!Path) reg.BackdropColor = inst & 0x00FFFFFFu;
			break;

		case 0xD9:
			reg.MosaicPixel[0] = inst & 0xFFu;
			break;
		case 0xDA:
			reg.MosaicPixel[1] = inst & 0xFFu;
			break;

		case 0xDB:
			reg.ICF[0] = inst & 0x3F;
			//MiniCDI::Log("[VDSC] P0 wfac_%s %d", inst & 0x2Fu);
			break;
		case 0xDC:
			reg.ICF[1] = inst & 0x3F;
			//MiniCDI::Log("[VDSC] P1 wfac_%s %d", inst & 0x2Fu);
			break;
	}
}

/********************************************
                END VDSC CODE
*********************************************/

template <size_t Path>
void MCD212::vsr_set(uint32_t value)
{
	VSR[Path] = value & 0x003FFFFFu;
	IC[Path] = 1;
}

template <size_t Path>
void MCD212::dcp_set(uint32_t value)
{
	DCP[Path] = value & 0x003FFFFCu;
	DC[Path] = 1;
}

template <size_t Path>
void MCD212::ICA_execute()
{
	uint32_t addr = SM && !PA ? (Path ? 0x200404 : 0x404) : (Path ? 0x200400 : 0x400);

	for (int i = 0; i < MCD212_HSYNC_CYCLES * MCD212_INACTIVE_VLINES; i++)
	{
		uint32_t inst = (memory[addr] << 24) | (memory[addr+1] << 16) | (memory[addr+2] << 8) | memory[addr+3];
		addr += 4;

		switch (inst >> 24 & 0xFF)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: // STOP
				//MiniCDI::Log("[ICA%d] stop", Path+1);
				return;

			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: // NOP
				break;

			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: // RELOAD DCP
				//MiniCDI::Log("[ICA%d] dcr $%x", Path+1, inst & 0x003FFFFCu);
				dcp_set<Path>(inst);
				break;

			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f: // RELOAD DCP + STOP
				//MiniCDI::Log("[ICA%d] dcr_stop $%x", Path+1, inst & 0x003FFFFCu);
				dcp_set<Path>(inst);
				return;

			case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
			case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: // RELOAD VCR
				//MiniCDI::Log("[ICA%d] vcr $%x", Path+1, inst & 0x003FFFFFu);
				addr = inst & 0x003FFFFFu;
				break;

			case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
			case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // RELOAD VCR + STOP
				//MiniCDI::Log("[ICA%d] vcr_stop $%x", Path+1, inst & 0x003FFFFFu);
				vsr_set<Path>(inst);
				return;

			case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
			case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // INTERRUPT
				IT[Path] = 0b01u;
				if (IT[Path] && !DI[Path])
					_68070->interrupt(SCC68070::IPL_INT1, true);
				break;

			case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: // RELOAD DISPLAY PARAMETERS
				CM[Path] = inst >> 8 & 0b01u;
				MF[Path] = inst >> 2 & 0b11u;
				FT[Path] = inst & 0b11u;
				vdsc.set_register<Path>(inst);
				break;

			default:
				vdsc.set_register<Path>(inst);
				break;
		}
	}
}

template <size_t Path>
void MCD212::DCA_execute()
{
	for (int i = 0; i < (CF ? 16 : 8); i++)
	{
		uint32_t inst = (memory[DCP[Path]] << 24) | (memory[DCP[Path]+1] << 16) | (memory[DCP[Path]+2] << 8) | memory[DCP[Path]+3];
		DCP[Path] += 4;

		switch (inst >> 24 & 0xFF)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
			case 0x08: case 0x09: case 0x0a: case 0x0b: case 0x0c: case 0x0d: case 0x0e: case 0x0f: // STOP
				//MiniCDI::Log("[DCA%d] stop", Path+1);
				return;

			case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
			case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c: case 0x1d: case 0x1e: case 0x1f: // NOP
				break;

			case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27:
			case 0x28: case 0x29: case 0x2a: case 0x2b: case 0x2c: case 0x2d: case 0x2e: case 0x2f: // RELOAD DCP
				break;

			case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x37:
			case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: case 0x3d: case 0x3e: case 0x3f: // RELOAD DCP + STOP
				//MiniCDI::Log("[DCA%d] dcr_stop $%x", Path+1, inst & 0x003FFFFCu);
				dcp_set<Path>(inst);
				return;

			case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
			case 0x48: case 0x49: case 0x4a: case 0x4b: case 0x4c: case 0x4d: case 0x4e: case 0x4f: // RELOAD VCR
				//MiniCDI::Log("[DCA%d] vcr $%x", Path+1, inst & 0x003FFFFFu);
				vsr_set<Path>(inst);
				break;

			case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
			case 0x58: case 0x59: case 0x5a: case 0x5b: case 0x5c: case 0x5d: case 0x5e: case 0x5f: // RELOAD VCR + STOP
				//MiniCDI::Log("[DCA%d] vcr_stop $%x", Path+1, inst & 0x003FFFFFu);
				vsr_set<Path>(inst);
				return;

			case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67:
			case 0x68: case 0x69: case 0x6a: case 0x6b: case 0x6c: case 0x6d: case 0x6e: case 0x6f: // INTERRUPT
				IT[Path] = 0b01u;
				if (IT[Path] && !DI[Path])
					_68070->interrupt(SCC68070::IPL_INT1, true);
				break;

			default:
				vdsc.set_register<Path>(inst);
				break;
		}
	}
}

void MCD212::reset()
{
	// clear write bits
	DI[0] = DD1 = DD2 = TD = DD = ST = BE[0] = 0;
	DI[1] = 0;
	DE = CF = FD = SM = CM[0] = IC[0] = DC[0] = 0;
	CM[1] = IC[1] = DC[1] = 0;
	MF[0] = FT[0] = 0;
	MF[1] = FT[1] = 0;

	// initialization
	CF = 1; // crystal frequency
	FD = MiniCDI::Config::PAL ? 0 : 1; // frame duration
	SM = 1; // interlace (unnecessary)

	interlace = false;
	linesV = 0;
	line = 0;

	vdsc.reset();
}

bool MCD212::tick()
{
	vdsc.skip_draw = this->skip_draw;
	linesV++;

	// Minimise CPU usage for drawing
	/*if (this->skip_draw)
	{
		if (linesV <= MCD212_INACTIVE_VLINES) return false;
		DA = 1;
		line += SM ? 2 : 1;
		if (linesV >= MCD212_VSYNC_LINES)
		{
			DA = 0;
			PA ^= 1;
			linesV = 0;
			line = 0;
			return true;
		}
		return false;
	}

	// Normal behaviour
	else*/
	{
		if (linesV <= MCD212_INACTIVE_VLINES)
		{
			if (linesV == 1 && DE)
			{
				if (IC[0]) ICA_execute<0>();
				if (IC[1]) ICA_execute<1>();
			}
			return false;
		}

		if (line == 0)
		{
			if (interlace && SM) line = 1;
			DA = 1;

			vdsc.set_mode(!CF || ST ? 360 : 384, FD || (!FD && ST) ? 240 : 280, CM[1]);
		}

		if (DE)
		{
			// render line onto bitmap
			VSR[0] = vdsc.draw_line_to_plane<0>(memory, VSR[0], line);
			VSR[1] = vdsc.draw_line_to_plane<1>(memory, VSR[1], line);
			if (!this->skip_draw) vdsc.mix_to_frame(line);

			if (DC[0] && IC[0]) DCA_execute<0>();
			if (DC[1] && IC[1]) DCA_execute<1>();
		}

		line += SM ? 2 : 1;

		if (linesV >= MCD212_VSYNC_LINES)
		{
			DA = 0;
			PA ^= 1;

			linesV = 0;
			line = 0;
			interlace = SM ? !interlace : false;

			return true;
		}

		return false;
	}
}

uint8_t MCD212::read8(uint32_t addr)
{
	switch (addr)
	{
		default:
			return memory[addr];
		case 0x4FFFF0:
		case 0x4FFFF1: // CSR1R
			return (PA << 5) | (DA << 7);
		case 0x4FFFE0:
		case 0x4FFFE1: // CSR2R
			const uint8_t value = BE[0] | (IT[1] << 1) | (IT[0] << 2);
			if (IT[1] || IT[0]) _68070->interrupt(SCC68070::IPL_INT1, false);
			BE[0] = IT[1] = IT[0] = 0;
			return value;
	}
}

uint16_t MCD212::read16(uint32_t addr)
{
	switch (addr)
	{
		default:
			return (memory[addr] << 8) | memory[addr+1];
		case 0x4FFFF0: // CSR1R
			return 0xFF00 | (PA << 5) | (DA << 7);
		case 0x4FFFE0: // CSR2R
			const uint8_t value = BE[0] | (IT[1] << 1) | (IT[0] << 2);
			if (IT[1] || IT[0]) _68070->interrupt(SCC68070::IPL_INT1, false);
			BE[0] = IT[1] = IT[0] = 0;
			return 0xFF00 | value;
	}
}

void MCD212::write16(uint32_t addr, uint16_t value)
{
	switch (addr)
	{
		case 0x4FFFF0: // CSR1W
			BE[1] = value & 0b01u;
			ST = value >> 1 & 0b01u;
			DD = value >> 3 & 0b01u;
			TD = value >> 5 & 0b01u;
			DD2 = value >> 8 & 0b01u;
			DD1 = value >> 9 & 0b01u;
			DI[0] = value >> 15 & 0b01u;
			break;
		case 0x4FFFE0: // CSR2W
			DI[1] = value >> 15 & 0b01u;
			break;
		case 0x4FFFF2: // DCR1
			IC[0] = value >> 8 & 0b01u;
			DC[0] = IC[0] ? value >> 7 & 0b01u : 0;
			CM[0] = value >> 10 & 0b01u;
			SM = value >> 12 & 0b01u;
			FD = value >> 13 & 0b01u;
			CF = value >> 14 & 0b01u;
			DE = value >> 15 & 0b01u;

			VSR[0] &= 0x0000FFFFu;
			VSR[0] |= (value & 0x3Fu) << 8;
			break;
		case 0x4FFFE2: // DCR2
			IC[1] = value >> 8 & 0b01u;
			DC[1] = IC[1] ? value >> 7 & 0b01u : 0;
			CM[1] = value >> 10 & 0b01u;

			VSR[1] &= 0x0000FFFFu;
			VSR[1] |= (value & 0x3Fu) << 8;
			break;
		case 0x4FFFF4: // VSR1
			VSR[0] &= 0xFFFF00000u;
			VSR[0] |= value;
			break;
		case 0x4FFFE4: // VSR2
			VSR[1] &= 0xFFFF00000u;
			VSR[1] |= value;
			break;
		case 0x4FFFF8: // DDR1
			FT[0] = value >> 8 & 0b11u;
			MF[0] = value >> 10 & 0b11u;

			DCP[0] &= 0x0000FFFFu;
			DCP[0] |= (value & 0x3Fu) << 8;
			break;
		case 0x4FFFE8: // DDR2
			FT[1] = value >> 8 & 0b11u;
			MF[1] = value >> 10 & 0b11u;

			DCP[1] &= 0x0000FFFFu;
			DCP[1] |= (value & 0x3Fu) << 8;
			break;
		case 0x4FFFFA: // DCP1
			DCP[0] &= 0xFFFF00000u;
			DCP[0] |= value;
			break;
		case 0x4FFFEA: // DCP2
			DCP[1] &= 0xFFFF00000u;
			DCP[1] |= value;
			break;
	}
}