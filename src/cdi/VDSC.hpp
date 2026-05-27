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
	bool isTransparent(uint32_t rgb)
	{
		switch (reg.Transparency[Path])
		{
			default:
				assert(0);
				return false;
			case TcrAlways:
				return true;
			case TcrIfCK:
				return (rgb & 0x00FCFCFC) == reg.TransparentCol[Path] || (rgb & 0x00FCFCFC) == reg.MaskCol[Path];
			case TcrIfTB:
				return rgb & 0xFF000000;
			case TcrIfMF0:
				return !MF[0];
			case TcrIfMF1:
				return !MF[1];
			case TcrIfCK_MF0:
				return (rgb & 0x00FCFCFC) == reg.TransparentCol[Path] || (rgb & 0x00FCFCFC) == reg.MaskCol[Path] || !MF[0];
			case TcrIfCK_MF1:
				return (rgb & 0x00FCFCFC) == reg.TransparentCol[Path] || (rgb & 0x00FCFCFC) == reg.MaskCol[Path] || !MF[1];
			case TcrNever:
				return false;
			case TcrIfNotCK:
				return (rgb & 0x00FCFCFC) != reg.TransparentCol[Path] && (rgb & 0x00FCFCFC) != reg.MaskCol[Path];
			case TcrIfNotTB:
				return rgb != 0xFF000000;
			case TcrIfNotMF0:
				return MF[0];
			case TcrIfNotMF1:
				return MF[1];
			case TcrIfNotCK_MF0:
				return (rgb & 0x00FCFCFC) != reg.TransparentCol[Path] && (rgb & 0x00FCFCFC) != reg.MaskCol[Path] && MF[0];
			case TcrIfNotCK_MF1:
				return (rgb & 0x00FCFCFC) != reg.TransparentCol[Path] && (rgb & 0x00FCFCFC) != reg.MaskCol[Path] && MF[1];
		}
	}

	uint32_t decodeDYUV(uint8_t* src, uint32_t *dst)
	{
		int Y = *src & 0x0F;
		int U = (*src >> 4) & 0x0F;
		int V = (*(src+1) >> 4) & 0x0F;

		int r = std::floor((Y*256 + 351*(V-128) ) / 256 );
		int g = std::floor(((Y*256)*(86*(U-128) + 179*(V-128))) / 256 );
		int b = std::floor(( Y*256 + 444*(U-128) ) / 256);

		*dst = ((r > 255 ? 255 : r < 0 ? 0 : r) << 24 |
				(g > 255 ? 255 : g < 0 ? 0 : g) << 16 |
				(b > 255 ? 255 : b < 0 ? 0 : b) << 8 |
				0xff);

		return 1;
	}

	template <size_t Path>
	uint32_t decodeRGB555(uint8_t* src, uint32_t *dst)
	{
		uint8_t r = ((*src >> 10) & 0x1F) << 3;
		uint8_t g = ((*src >> 5) & 0x1F) << 3;
		uint8_t b = (*src & 0x1F) << 3;

		*dst = ((r > 255 ? 255 : r < 0 ? 0 : r) << 24 |
				(g > 255 ? 255 : g < 0 ? 0 : g) << 16 |
				(b > 255 ? 255 : b < 0 ? 0 : b) << 8 |
				(isTransparent<Path>(((*src & 0x8000) << 9) | (r << 16) | (g << 8) | b) ? 0 : 0xff));

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
		uint8_t index = 0;
		switch (reg.Icm[Path]) {
			default:
				assert(0);
				return 0;

			case CLUT7:
			case CLUT77:
				index = Path && (*src & 0x7F) < 128 ? (*src & 0x7F) + 128 : *src & 0x7F;
				*dst = (reg.ColorCLUT[index] << 8) | (isTransparent<Path>(reg.ColorCLUT[index]) ? 0 : 0xff);
				return 1;

			case CLUT8:
				index = *src;
				*dst = (reg.ColorCLUT[index] << 8) | (isTransparent<Path>(reg.ColorCLUT[index]) ? 0 : 0xff);
				return 1;

			case CLUT4:
				index = Path && (*src >> 4 & 0x07) < 128 ? (*src >> 4 & 0x07) + 128 : *src >> 4 & 0x07;
				*dst = (reg.ColorCLUT[index] << 8) | (isTransparent<Path>(reg.ColorCLUT[index]) ? 0 : 0xff);

				index = Path && (*src & 0x07) < 128 ? (*src & 0x07) + 128 : *src & 0x07;
				*(dst+1) = (reg.ColorCLUT[index] << 8) | (isTransparent<Path>(reg.ColorCLUT[index]) ? 0 : 0xff);
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

	void set_mode(enum Type type, bool hRes = false, bool vRes = false)
	{
		FG[1].width = FG[0].width = (type == NTSCMonitor ? 360 : 384) * (hRes || vRes ? 2 : 1);
		FG[1].height = FG[0].height = (type == PAL ? 280 : 240) * (vRes ? 2 : 1);

		FG[0].decoded.assign(FG[0].width * FG[0].height, 0);
		FG[1].decoded.assign(FG[1].width * FG[1].height, 0);
	}

	/**
	 * @brief  Mixes all planes to the framebuffer.
	 */
	void draw_frame()
	{
		output.assign(FG[0].width * FG[0].height, 0);

		#define PLANEA FG[reg.PlaneOrder ? 1 : 0]
		#define PLANEB FG[reg.PlaneOrder ? 0 : 1]
		#define GET_R(V) ((V) >> 24 & 0xFF)
		#define GET_G(V) ((V) >> 16 & 0xFF)
		#define GET_B(V) ((V) >> 8 & 0xFF)
		#define GET_A(V) ((V) & 0xFF)

		#define WF_MIX(V1, V2) std::clamp((int)(((V1-16.0f) * (float)reg.ICF[0]/64.0f) + ((V2-16.0f) * (float)reg.ICF[1]/64.0f) + 16.0f), 0, 255)

		for (int y = 0; y < FG[0].height; y++) {
			for (int x = 0; x < FG[0].width; x++) {
				int outputPixel = (y*FG[0].width) + x;

				uint8_t rA = GET_R(PLANEA.decoded[(y*PLANEA.width)+x]),
						gA = GET_G(PLANEA.decoded[(y*PLANEA.width)+x]),
						bA = GET_B(PLANEA.decoded[(y*PLANEA.width)+x]),
						aA = GET_A(PLANEA.decoded[(y*PLANEA.width)+x]),

						rB = GET_R(PLANEB.decoded[(y*PLANEB.width)+x]),
						gB = GET_G(PLANEB.decoded[(y*PLANEB.width)+x]),
						bB = GET_B(PLANEB.decoded[(y*PLANEB.width)+x]),
						aB = GET_A(PLANEB.decoded[(y*PLANEB.width)+x]);

				// Mixing technique
				if (((rA & 0xFC) || (gA & 0xFC) || (bA & 0xFC)) && ((rB & 0xFC) || (gB & 0xFC) || (bB & 0xFC)) && reg.Mixing) {
					output[outputPixel] = (WF_MIX(rA, rB) << 24) | (WF_MIX(gA, gB) << 16) | (WF_MIX(bA, bB) << 8) | (WF_MIX(aA, aB));
				}

				// Overlay technique
				else {
					// Transparent, draw backdrop.
					switch (reg.BackdropColor & 0x07) {
						default: output[outputPixel] = 0x000000ff; break;
						case 0x01: output[outputPixel] = reg.BackdropColor & 0x08 ? 0x0000FFff : 0x000080ff; break;
						case 0x02: output[outputPixel] = reg.BackdropColor & 0x08 ? 0x00FF00ff : 0x008000ff; break;
						case 0x03: output[outputPixel] = reg.BackdropColor & 0x08 ? 0x00FFFFff : 0x008080ff; break;
						case 0x04: output[outputPixel] = reg.BackdropColor & 0x08 ? 0xFF0000ff : 0x800000ff; break;
						case 0x05: output[outputPixel] = reg.BackdropColor & 0x08 ? 0xFF00FFff : 0x800080ff; break;
						case 0x06: output[outputPixel] = reg.BackdropColor & 0x08 ? 0xFFFF00ff : 0x808000ff; break;
						case 0x07: output[outputPixel] = reg.BackdropColor & 0x08 ? 0xFFFFFFff : 0x808080ff; break;
					}

					if (((rA & 0xFC) || (gA & 0xFC) || (bA & 0xFC)) && aA) {
						output[outputPixel] = (rA << 24) | (gA << 16) | (bA << 8) | 0xFF;
					}

					if (((rB & 0xFC) || (gB & 0xFC) || (bB & 0xFC)) && aB) {
						output[outputPixel] = (rB << 24) | (gB << 16) | (bB << 8) | 0xFF;
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
		}

		#undef PLANEA
		#undef PLANEB
		#undef GET_R
		#undef GET_G
		#undef GET_B
		#undef GET_A
		#undef WF_MIX
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
		if (reg.Icm[Path] == Off) {
			memset(&FG[Path].decoded[(y * FG[Path].width)], 0x00000000, FG[Path].width * sizeof(uint32_t));
			return 0;
		}

		for (int x = 0; x < FG[Path].width;)
		{
			matteCheck(FG[Path].width > 400 ? x*2 : x);

			uint8_t* src = &memory[++vsr];
			uint32_t* dst = &FG[Path].decoded[(y * FG[Path].width) + x];

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
								x += decodeDYUV(src, dst);
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
						case CLUT4:
						case CLUT7:
							int length = 1;
							if (*src & 0x80) {
								length = *(src+1);
								++vsr;
							}

							if (length == 0) {
								while (x < FG[Path].width) {
									x += decodeCLUT<Path>(src, &FG[Path].decoded[(y * FG[Path].width) + x]);
								}
							} else {
								for (int i = 0; i < length;) {
									i += decodeCLUT<Path>(src, dst+i);
								}
								x += length;
							}
							continue;
					}

				case Mosaic:
					// to-do
					// reg.ICF[Path] /= 2;
					assert(0);
					continue;
			}
		}

		// reset matte flag for plane
		Matte.reset();
		MF[0] = MF[1] = 0;

		return vsr;
	}

	template <size_t Path>
	void set_register(uint32_t inst)
	{
		switch (inst >> 24 & 0xFF)
		{
			default:
				if ((inst >> 24 & 0xFF) >= 0x80u && (inst >> 24 & 0xFF) < 0xC0u) {
					reg.ColorCLUT[(inst >> 24 & 0xFF) - 0x80u + (reg.BankCLUT * 0x40)] = inst & 0x00FFFFFFu;
					//MiniCDI::Log("[DCA%d] color $%06x", Path, reg.ColorCLUT[(inst >> 24 & 0xFF) - 0x80u + (reg.BankCLUT * 0x40)]);
				}
				break;

			case 0x78:
			case 0x79:
			case 0x7a:
			case 0x7b:
			case 0x7c:
			case 0x7d:
			case 0x7e:
			case 0x7f:
				reg.FT[Path] = (inst & 0b0011u) == 0b11 ? Mosaic : (inst & 0b0011u) == 0b10 ? RunLength : Bitmap;
				reg.MF[Path] = (enum MosaicFactor)((inst & 0b1100u) >> 2);
				reg.CM[Path] = (enum ColorMode)((inst & 0b100000000u) >> 8);
				/*MiniCDI::Log("[DCA%d] dprm cm=%s,mf=%s,ft=%s", Path, reg.CM[Path] == Double4 ? "p4" : "p8",
																reg.MF[Path] == x16 ? "x16" : reg.MF[Path] == x8 ? "x8"
															  : reg.MF[Path] == x4 ? "x4" : "x2",
																reg.FT[Path] == Mosaic ? "m" : reg.FT[Path] == RunLength ? "rl"
															  : "bmp");*/
				break;

			case 0xC0:
				reg.IcmCS = (inst & 0x00400000u) >> 22;
				reg.MatteCount = (inst & 0x00100000u) >> 19;
				reg.ExternalVideo = (inst & 0x00080000u) >> 18;
				reg.Icm[1] = (enum Icm)((inst & 0b111100000000u) >> 8);
				reg.Icm[0] = (enum Icm)(inst & 0b1111u);
				/*MiniCDI::Log("[DCA%d] icm cs=%d,nr=%d,ev=%d,cma=%s,cmb=%s", Path,
						  reg.IcmCS, reg.MatteCount, reg.ExternalVideo,
						  reg.Icm[0] == CLUT8 ? "clut8" : reg.Icm[0] == CLUT7 ? "clut7"
						: reg.Icm[0] == CLUT77 ? "clut7+7" : reg.Icm[0] == DYUV ? "dyuv"
						: reg.Icm[0] == CLUT4 ? "clut4" : "off",
						  reg.Icm[1] == RGB555 ? "rgb555" : reg.Icm[1] == DYUV ? "dyuv"
						: reg.Icm[1] == CLUT4 ? "clut4" : "off");*/
				break;

			case 0xC1:
				if (!Path) reg.Transparency[0] = (enum Tcr)(inst & 0x0Fu);
				if (!Path) reg.Transparency[1] = (enum Tcr)((inst & 0x0Fu) >> 8);
				if (!Path) reg.Mixing = (inst & 0x00800000u) >> 23;
				break;

			case 0xC2:
				if (!Path) reg.PlaneOrder = inst & 0x0Fu;
				//MiniCDI::Log("[DCA%d] po %s", Path, reg.PlaneOrder ? "b,a" : "a,b");
				break;

			case 0xC3:
				reg.BankCLUT = inst & 0x0Fu;
				//MiniCDI::Log("[DCA%d] cbnk %d", Path, reg.BankCLUT);
				break;

			case 0xC4:
			case 0xC6:
				reg.TransparentCol[Path] = inst & 0x00FCFCFCu;
				break;

			case 0xC7:
			case 0xC9:
				reg.MaskCol[Path] = inst & 0x00FCFCFCu;
				break;

			case 0xCA:
			case 0xCB:
				reg.ColorDYUV[Path] = inst & 0x00FFFFFFu;
				break;

			case 0xCD: // channel 1
				reg.CursorPosition[0] = (inst & 0x00000FFFu) / (FG[0].width < 400 ? 2 : 1); // double-resolution
				reg.CursorPosition[1] = (inst >> 12) & 0x00000FFFu;
				//MiniCDI::Log("[DCA%d] cpos x=%d,y=%d", Path, reg.CursorPosition[0], reg.CursorPosition[1]);
				break;

			case 0xCE: // channel 1
				reg.CursorColor = inst & 0x000000FFu;
				reg.CursorRes = (inst >> 15) & 0x01u;
				reg.CursorOffTime = (inst >> 16) & 0x07u;
				reg.CursorOnTime = (inst >> 19) & 0x07u;
				reg.CursorBlink = (inst >> 22) & 0x01u;
				reg.CursorEnable = (inst >> 23) & 0x01u;
				break;

			case 0xCF: // channel 1
				reg.CursorPatternX = inst & 0x0000FFFFu;
				reg.CursorPatternY = (inst >> 16) & 0x0Fu;
				for (uint8_t x = 0; x < 16; x++) {
					cursor[(reg.CursorPatternY*16)+x] = (reg.CursorPatternX >> (15-x) & 0x01) != 0 ? 0xFF : 0x00;
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
						Matte.X[rIndex] = inst & 0x000003FFu;
						Matte.ICF[rIndex] = (inst & 0x0000FC00u) >> 10;
						Matte.flag[rIndex] = reg.MatteCount ? rIndex >= 4 ? 1 : 0 : (inst & 0x00010000u) >> 16;
						Matte.opcode[rIndex] = (inst & 0x00F80000u) >> 20;
						//if (inst & 0x00FFFFFF) MiniCDI::Log("[DCA%d] mc x=%d,wf=%d,f=%d,op=%01X", Path, Matte.X[rIndex], Matte.ICF[rIndex], Matte.flag[rIndex], Matte.opcode[rIndex]);
					}
				}
				break;

			case 0xD8: // channel 1
				reg.BackdropColor = inst & 0x00FFFFFFu;
				break;

			case 0xD9:
			case 0xDA:
				reg.MosaicPixel[Path] = inst & 0x00FFFFFFu;
				break;

			case 0xDB:
			case 0xDC:
				reg.ICF[Path] = inst & 0x3Fu;
				break;
		}
	}
};

}

#undef YUV_GET_Y
#undef YUV_GET_U
#undef YUV_GET_V

#endif