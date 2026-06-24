#ifndef MINICDI_MCD212
#define MINICDI_MCD212


/*****
  DISCLAIMER:
  Sourced from official documentation of MCD212 by Motorola.
 *****/

#define MCD212_VSYNC_LINES		(FD ? 262 : 312)
#define MCD212_HSYNC_CYCLES		(CF ? 120 : 112)
#define MCD212_INACTIVE_VLINES	(FD ? 262 - 240 : ST ? 312 - 240 : 312 - 280)

class MCD212
{
	SCC68070 *_68070;
	uint8_t* memory;

	class VDSC
	{

		/*****
		  DISCLAIMER:
		  Partially sourced from official documentation of MCD212 by Motorola and
		  MAME CD-i driver by Ryan Holtz and Vincent Halver (licensed under BSD-3-Clause).
		 *****/

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

		std::vector<uint32_t> output;

		struct
		{
			/* 80 */ uint32_t ColorCLUT[256];
			/* C0 */ enum Icm Icm[2];
					 uint8_t IcmCS, MatteCount, ExternalVideo; // Dual CLUT bank select (0-1 or 2-3); enable external video
			/* C1 */ uint8_t TransparencyCtrl[2]; uint8_t Mixing;
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
			/* D0 */ uint32_t MCR[8];
			/* D8 */ uint8_t BackdropColor;
			/* D9 */ uint32_t MosaicPixel[2];
			/* DB */ float ICF[2];

			enum MosaicFactor MF[2];
			enum FileType FT[2];
			enum ColorMode CM[2];
		} reg;

		bool Matte[2];
		size_t MatteSlots = 0;

		template <size_t Path>
		void matteCheck(size_t x)
		{
			if (MatteSlots == 0 || (reg.MCR[8 - MatteSlots] & 0x3FF) != x) return;

			uint8_t opcode = reg.MCR[8 - MatteSlots] >> 20 & 0x0F;
			float icf = (float)(reg.MCR[8 - MatteSlots] >> 10 & 63) / 63.0f;
			size_t mf = reg.MatteCount ? (MatteSlots <= 4 ? 0 : 1) : reg.MCR[8 - MatteSlots] >> 16 & 0x01;

			switch (opcode) {
				case 0b0000: // end of matte control
					MatteSlots = 0;
					//MiniCDI::Log("[VDSC] matte end");
					return;
				case 0b0100: // change weight of pA
					reg.ICF[0] = icf;
					//MiniCDI::Log("[VDSC] matte changed ICF for planeA: %.02f", reg.ICF[0]);
					break;
				case 0b0110: // change weight of pB
					reg.ICF[1] = icf;
					//MiniCDI::Log("[VDSC] matte changed ICF for planeB: %.02f", reg.ICF[1]);
					break;
				case 0b1000: // reset
					Matte[mf] = false;
					//MiniCDI::Log("[VDSC] matte flag %d unset", Matte[mf]);
					break;
				case 0b1001: // set
					Matte[mf] = true;
					//MiniCDI::Log("[VDSC] matte flag %d set", Matte[mf]);
					break;
				case 0b1100: // reset & change weight of pA
					Matte[mf] = false;
					reg.ICF[0] = icf;
					//MiniCDI::Log("[VDSC] matte flag %d unset + ICF for planeA: %.02f", Matte[mf], reg.ICF[0]);
					break;
				case 0b1101: // set & change weight of pA
					Matte[mf] = true;
					reg.ICF[0] = icf;
					//MiniCDI::Log("[VDSC] matte flag %d set + ICF for planeA: %.02f", Matte[mf], reg.ICF[0]);
					break;
				case 0b1110: // reset & change weight of pB
					Matte[mf] = false;
					reg.ICF[1] = icf;
					//MiniCDI::Log("[VDSC] matte flag %d unset + ICF for planeB: %.02f", Matte[mf], reg.ICF[1]);
					break;
				case 0b1111: // set & change weight of pB
					Matte[mf] = true;
					reg.ICF[1] = icf;
					//MiniCDI::Log("[VDSC] matte flag %d set + ICF for planeB: %.02f", Matte[mf], reg.ICF[1]);
					break;
			}

			MatteSlots--;
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
					return !Path && reg.IcmCS ? std::clamp(*src & 0x7F, 0, 127) + 128 : *src & 0x7F;

				case CLUT8:
					return *src;

				case CLUT4:
					return second ? (Path ? std::clamp(*src & 0x07, 0, 127) + 128 : *src & 0x07)
								  : (Path ? std::clamp(*src >> 4 & 0x07, 0, 127) + 128 : *src >> 4 & 0x07);
			}
		}

