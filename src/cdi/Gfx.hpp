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

#define MAX_DISPLAY_WIDTH 384
#define MAX_DISPLAY_HEIGHT 240

#define YUV_GET_Y(x) ((x >> 8) & 0x0F)
#define YUV_GET_U(x) ((x >> 12) & 0x0F)
#define YUV_GET_V(x) ((x >> 4) & 0x0F)

namespace VideoCDI
{

enum Icm
{
	Off = 0x00,
	CLUT8 = 0x01,
	CLUT7 = 0x03,
	CLUT77 = 0x04,
	DYUV = 0x05,
	CLUT4 = 0x0B, // double res
	RGB555 = 0x01, // path 2 only
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
	size_t width, height;
	bool doubleRes;
	std::vector<uint32_t> decoded;
};

class Video
{
	std::vector<uint32_t> output;

	struct
	{
		/* 80 */ uint32_t ColorCLUT[64]; // A CLUT is a color lookup table holding a number of colors in RGB888.
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
		/* CE */ uint8_t CursorEnable, CursorBlink, CursorColor;
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

	uint32_t YUVtoRGB(uint16_t v)
	{
		int r = std::floor((YUV_GET_Y(v)*256 + 351*(YUV_GET_V(v)-128) ) / 256 );
		int g = std::floor(((YUV_GET_Y(v)*256)*(86*(YUV_GET_U(v)-128) + 179*(YUV_GET_V(v)-128))) / 256 );
		int b = std::floor(( YUV_GET_Y(v)*256 + 444*(YUV_GET_U(v)-128) ) / 256);

		return ((r > 255 ? 255 : r < 0 ? 0 : r) << 24 |
				(g > 255 ? 255 : g < 0 ? 0 : g) << 16 |
				(b > 255 ? 255 : b < 0 ? 0 : b) << 8);
	}

public:
	Plane cursor, FG[2], BG;

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

	size_t get_display_width()
	{
		return FG[0].width;
	}

	void reset()
	{
		cursor.width = cursor.height = 16;
		Decoder = {0};
	}

	void set_mode(enum Type type, bool hRes1, bool hRes2, bool vRes = false)
	{
		FG[0].doubleRes = hRes1;
		FG[1].doubleRes = hRes2;

		BG.width = FG[1].width = FG[0].width = type == NTSCMonitor ? 720 : 768;
		BG.height = FG[1].height = FG[0].height = (type == PAL ? 280 : 240) * (vRes ? 2 : 1);

		if (output.size() != FG[0].width * FG[0].height) {
			BG.decoded.resize(BG.width * BG.height, 0);
			FG[0].decoded.resize(FG[0].width * FG[0].height, 0);
			FG[1].decoded.resize(FG[1].width * FG[1].height, 0);
			output.resize(FG[0].width * FG[0].height, 0x000000ff);
		}
	}

	void draw_frame()
	{
		for (size_t y = 0; y < FG[0].width; y++) {
			for (size_t x = 0; x < FG[0].width; x++) {
				// TO-DO: draw backdrop color if both planes are transparent.
				// Backdrop should just be a single solid color, cursor is handled by byte pattern so they can be drawn directly in this function.

				uint8_t rA, gA, bA, rB, gB, bB, rAB, gAB, bAB;

				rA = (FG[0].decoded[(y*x)+x] & 0xff000000) >> 24;
				gA = (FG[0].decoded[(y*x)+x] & 0x00ff0000) >> 16;
				bA = (FG[0].decoded[(y*x)+x] & 0x0000ff00) >> 8;

				rB = (FG[0].decoded[(y*x)+x] & 0xff000000) >> 24;
				gB = (FG[0].decoded[(y*x)+x] & 0x00ff0000) >> 16;
				bB = (FG[0].decoded[(y*x)+x] & 0x0000ff00) >> 8;

				rAB = std::clamp((rA * (Decoder.WeightFactor[0]/64)) + (rB * (Decoder.WeightFactor[1]/64)) + 16, 0, 255);
				gAB = std::clamp((gA * (Decoder.WeightFactor[0]/64)) + (gB * (Decoder.WeightFactor[1]/64)) + 16, 0, 255);
				bAB = std::clamp((bA * (Decoder.WeightFactor[0]/64)) + (bB * (Decoder.WeightFactor[1]/64)) + 16, 0, 255);

				output[(y*x)+x] = (rAB << 24) | (gAB << 16) | (bAB << 8) | 0xff;
			}
		}
	}

