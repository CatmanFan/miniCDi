#ifndef MINICDI_PLAYERS
#define MINICDI_PLAYERS

class CDi
{
public:
	enum BoardType {
		Invalid = 0,
		MonoI,
		MonoII,
		MonoIII,
		MonoIV
	};

protected:
	uint8_t *memory; // Contains full memory map
	static constexpr int memsize = 8*1024*1024;

	uint32_t nvram;
	bool nvram_save();
	void nvram_load();

	// ONLY the chips shared in common by supported boards (MMC and Mono)
	SCC68070 cpu;
	enum BoardType board = CDi::Invalid;

public:
	PointingDevice pd;
	CDiDisc disc;

	virtual bool init(const std::string &bios, enum BoardType board)
	{
		if (memory != NULL) return true;
		if (board == CDi::Invalid) return false;

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

		// TO-DO: reduce overhead ?
		// The memory size is allocated this way to allow for addresses used by DMA transfer.
		// In practice Mono-I only has 5.5 MB total (not including DVC RAM).
		// cdifan: max possible CD-i memory size is roughly 6.5 MB (CD-i 605 with DVC and expansion card)

		#ifdef _WIN32
		memory = (uint8_t *)_aligned_malloc(memsize*sizeof(uint8_t), 32);
		#elif defined(__APPLE__)
		posix_memalign((void**)&memory, 32, memsize*sizeof(uint8_t));
		#else
		memory = (uint8_t *)memalign(32, memsize*sizeof(uint8_t));
		#endif
		if (memory) {
			memset(memory, 0, memsize*sizeof(uint8_t));
			return true;
		}

		return false;
	}

	CDi() : memory(nullptr), cpu(memory) {}
	virtual ~CDi() {}

	/**
	 * @brief  Runs until VSync signal on video driver (i.e. a frame).
	 */
	inline virtual void run(bool no_draw = false) { ; }
	inline virtual void reset() { ; }

	inline virtual uint32_t* get_display() { return nullptr; }
	inline virtual size_t get_display_width() { return 0; }

	inline virtual bool get_cd_read_status() { return false; }

	inline uint8_t* get_memory() { return memory; }
	inline int get_memory_size() { return memsize; }
};

#endif