		template <size_t Path>
		bool isTransparent(uint8_t* src, bool second = false)
		{
			// Color key boolean (CLUT only)
			bool ColorKey = (Path == 1 && reg.Icm[Path] == RGB555) || reg.Icm[Path] == DYUV ? false
						  : (reg.ColorCLUT[getCLUTindex<Path>(src, second)] & 0xFCFCFC) == reg.TransparentCol[Path]
						  || reg.MaskCol[Path];

			switch (reg.TransparencyCtrl[Path])
			{
				default:
					// assert(0);
					return false;
				case 0b0000:
					return true;
				case 0b0001:
					return ColorKey;
				case 0b0010:
					return reg.Icm[Path] == RGB555 && Path && (*src & 0x8000);
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
					return !(reg.Icm[Path] == RGB555 && Path && (*src & 0x8000));
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

		struct
		{
			uint8_t Y;
			uint8_t U;
			uint8_t V;

			// Use only ONE LUT, for the dequantizer (small enough to fit in host memory, unlike a 16-million RGB LUT).
			// The RGB color is generated dynamically so as to prevent crashing from full RAM usage.
			uint8_t LUT_deq[16] = {0,1,4,9,16,27,44,79,128,177,212,229,240,247,252,255};
		} DYUVDecoder;

		template <size_t Path>
		uint32_t decodeDYUV(uint8_t* src, uint32_t *dst)
		{
			uint8_t DYUV_Y1, DYUV_Y2, DYUV_U2, DYUV_V2;

			DYUV_Y1 = src[0] & 0x0F;
			DYUV_Y2 = src[1] & 0x0F;
			DYUV_U2 = src[0] & 0xF0 >> 4;
			DYUV_V2 = src[1] & 0xF0 >> 4;

			/// Formula adapted from ogarvey's CD-i Image Parser (licensed under MIT).
			/// https://github.com/ogarvey/CD-i-Image-Parser/blob/main/CD-i%20Image%20Parser/Helpers/ImageFormatHelper.cs#L119

			uint8_t Y[2], U[2], V[2];
			Y[0] = (DYUVDecoder.Y + DYUVDecoder.LUT_deq[DYUV_Y1]) % 256;
			Y[1] = (Y[0] + DYUVDecoder.LUT_deq[DYUV_Y2]) % 256;
			U[1] = (DYUVDecoder.U + DYUVDecoder.LUT_deq[DYUV_U2]) % 256;
			V[1] = (DYUVDecoder.V + DYUVDecoder.LUT_deq[DYUV_V2]) % 256;
			U[0] = (DYUVDecoder.U + U[1]) / 2;
			V[0] = (DYUVDecoder.V + V[1]) / 2;

			DYUVDecoder.Y = Y[1];
			DYUVDecoder.U = U[1];
			DYUVDecoder.V = V[1];

			for (size_t i = 0; i < 2; i++) {
				int R = std::clamp((Y[i] * 256 + 351 * (V[i] - 128)) / 256, 0, 255);
				int G = std::clamp(((Y[i] * 256) - (86 * (U[i] - 128) + 179 * (V[i] - 128))) / 256, 0, 255);
				int B = std::clamp((Y[i] * 256 + 444 * (U[i] - 128)) / 256, 0, 255);

				dst[i] = (G << 24) | (B << 16) | (R << 8) | 0xFF;
			}

			return 2;
		}

		template <size_t Path>
		uint32_t decodeRGB555(uint8_t* src, uint32_t *dst)
		{
			if (!isTransparent<Path>(src)) {
				uint8_t r = ((*src >> 10) & 0x1F) << 3;
				uint8_t g = ((*src >> 5) & 0x1F) << 3;
				uint8_t b = (*src & 0x1F) << 3;

				*dst = ((r > 255 ? 255 : r < 0 ? 0 : r) << 24) |
					   ((g > 255 ? 255 : g < 0 ? 0 : g) << 16) |
					   ((b > 255 ? 255 : b < 0 ? 0 : b) << 8) |
					   0xFF;
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
					if (!isTransparent<Path>(src)) dst[0] = (reg.ColorCLUT[getCLUTindex<Path>(src)] << 8) | 0xFF;
					if (!isTransparent<Path>(src)) dst[1] = (reg.ColorCLUT[getCLUTindex<Path>(src, true)] << 8) | 0xFF;
					return 2;
			}
		}

	public:
		uint8_t cursor[16*16];
		Plane FG[2];

		uint32_t* get_display()
		{
			return &output[0];
		}

		int get_display_width()
		{
			return 768;
		}

		void reset()
		{
			output.assign(768 * 280, 0x000000FF); // max bounds
			memset(cursor, 0, sizeof(cursor));
			reg = {0};
		}

		void set_mode(int hRes, int vRes, bool hDouble = false, bool vDouble = false)
		{
			FG[1].width = FG[0].width = hRes * (hDouble || vDouble ? 2 : 1);
			FG[1].height = FG[0].height = vRes * (vDouble ? 2 : 1);

			FG[0].decoded.assign(FG[0].width * FG[0].height, 0);
			FG[1].decoded.assign(FG[1].width * FG[1].height, 0);

			output.assign(768 * 280, 0x000000FF); // max bounds
		}

		/**
		 * @brief  Mixes all planes to the framebuffer.
		 */
		void mix_to_frame(int y)
		{
			#define PLANEA reg.PlaneOrder ? 0 : 1
			#define PLANEB reg.PlaneOrder ? 1 : 0
			#define GET_R(V) ((V) >> 24 & 0xFF)
			#define GET_G(V) ((V) >> 16 & 0xFF)
			#define GET_B(V) ((V) >> 8 & 0xFF)
			#define GET_A(V) ((V) & 0xFF)

			/// per Green Book:
			/// "C = ICF * (C'-16) + 16
			/// where ICF = Image Contribution Factor (between 0 and 1)
			/// 	C = One of the color components, R G or B.
			/// 	C' = The corresponding component after decoding."
			#define ICF_APPLY(C, ICF) (int)((ICF) * (float)((C)-16) + 16.0f)
			#define ICF_MIX(C1, C2, ICF1, ICF2) std::clamp(ICF_APPLY(C1, ICF1) + ICF_APPLY(C2, ICF2) - 16, 0, 255)

			// Output screen coords.
			int outputPixel = (FG[PLANEA].height == 240 ? y + 20 : y) * 768 + (FG[PLANEA].width % 360 == 0 ? 12 : 0);
			for (int x = 0; x < FG[PLANEA].width; x++) {
				int PIXELA = (y*FG[PLANEA].width) + x;
				int PIXELB = (y*FG[PLANEB].width) + x;

				uint8_t rA = GET_R(FG[PLANEA].decoded[PIXELA]),
						gA = GET_G(FG[PLANEA].decoded[PIXELA]),
						bA = GET_B(FG[PLANEA].decoded[PIXELA]),
						rB = GET_R(FG[PLANEB].decoded[PIXELB]),
						gB = GET_G(FG[PLANEB].decoded[PIXELB]),
						bB = GET_B(FG[PLANEB].decoded[PIXELB]);

				if (reg.Icm[0] == Off && reg.Icm[1] == Off)
					output[outputPixel] = 0x101010ff; // TO-DO: Cleaner way of doing this?
				else if (reg.Mixing && (FG[PLANEB].decoded[PIXELB] & 0x000000FF) && (FG[PLANEA].decoded[PIXELA] & 0x000000FF))
					output[outputPixel] = (ICF_MIX(rA, rB, reg.ICF[PLANEA], reg.ICF[PLANEB]) << 24)
										| (ICF_MIX(gA, gB, reg.ICF[PLANEA], reg.ICF[PLANEB]) << 16)
										| (ICF_MIX(bA, bB, reg.ICF[PLANEA], reg.ICF[PLANEB]) << 8)
										| 0xFF;
				else if (FG[PLANEB].decoded[PIXELB] & 0x000000FF)
					output[outputPixel] = (ICF_APPLY(rB, reg.ICF[PLANEB]) << 24)
										| (ICF_APPLY(gB, reg.ICF[PLANEB]) << 16)
										| (ICF_APPLY(bB, reg.ICF[PLANEB]) << 8)
										| 0xFF;
				else if (FG[PLANEA].decoded[PIXELA] & 0x000000FF)
					output[outputPixel] = (ICF_APPLY(rA, reg.ICF[PLANEA]) << 24)
										| (ICF_APPLY(gA, reg.ICF[PLANEA]) << 16)
										| (ICF_APPLY(bA, reg.ICF[PLANEA]) << 8)
										| 0xFF;
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

				if (x >= reg.CursorPosition[0] && x < reg.CursorPosition[0]+16
				 && y >= reg.CursorPosition[1] && y < reg.CursorPosition[1]+16
				 && cursor[(y-reg.CursorPosition[1])*16 + (x-reg.CursorPosition[0])] != 0
				 && reg.CursorEnable)
				{
					switch (reg.CursorColor & 0x07) {
						default: output[outputPixel] = 0x000000ff; break;
						case 0x01: output[outputPixel] = reg.CursorColor & 0x08 ? 0x0000FFff : 0x000080ff; break;
						case 0x02: output[outputPixel] = reg.CursorColor & 0x08 ? 0x00FF00ff : 0x008000ff; break;
						case 0x03: output[outputPixel] = reg.CursorColor & 0x08 ? 0x00FFFFff : 0x008080ff; break;
						case 0x04: output[outputPixel] = reg.CursorColor & 0x08 ? 0xFF0000ff : 0x800000ff; break;
						case 0x05: output[outputPixel] = reg.CursorColor & 0x08 ? 0xFF00FFff : 0x800080ff; break;
						case 0x06: output[outputPixel] = reg.CursorColor & 0x08 ? 0xFFFF00ff : 0x808000ff; break;
						case 0x07: output[outputPixel] = reg.CursorColor & 0x08 ? 0xFFFFFFff : 0x808080ff; break;
					}
				}

				if (FG[PLANEA].width < 400) {
					output[outputPixel+1] = output[outputPixel];
					outputPixel += 2;
				} else {
					outputPixel++;
				}
			}

			#undef PLANEA
			#undef PLANEB
			#undef GET_R
			#undef GET_G
			#undef GET_B
			#undef GET_A
			#undef ICF_APPLY
			#undef ICF_MIX
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
			// reset DYUV to initial values
			DYUVDecoder.Y = 0;
			DYUVDecoder.U = reg.ColorDYUV[Path] >> 8 & 0xFF;
			DYUVDecoder.V = reg.ColorDYUV[Path] & 0xFF;

			if (reg.Icm[Path] == Off) {
				memset(&FG[Path].decoded[(y * FG[Path].width)], 0, FG[Path].width * sizeof(uint32_t));
				return 0;
			}

			for (int x = 0; x < FG[Path].width;)
			{
				matteCheck<Path>(FG[Path].width < 400 ? x*2 : x);
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
									int length = memory[++vsr];

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

			// reset matte flag for plane
			Matte[0] = Matte[1] = false;
			MatteSlots = 0;
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

				case 0xC0: // channel 1
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
					break;

				case 0xC1: // channel 1
					if (!Path) {
						reg.TransparencyCtrl[0] = inst & 0x0Fu;
						reg.TransparencyCtrl[1] = inst >> 8 & 0x0Fu;
						reg.Mixing = !(inst >> 23 & 0x01u);
						//MiniCDI::Log("[VDSC] P%d tctl mx=%d,tca=%02d,tcb=%02d", Path, reg.Mixing, reg.TransparencyCtrl[0], reg.TransparencyCtrl[1]);
					}
					break;

				case 0xC2: // channel 1
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
					reg.TransparentCol[0] = inst & 0x00FCFCFCu;
					break;
				case 0xC6:
					reg.TransparentCol[1] = inst & 0x00FCFCFCu;
					break;

				case 0xC7:
					reg.MaskCol[0] = inst & 0x00FCFCFCu;
					break;
				case 0xC9:
					reg.MaskCol[1] = inst & 0x00FCFCFCu;
					break;

				case 0xCA:
					reg.ColorDYUV[0] = inst & 0x00FFFFFFu;
					//MiniCDI::Log("[VDSC] P0 yuv_b y=$%02x,u=$%02x,v=$%02x", inst >> 16 & 0xFFu, inst >> 8 & 0xFFu, inst & 0xFFu);
					break;
				case 0xCB:
					reg.ColorDYUV[1] = inst & 0x00FFFFFFu;
					//MiniCDI::Log("[VDSC] P1 yuv_b y=$%02x,u=$%02x,v=$%02x", inst >> 16 & 0xFFu, inst >> 8 & 0xFFu, inst & 0xFFu);
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
					if (MatteSlots < 8) {
						reg.MCR[MatteSlots++] = inst & 0x00FFFFFFu;
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
					reg.ICF[0] = (float)(inst & 63) / 63.0f;
					//MiniCDI::Log("[VDSC] P0 wfac_%s %d", inst & 0x2Fu);
					break;
				case 0xDC:
					reg.ICF[1] = (float)(inst & 63) / 63.0f;
					//MiniCDI::Log("[VDSC] P1 wfac_%s %d", inst & 0x2Fu);
					break;
			}
		}
	};
	VDSC vdsc;

	size_t linesV, line;
	bool interlace;

	// internal registers
	uint32_t VSR[2];
	uint32_t DCP[2];

	// bits (components) of internal registers
	uint8_t DA,			/** (Display Active)	1 = fetching information from video memory **/
			PA,			/** (Parity)			0 = even frame; 1 = odd frame (interlaced mode only) **/
			DD,			/** (DTACK Delay)		1 = DTACK active **/
			DD1, DD2,	/**	selects DTack type **/
			TD,			/** (Type DRAM)			0 = 256KB * 4 or 256KB * 16; 1 = 1MB * 4 **/
			ST,			/**	(Standard)			1 = screen height -= 40 in PAL, screen width shifted to 360 in NTSC or bitmap width made to 384 in PAL **/
			BE[2],		/** (Bus Error) **/
			DE,			/** (Display Enable)	1 = DRAM display access and synchronisation output **/
			CF,			/** (Crystal Frequency)	0 = PAL (28MHz); 1 = NTSC (30MHz) **/
			FD,			/** (Frame Duration)	0 = 50fps; 1 = 60fps **/
			SM,			/** (Scan Mode)			0 = non-interlaced; 1 = interlaced **/
			CM[2],		/** (Color Mode)		0 = 8bpp & CLK/4 pixel output; 1 = 4bpp & CLK/2 pixel output **/
			IC[2],		/** (ICA) 0 = corresponding ICA off, 1 = corresponding ICA on **/
			DC[2],		/** (DCA) 0 = corresponding DCA off, 1 = corresponding DCA on **/
			IT[2],		/** (Interrupt) **/
			DI[2],		/** (Disable Interrupts) **/
			MF1[2],		/** (Mosaic Factor) separate for each channel **/
			MF2[2],
			FT1[2],		/** (File Type) separate for each channel **/
			FT2[2];

	template <size_t Path>
	void vsr_set(uint32_t value) {
		VSR[Path] = value & 0x003FFFFFu;
		IC[Path] = 1;
	}

	template <size_t Path>
	void dcp_set(uint32_t value) {
		DCP[Path] = value & 0x003FFFFCu;
		DC[Path] = 1;
	}

	template <size_t Path>
	void ICA_execute()
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
					IT[Path] = 0b01u; if (!DI[Path]) { _68070->interrupt(SCC68070::IPL_INT1, true); }
					break;

				case 0x78: case 0x79: case 0x7a: case 0x7b: case 0x7c: case 0x7d: case 0x7e: case 0x7f: // RELOAD DISPLAY PARAMETERS
					CM[Path] = inst >> 4 & 0b01u;
					MF1[Path] = inst >> 3 & 0b01u;
					MF2[Path] = inst >> 2 & 0b01u;
					FT1[Path] = inst >> 1 & 0b01u;
					FT2[Path] = inst & 0b01u;
					vdsc.set_register<Path>(inst);
					break;

				default:
					vdsc.set_register<Path>(inst);
					break;
			}
		}
	}

	template <size_t Path>
	void DCA_execute()
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
					IT[Path] = 0b01u; if (!DI[Path]) { _68070->interrupt(SCC68070::IPL_INT1, true); }
					break;

				default:
					vdsc.set_register<Path>(inst);
					break;
			}
		}
	}

