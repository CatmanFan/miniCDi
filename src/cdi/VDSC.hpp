#ifndef MINICDI_VDSC
#define MINICDI_VDSC

/*****
  DISCLAIMER:
  Sourced partially from official documentation of MCD212 by Motorola and
  MAME CD-i driver by Ryan Holtz and Vincent Halver (licensed under BSD-3-Clause).
 *****/

namespace VDSC
{

enum Icm
{
	// All coding methods are usable in standard-res except for CLUT4.
	Off = 0x00,
	CLUT8 = 0x01, // plane A only
	CLUT7 = 0x03,
	CLUT77 = 0x04, // plane A only
	DYUV = 0x05,
	CLUT4 = 0x0B, // double-res only
	RGB555 = 0x01, // plane B only
};

enum Tcr
{
	TcrAlways = 0,
	TcrIfCK = 1,
	TcrIfTB,
	TcrIfMF0,
	TcrIfMF1,
	TcrIfCK_MF0,
	TcrIfCK_MF1,
	TcrUnusedA,
	TcrNever,
	TcrIfNotCK,
	TcrIfNotTB,
	TcrIfNotMF0,
	TcrIfNotMF1,
	TcrIfNotCK_MF0,
	TcrIfNotCK_MF1,
	TcrUnusedB
};

enum Type
{
	NTSCMonitor,	// 525 (360x240)
	NTSCTV,			// 525 (384x240)
	PAL				// 625 (384x280)
};

enum Resolution
{
	NormalRes,		// 1:1
	DoubleRes,		// 2:1 (horizontal doubled)
	HighRes			// 2:2 (both doubled)
};

enum MosaicFactor
{
	x2 = 0b00,
	x4 = 0b01,
	x8 = 0b10,
	x16 = 0b11
};

enum FileType
{
	Bitmap = 0b00,
	RunLength = 0b10,
	Mosaic = 0b11
};

enum ColorMode
{
	Normal8 = 0b00,
	Double4 = 0b01,
	High8 = 0b10,
	Reserved = 0b11
};

class Plane
{
public:
	int width, height;
	std::vector<uint32_t> decoded;
};

class Decoder
{
	std::vector<uint32_t> output;

	struct
	{
		/* 80 */ uint32_t ColorCLUT[256];
		/* C0 */ enum Icm Icm[2];
				 uint8_t IcmCS, MatteCount, ExternalVideo; // Dual CLUT bank select (0-1 or 2-3); enable external video
		/* C1 */ enum Tcr Transparency[2]; uint8_t Mixing;
		/* C2 */ uint8_t PlaneOrder;
		/* C3 */ uint8_t BankCLUT;
		/* C4 */ uint32_t TransparentCol[2];
		/** reserved (C6) **/
		/* C7 */ uint32_t MaskCol[2];
		/** reserved (C9) **/
		/* CA */ uint32_t ColorDYUV[2]; // starting color
		/** reserved (CC) **/
		/* CD */ uint16_t CursorPosition[2];
		/* CE */ uint8_t CursorEnable, CursorBlink, CursorOnTime, CursorOffTime, CursorColor, CursorRes;
		/* CF */ uint16_t CursorPatternX; uint8_t CursorPatternY;
		/* D0 */ /*** see below ***/
		/* D8 */ uint8_t BackdropColor;
		/* D9 */ uint32_t MosaicPixel[2];
		/* DB */ uint8_t ICF[2];

		enum MosaicFactor MF[2];
		enum FileType FT[2];
		enum ColorMode CM[2];
	} reg;

	struct
	{
		// Registers
		std::vector<uint8_t> active;
		std::vector<uint8_t> opcode;
		std::vector<uint16_t> X;
		std::vector<uint8_t> ICF;
		std::vector<uint8_t> flag;

		// Current index
		size_t p;

		void reset()
		{
			active.assign(8, 0);
			opcode.assign(8, 0);
			X.assign(8, 0);
			ICF.assign(8, 0);
			flag.assign(8, 0);
			p = 0;
		}
	} Matte;
	bool MF[2];

