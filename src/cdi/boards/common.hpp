#ifndef MINICDI_PLAYERS
#define MINICDI_PLAYERS

class CDi
{
protected:
	uint8_t *memory; // Contains full memory map

	uint32_t nvram;
	bool nvram_save() {
		/*if (MiniCDI::Config::NvramFile.empty() || !memory || this->nvram == 0) {
			return false;
		}

		uint32_t nvram_size = (board == CDi::MonoI ? 8 : 32) * 1024;
		for (size_t i = 0; i < MiniCDI::OS9::modules.size(); i++) {
			if (MiniCDI::OS9::modules[i].name.compare("nvr") == 0) {
				MiniCDI::Log("[NVRAM] detected %dKB", MiniCDI::OS9::modules[i].size > nvram_size ? 32 : 8);
				nvram_size = (MiniCDI::OS9::modules[i].size > nvram_size ? 32 : 8) * 1024;
			}
		}

		FILE *file = fopen(MiniCDI::Config::NvramFile.c_str(), "wb");
		if (!file) {
			MiniCDI::Log("[NVRAM] failed to write to %s", MiniCDI::Config::NvramFile.c_str());
			return false;
		}
		fwrite(&memory[this->nvram], sizeof(memory[0]), nvram_size, file);
		fclose(file);

		MiniCDI::Log("[NVRAM] saved to %s", MiniCDI::Config::NvramFile.c_str());*/
		return true;
	}
	void nvram_load() {
		/*if (!MiniCDI::Config::NvramFile.empty() && access(MiniCDI::Config::NvramFile.c_str(), F_OK) == 0 && this->nvram > 0) {
			MiniCDI::Log("[NVRAM] loading %s to memory", MiniCDI::Config::NvramFile.c_str());
			std::ifstream nvrStream(MiniCDI::Config::NvramFile);
			std::vector<char> nvr((std::istreambuf_iterator<char>(nvrStream)),(std::istreambuf_iterator<char>()));
			nvrStream.close();

			memcpy(&memory[this->nvram], &nvr[0], nvr.size());
		}*/
	}

	// ONLY the chips shared in common by supported boards (MMC and Mono)
	SCC68070 cpu;

public:
	enum BoardType {
		MonoI = 0,
		MonoII,
		MonoIII,
		MonoIV
	};
	enum BoardType board;

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

		int memsize = 8*1024*1024;
		// TO-DO: reduce overhead ?
		// The memory size is allocated this way to allow for addresses used by DMA transfer.
		// In practice Mono-I only has 5.5 MB total (not including DVC RAM).
		// cdifan: max possible CD-i memory size is roughly 6.5 MB (CD-i 605 with DVC and expansion card)

		memory = (uint8_t *)memalign(32, memsize);

		if (memory) {
			memset(memory, 0, memsize);
			return true;
		}

		return false;
	}

	CDi() : memory(nullptr), cpu(memory) {}

	/**
	 * @brief  Runs until VSync signal on video driver (i.e. a frame).
	 */
	inline virtual void run(bool skip_draw = false) { ; }
	inline virtual void reset() { ; }

	inline virtual uint32_t* get_display() { return nullptr; }
	inline virtual size_t get_display_width() { return 0; }

	inline virtual uint32_t* get_lcd() { return nullptr; }
};

#endif