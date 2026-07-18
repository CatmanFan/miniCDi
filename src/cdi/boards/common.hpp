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

	uint32_t nvram;
	bool nvram_save() {
		if (MiniCDI::Config::NvramFile.empty() || memory == NULL || this->nvram == 0) {
			return false;
		}

		uint32_t nvram_size = board == CDi::MonoIV ? 32*1024 : 8*1024;
		MiniCDI::OS9::Module *module = MiniCDI::OS9::get_module_from_name("nvr");
		if (module != NULL && module->size < nvram_size) {
			nvram_size = (module->size > 12*1024 ? 16 : 8) * 1024;
			MiniCDI::Log("[NVRAM] detected %dKB", nvram_size);
		} else {
			MiniCDI::Log("[NVRAM] \"nvr\" system module not found, defaulting to max %dKB", nvram_size/1024);
		}

		FILE *file = fopen(MiniCDI::Config::NvramFile.c_str(), "wb");
		if (!file) {
			MiniCDI::Log("[NVRAM] failed to write to %s", MiniCDI::Config::NvramFile.c_str());
			return false;
		}
		fwrite(&memory[this->nvram], sizeof(memory[0]), nvram_size, file);
		fclose(file);

		MiniCDI::Log("[NVRAM] saved to %s", MiniCDI::Config::NvramFile.c_str());
		return true;
	}
	void nvram_load() {
		/*if (this->memory != NULL)
		{
			// Write a default value to where the M48T08 (8KB NVRAM) registers should be stored.
			memory[this->nvram + 0x1FFF] = 0x01; // (BCD) yy: 2001
			memory[this->nvram + 0x1FFE] = 0x01; // (BCD) mm: 1
			memory[this->nvram + 0x1FFD] = 0x01; // (BCD) dd: 1
			memory[this->nvram + 0x1FFC] = 0x01; // (BCD) dd: Monday + normal clock operation
			memory[this->nvram + 0x1FFB] = 0x12; // (BCD) HH: 12
			memory[this->nvram + 0x1FFA] = 0x00; // (BCD) MM: 00
			memory[this->nvram + 0x1FF9] = 0x00; // (BCD) SS: 00
			memory[this->nvram + 0x1FF8] = 0; // control

			// Ditto for DS1216 (32KB NVRAM)
			memory[this->nvram + 0x0000] = 0x00; // (BCD) centiseconds
			memory[this->nvram + 0x0001] = 0x00; // (BCD) SS: 00
			memory[this->nvram + 0x0002] = 0x00; // (BCD) MM: 00
			memory[this->nvram + 0x0005] = 0x01; // (BCD) dd: 1
			memory[this->nvram + 0x0006] = 0x01; // (BCD) mm: 1
			memory[this->nvram + 0x0007] = 0x01; // (BCD) yy: 2001
		}*/
		if (!MiniCDI::Config::NvramFile.empty() && access(MiniCDI::Config::NvramFile.c_str(), F_OK) == 0 && this->nvram > 0) {
			MiniCDI::Log("[NVRAM] loading %s to memory", MiniCDI::Config::NvramFile.c_str());
			std::ifstream nvrStream(MiniCDI::Config::NvramFile, std::ios::binary);
			std::vector<char> nvr((std::istreambuf_iterator<char>(nvrStream)),(std::istreambuf_iterator<char>()));
			nvrStream.close();

			memcpy(&memory[this->nvram], &nvr[0], nvr.size());
		}
	}

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

		int memsize = 8*1024*1024;
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

	/**
	 * @brief  Runs until VSync signal on video driver (i.e. a frame).
	 */
	inline virtual void run(int frames = 1) { ; }
	inline virtual void reset() { ; }

	inline virtual uint32_t* get_display() { return nullptr; }
	inline virtual size_t get_display_width() { return 0; }

	inline virtual uint32_t* get_lcd() { return nullptr; }
	inline virtual bool get_cd_read_status() { return false; }
};

#endif