#ifndef MINICDI_VDSC
#define MINICDI_VDSC

/*****
  DISCLAIMER:
  Sourced partially from official documentation of MCD212 by Motorola and
  MAME CD-i driver by Ryan Holtz and Vincent Halver (licensed under BSD-3-Clause).
 *****/

#include <cassert>
#include <cmath>
#include <algorithm>

namespace VideoCDI
{

enum Icm
{
	// All coding methods are usable in standard-res except for CLUT4.
	Off = 0x00,
	CLUT8 = 0x01, // plane A only
	CLUT7 = 0x03,
	CLUT77 = 0x04,
	DYUV = 0x05,
	CLUT4 = 0x0B, // double-res only
	RGB555 = 0x01, // plane B only
};

enum Tcr
{
	TcrAlways = 0,
	TcrIfCK = 1,
	TcrIfTB,
	TcrIfRF0,
	TcrIfRF1,
	TcrIfCK_RF0,
	TcrIfCK_RF1,
	TcrUnusedA,
	TcrNever,
	TcrIfNotCK,
	TcrIfNotTB,
	TcrIfNotRF0,
	TcrIfNotRF1,
	TcrIfNotCK_RF0,
	TcrIfNotCK_RF1,
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

class Video
{
	std::vector<uint32_t> output;

	struct
	{
		/* 80 */ uint32_t ColorCLUT[256]; // A CLUT is a color lookup table holding a number of colors in RGB888.
		/* C0 */ enum Icm Icm[2];
				 uint8_t IcmCS, IcmNR, IcmEV;
		/* C1 */ enum Tcr Transparency;
		/* C2 */ uint8_t PlaneOrder;
		/* C3 */ uint8_t BankCLUT;
		/* C4 */ uint8_t TransparentCol[2];
		/** reserved (C6) **/
		/* C7 */ uint8_t MaskCol[2];
		/** reserved (C9) **/
		/* CA */ uint8_t ColorDYUV[2]; // starting color
		/** reserved (CC) **/
		/* CD */ uint16_t CursorPosition[2];
		/* CE */ uint8_t CursorEnable, CursorBlink, CursorOnTime, CursorOffTime, CursorColor, CursorRes;
		/* CF */ uint16_t CursorPatternX;
				 uint8_t CursorPatternY;
		/* D0 */ uint8_t RegionControl[8];
		/* D8 */ uint8_t BackdropColor;
		/* D9 */ uint8_t MosaicPixel[2];
		/* DB */ uint8_t WeightFactor[2];

		enum MosaicFactor MF[2];
		enum FileType FT[2];
		enum ColorMode CM[2];
	} Decoder;

	template <size_t Path>
	uint8_t getAlpha(uint8_t vsr)
	{
		switch (Decoder.Transparency)
		{
			default:
				exit(0);
				return 0xFF;
			case TcrNever:
				return 0xFF;
			case TcrAlways:
				return 0x00;
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
		switch (Decoder.Icm[Path]) {
			default:
				return 0;

			case CLUT7:
			case CLUT77:
				*dst = (Decoder.ColorCLUT[*src & 0x7F] << 8) | getAlpha<Path>(*src);
				return 1;

			case CLUT8:
				*dst = (Decoder.ColorCLUT[*src] << 8) | getAlpha<Path>(*src);
				return 1;

			case CLUT4:
				*dst = (Decoder.ColorCLUT[(*src & 0x70) >> 4] << 8) | getAlpha<Path>(*src);
				*(dst+1) = (Decoder.ColorCLUT[*src & 0x07] << 8) | getAlpha<Path>(*src);
				return 2;
		}
	}

public:
	uint8_t cursor[16*16];
	Plane FG[2];

	Video()
	{
	}

	~Video()
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
		Decoder = {0};
	}

	void set_mode(enum Type type, bool hRes = false, bool vRes = false)
	{
		FG[1].width = FG[0].width = (type == NTSCMonitor ? 360 : 384) * (hRes || vRes ? 2 : 1);
		FG[1].height = FG[0].height = (type == PAL ? 280 : 240) * (vRes ? 2 : 1);

		FG[0].decoded.resize(FG[0].width * FG[0].height, 0);
		FG[1].decoded.resize(FG[1].width * FG[1].height, 0);
	}

