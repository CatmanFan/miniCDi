#ifndef MINICDI_DISCFORMAT
#define MINICDI_DISCFORMAT

class CDiDisc
{
	FILE* disc;

	// Addresses of relevant bytes, relative to the header's minute byte.
	enum {
		// Header, contains address and mode
		H_MIN = 0,
		H_SEC,
		H_FRAME,
		H_MODE,

		// Subheader, is repeated twice
		SH_FILE1,
		SH_CHAN1,
		SH_SUBMODE1,
		SH_CODING1,
		SH_FILE2,
		SH_CHAN2,
		SH_SUBMODE2,
		SH_CODING2,

		SECTOR_SIZE = 2352
	};

	// CD-i sector, not CD-DA
	/// The sector is structured as follows (Green Book 5/1994 II.4.1.1):
	/// Sync field (12 bytes), is 00FFFFFFFFFFFFFFFFFFFF00 and is unscrambled, can be ignored
	/// Header field (4 bytes)
	/// Subheader field (8 bytes)
	/// Data field (2328 bytes)

	/// Subheader codes (Green Book 5/1994 II.4.5.3):
	/// 0x01 = EOR
	/// 0x02 = Video
	/// 0x04 = Audio
	/// 0x08 = Data
	/// 0x10 = Trigger
	/// 0x20 = Form
	/// 0x40 = Real-Time Sector
	/// 0x80 = EOF
	char Sector[SECTOR_SIZE];
	std::string Label;

	/**
	 * @brief  Decodes a time value to an LBA and automatically seeks the disc filestream to that address.
	 *
	 * @param  time:  Time value in unsigned long form (e.g. `00'02'16'00`).
	 *
	 * @return The LBA address, subtracted by 150.
	 */
	uint32_t get_lba_from_time(uint32_t time);

	bool is_byteswapped(int lba);
	bool is_valid_sector(int lba);
	void read_sector(int lba = 0);

public:
	friend class CDIC;
	friend class DRVDSP;
	friend class CIAP;

	inline bool is_open() { return disc != NULL; }
	bool open(const std::string &path);
	void eject();
};

#endif