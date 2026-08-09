#ifndef MINICDI_DISCFORMAT
#define MINICDI_DISCFORMAT
#include <fstream>

class CDiDisc
{
	std::ifstream disc;

	// CD-i sector, not CD-DA
	struct {
		/// The sector is structured as follows (Green Book 5/1994 II.4.1.1):
		/// Sync field (12 bytes), is 00FFFFFFFFFFFFFFFFFFFF00 and is unscrambled, can be ignored
		/// Header field (4 bytes)
		/// Subheader field (8 bytes)
		/// Data field (2328 bytes)

		// Header, contains address and mode
		char Min;
		char Sec;
		char Frame;
		char Mode;

		// Subheader, is repeated twice
		char FileNum[2];
		char ChNum[2];
		char Submode[2];
		char CodingInfo[2];

		/// Subheader codes (Green Book 5/1994 II.4.5.3):
		/// 0x01 = EOR
		/// 0x02 = Video
		/// 0x04 = Audio
		/// 0x08 = Data
		/// 0x10 = Trigger
		/// 0x20 = Form
		/// 0x40 = Real-Time Sector
		/// 0x80 = EOF

		char Data[2328];
	} Sector;

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

	inline bool is_open() { return disc.is_open(); }
	bool open(const std::string &path);
	void eject();
};

#endif