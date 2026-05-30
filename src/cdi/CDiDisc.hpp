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

		char Data[2328];
	} Sector; // CD-i, not CD-DA*/

	int lba = -1;

	bool sector_valid()
	{
		return disc.is_open() && lba >= 0;
	}

	uint32_t get_lba_from_time(uint32_t time)
	{
		/** Copied from MAME source code of CD-i CDIC driver **/
		Sector.Min = time >> 24 & 0xFF;
		Sector.Sec = time >> 16 & 0xFF;
		Sector.Frame = time >> 8 & 0xFF;

		// Convert to raw mm:ss:ff
		const uint8_t raw_min = ((Sector.Min >> 4) * 10) + (Sector.Min & 0xf);
		const uint8_t raw_sec = ((Sector.Sec >> 4) * 10) + (Sector.Sec & 0xf);

		lba = ((raw_min * 60) + raw_sec) * 75;
		if (!(Sector.Frame & 0x80))
		{
			const uint8_t raw_frame = ((Sector.Frame >> 4) * 10) + (Sector.Frame & 0xf);
			lba += raw_frame;
		}

		if (lba >= 150)
			lba -= 150;

		disc.clear();
		disc.seekg(lba*2352, std::ios::beg);

		return lba;
	}

	void read_sector(int lba = -1)
	{
		if (sector_valid()) {
			if (lba >= 0) { disc.seekg(lba*2352, std::ios::beg); }

			disc.seekg(12, std::ios::cur); // sync field
			disc.get(Sector.Min);
			disc.get(Sector.Sec);
			disc.get(Sector.Frame);
			disc.get(Sector.Mode);
			disc.get(Sector.FileNum);
			disc.get(Sector.ChNum);
			disc.get(Sector.Submode);
			disc.get(Sector.CodingInfo);
			disc.seekg(4, std::ios::cur);

			// MiniCDI::Log("[Disc] sector read. time: %02X:%02X:%02X, mode: %02X", Sector.Min, Sector.Sec, Sector.Frame, Sector.Mode);
			// MiniCDI::Log("                    file: %d, ch: %d, submode: %02X, coding: %02X", Sector.FileNum, Sector.ChNum, Sector.Submode, Sector.CodingInfo);
			disc.read(&Sector.Data[0], 2328);
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
		// read_sector();
	}
};

#endif