#ifndef MINICDI_DISCFORMAT
#define MINICDI_DISCFORMAT

class CDiDisc
{
	std::ifstream disc;

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
		char Min;
		char Sec;
		char Frame;
		char Mode;

		// Subheader, is repeated twice
		char FileNum;
		char ChNum;
		char Submode; // 0x01 = EOR, 0x02 = Video, 0x04 = Audio, 0x08 = Data, 0x10 = Trigger, 0x20 = Form, 0x40 = Real-Time Sector, 0x80 = EOF
		char CodingInfo;

		char Data[2340];
	} Sector; // CD-i, not CD-DA*/

	int lba = -1;

	bool sector_valid()
	{
		return disc.is_open() && lba >= 0;
	}

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

		disc.clear();
		disc.seekg(lba*2352 + 12, std::ios::beg);
	}

	void read_sector()
	{
		if (sector_valid()) {
			// const uint32_t real_lba = lba + 150;

			disc.get(Sector.Min);
			disc.get(Sector.Sec);
			disc.get(Sector.Frame);
			disc.get(Sector.Mode);
			disc.get(Sector.FileNum);
			disc.get(Sector.ChNum);
			disc.get(Sector.Submode);
			disc.get(Sector.CodingInfo);
			disc.seekg(4, std::ios::cur);

			MiniCDI::Log("[Disc] sector read. time: %02d:%02d:%02d, mode: %02X", Sector.Min, Sector.Sec, Sector.Frame, Sector.Mode);
			MiniCDI::Log("                    file: %d, ch: %d, submode: %02X, coding: %02X", Sector.FileNum, Sector.ChNum, Sector.Submode, Sector.CodingInfo);
			disc.read(&Sector.Data[0], 2340);
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
		read_sector();
	}
};

#endif