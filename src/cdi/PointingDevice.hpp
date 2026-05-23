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
		AbsolutePD
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
		SLAVE* slave;
		IKAT* ikat;
	} IO;

	PointingDevice() { type = AbsolutePD; }

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
					type == AbsolutePD ? 'T' : 'M',
					0b11000000,
					0b10000000,
					0b10000000
				};
				IO.slave->pointer = true;
				m68k_set_irq(2);
			}

			else if (changed || buttons[Left] || buttons[Right] || buttons[Down] || buttons[Up])
			{
				x = std::clamp(x + (buttons[Left] && !buttons[Right] ? -2
																	 : !buttons[Left] && buttons[Right] ? 2
																	 : 0), 0, 767);
				y = std::clamp(y + (buttons[Up] && !buttons[Down] ? -2
																  : !buttons[Up] && buttons[Down] ? 2
																  : 0), 0, 559);

				// Convert to SLAVE response (allowed coord bounds: 54x97 to 704x679?)
				IO.slave->Ch[0].Out =
				{
					(uint8_t)((x & 0x380 >> 7) | (buttons[Button2] << 5) | (buttons[Button1] << 4) | 0x08),
					(uint8_t)(x & 0x7f),
					(uint8_t)(y & 0x380 >> 7),
					(uint8_t)(y & 0x7f)
				};
				// MiniCDI::Log("[PD] x=%d,y=%d,b1=%d,b2=%d", x, y, buttons[Button1], buttons[Button2]);
				m68k_set_irq(2);
				changed = false;
			}
		}

		if (IO.ikat && IO.ikat->has_pointer && (changed || buttons[Left] || buttons[Right] || buttons[Down] || buttons[Up])) {
			switch (type)
			{
				case RelativePD:
					x = buttons[Left] && !buttons[Right] ? -1 : !buttons[Left] && buttons[Right] ? 1 : 0;
					y = buttons[Up] && !buttons[Down] ? -1 : !buttons[Up] && buttons[Down] ? 1 : 0;
					IO.ikat->Ch[1].Out =
					{
						(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x >> 4 & 0x0C) | (y >> 6 & 0x03)),
						(uint8_t)(x & 0x3F),
						(uint8_t)(y & 0x3F),
						0x00
					};
					break;

				case AbsolutePD:
					x = std::clamp(x + (buttons[Left] && !buttons[Right] ? -2
																		 : !buttons[Left] && buttons[Right] ? 2
																		 : 0), 0, 1023);
					y = std::clamp(y + (buttons[Up] && !buttons[Down] ? -2
																	  : !buttons[Up] && buttons[Down] ? 2
																	  : 0), 0, 1023);
					IO.ikat->Ch[1].Out =
					{
						(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x >> 6 & 0x0F)),
						(uint8_t)(0x10 | (y >> 6 & 0x0F)),
						(uint8_t)(x & 0x3F),
						(uint8_t)(0x80 | (y & 0x3F)),
					};
					break;
			}

			changed = false;

			// REMTY OFF + INT
			IO.ikat->Ch[1].SR &= ~(0x10);
			IO.ikat->ISR |= 0x08;
			if (IO.ikat->IMR & 0x08) m68k_set_irq(2);
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