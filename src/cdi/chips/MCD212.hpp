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
	SCC68070* _68070;
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
			Off = 0b0000,
			CLUT8 = 0b0001, // plane A only
			RGB555 = 0b0001, // plane B only
			CLUT7 = 0b0011,
			CLUT77 = 0b0100, // plane A only
			DYUV = 0b0101,
			CLUT4 = 0b1011, // double-res only
			QHY = 0b1111
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
			uint32_t decoded[768 * 280]; // max bounds
		};

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
			/* D0 */ // see below (MCR struct)
			/* D8 */ uint8_t BackdropColor;
			/* D9 */ uint32_t MosaicPixel[2];
			/* DB */ float ICF[2];

			enum MosaicFactor MF[2];
			enum FileType FT[2];
			enum ColorMode CM[2];
		} reg;

		bool Matte[2];
		struct {
			size_t current = 0;
			size_t x[8];
			float icf[8];
			size_t mf[8];
			uint8_t opcode[8];
		} MCR;

		template <size_t Path>
		void matte_set_flag(size_t x)
		{
			if (MCR.current >= 8 || MCR.x[MCR.current] != x) return;

			switch (MCR.opcode[MCR.current]) {
				case 0b0000: // end of matte control
					MCR.current = 8;
					return;
				case 0b1000: // reset
				case 0b1100: // reset & change weight of pA
				case 0b1110: // reset & change weight of pB
					Matte[MCR.mf[MCR.current++]] = false;
					return;
				case 0b1001: // set
				case 0b1101: // set & change weight of pA
				case 0b1111: // set & change weight of pB
					Matte[MCR.mf[MCR.current++]] = true;
					return;
			}
		}

		void matte_set_icf(size_t x)
		{
			if (MCR.current >= 8 || MCR.x[MCR.current] != x) return;

			switch (MCR.opcode[MCR.current]) {
				case 0b0000: // end of matte control
					MCR.current = 8;
					return;
				case 0b0100: // change weight of pA
				case 0b1100: // reset & change weight of pA
				case 0b1101: // set & change weight of pA
					reg.ICF[0] = MCR.icf[MCR.current++];
					return;
				case 0b0110: // change weight of pB
				case 0b1110: // reset & change weight of pB
				case 0b1111: // set & change weight of pB
					reg.ICF[1] = MCR.icf[MCR.current++];
					return;
			}
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
					return second ? (Path ? std::clamp(*src & (reg.FT[Path] == Bitmap ? 0x0F : 0x07), 0, 127) + 128
										  : *src & (reg.FT[Path] == Bitmap ? 0x0F : 0x07))
								  : (Path ? std::clamp(*src >> 4 & (reg.FT[Path] == Bitmap ? 0x0F : 0x07), 0, 127) + 128
										  : *src >> 4 & (reg.FT[Path] == Bitmap ? 0x0F : 0x07));
			}
		}

		template <size_t Path>
		bool isTransparent(uint8_t* src, bool second = false)
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
					assert(0);
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

		struct
		{
			uint8_t Y = 0;
			uint8_t U = 0;
			uint8_t V = 0;

			/// Table 7–1 in MCD212 datasheet
			uint8_t LUT_deq[16] = {0,1,4,9,16,27,44,79,128,177,212,229,240,247,252,255};
		} DYUVDecoder;

		template <size_t Path>
		uint32_t decodeDYUV(uint8_t* src, uint32_t *dst)
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
			for (size_t i = 0; i < 2; i++) {
				/// Green Book V.4.4.2.2
				int B = std::clamp((int)((float)Y[i] + (float)(U[i] - 128) * 1.733f), 0, 255);
				int R = std::clamp((int)((float)Y[i] + (float)(V[i] - 128) * 1.371f), 0, 255);
				int G = std::clamp((int)(((float)Y[i] - 0.299f * (float)R - 0.114f * (float)B) / 0.587f), 0, 255);

				dst[i] = (R << 24) | (G << 16) | (B << 8) | 0xFF;
			}

			return 2;
		}

		template <size_t Path>
		uint32_t decodeRGB555(uint8_t* src, uint32_t *dst)
		{
			if (!isTransparent<Path>(src)) {
				uint16_t rgb555 = (*src << 8) | *(src+1);
				uint8_t r = std::clamp((rgb555 & 0b0111110000000000) >> 7, 0, 255);
				uint8_t g = std::clamp((rgb555 & 0b0000001111100000) >> 2, 0, 255);
				uint8_t b = std::clamp((rgb555 & 0b0000000000011111) << 3, 0, 255);

				*dst = (r << 24) | (g << 16) | (b << 8) | 0xFF;
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
					if (!isTransparent<Path>(src, true)) dst[1] = (reg.ColorCLUT[getCLUTindex<Path>(src, true)] << 8) | 0xFF;
					return 2;
			}
		}

		uint32_t framebuffer[768 * 280]; // max bounds

	public:
		uint8_t cursor[16*16];
		Plane FG[2];

		uint32_t* get_display()
		{
			return &framebuffer[0];
		}

		int get_display_width()
		{
			return 768;
		}

		void reset()
		{
			memset(cursor, 0, sizeof(cursor));
			reg = {0};
		}

		void set_mode(int hRes, int vRes, bool hDouble = false, bool vDouble = false)
		{
			if (reg.Icm[0] == CLUT4 || reg.Icm[1] == CLUT4) hDouble = true;

			memset(FG[0].decoded, 0, sizeof(FG[0].decoded));
			memset(FG[1].decoded, 0, sizeof(FG[1].decoded));
			memset(framebuffer, 0xFF000000, sizeof(framebuffer));

			FG[1].width = FG[0].width = hRes * (hDouble || vDouble ? 2 : 1);
			FG[1].height = FG[0].height = vRes * (vDouble ? 2 : 1);
		}

		/**
		 * @brief  Mixes all planes to the framebuffer.
		 */
		void mix_to_frame(int y)
		{
			#define PLANEA reg.PlaneOrder ? 0 : 1
			#define PLANEB reg.PlaneOrder ? 1 : 0

			/// per Green Book:
			/// "C = ICF * (C'-16) + 16
			/// where ICF = Image Contribution Factor (between 0 and 1)
			/// 	C = One of the color components, R G or B.
			/// 	C' = The corresponding component after decoding."
			#define ICF_APPLY(C, ICF) (int)((ICF) * ((float)(C)-16.0f) + 16.0f)

			/// MCD212 datasheet states that the color has to be first combined with the black level of 16 (MUX: BL_A or BL_B).
			#define CLAMP_TO_16(C) ((C) < 16 ? 16 : (C))

			// Reset matte ICF and count
			MCR.current = 0;

			// Output screen coords.
			int fb_xy = (FG[0].height == 240 ? y + 20 : y) * 768 + (FG[0].width % 360 == 0 ? 24 : 0);
			for (int x = 0; x < FG[0].width; x++)
			{
				matte_set_icf(FG[0].width < 400 ? (x > 0 ? x+1 : x)*2 : x);
				uint8_t rA = ICF_APPLY(CLAMP_TO_16(FG[PLANEA].decoded[(y*768)+x] >> 24 & 0xFF), reg.ICF[PLANEA]),
						gA = ICF_APPLY(CLAMP_TO_16(FG[PLANEA].decoded[(y*768)+x] >> 16 & 0xFF), reg.ICF[PLANEA]),
						bA = ICF_APPLY(CLAMP_TO_16(FG[PLANEA].decoded[(y*768)+x] >> 8 & 0xFF), reg.ICF[PLANEA]),
						aA = FG[PLANEA].decoded[(y*768)+x] & 0xFF,
						rB = ICF_APPLY(CLAMP_TO_16(FG[PLANEB].decoded[(y*768)+x] >> 24 & 0xFF), reg.ICF[PLANEB]),
						gB = ICF_APPLY(CLAMP_TO_16(FG[PLANEB].decoded[(y*768)+x] >> 16 & 0xFF), reg.ICF[PLANEB]),
						bB = ICF_APPLY(CLAMP_TO_16(FG[PLANEB].decoded[(y*768)+x] >> 8 & 0xFF), reg.ICF[PLANEB]),
						aB = FG[PLANEB].decoded[(y*768)+x] & 0xFF;

				if (reg.Icm[0] == Off && reg.Icm[1] == Off)
					framebuffer[fb_xy] = 0x101010ff;
				else if (reg.Mixing && aA && aB)
					framebuffer[fb_xy] = (std::clamp(rA + rB - 16, 0, 255) << 24)
										| (std::clamp(gA + gB - 16, 0, 255) << 16)
										| (std::clamp(bA + bB - 16, 0, 255) << 8)
										| 0xFF;
				else if (aB)
					framebuffer[fb_xy] = (rB << 24) | (gB << 16) | (bB << 8) | 0xFF;
				else if (aA)
					framebuffer[fb_xy] = (rA << 24) | (gA << 16) | (bA << 8) | 0xFF;
				else {
					// Transparent, draw backdrop.
					switch (reg.BackdropColor & 0x07) {
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
					switch (reg.CursorColor & 0x07) {
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

				if (MiniCDI::Config::AnalogColors) {
					/// Subtract to get the analog output (per Green Book 4.4.1.2).
					framebuffer[fb_xy] = (std::clamp((int)((((int)framebuffer[fb_xy] >> 24 & 0x000000FF) - 16) / 219.0f * 219.0f), 0, 255) << 24)
										| (std::clamp((int)((((int)framebuffer[fb_xy] >> 16 & 0x000000FF) - 16) / 219.0f * 219.0f), 0, 255) << 16)
										| (std::clamp((int)((((int)framebuffer[fb_xy] >> 8 & 0x000000FF) - 16) / 219.0f * 219.0f), 0, 255) << 8)
										| (framebuffer[fb_xy] & 0x000000FF);
				}

				if (FG[0].width < 400) {
					framebuffer[fb_xy+1] = framebuffer[fb_xy];
					fb_xy += 2;
				} else {
					fb_xy++;
				}
			}

			#undef ICF_APPLY
			#undef CLAMP_TO_16

			#undef PLANEA
			#undef PLANEB
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
			// Reset matte flags and count
			Matte[0] = Matte[1] = false;
			MCR.current = 0;

			// reset DYUV to initial values
			DYUVDecoder.Y = reg.ColorDYUV[Path] >> 16 & 0xFF;
			DYUVDecoder.U = reg.ColorDYUV[Path] >> 8 & 0xFF;
			DYUVDecoder.V = reg.ColorDYUV[Path] & 0xFF;

			if (reg.Icm[Path] == Off) {
				memset(&FG[Path].decoded[(y * 768)], 0, FG[Path].width * sizeof(uint32_t));
				return 0;
			}

			for (int x = 0; x < FG[Path].width;)
			{
				matte_set_flag<Path>(FG[0].width < 400 ? (x > 0 ? x+1 : x)*2 : x);
				uint8_t* src = &memory[vsr];
				uint32_t* dst = &FG[Path].decoded[(y * 768) + x];

				switch (reg.FT[Path]) {
					default:
					case Bitmap:
						if (reg.Icm[0] == Off && reg.Icm[1] == RGB555) {
							x += decodeRGB555<Path>(src, dst);
							vsr++;
							continue;
						} else {
							switch (reg.Icm[Path])
							{
								default:
									assert(0);
									vsr++;
									continue;

								case DYUV:
									x += decodeDYUV<Path>(src, dst);
									vsr += 2;
									continue;

								case CLUT4:
								case CLUT7:
								case CLUT77:
								case CLUT8:
									x += decodeCLUT<Path>(src, dst);
									vsr++;
									continue;
							}
						}

					case RunLength:
						switch (reg.Icm[Path])
						{
							default:
							case CLUT4: // RL3
							case CLUT7: // RL7
								int length = (*src & 0x80 ? memory[vsr+1] : 1) * (reg.Icm[Path] == CLUT4 ? 2 : 1);
								int endX = length == 0 ? FG[Path].width : std::min({x+length, FG[Path].width});
								while (x < endX) {
									x += decodeCLUT<Path>(src, &FG[Path].decoded[(y * 768) + x]);
								}
								vsr += (*src & 0x80 ? 2 : 1);
								continue;
						}

					case Mosaic:
						// TO-DO
						// reg.ICF[Path] /= 2;
						assert(0);
						vsr++;
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

				case 0xC0: // channel 1
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
					reg.BankCLUT = Path ? (inst & 0x01u) + 2 : inst & 0x0Fu;
					//MiniCDI::Log("[VDSC] P%d cbnk %d", Path, reg.BankCLUT);
					break;

				case 0xC4:
					if (!Path) {
						reg.TransparentCol[0] = inst & 0x00FCFCFCu;
					}
					break;
				case 0xC6:
					if (Path) {
						reg.TransparentCol[1] = inst & 0x00FCFCFCu;
					}
					break;

				case 0xC7:
					if (!Path) {
						reg.MaskCol[0] = inst & 0x00FCFCFCu;
					}
					break;
				case 0xC9:
					if (Path) {
						reg.MaskCol[1] = inst & 0x00FCFCFCu;
					}
					break;

				case 0xCA:
					if (!Path) {
						reg.ColorDYUV[0] = inst & 0x00FFFFFFu;
						//MiniCDI::Log("[VDSC] P0 yuv_b y=$%02x,u=$%02x,v=$%02x", inst >> 16 & 0xFFu, inst >> 8 & 0xFFu, inst & 0xFFu);
					}
					break;
				case 0xCB:
					if (Path) {
						reg.ColorDYUV[1] = inst & 0x00FFFFFFu;
						//MiniCDI::Log("[VDSC] P1 yuv_b y=$%02x,u=$%02x,v=$%02x", inst >> 16 & 0xFFu, inst >> 8 & 0xFFu, inst & 0xFFu);
					}
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
						uint8_t mcr = (inst >> 24 & 0xFF) - 0xD0;
						MCR.opcode[mcr] = inst >> 20 & 0x0F;
						MCR.mf[mcr] = reg.MatteCount ? (mcr < 4 ? 0 : 1) : inst >> 16 & 0x01;
						MCR.icf[mcr] = (float)(inst >> 10 & 0x3F) / 63.0f;
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
					reg.ICF[0] = (float)(inst & 0x3F) / 63.0f;
					//MiniCDI::Log("[VDSC] P0 wfac_%s %d", inst & 0x2Fu);
					break;
				case 0xDC:
					reg.ICF[1] = (float)(inst & 0x3F) / 63.0f;
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
			MF[2],		/** (Mosaic Factor) separate for each channel **/
			FT[2];		/** (File Type) separate for each channel **/

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
		MF[0] = FT[0] = 0;
		MF[1] = FT[1] = 0;

		// initialization
		CF = MiniCDI::Config::PAL ? 0 : 1; // crystal frequency
		FD = MiniCDI::Config::PAL ? 0 : 1; // frame duration
		SM = 0; // interlace (unnecessary)

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
			//MiniCDI::Log("[MCD212] VSYNC");
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
				const uint8_t value = BE[0] | (IT[1] << 1) | (IT[0] << 2);
				BE[0] = IT[1] = IT[0] = 0;
				if (value & 0b0110) _68070->interrupt(SCC68070::IPL_INT1, false);
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
				const uint8_t value = BE[0] | (IT[1] << 1) | (IT[0] << 2);
				BE[0] = IT[1] = IT[0] = 0;
				if (value & 0b0110) _68070->interrupt(SCC68070::IPL_INT1, false);
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