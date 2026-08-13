#ifndef MINICDI_PLAYERS_PHILIPS
#define MINICDI_PLAYERS_PHILIPS

/** ******* Mono-I memory map *******
	$00000000   512KB.ram    name=planea
	$00200000   512KB.ram    name=planeb
	$00300000   cdic.dev     level=4
	$00310000   slave.dev    level=2 vec=26
	$00320000   nvr.dev
	$00400000   sysrom.rom   size=512KB
	$004fffe0   vdsc.dev
	$80000000   68070.dev
**/

class PhilipsCDI : public CDi
{
private:
	MCD212* vpu = NULL;
	SLAVE* slave = NULL;
	IKAT* ikat = NULL;
	CDIC* cdic = NULL;
	DRVDSP* dsp = NULL;
	CIAP* ciap = NULL;
	FTD* ftd = NULL;

	inline bool check_for_unmapped(int address)
	{
		if ((address >= 0x080000 && address < 0x200000) // in-between DRAM banks
		 || (address >= 0x328000 && address < 0x400000) // between NVRAM and ROM
		 || (address >= 0x500000 && address < 0xD00000) // between MCD212 and VMPEG
		 || (address >= 0xD00000 && address < 0xF00000)) // VMPEG is not connected
		{
			m68k_pulse_bus_error();
			return false;
		}
		return true;
	}

public:
	bool init(const std::string &bios, enum BoardType board) override;
	~PhilipsCDI();

	void run(bool no_draw = false) override;
	void reset() override;

	uint8_t read8(int address) override;
	uint16_t read16(int address) override;
	uint32_t read32(int address) override;

	void write8(int address, uint8_t value) override;
	void write16(int address, uint16_t value) override;
	void write32(int address, uint32_t value) override;

	void play_disc();
	void swap_disc(const std::string &path);

	inline uint32_t* get_display() override { return vpu->get_display(); }
	inline size_t get_display_width() override { return vpu->get_display_width(); }

	inline uint8_t* get_ftd() { return ftd != NULL ? ftd->get_display() : NULL; }
	inline size_t get_ftd_width() { return ftd != NULL ? ftd->get_display_width() : 0; }
	inline size_t get_ftd_height() { return ftd != NULL ? ftd->get_display_height() : 0; }

	inline bool get_cd_read_status() override {
		if (cdic != NULL) return cdic->is_reading();
		if (ciap != NULL) return ciap->is_reading();
		return false;
	}
};

#endif