	void matteCheck(int x)
	{
		if (Matte.X[Matte.p] != x) return;

		switch (Matte.opcode[Matte.p]) {
			default:
			case 0x0: // end of matte control
				Matte.p = Matte.opcode.size();
				//MiniCDI::Log("[VDSC] matte end");
				return;
			case 0x4: // change weight of pA
				reg.ICF[0] = Matte.ICF[Matte.p];
				//MiniCDI::Log("[VDSC] matte changed ICF for planeA: %.02f", reg.ICF[0]);
				break;
			case 0x6: // change weight of pB
				reg.ICF[1] = Matte.ICF[Matte.p];
				//MiniCDI::Log("[VDSC] matte changed ICF for planeB: %.02f", reg.ICF[0]);
				break;
			case 0x8: // reset
				MF[Matte.flag[Matte.p]] = 0;
				//MiniCDI::Log("[VDSC] matte flag %d unset", Matte.flag[Matte.p]);
				break;
			case 0x9: // set
				MF[Matte.flag[Matte.p]] = 1;
				//MiniCDI::Log("[VDSC] matte flag %d set", Matte.flag[Matte.p]);
				break;
			case 0xC: // reset & change weight of pA
				MF[Matte.flag[Matte.p]] = 0;
				reg.ICF[0] = Matte.ICF[Matte.p];
				//MiniCDI::Log("[VDSC] matte flag %d unset + ICF for planeA: %.02f", Matte.flag[Matte.p], reg.ICF[0]);
				break;
			case 0xD: // set & change weight of pA
				MF[Matte.flag[Matte.p]] = 1;
				reg.ICF[0] = Matte.ICF[Matte.p];
				//MiniCDI::Log("[VDSC] matte flag %d set + ICF for planeA: %.02f", Matte.flag[Matte.p], reg.ICF[0]);
				break;
			case 0xE: // reset & change weight of pB
				MF[Matte.flag[Matte.p]] = 0;
				reg.ICF[1] = Matte.ICF[Matte.p];
				//MiniCDI::Log("[VDSC] matte flag %d unset + ICF for planeB: %.02f", Matte.flag[Matte.p], reg.ICF[1]);
				break;
			case 0xF: // set & change weight of pB
				MF[Matte.flag[Matte.p]] = 1;
				reg.ICF[1] = Matte.ICF[Matte.p];
				//MiniCDI::Log("[VDSC] matte flag %d set + ICF for planeB: %.02f", Matte.flag[Matte.p], reg.ICF[1]);
				break;
		}
		if (Matte.p < Matte.opcode.size()) Matte.p++;
	}

	template <size_t Path>
	uint8_t getCLUTindex(uint8_t* src, bool second = false)
	{
		switch (reg.Icm[Path]) {
			default:
				assert(0);
				return 0;

			case CLUT7:
				return Path ? std::clamp(*src & 0x7F, 0, 127) + 128 : *src & 0x7F;

			case CLUT77:
				return Path == 0 && reg.IcmCS ? std::clamp(*src & 0x7F, 0, 127) + 128 : *src & 0x7F;

			case CLUT8:
				return *src;

			case CLUT4:
				return second ? (Path ? std::clamp(*src & 0x07, 0, 127) + 128 : *src & 0x07)
							  : (Path ? std::clamp(*src >> 4 & 0x07, 0, 127) + 128 : *src >> 4 & 0x07);
		}
	}

	template <size_t Path>
	bool isTransparent(uint8_t* src)
	{
		bool ColorKey = (Path == 1 && reg.Icm[Path] == RGB555) || reg.Icm[Path] == DYUV ? false
					  : (reg.ColorCLUT[getCLUTindex<Path>(src)] & 0xFCFCFC) == (reg.TransparentCol[Path] & 0xFCFCFC)
						|| (reg.MaskCol[Path] & 0xFCFCFC);

		switch (reg.Transparency[Path])
		{
			default:
				assert(0);
				return false;
			case TcrAlways:
				return true;
			case TcrIfCK:
				return ColorKey;
			case TcrIfTB:
				return reg.Icm[Path] == RGB555 && Path && (*src & 0x8000);
			case TcrIfMF0:
				return !MF[0];
			case TcrIfMF1:
				return !MF[1];
			case TcrIfCK_MF0:
				return ColorKey || !MF[0];
			case TcrIfCK_MF1:
				return ColorKey || !MF[1];
			case TcrNever:
				return false;
			case TcrIfNotCK:
				return !ColorKey;
			case TcrIfNotTB:
				return reg.Icm[Path] == RGB555 && Path && !(*src & 0x8000);
			case TcrIfNotMF0:
				return MF[0];
			case TcrIfNotMF1:
				return MF[1];
			case TcrIfNotCK_MF0:
				return !ColorKey && MF[0];
			case TcrIfNotCK_MF1:
				return !ColorKey && MF[1];
		}
	}

