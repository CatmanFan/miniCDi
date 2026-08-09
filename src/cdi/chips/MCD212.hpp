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
			/* DB */ uint8_t ICF[2];

			enum MosaicFactor MF[2];
			enum FileType FT[2];
			enum ColorMode CM[2];
		} reg;

		bool Matte[2];
		struct {
			size_t current = 0;
			size_t x[8];
			uint8_t icf[8];
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

		struct
		{
			uint8_t Y = 0;
			uint8_t U = 0;
			uint8_t V = 0;

			/// Table 7–1 in MCD212 datasheet
			uint8_t LUT_deq[16] = {0,1,4,9,16,27,44,79,128,177,212,229,240,247,252,255};
		} DYUVDecoder;

		template <size_t Path> bool isTransparent(uint8_t* src, bool second = false);
		template <size_t Path> uint32_t decodeDYUV(uint8_t* src, uint32_t *dst);
		template <size_t Path> uint32_t decodeRGB555(uint8_t* src, uint32_t *dst);

		/**
		 * @brief  Decodes CLUT to an RGB pixel.
		 *
		 * @param  src:  pointer to the VSR buffer
		 * @param  dst:  pointer to the uint32_t pixel
		 *
		 * @return The number of RGB pixels incremented
		 */
		template <size_t Path> uint32_t decodeCLUT(uint8_t* src, uint32_t *dst);
		template <size_t Path> uint8_t getCLUTindex(uint8_t* src, bool second = false);

		uint32_t framebuffer[768 * 280]; // max bounds

	public:
		bool skip_draw = false;
		uint8_t cursor[16*16];
		Plane FG[2];

		inline uint32_t* get_display()
		{
			return &framebuffer[0];
		}

		inline int get_display_width()
		{
			return 768;
		}

		inline void reset()
		{
			memset(cursor, 0, sizeof(cursor));
			reg = {0};
		}

		void set_mode(int hRes, int vRes, bool hDouble = false, bool vDouble = false)
		{
			if (reg.Icm[0] == CLUT4 || reg.Icm[1] == CLUT4) hDouble = true;

			FG[1].width = FG[0].width = hRes * (hDouble || vDouble ? 2 : 1);
			FG[1].height = FG[0].height = vRes * (vDouble ? 2 : 1);

			if (this->skip_draw) return;
			memset(FG[0].decoded, 0, sizeof(FG[0].decoded));
			memset(FG[1].decoded, 0, sizeof(FG[1].decoded));
			memset(framebuffer, 0xFF000000, sizeof(framebuffer));
		}

		/**
		 * @brief  Draws a VSR line to a plane.
		 *
		 * @param  vsr:  memory index for the start of the VSR
		 * @param  y:    the line
		 *
		 * @return The incremented VSR
		 */
		template <size_t Path> uint32_t draw_line_to_plane(uint8_t* memory, uint32_t vsr, int y);

		/**
		 * @brief  Mixes all planes to the framebuffer.
		 */
		void mix_to_frame(int y);

		template <size_t Path> void set_register(uint32_t inst);
	} vdsc;

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

	template <size_t Path> void vsr_set(uint32_t value);
	template <size_t Path> void dcp_set(uint32_t value);
	template <size_t Path> void ICA_execute();
	template <size_t Path> void DCA_execute();

public:
	MCD212() {}

	MCD212(SCC68070 *_68070, uint8_t *memory) : _68070(_68070), memory(memory) {
		reset();
	}

	/**
	 * @brief  Resets the chip.
	 */
	void reset();

	/**
	 * @brief  Draws a video line.
	 */
	bool tick();
	bool skip_draw = false;

	uint8_t read8(uint32_t addr);
	uint16_t read16(uint32_t addr);
	void write16(uint32_t addr, uint16_t value);

	inline uint32_t* get_display()
	{
		return vdsc.get_display();
	}

	inline size_t get_display_width()
	{
		return vdsc.get_display_width();
	}
};

#endif