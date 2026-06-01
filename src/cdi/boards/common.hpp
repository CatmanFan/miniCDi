#ifndef MINICDI_PLAYERS
#define MINICDI_PLAYERS

class CDi
{
protected:
	uint8_t *memory; // Contains full memory map
	const int memSize	= 0x680000; // cdifan: max possible CD-i memory size is roughly 6.5 MB (CD-i 605 with DVC and expansion card)

	// ONLY the chips shared in common by supported boards (MMC and Mono)
	SCC68070 cpu;

public:
	PointingDevice pd;
	CDiDisc disc;

	virtual bool init(const std::string &bios)
	{
		/** Order of initialization:
		1) Initialising slave processor
		2) Initialising video processor
		3) Clearing system RAM
		4) Building system exception table
		5) Determining the cpu type
		6) Initialising video (blue screen)
		7) Determining and enabling the display
		8) Executing RAM/ROM search
		9) Starting the kernel */

		memory = (uint8_t *)memalign(32, memSize);
		if (memory) {
			memset(memory, 0, memSize);
			return true;
		}

		return false;
	}

	CDi() : memory(nullptr), cpu(memory) {}

	virtual ~CDi()
	{
		if (memory) {
			free(memory);
		}
	}

	/**
	 * @brief  Runs until VSync signal on video driver (i.e. a frame).
	 */
	inline virtual void do_frame(bool draw = true) { ; }
	inline virtual void reset() { ; }

	inline virtual uint32_t* get_display() { return nullptr; }
	inline virtual size_t get_display_width() { return 0; }

	inline virtual uint32_t* get_lcd() { return nullptr; }
};

#endif