	template <size_t Path>
	uint32_t decodeDYUV(uint8_t* src, uint32_t *dst)
	{
		char dequantizer[16] = {0,1,4,9,16,27,44,79,128,177,212,229,240,247,252,255};
		int U2 = ((*src & 0xF0) >> 4);
		int Y1 = (*src & 0x0F);
		int V2 = ((*(src+1) & 0xF0) >> 4);
		int Y2 = (*(src+1) & 0x0F);

		int start_Y = (reg.ColorDYUV[Path] >> 16 & 0xFF);
		int start_U = (reg.ColorDYUV[Path] >> 8 & 0xFF);
		int start_V = (reg.ColorDYUV[Path] & 0xFF);
		Y2 = start_Y + dequantizer[Y2];
		U2 = start_U + dequantizer[U2];
		V2 = start_V + dequantizer[V2];
		Y1 = start_Y + dequantizer[Y1];
		int U1 = (start_U + U2) >> 1;
		int V1 = (start_V + V2) >> 1;

		int r1 = V1;
		int g1 = V1;
		int b1 = V1;

		int r2 = V2;
		int g2 = V2;
		int b2 = V2;

		*dst	 = (r1 << 24) | (g1 << 24) | (b1 << 8) | 0xff;
		*(dst+1) = (r2 << 24) | (g2 << 24) | (b2 << 8) | 0xff;

		return 2;
	}

	template <size_t Path>
	uint32_t decodeRGB555(uint8_t* src, uint32_t *dst)
	{
		if (!isTransparent<Path>(src)) {
			uint8_t r = ((*src >> 10) & 0x1F) << 3;
			uint8_t g = ((*src >> 5) & 0x1F) << 3;
			uint8_t b = (*src & 0x1F) << 3;

			*dst = ((r > 255 ? 255 : r < 0 ? 0 : r) << 24 |
					(g > 255 ? 255 : g < 0 ? 0 : g) << 16 |
					(b > 255 ? 255 : b < 0 ? 0 : b) << 8 |
					0xFF);
		}

		return 1;
	}

	/**
	 * @brief  Decodes CLUT to an RGB pixel.
	 *
	 * @param  src:  pointer to the VSR buffer
	 * @param  dst:  pointer to the uint32_t pixel
	 *
	 * @return The number of RGB pixels incremented
	 */
	template <size_t Path>
	uint32_t decodeCLUT(uint8_t* src, uint32_t *dst)
	{
		switch (reg.Icm[Path]) {
			default:
				if (!isTransparent<Path>(src)) *dst = (reg.ColorCLUT[getCLUTindex<Path>(src)] << 8) | 0xFF;
				return 1;

			case CLUT4:
				if (!isTransparent<Path>(src)) *dst = (reg.ColorCLUT[getCLUTindex<Path>(src)] << 8) | 0xFF;
				if (!isTransparent<Path>(src)) *(dst+1) = (reg.ColorCLUT[getCLUTindex<Path>(src, true)] << 8) | 0xFF;
				return 2;
		}
	}

public:
	uint8_t cursor[16*16];
	Plane FG[2];

	Decoder()
	{
	}

	~Decoder()
	{
	}

	uint32_t* get_display()
	{
		return &output[0];
	}

	int get_display_width()
	{
		return FG[0].width;
	}

	void reset()
	{
		memset(cursor, 0, sizeof(cursor));
		reg = {0};
	}