public:
	MCD212() {}

	MCD212(SCC68070 *_68070, uint8_t *memory) : _68070(_68070), memory(memory)
	{
		reset();
	}

	/**
	 * @brief  Resets the chip.
	 */
	void reset()
	{
		// clear write bits
		DI[0] = DD1 = DD2 = TD = DD = ST = BE[0] = 0;
		DI[1] = 0;
		DE = CF = FD = SM = CM[0] = IC[0] = DC[0] = 0;
		CM[1] = IC[1] = DC[1] = 0;
		MF1[0] = MF2[0] = FT1[0] = FT2[0] = 0;
		MF1[1] = MF2[1] = FT1[1] = FT2[1] = 0;

		// initialization
		CF = FD = MiniCDI::Config::PAL ? 0 : 1;
		SM = /* to-do: interlace */ 0;

		interlace = false;
		linesV = 0;
		line = 0;

		vdsc.reset();
	}

	/**
	 * @brief  Draws a video line.
	 */
	bool tick(bool skip_draw = false)
	{
		if (linesV++ <= MCD212_INACTIVE_VLINES) {
			if (linesV == 1 && DE) {
				if (IC[0]) ICA_execute<0>();
				if (IC[1]) ICA_execute<1>();
			}
			return false;
		}

		if (line == 0) {
			DA = 1;

			if (interlace && SM)
				line = 1;

			if (!skip_draw)
				vdsc.set_mode(!CF || ST ? 360 : 384, FD || (!FD && ST) ? 240 : 280, CM[1]);
		}

		if (DE) {
			if (!skip_draw) {
				// render line onto bitmap
				VSR[0] = vdsc.draw_line_to_plane<0>(memory, VSR[0], line);
				VSR[1] = vdsc.draw_line_to_plane<1>(memory, VSR[1], line);
				vdsc.mix_to_frame(line);
			}

			if (DC[0] && IC[0]) DCA_execute<0>();
			if (DC[1] && IC[1]) DCA_execute<1>();
		}

		line += SM ? 2 : 1;

		if (linesV >= MCD212_VSYNC_LINES) {
			// MiniCDI::Log("[MCD212] VSYNC");
			DA = 0;
			PA ^= 1;

			linesV = 0;
			line = 0;
			interlace = SM ? !interlace : false;

			return true;
		}

		return false;
	}

	uint8_t read8(uint32_t addr)
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
				uint8_t value = BE[0] | (IT[1] << 1) | (IT[0] << 2);
				BE[0] = IT[1] = IT[0] = 0;
				_68070->interrupt(SCC68070::IPL_INT1, false);
				return value;
		}
	}

	uint16_t read16(uint32_t addr)
	{
		switch (addr)
		{
			default:
				return (memory[addr] << 8) | memory[addr+1];
			case 0x4FFFF0: // CSR1R
				return 0xFF00 | (PA << 5) | (DA << 7);
			case 0x4FFFE0: // CSR2R
				uint8_t value = BE[0] | (IT[1] << 1) | (IT[0] << 2);
				BE[0] = IT[1] = IT[0] = 0;
				_68070->interrupt(SCC68070::IPL_INT1, false);
				return 0xFF00 | value;
		}
	}

	void write16(uint32_t addr, uint16_t value)
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
				if (DI[0] && IT[0]) _68070->interrupt(SCC68070::IPL_INT1, false);
				break;
			case 0x4FFFE0: // CSR2W
				DI[1] = value >> 15 & 0b01u;
				if (DI[1] && IT[1]) _68070->interrupt(SCC68070::IPL_INT1, false);
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
				FT2[0] = value >> 7 & 0b01u;
				FT1[0] = value >> 8 & 0b01u;
				MF2[0] = value >> 9 & 0b01u;
				MF1[0] = value >> 10 & 0b01u;

				DCP[0] &= 0x0000FFFFu;
				DCP[0] |= (value & 0x3Fu) << 8;
				break;
			case 0x4FFFE8: // DDR2
				FT2[1] = value >> 7 & 0b01u;
				FT1[1] = value >> 8 & 0b01u;
				MF2[1] = value >> 9 & 0b01u;
				MF1[1] = value >> 10 & 0b01u;

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

	uint32_t* get_display()
	{
		return vdsc.get_display();
	}

	size_t get_display_width()
	{
		return vdsc.get_display_width();
	}
};

#endif