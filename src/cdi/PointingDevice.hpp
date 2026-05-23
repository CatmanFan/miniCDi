#ifndef MINICDI_POINTINGDEVICE
#define MINICDI_POINTINGDEVICE

class PointingDevice
{
	static constexpr int MAX_POINTER_X = 768;
	static constexpr int MAX_POINTER_Y = 560;

public:
	enum Types
	{
		RelativePD = 0,
		ManeuveringPD,
		AbsolutePD,
		ScreenPD
	} type;

	enum Buttons
	{
		Up = 0,
		Down,
		Left,
		Right,
		Button1,
		Button2
	};

	struct
	{
		std::vector<uint8_t> data;
		SLAVE* slave;
		IKAT* ikat;
	} IO;

	PointingDevice() { type = ManeuveringPD; }

	bool buttons[6];
	bool changed;
	int x = 0, y = 0;

	void send()
	{
		if (IO.slave && IO.slave->polling) {
			if (!IO.slave->pointer)
			{
				MiniCDI::Log("[PD] handshake to SLAVE");
				IO.slave->Ch[0].Out =
				{
					type == ScreenPD ? 'S' : type == AbsolutePD ? 'T' : type == RelativePD ? 'M' : 'J',
					0b11000000,
					0b10000000,
					0b10000000
				};
				IO.slave->pointer = true;
				m68k_set_irq(2);
			}

			else if (changed || buttons[Left] || buttons[Right] || buttons[Down] || buttons[Up])
			{
				switch (type)
				{
					default:
					case RelativePD:
					case ManeuveringPD:
						x = std::clamp(x + (buttons[Left] && !buttons[Right] ? -1
																			 : !buttons[Left] && buttons[Right] ? 1
																			 : 0), 0, 767);
						y = std::clamp(y + (buttons[Up] && !buttons[Down] ? -1
																		  : !buttons[Up] && buttons[Down] ? 1
																		  : 0), 0, 559);
						break;
					case AbsolutePD:
					case ScreenPD:
						break;
				}

				// Convert to SLAVE response (coord bounds: 54x97 to 704x679?)
				IO.slave->Ch[0].Out =
				{
					(uint8_t)(((x & 0x380) >> 7) | (0x01 << 3)),
					(uint8_t)(x & 0x7f),
					(uint8_t)((y & 0x380) >> 7),
					(uint8_t)(y & 0x7f)
				};
				// MiniCDI::Log("[PD] x=%d,y=%d,b1=%d,b2=%d", x, y, buttons[Button1], buttons[Button2]);
				m68k_set_irq(2);
				changed = false;
			}
		}
	}

	void set_button(enum Buttons b, bool value)
	{
		/*if (this->buttons[(int)b] != value)
			MiniCDI::Log("[PD] set pointer button %d = %d", (int)b, value);*/

		if (b == Left || b == Right || b == Down || b == Up) {
			this->buttons[(int)b] = value;
		} else if (this->buttons[(int)b] != value) {
			this->buttons[(int)b] = value;
			changed = true;
		}
	}
};

#endif