	/**
	 * @brief  Mixes all planes to the framebuffer.
	 */
	void draw_frame()
	{
		output.assign(FG[0].width * FG[0].height, 0x000000ff);

		for (int y = 0; y < FG[0].height; y++) {
			for (int x = 0; x < FG[0].width; x++) {
				int outputPixel = (y*FG[0].width) + x;

				// Backdrop should just be a single solid color, cursor is handled by byte pattern so they can be drawn directly in this function.
				float wfA = (float)Decoder.WeightFactor[Decoder.PlaneOrder ? 1 : 0] / 32.0f;
				float wfB = (float)Decoder.WeightFactor[Decoder.PlaneOrder ? 0 : 1] / 32.0f;

				float rA = (FG[Decoder.PlaneOrder ? 1 : 0].decoded[(y*FG[Decoder.PlaneOrder ? 1 : 0].width)+x] & 0xff000000) >> 24;
				float gA = (FG[Decoder.PlaneOrder ? 1 : 0].decoded[(y*FG[Decoder.PlaneOrder ? 1 : 0].width)+x] & 0x00ff0000) >> 16;
				float bA = (FG[Decoder.PlaneOrder ? 1 : 0].decoded[(y*FG[Decoder.PlaneOrder ? 1 : 0].width)+x] & 0x0000ff00) >> 8;
				float aA = FG[Decoder.PlaneOrder ? 1 : 0].decoded[(y*FG[Decoder.PlaneOrder ? 1 : 0].width)+x] & 0x000000ff;

				float rB = (FG[Decoder.PlaneOrder ? 0 : 1].decoded[(y*FG[Decoder.PlaneOrder ? 0 : 1].width)+x] & 0xff000000) >> 24;
				float gB = (FG[Decoder.PlaneOrder ? 0 : 1].decoded[(y*FG[Decoder.PlaneOrder ? 0 : 1].width)+x] & 0x00ff0000) >> 16;
				float bB = (FG[Decoder.PlaneOrder ? 0 : 1].decoded[(y*FG[Decoder.PlaneOrder ? 0 : 1].width)+x] & 0x0000ff00) >> 8;
				float aB = FG[Decoder.PlaneOrder ? 0 : 1].decoded[(y*FG[Decoder.PlaneOrder ? 0 : 1].width)+x] & 0x000000ff;

				if (!rA && !gA && !bA && !rB && !gB && !bB) {
					// Transparent, draw backdrop.
					uint32_t bgColor;

					switch (Decoder.BackdropColor & 0x07) {
						default: bgColor = 0x000000ff; break;
						case 0x01: bgColor = Decoder.BackdropColor & 0x08 ? 0x0000FFff : 0x000080ff; break;
						case 0x02: bgColor = Decoder.BackdropColor & 0x08 ? 0x00FF00ff : 0x008000ff; break;
						case 0x03: bgColor = Decoder.BackdropColor & 0x08 ? 0x00FFFFff : 0x008080ff; break;
						case 0x04: bgColor = Decoder.BackdropColor & 0x08 ? 0xFF0000ff : 0x800000ff; break;
						case 0x05: bgColor = Decoder.BackdropColor & 0x08 ? 0xFF00FFff : 0x800080ff; break;
						case 0x06: bgColor = Decoder.BackdropColor & 0x08 ? 0xFFFF00ff : 0x808000ff; break;
						case 0x07: bgColor = Decoder.BackdropColor & 0x08 ? 0xFFFFFFff : 0x808080ff; break;
					}

					output[outputPixel] = bgColor;
				} else {
					uint8_t rAB = std::clamp((int)(rA * wfA) + (int)(rB * wfB) + 16, 0, 255);
					uint8_t gAB = std::clamp((int)(gA * wfA) + (int)(gB * wfB) + 16, 0, 255);
					uint8_t bAB = std::clamp((int)(bA * wfA) + (int)(bB * wfB) + 16, 0, 255);
					// uint8_t aAB = std::clamp((int)(aA * wfA) + (int)(aB * wfB), 0, 255);

					output[outputPixel] = (rAB << 24) | (gAB << 16) | (bAB << 8) | 0xFF;
				}

				if (x >= Decoder.CursorPosition[0] && x < Decoder.CursorPosition[0]+16
				 && y >= Decoder.CursorPosition[1] && y < Decoder.CursorPosition[1]+16
				 && cursor[(y-Decoder.CursorPosition[1])*16 + (x-Decoder.CursorPosition[0])] != 0
				 && Decoder.CursorEnable)
				{
					switch (Decoder.CursorColor & 0x07) {
						default: output[outputPixel] = 0x000000ff; break;
						case 0x01: output[outputPixel] = Decoder.CursorColor & 0x08 ? 0x0000FFff : 0x000080ff; break;
						case 0x02: output[outputPixel] = Decoder.CursorColor & 0x08 ? 0x00FF00ff : 0x008000ff; break;
						case 0x03: output[outputPixel] = Decoder.CursorColor & 0x08 ? 0x00FFFFff : 0x008080ff; break;
						case 0x04: output[outputPixel] = Decoder.CursorColor & 0x08 ? 0xFF0000ff : 0x800000ff; break;
						case 0x05: output[outputPixel] = Decoder.CursorColor & 0x08 ? 0xFF00FFff : 0x800080ff; break;
						case 0x06: output[outputPixel] = Decoder.CursorColor & 0x08 ? 0xFFFF00ff : 0x808000ff; break;
						case 0x07: output[outputPixel] = Decoder.CursorColor & 0x08 ? 0xFFFFFFff : 0x808080ff; break;
					}
				}
			}
		}
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
		if (Decoder.Icm[Path] == Off) {
			memset(&FG[Path].decoded[(y * FG[Path].width)], 0x00000000, FG[Path].width * sizeof(uint32_t));
			return 0;
		}

		for (int x = 0; x < FG[Path].width;)
		{
			uint8_t* src = &memory[++vsr];
			uint32_t* dst = &FG[Path].decoded[(y * FG[Path].width) + x];

			switch (Decoder.FT[Path]) {
				default:
				case Bitmap:
					if (Path == 1 && Decoder.Icm[Path] == RGB555) {
						assert(0); // Not implemented
					} else {
						switch (Decoder.Icm[Path])
						{
							default:
								break;

							case DYUV:
								x += decodeDYUV(src, dst);
								break;

							case CLUT4:
							case CLUT7:
							case CLUT77: // plane A only
							case CLUT8: // plane A only
								x += decodeCLUT<Path>(src, dst);
								break;
						}
					}
					break;

				case RunLength:
					switch (Decoder.Icm[Path])
					{
						default:
							break;

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
							break;
					}
					break;

				case Mosaic:
					// to-do
					// Decoder.WeightFactor[Path] /= 2;
					assert(0);
					break;
			}
		}

		return vsr;
	}

