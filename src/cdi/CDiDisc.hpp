#ifndef MINICDI_DISCFORMAT
#define MINICDI_DISCFORMAT

class CDiDisc
{
	std::ifstream disc;

	enum SubmodeType
	{
		Submode_EOF,
		Submode_RT,
		Submode_F,
		Submode_T,
		Submode_D,
		Submode_A,
		Submode_V,
		Submode_EOR,
	};

	/**
	The sector is structured as follows:
	* Sync field (12 bytes)
	* Header field (4 bytes)
	* Subheader field (8 bytes)
	* Data field (2328 bytes)
	**/
	struct {
		// Sync field is 00FFFFFFFFFFFFFFFFFFFF00, can be ignored(?)

		// Header, contains address and mode
		char Mins;
		char Secs;
		char Sects;
		char Mode;

		// Subheader, is repeated twice
		char FileNum;
		char ChNum;
		char Submode;
		char CodingInfo;

		char buffer[2352];
	} Sector; // CD-i, not CD-DA

	int lba;
	int lbn;

	void get_lba_from_time(uint32_t time)
	{
		/** from MAME source code of CD-i driver **/
		const uint8_t bcd_mins = (time >> 24) & 0xff;
		const uint8_t mins_upper_digit = bcd_mins >> 4;
		const uint8_t mins_lower_digit = bcd_mins & 0xf;
		const uint8_t raw_mins = (mins_upper_digit * 10) + mins_lower_digit;

		const uint8_t bcd_secs = (time >> 16) & 0xff;
		const uint8_t secs_upper_digit = bcd_secs >> 4;
		const uint8_t secs_lower_digit = bcd_secs & 0xf;
		const uint8_t raw_secs = (secs_upper_digit * 10) + secs_lower_digit;

		lba = ((raw_mins * 60) + raw_secs) * 75;

		const uint8_t bcd_frac = (time >> 8) & 0xff;
		const bool even_second = bcd_frac & 0x80;
		if (!even_second)
		{
			const uint8_t frac_upper_digit = bcd_frac >> 4;
			const uint8_t frac_lower_digit = bcd_frac & 0xf;
			const uint8_t raw_frac = (frac_upper_digit * 10) + frac_lower_digit;
			lba += raw_frac;
		}

		if (lba >= 150)
			lba -= 150;
	}

	void read_sector()
	{
		if (lba >= 0) {
			lbn = lba / 2352;

			disc.clear();
			disc.seekg(lba - (lba % 2352), std::ios::beg);
			disc.get(Sector.Mins);
			disc.get(Sector.Secs);
			disc.get(Sector.Sects);
			disc.get(Sector.Mode);
			disc.get(Sector.FileNum);
			disc.get(Sector.ChNum);
			disc.get(Sector.Submode);
			disc.get(Sector.CodingInfo);
			disc.seekg(4, std::ios::cur);
			disc.read(Sector.buffer, 2352);

			MiniCDI::Log("[CD] Read sector %d", lbn);
		}
	}

public:
	friend class CDIC;
	friend class CIAP;

	void open(const std::string &path)
	{
		disc.open(path, std::ios::in | std::ios::binary);
		if (disc.is_open()) {
			lba = -1;
		}
	}

	void increment_lba()
	{
		if (disc.is_open() && lba >= 0) {
			lba++;
		}
	}
};

#endif