	/** Draws planes and corresponding data/parameters. **/
	template <size_t Path>
	void draw_line_to_plane(uint8_t* memory, uint32_t vsr, size_t line)
	{
		size_t pixel = 0, dest = 0;
		do
		{
			if (Decoder.Icm[Path] == Off) {
				FG[Path].decoded[(FG[Path].width*line) + dest] = 0;
				FG[Path].decoded[(FG[Path].width*line) + dest+1] = 0;
			} else {
				const uint8_t byte = memory[((vsr + pixel) & 0x0007ffff) ^ 1];
				uint32_t color0 = 0, color1 = 0;

				switch (Decoder.FT[Path]) {
					default:
						break;

					case Bitmap:
						switch (Decoder.Icm[Path])
						{
							default:
								break;
							case DYUV:
								// Decoder.ColorDYUV[Path];
								break;
						}
						break;

					case RunLength:
						switch (Decoder.Icm[Path])
						{
							default:
								break;
							case CLUT4:
								color0 = (Decoder.ColorCLUT[(byte >> 4) & 0x0F] << 8) | 0xff;
								color1 = (Decoder.ColorCLUT[byte & 0x0F] << 8) | 0xff;
								break;
							case CLUT7:
								color0 = color1 = (Decoder.ColorCLUT[byte/* & 0x7F */] << 8) | 0xff;
								break;
							case CLUT8:
								color0 = color1 = (Decoder.ColorCLUT[byte] << 8) | 0xff;
								break;
						}
						break;

					case Mosaic:
						// to-do
						Decoder.WeightFactor[Path] /= 2;
						assert(0);
						break;
				}

				FG[Path].decoded[(FG[Path].width*line) + dest] = color0;
				FG[Path].decoded[(FG[Path].width*line) + dest+1] = color1;
			}

			pixel += (FG[Path].doubleRes ? 2 : 1);
			dest += 2;
		} while (dest < FG[Path].width);
	}

	void draw_line(size_t line, uint8_t* memory, const uint32_t vsr1, const uint32_t vsr2)
	{
		// Backdrop
		uint32_t bgColor = 0x000000ff;

		if (Decoder.BackdropColor & 0b1001)
			bgColor |= 0x0000ff00;
		else if (Decoder.BackdropColor & 0b0001)
			bgColor |= 0x00008000;

		if (Decoder.BackdropColor & 0b1010)
			bgColor |= 0x00ff0000;
		else if (Decoder.BackdropColor & 0b0010)
			bgColor |= 0x00800000;

		if (Decoder.BackdropColor & 0b1100)
			bgColor |= 0xff000000;
		else if (Decoder.BackdropColor & 0b0100)
			bgColor |= 0x80000000;

		std::fill(BG.decoded.begin() + (BG.width*(line)), BG.decoded.begin() + (BG.width*(line+1)), bgColor);

		// Planes 1 and 2
		draw_line_to_plane<0>(memory, vsr1, line);
		draw_line_to_plane<1>(memory, vsr2, line);
	}