	template <size_t Path>
	void set_register(uint32_t inst)
	{
		switch ((inst & 0xFF000000u) >> 24)
		{
			default:
				if (((inst & 0xFF000000u) >> 24) >= 0x80u && ((inst & 0xFF000000u) >> 24) < 0xC0u) {
					int index = ((inst & 0xFF000000u) >> 24) + (Decoder.BankCLUT * 64) - 0x80u;
					Decoder.ColorCLUT[index] = inst & 0x00FFFFFFu;
					MiniCDI::Log("[DCA%d] color $%06x", Path, Decoder.ColorCLUT[index]);
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
				Decoder.FT[Path] = (inst & 0b0011u) == 0b11 ? Mosaic : (inst & 0b0011u) == 0b10 ? RunLength : Bitmap;
				Decoder.MF[Path] = (enum VideoCDI::MosaicFactor)((inst & 0b1100u) >> 2);
				Decoder.CM[Path] = (enum VideoCDI::ColorMode)((inst & 0b100000000u) >> 8);
				MiniCDI::Log("[DCA%d] dprm cm=%s,mf=%s,ft=%s", Path, Decoder.CM[Path] == Double4 ? "p4" : "p8",
																Decoder.MF[Path] == x16 ? "x16" : Decoder.MF[Path] == x8 ? "x8"
															  : Decoder.MF[Path] == x4 ? "x4" : "x2",
																Decoder.FT[Path] == Mosaic ? "m" : Decoder.FT[Path] == RunLength ? "rl"
															  : "bmp");
				break;

			case 0xC0:
				Decoder.IcmCS = (inst & 0x00400000u) >> 22;
				Decoder.IcmNR = (inst & 0x00100000u) >> 19;
				Decoder.IcmEV = (inst & 0x00080000u) >> 18;
				Decoder.Icm[1] = (enum VideoCDI::Icm)((inst & 0b111100000000u) >> 8);
				Decoder.Icm[0] = (enum VideoCDI::Icm)(inst & 0b1111u);
				MiniCDI::Log("[DCA%d] icm cs=%d,nr=%d,ev=%d,cma=%s,cmb=%s", Path,
						  Decoder.IcmCS, Decoder.IcmNR, Decoder.IcmEV,
						  Decoder.Icm[0] == CLUT8 ? "clut8" : Decoder.Icm[0] == CLUT7 ? "clut7"
						: Decoder.Icm[0] == CLUT77 ? "clut7+7" : Decoder.Icm[0] == DYUV ? "dyuv"
						: Decoder.Icm[0] == CLUT4 ? "clut4" : "off",
						  Decoder.Icm[1] == RGB555 ? "rgb555" : Decoder.Icm[1] == DYUV ? "dyuv"
						: Decoder.Icm[1] == CLUT4 ? "clut4" : "off");
				break;

			case 0xC2:
				Decoder.PlaneOrder = inst & 0x00FFFFFFu;
				MiniCDI::Log("[DCA%d] po %s", Path, Decoder.PlaneOrder ? "b,a" : "a,b");
				break;

			case 0xC3:
				Decoder.BankCLUT = inst & 0x0Fu;
				MiniCDI::Log("[DCA%d] cbnk %d", Path, Decoder.BankCLUT);
				break;

			case 0xC4:
			case 0xC6:
				Decoder.TransparentCol[Path] = inst & 0x00FFFFFFu;
				break;

			case 0xC7:
			case 0xC9:
				Decoder.MaskCol[Path] = inst & 0x00FFFFFFu;
				break;

			case 0xCA:
			case 0xCB:
				Decoder.ColorDYUV[Path] = inst & 0x00FFFFFFu;
				break;

			case 0xCD: // channel 1
				Decoder.CursorPosition[0] = (inst & 0x00000FFFu) / (FG[0].width < 400 ? 2 : 1); // double-resolution
				Decoder.CursorPosition[1] = (inst >> 12) & 0x00000FFFu;
				MiniCDI::Log("[DCA%d] cpos x=%d,y=%d", Path, Decoder.CursorPosition[0], Decoder.CursorPosition[1]);
				break;

			case 0xCE: // channel 1
				Decoder.CursorColor = inst & 0x000000FFu;
				Decoder.CursorRes = (inst >> 15) & 0x01u;
				Decoder.CursorOffTime = (inst >> 16) & 0x07u;
				Decoder.CursorOnTime = (inst >> 19) & 0x07u;
				Decoder.CursorBlink = (inst >> 22) & 0x01u;
				Decoder.CursorEnable = (inst >> 23) & 0x01u;
				break;

			case 0xCF: // channel 1
				Decoder.CursorPatternX = inst & 0x0000FFFFu;
				Decoder.CursorPatternY = (inst >> 16) & 0x0Fu;
				for (uint8_t x = 0; x < 16; x++) {
					cursor[(Decoder.CursorPatternY*16)+x] = (Decoder.CursorPatternX >> (15-x) & 0x01) != 0 ? 0xFF : 0x00;
				}
				break;

			case 0xD8: // channel 1
				Decoder.BackdropColor = inst & 0x00FFFFFFu;
				break;

			case 0xDB:
			case 0xDC:
				Decoder.WeightFactor[Path] = inst & 0x0000001Fu;
				break;
		}
	}
};

}

#undef YUV_GET_Y
#undef YUV_GET_U
#undef YUV_GET_V

#endif