	void set_mode(int hRes, int vRes, bool hDouble = false, bool vDouble = false)
	{
		FG[1].width = FG[0].width = hRes * (hDouble || vDouble ? 2 : 1);
		FG[1].height = FG[0].height = vRes * (vDouble ? 2 : 1);

		FG[0].decoded.assign(FG[0].width * FG[0].height, 0);
		FG[1].decoded.assign(FG[1].width * FG[1].height, 0);

		output.assign(FG[0].width * 280, 0x000000FF); // max bounds
	}

	/**
	 * @brief  Mixes all planes to the framebuffer.
	 */
	void mix_to_frame(int y)
	{
		#define PLANEA FG[reg.PlaneOrder ? 0 : 1]
		#define PLANEB FG[reg.PlaneOrder ? 1 : 0]
		#define GET_R(V) ((V) >> 24 & 0xFF)
		#define GET_G(V) ((V) >> 16 & 0xFF)
		#define GET_B(V) ((V) >> 8 & 0xFF)
		#define GET_A(V) ((V) & 0xFF)

		#define WF_MIX(V1, V2) std::clamp((int)((std::clamp(V1-16, 0, 255) * ((float)reg.ICF[reg.PlaneOrder ? 0 : 1]/64.0f)) \
											  + (std::clamp(V2-16, 0, 255) * ((float)reg.ICF[reg.PlaneOrder ? 1 : 0]/64.0f)) \
											  + 16.0f), 0, 255)

