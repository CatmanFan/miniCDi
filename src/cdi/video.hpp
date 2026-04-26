#ifndef MINICDI_VDSC
#define MINICDI_VDSC

#include <vector>

#define MAX_DISPLAY_WIDTH 384
#define MAX_DISPLAY_HEIGHT 240

namespace VideoCDI

{

enum IcmA
{
	aOff = 0x00,
	aCLUT8 = 0x01,
	aCLUT7 = 0x03,
	aCLUT77 = 0x04,
	aDYUV = 0x05,
	aCLUT4 = 0x0B // double res
};

enum IcmB
{
	bOff = 0x00,
	bRGB555 = 0x01,
	bDYUV = 0x05,
	bCLUT4 = 0x0B // double res
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
};

class Video
{
	Plane cursor, planes[2], bg;
	std::vector<uint32_t> output;

	struct
	{
		/* 80 */ uint32_t ColorCLUT[64]; // A CLUT is a color lookup table holding a number of colors in RGB888.
		/* C0 */ uint8_t IcmCS, IcmNR, IcmEV;
				 enum IcmA IcmA;
				 enum IcmB IcmB;
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
		/* CF */ uint8_t CursorPattern;
		/* D0 */ uint8_t RegionControl[8];
		/* D8 */ uint8_t BackdropColor;
		/* D9 */ uint8_t MosaicPixel[2];
		/* DB */ uint8_t WeightFactor[2];

		enum MosaicFactor MF[2];
		enum FileType FT[2];
		enum ColorMode CM[2];
	} VdscConfig;

public:
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

	void reset()
	{
		cursor.width = cursor.height = 16;
		VdscConfig = {0};
	}

	/** Draws planes and corresponding data/parameters. **/
	void draw(enum Type type, enum Resolution res, size_t line)
	{
		planes[0].width = (type == NTSCMonitor ? 360 : 384) * (res == NormalRes ? 1 : 2);
		planes[0].height = (type == PAL ? 280 : 240) * (res == HighRes ? 2 : 1);
		planes[1].width = planes[0].width;
		planes[1].height = planes[0].height;

		if (output.size() != planes[0].width * planes[0].height)
		{
			output.resize(planes[0].width * planes[0].height, 0x000000ff);
		}

		for (size_t i = line * planes[0].width; i < (line+1) * planes[0].width; i++)
		{
			switch (VdscConfig.FT[0])
			{
				default:
					break;

				case RunLength:
					output[i] = (VdscConfig.ColorCLUT[0] << 8) | 0xff;
					break;
			}
		}
	}

	void process(uint32_t inst, bool pathB)
	{
		switch ((inst & 0xFF000000u) >> 24)
		{
			default:
				if (((inst & 0xFF000000u) >> 24) >= 0x80u && ((inst & 0xFF000000u) >> 24) <= 0xBFu) {
					VdscConfig.ColorCLUT[((inst & 0xFF000000u) >> 24) - 0x80u] = inst & 0x00FFFFFFu;
					printf("[DCA%d] color $%06x\n", pathB, VdscConfig.ColorCLUT[((inst & 0xFF000000u) >> 24) - 0x80u]);
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
				VdscConfig.FT[pathB] = (inst & 0b0011u) == 0b11 ? Mosaic : (inst & 0b0011u) == 0b10 ? RunLength : Bitmap;
				VdscConfig.MF[pathB] = (enum VideoCDI::MosaicFactor)((inst & 0b1100u) >> 2);
				VdscConfig.CM[pathB] = (enum VideoCDI::ColorMode)((inst & 0b100000000u) >> 8);
				printf("[DCA%d] dprm cm=%s,mf=%s,ft=%s\n", pathB, VdscConfig.CM[pathB] == Double4 ? "p4" : "p8",
																	VdscConfig.MF[pathB] == x16 ? "x16" : VdscConfig.MF[pathB] == x8 ? "x8"
																  : VdscConfig.MF[pathB] == x4 ? "x4" : "x2",
																	VdscConfig.FT[pathB] == Mosaic ? "m" : VdscConfig.FT[pathB] == RunLength ? "rl"
																  : "bmp");
				break;

			case 0xC0:
				VdscConfig.IcmCS = (inst & 0x00400000u) >> 22;
				VdscConfig.IcmNR = (inst & 0x00100000u) >> 19;
				VdscConfig.IcmEV = (inst & 0x00080000u) >> 18;
				VdscConfig.IcmB = (enum VideoCDI::IcmB)((inst & 0b111100000000u) >> 8);
				VdscConfig.IcmA = (enum VideoCDI::IcmA)(inst & 0b1111u);
				printf("[DCA%d] icm cs=%d,nr=%d,ev=%d,cma=%s,cmb=%s\n", pathB,
						  VdscConfig.IcmCS, VdscConfig.IcmNR, VdscConfig.IcmEV,
						  VdscConfig.IcmA == aCLUT8 ? "clut8" : VdscConfig.IcmA == aCLUT7 ? "clut7"
						: VdscConfig.IcmA == aCLUT77 ? "clut7+7" : VdscConfig.IcmA == aDYUV ? "dyuv"
						: VdscConfig.IcmA == aCLUT4 ? "clut4" : "off",
						  VdscConfig.IcmB == bRGB555 ? "rgb555" : VdscConfig.IcmB == bDYUV ? "dyuv"
						: VdscConfig.IcmB == bCLUT4 ? "clut4" : "off");
				break;

			case 0xC2:
				VdscConfig.PlaneOrder = inst & 0x00FFFFFFu;
				printf("[DCA%d] po %s\n", pathB, VdscConfig.PlaneOrder ? "b,a" : "a,b");
				break;

			case 0xC3:
				VdscConfig.BankCLUT = inst & 0x00FFFFFFu;
				printf("[DCA%d] cbnk %d\n", pathB, VdscConfig.BankCLUT);
				break;

			case 0xC4:
			case 0xC6:
				VdscConfig.TransparentCol[pathB] = inst & 0x00FFFFFFu;
				break;

			case 0xC7:
			case 0xC9:
				VdscConfig.MaskCol[pathB] = inst & 0x00FFFFFFu;
				break;

			case 0xCA:
			case 0xCB:
				VdscConfig.ColorDYUV[pathB] = inst & 0x00FFFFFFu;
				break;

			case 0xCD: // channel 1
				VdscConfig.CursorPosition[0] = inst & 0x00000FFFu; // double-resolution
				VdscConfig.CursorPosition[1] = (inst & 0x00FFF000u) >> 12;
				printf("[DCA%d] cpos x=%d,y=%d\n", pathB, VdscConfig.CursorPosition[0], VdscConfig.CursorPosition[1]);
				break;

			case 0xCE: // channel 1
				VdscConfig.CursorColor = inst & 0x000000FFu;
				VdscConfig.CursorBlink = (inst >> 22) & 0b01u;
				VdscConfig.CursorEnable = (inst >> 23) & 0b01u;
				break;
		}
	}
};

}

#endif