	void process(uint32_t inst, bool pathB)
	{
		switch ((inst & 0xFF000000u) >> 24)
		{
			default:
				if (((inst & 0xFF000000u) >> 24) >= 0x80u && ((inst & 0xFF000000u) >> 24) <= 0xBFu) {
					Decoder.ColorCLUT[((inst & 0xFF000000u) >> 24) - 0x80u] = inst & 0x00FFFFFFu;
					printf("[DCA%d] color $%06x\n", pathB, Decoder.ColorCLUT[((inst & 0xFF000000u) >> 24) - 0x80u]);
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
				Decoder.FT[pathB] = (inst & 0b0011u) == 0b11 ? Mosaic : (inst & 0b0011u) == 0b10 ? RunLength : Bitmap;
				Decoder.MF[pathB] = (enum VideoCDI::MosaicFactor)((inst & 0b1100u) >> 2);
				Decoder.CM[pathB] = (enum VideoCDI::ColorMode)((inst & 0b100000000u) >> 8);
				printf("[DCA%d] dprm cm=%s,mf=%s,ft=%s\n", pathB, Decoder.CM[pathB] == Double4 ? "p4" : "p8",
																	Decoder.MF[pathB] == x16 ? "x16" : Decoder.MF[pathB] == x8 ? "x8"
																  : Decoder.MF[pathB] == x4 ? "x4" : "x2",
																	Decoder.FT[pathB] == Mosaic ? "m" : Decoder.FT[pathB] == RunLength ? "rl"
																  : "bmp");
				break;

			case 0xC0:
				Decoder.IcmCS = (inst & 0x00400000u) >> 22;
				Decoder.IcmNR = (inst & 0x00100000u) >> 19;
				Decoder.IcmEV = (inst & 0x00080000u) >> 18;
				Decoder.Icm[1] = (enum VideoCDI::Icm)((inst & 0b111100000000u) >> 8);
				Decoder.Icm[0] = (enum VideoCDI::Icm)(inst & 0b1111u);
				printf("[DCA%d] icm cs=%d,nr=%d,ev=%d,cma=%s,cmb=%s\n", pathB,
						  Decoder.IcmCS, Decoder.IcmNR, Decoder.IcmEV,
						  Decoder.Icm[0] == CLUT8 ? "clut8" : Decoder.Icm[0] == CLUT7 ? "clut7"
						: Decoder.Icm[0] == CLUT77 ? "clut7+7" : Decoder.Icm[0] == DYUV ? "dyuv"
						: Decoder.Icm[0] == CLUT4 ? "clut4" : "off",
						  Decoder.Icm[1] == RGB555 ? "rgb555" : Decoder.Icm[1] == DYUV ? "dyuv"
						: Decoder.Icm[1] == CLUT4 ? "clut4" : "off");
				break;

			case 0xC2:
				Decoder.PlaneOrder = inst & 0x00FFFFFFu;
				printf("[DCA%d] po %s\n", pathB, Decoder.PlaneOrder ? "b,a" : "a,b");
				break;

			case 0xC3:
				Decoder.BankCLUT = inst & 0x00FFFFFFu;
				printf("[DCA%d] cbnk %d\n", pathB, Decoder.BankCLUT);
				break;

			case 0xC4:
			case 0xC6:
				Decoder.TransparentCol[pathB] = inst & 0x00FFFFFFu;
				break;

			case 0xC7:
			case 0xC9:
				Decoder.MaskCol[pathB] = inst & 0x00FFFFFFu;
				break;

			case 0xCA:
			case 0xCB:
				Decoder.ColorDYUV[pathB] = inst & 0x00FFFFFFu;
				break;

			case 0xCD: // channel 1
				Decoder.CursorPosition[0] = inst & 0x00000FFFu; // double-resolution
				Decoder.CursorPosition[1] = (inst & 0x00FFF000u) >> 12;
				printf("[DCA%d] cpos x=%d,y=%d\n", pathB, Decoder.CursorPosition[0], Decoder.CursorPosition[1]);
				break;

			case 0xCE: // channel 1
				Decoder.CursorColor = inst & 0x000000FFu;
				Decoder.CursorBlink = (inst >> 22) & 0b01u;
				Decoder.CursorEnable = (inst >> 23) & 0b01u;
				break;

			case 0xCF: // channel 1
				Decoder.CursorPatternX = inst & 0x0000FFFFu;
				Decoder.CursorPatternY = (inst >> 16) & 0x0Fu;
				break;

			case 0xD8: // channel 1
				Decoder.BackdropColor = inst & 0x00FFFFFFu;
				break;

			case 0xDB:
			case 0xDC:
				Decoder.WeightFactor[pathB] = inst & 0x0000001Fu;
				break;
		}
	}
};

}

#undef YUV_GET_Y
#undef YUV_GET_U
#undef YUV_GET_V

#endif