		#define WF_MIX_SINGLE(V1, WF) std::clamp((int)((std::clamp(V1-16, 0, 255) \
											  * ((float)reg.ICF[WF ? (reg.PlaneOrder ? 1 : 0) : (reg.PlaneOrder ? 0 : 1)] \
											  /64.0f) + 16.0f)), 0, 255)

		for (int x = 0; x < FG[0].width; x++) {
			int PIXELA = (y*PLANEA.width) + x;
			int PIXELB = (y*PLANEB.width) + x;
			int outputPixel = ((FG[0].height == 240 ? y+20 : y)*FG[0].width) + x;

			uint8_t rA = GET_R(PLANEA.decoded[PIXELA]),
					gA = GET_G(PLANEA.decoded[PIXELA]),
					bA = GET_B(PLANEA.decoded[PIXELA]),
					rB = GET_R(PLANEB.decoded[PIXELB]),
					gB = GET_G(PLANEB.decoded[PIXELB]),
					bB = GET_B(PLANEB.decoded[PIXELB]);

			if (reg.Mixing) {
				output[outputPixel] = (WF_MIX(rA, rB) << 24) | (WF_MIX(gA, gB) << 16) | (WF_MIX(bA, bB) << 8) | 0xFF;
			} else {
				if (PLANEB.decoded[PIXELB])
					output[outputPixel] = (WF_MIX_SINGLE(rB, 1) << 24) | (WF_MIX_SINGLE(gB, 1) << 16) | (WF_MIX_SINGLE(bB, 1) << 8) | 0xFF;
				else if (PLANEA.decoded[PIXELA])
					output[outputPixel] = (WF_MIX_SINGLE(rA, 0) << 24) | (WF_MIX_SINGLE(gA, 0) << 16) | (WF_MIX_SINGLE(bA, 0) << 8) | 0xFF;
				else {
					// Transparent, draw backdrop.
					switch (reg.BackdropColor & 0x07) {
						default: output[outputPixel] = 0x101010ff; break;
						case 0x01: output[outputPixel] = reg.BackdropColor & 0x08 ? 0x1010FFff : 0x101090ff; break;
						case 0x02: output[outputPixel] = reg.BackdropColor & 0x08 ? 0x10FF10ff : 0x109010ff; break;
						case 0x03: output[outputPixel] = reg.BackdropColor & 0x08 ? 0x10FFFFff : 0x109090ff; break;
						case 0x04: output[outputPixel] = reg.BackdropColor & 0x08 ? 0xFF1010ff : 0x901010ff; break;
						case 0x05: output[outputPixel] = reg.BackdropColor & 0x08 ? 0xFF10FFff : 0x901090ff; break;
						case 0x06: output[outputPixel] = reg.BackdropColor & 0x08 ? 0xFFFF10ff : 0x909010ff; break;
						case 0x07: output[outputPixel] = reg.BackdropColor & 0x08 ? 0xFFFFFFff : 0x909090ff; break;
					}
				}
			}

			if (x >= reg.CursorPosition[0] && x < reg.CursorPosition[0]+16
			 && y >= reg.CursorPosition[1] && y < reg.CursorPosition[1]+16
			 && cursor[(y-reg.CursorPosition[1])*16 + (x-reg.CursorPosition[0])] != 0
			 && reg.CursorEnable)
			{
				switch (reg.CursorColor & 0x07) {
					default: output[outputPixel] = 0x000000ff; continue;
					case 0x01: output[outputPixel] = reg.CursorColor & 0x08 ? 0x0000FFff : 0x000080ff; continue;
					case 0x02: output[outputPixel] = reg.CursorColor & 0x08 ? 0x00FF00ff : 0x008000ff; continue;
					case 0x03: output[outputPixel] = reg.CursorColor & 0x08 ? 0x00FFFFff : 0x008080ff; continue;
					case 0x04: output[outputPixel] = reg.CursorColor & 0x08 ? 0xFF0000ff : 0x800000ff; continue;
					case 0x05: output[outputPixel] = reg.CursorColor & 0x08 ? 0xFF00FFff : 0x800080ff; continue;
					case 0x06: output[outputPixel] = reg.CursorColor & 0x08 ? 0xFFFF00ff : 0x808000ff; continue;
					case 0x07: output[outputPixel] = reg.CursorColor & 0x08 ? 0xFFFFFFff : 0x808080ff; continue;
				}
			}
		}

		#undef PLANEA
		#undef PLANEB
		#undef GET_R
		#undef GET_G
		#undef GET_B
		#undef GET_A
		#undef WF_MIX
		#undef WF_MIX_SINGLE
	}

	/**
	 * @brief  Draws a VSR line to a plane.
	 *
	 * @param  vsr:  memory index for the start of the VSR
	 * @param  y:    the line
	 *
	 * @return The incremented VSR
	 */
	template <size_t Path>
	uint32_t draw_line_to_plane(uint8_t* memory, uint32_t vsr, int y)
	{
		// reset matte flag for plane
		Matte.reset();

		if (reg.Icm[Path] == Off) {
			memset(&FG[Path].decoded[(y * FG[Path].width)], 0x00000000, FG[Path].width * sizeof(uint32_t));
			return 0;
		}

		for (int x = 0; x < FG[Path].width;)
		{
			uint8_t* src = &memory[++vsr];
			uint32_t* dst = &FG[Path].decoded[(y * FG[Path].width) + x];

			matteCheck(FG[Path].width > 400 ? x*2 : x);

			switch (reg.FT[Path]) {
				default:
				case Bitmap:
					if (Path == 1 && reg.Icm[Path] == RGB555) {
						x += decodeRGB555<Path>(src, dst);
						continue;
					} else {
						switch (reg.Icm[Path])
						{
							default:
								assert(0);
								continue;

							case DYUV:
								x += decodeDYUV<Path>(src, dst);
								++vsr;
								continue;

							case CLUT4:
							case CLUT7:
							case CLUT77:
							case CLUT8:
								x += decodeCLUT<Path>(src, dst);
								continue;
						}
					}

				case RunLength:
					switch (reg.Icm[Path])
					{
						default:
						case CLUT4: // RL3
						case CLUT7: // RL7
							if (*src & 0x80) {
								int length = *(src+1);
								++vsr;

								if (length != 1) {
									if (length == 0)
										length = FG[Path].width - x;
									for (int i = 0; i < length;) {
										i += decodeCLUT<Path>(src, dst+i);
									}
								}
								x += length;
							} else {
								x += decodeCLUT<Path>(src, dst);
							}
							continue;
					}

				case Mosaic:
					// TO-DO
					// reg.ICF[Path] /= 2;
					assert(0);
					continue;
			}
		}

		return vsr;
	}

	template <size_t Path>
	void set_register(uint32_t inst)
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

			case 0xC0:
				if (!Path) {
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

			case 0xC1:
				if (!Path) {
					reg.Transparency[0] = (enum Tcr)(inst & 0x0Fu);
					reg.Transparency[1] = (enum Tcr)(inst >> 8 & 0x0Fu);
					reg.Mixing = inst & 0x00800000u ? 0 : 1;
					//MiniCDI::Log("[VDSC] P%d tctl mx=%d,tca=%02d,tcb=%02d", Path, reg.Mixing, reg.Transparency[0], reg.Transparency[1]);
				}
				break;

			case 0xC2:
				if (!Path) {
					reg.PlaneOrder = inst & 0x0Fu;
					//MiniCDI::Log("[VDSC] P%d po %s", Path, reg.PlaneOrder ? "a,b" : "b,a");
				}
				break;

			case 0xC3:
				reg.BankCLUT = inst & 0xFFu;
				//MiniCDI::Log("[VDSC] P%d cbnk %d", Path, reg.BankCLUT);
				break;

			case 0xC4:
			case 0xC6:
				reg.TransparentCol[(inst >> 24 & 0xFF) == 0xC6 ? 1 : 0] = inst & 0x00FFFFFFu;
				break;

			case 0xC7:
			case 0xC9:
				reg.MaskCol[(inst >> 24 & 0xFF) == 0xC9 ? 1 : 0] = inst & 0x00FFFFFFu;
				break;

			case 0xCA:
			case 0xCB:
				reg.ColorDYUV[(inst >> 24 & 0xFF) == 0xCB ? 1 : 0] = inst & 0x00FFFFFFu;
				//MiniCDI::Log("[VDSC] P%d yuv_b y=$%02x,u=$%02x,v=$%02x", Path, inst >> 16 & 0xFFu, inst >> 8 & 0xFFu, inst & 0xFFu);
				break;

			case 0xCD: // channel 1
				if (!Path) {
					reg.CursorPosition[0] = (inst & 0x00000FFFu) / (FG[0].width < 400 ? 2 : 1); // double-resolution
					reg.CursorPosition[1] = inst >> 12 & 0x0FFFu;
					//MiniCDI::Log("[VDSC] P%d cpos x=%d,y=%d", Path, reg.CursorPosition[0], reg.CursorPosition[1]);
				}
				break;

			case 0xCE: // channel 1
				if (!Path) {
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
				if (!Path) {
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
					uint8_t rIndex = (inst >> 24 & 0xFF) - 0xD0;

					if (!Matte.opcode.size()) {
						Matte.reset();
					}

					if (!Matte.active[rIndex]) {
						Matte.active[rIndex] = true;
						Matte.X[rIndex] = inst & 0x3FFu;
						Matte.ICF[rIndex] = inst >> 10 & 0x3Fu;
						Matte.flag[rIndex] = reg.MatteCount ? rIndex >= 4 ? 1 : 0 : inst >> 16 & 0x01u;
						Matte.opcode[rIndex] = inst >> 20 & 0x0Fu;
						//MiniCDI::Log("[VDSC] P%d rctl %d,op=%1X,rf=%d,wf=%d,x=%d", Path, rIndex, Matte.opcode[rIndex], Matte.flag[rIndex], Matte.ICF[rIndex], Matte.X[rIndex]);
					}
				}
				break;

			case 0xD8: // channel 1
				if (!Path) {
					reg.BackdropColor = inst & 0x00FFFFFFu;
				}
				break;

			case 0xD9:
			case 0xDA:
				reg.MosaicPixel[(inst >> 24 & 0xFF) == 0xDA ? 1 : 0] = inst & 0xFFu;
				break;

			case 0xDB:
			case 0xDC:
				reg.ICF[(inst >> 24 & 0xFF) == 0xDC ? 1 : 0] = inst & 0x3Fu;
				//if (reg.ICF[Path]) MiniCDI::Log("[VDSC] P%d wfac_%s %d", Path ? "b" : "a", reg.ICF[Path]);
				break;
		}
	}
};

}

#undef YUV_GET_Y
#undef YUV_GET_U
#undef YUV_GET_V

#endif