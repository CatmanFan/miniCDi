#ifndef MINICDI_POINTINGDEVICE
#define MINICDI_POINTINGDEVICE

class PointingDevice
{
	static constexpr int MAX_POINTER_X = 768;
	static constexpr int MAX_POINTER_Y = 560;

public:
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

	bool buttons[6];
	bool changed_Face;
	bool changed_DPad;
	int x = 0, y = 0;

	void send()
	{
		if (IO.slave && IO.slave->pointer_used) {
			if (!IO.slave->pointer)
			{
				IO.slave->Ch[0].Out = {'M'};
				IO.slave->pointer = true;
				m68k_set_irq(2);
			}

			if (IO.slave->pointer_posChanged) {
				// x = IO.slave->pointer_x;
				// y = IO.slave->pointer_y;
				IO.slave->pointer_posChanged = false;
			}

			else if (changed_Face || changed_DPad)
			{
				x = 550; y = 306;
				if (changed_Face) { changed_Face = false; }
				if (changed_DPad)
				{
					x = std::clamp(x + (buttons[Left] && !buttons[Right] ? -2
																		 : !buttons[Left] && buttons[Right] ? 2
																		 : 0), 0, 767);
					y = std::clamp(y + (buttons[Up] && !buttons[Down] ? -2
																	  : !buttons[Up] && buttons[Down] ? 2
																	  : 0), 0, 559);
				}

				// Convert to SLAVE response (allowed coord bounds: 54x97 to 704x679?)
				IO.slave->Ch[0].Out =
				{
					(uint8_t)((x >> 7 & 0x07) | (buttons[Button2] << 5) | (buttons[Button1] << 4) | 0x08),
					(uint8_t)(x & 0x7f),
					(uint8_t)(y >> 7 & 0x07),
					(uint8_t)(y & 0x7f)
				};
				MiniCDI::Log("[PD] x=%d,y=%d", x, y);
				m68k_set_irq(2);
			}
		}

		if (IO.ikat && IO.ikat->has_pointer && (changed_Face || changed_DPad)) {
			if (changed_Face) { changed_Face = false; }
			if (changed_DPad)
			{
				x = std::clamp(x + (buttons[Left] && !buttons[Right] ? -1
																	 : !buttons[Left] && buttons[Right] ? 1
																	 : 0), 0, 1023);
				y = std::clamp(y + (buttons[Up] && !buttons[Down] ? -1
																  : !buttons[Up] && buttons[Down] ? 1
																  : 0), 0, 1023);
			}

			IO.ikat->Ch[1].Out =
			{
				(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x >> 6 & 0x0F)),
				(uint8_t)(0x10 | (y >> 6 & 0x0F)),
				(uint8_t)(x & 0x3F),
				(uint8_t)(0x80 | (y & 0x3F)),
			};

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
			changed_DPad = true;
		} else {
			changed_DPad = this->buttons[Left] || this->buttons[Right] || this->buttons[Down] || this->buttons[Up];
		}

		if ((b == Button1 || b == Button2) && this->buttons[(int)b] != value) {
			this->buttons[(int)b] = value;
			changed_Face = true;
		}
	}
};

#endif