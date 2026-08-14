#ifndef MINICDI_MC6805_SLAVE
#define MINICDI_MC6805_SLAVE

#include <deque>

/// HLE implementation of SLAVE as found in MiniMMC & Mono-I.
class SLAVE
{
	SCC68070* _68070;
	uint8_t* memory;

	uint32_t DR[4]; // addresses to data registers

	struct
	{
		std::deque<uint8_t> In;
		std::deque<uint8_t> Out;
		size_t InSize;
		bool Available;
	} Ch[4];

	struct
	{
		bool connected;
		bool enabled;
		bool posChanged;
		int x, y;
	} PointerInterface;
	bool Disc = false;
	FTD* ftd;

	void assert_irq(size_t c); // For PointingDevice !!
	uint8_t revision = 0;

public:
	friend class FTD;
	friend class PointingDevice;

	SLAVE(SCC68070* _68070, uint8_t* memory, uint32_t start);

	void set_ftd(FTD* ftd);
	void send_play_button();
	void send_eject_button();
	void send_disc_status(bool value);

	void reset();
	uint8_t read8(uint32_t addr);
	void write8(uint32_t addr, uint8_t value);
};

#endif