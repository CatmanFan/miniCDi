#ifndef MINICDI_FPD
#define MINICDI_FPD

#include <deque>

class FPD
{
	std::vector<uint8_t> display = {0x00};
	size_t digit_width = 1, digit_height = 1, digit_count = 1;
	size_t digit_spacingX = 0, digit_spacingY = 0;

public:
	enum FPDType {
		FPD_220_20 = 0,
		FPD_220_40,
		// FPD_450 (ABSENT)
		FPD_470 // 490 uses same LCD?
	};
	enum FPDType type;

	FPD(enum FPDType type)
	{
		this->type = type;
		switch (this->type)
		{
			default:
			case FPD_220_20:
				digit_width = 5;
				digit_height = 7;
				digit_count = 7;
				break;
			case FPD_220_40:
				digit_width = 5;
				digit_height = 7;
				digit_count = 7;
				break;
			case FPD_470:
				digit_width = 5;
				digit_height = 7;
				digit_count = 3;
				break;
		}
		digit_spacingX = 3;
		digit_spacingY = 1;
		digit_count = 7;

		display.assign(get_display_width() * get_display_height(), 0x00);
	}

	inline void reset()
	{
		std::fill(begin(display), end(display), 0x00);
	}

	inline void update(std::deque<uint8_t> &cmd)
	{
		std::fill(begin(display), end(display), 0x00);

		for (size_t i = cmd.size() - 2, digit = 0; i > 0 && digit < std::min((cmd.size()-1)/2, digit_count); i-=2, digit++)
		{
			uint8_t glyph[digit_width*digit_height];
			memset(glyph, 0, sizeof(glyph));

			switch (type)
			{
				default:
				case FPD_220_20:
				case FPD_220_40:
					if (cmd[i] & 0x01)
					{
						glyph[0*digit_width + 1] = 0xFF;
						glyph[0*digit_width + 2] = 0xFF;
						glyph[0*digit_width + 3] = 0xFF;
					}
					if (cmd[i] & 0x02)
					{
						glyph[0*digit_width + 4] = 0xFF;
						glyph[1*digit_width + 4] = 0xFF;
						glyph[2*digit_width + 4] = 0xFF;
					}
					if (cmd[i] & 0x04)
					{
						glyph[3*digit_width + 4] = 0xFF;
						glyph[4*digit_width + 4] = 0xFF;
						glyph[5*digit_width + 4] = 0xFF;
						glyph[6*digit_width + 4] = 0xFF;
					}
					if (cmd[i] & 0x08)
					{
						glyph[6*digit_width + 1] = 0xFF;
						glyph[6*digit_width + 2] = 0xFF;
						glyph[6*digit_width + 3] = 0xFF;
					}
					if (cmd[i] & 0x10)
					{
						glyph[4*digit_width + 0] = 0xFF;
						glyph[5*digit_width + 0] = 0xFF;
						glyph[6*digit_width + 0] = 0xFF;
					}
					if (cmd[i] & 0x20)
					{
						glyph[0*digit_width + 0] = 0xFF;
						glyph[1*digit_width + 0] = 0xFF;
						glyph[2*digit_width + 0] = 0xFF;
						glyph[3*digit_width + 0] = 0xFF;
					}
					if (cmd[i] & 0x40)
					{
						glyph[3*digit_width + 1] = 0xFF;
					}
					if (cmd[i] & 0x80)
					{
						glyph[1*digit_width + 1] = 0xFF;
						glyph[2*digit_width + 1] = 0xFF;
					}
					if (cmd[i+1] & 0x01)
					{
						glyph[1*digit_width + 2] = 0xFF;
						glyph[2*digit_width + 2] = 0xFF;
						glyph[4*digit_width + 2] = 0xFF;
						glyph[5*digit_width + 2] = 0xFF;
					}
					if (cmd[i+1] & 0x02)
					{
						glyph[1*digit_width + 3] = 0xFF;
						glyph[2*digit_width + 3] = 0xFF;
					}
					if (cmd[i+1] & 0x04)
					{
						glyph[3*digit_width + 3] = 0xFF;
					}
					if (cmd[i+1] & 0x08)
					{
						glyph[4*digit_width + 3] = 0xFF;
						glyph[5*digit_width + 3] = 0xFF;
					}
					if (cmd[i+1] & 0x10)
					{
						glyph[4*digit_width + 1] = 0xFF;
						glyph[5*digit_width + 1] = 0xFF;
					}
					if ((cmd[i+1] & 0x20) && !(cmd[i+1] & 0x01))
					{
						glyph[3*digit_width + 2] = 0xFF;
					}
					break;

				case FPD_470:
					if (cmd[i+1] & 0x08)
					{
						glyph[1*digit_width + 2] = 0xFF;
						glyph[2*digit_width + 2] = 0xFF;
						glyph[4*digit_width + 2] = 0xFF;
						glyph[5*digit_width + 2] = 0xFF;
					}
					break;
			}

			for (size_t x = 0; x < digit_width; x++)
			{
				for (size_t y = 0; y < digit_height; y++)
				{
					display[y*get_display_width() + (digit*(digit_width+digit_spacingX))+x] = glyph[y*digit_width + x];
				}
			}
		}
	}

	inline uint8_t* get_display() { return &display[0]; }
	inline size_t get_display_width() { return (digit_width+digit_spacingX) * digit_count; }
	inline size_t get_display_height() { return (digit_height+digit_spacingY); }
};

#endif