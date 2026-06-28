#ifndef MINICDI_POINTINGDEVICE
#define MINICDI_POINTINGDEVICE

class PointingDevice
{
	static constexpr int MAX_POINTER_X = 768;
	static constexpr int MAX_POINTER_Y = 560;
	static constexpr int POINTER_ADVANCE = 1;

	bool buttons[6];
	bool must_poll = false; // set to true if and ONLY if a face button has changed state or D-Pad is active.
	int x = 0, y = 0;

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
		SLAVE* slave = NULL;
		IKAT* ikat = NULL;
	} IO;

	void send_packet()
	{
		if (IO.slave != NULL) {
			if (IO.slave->PointerInterface.posChanged) {
				x = IO.slave->PointerInterface.x;
				// y = IO.slave->PointerInterface.y;
				IO.slave->PointerInterface.posChanged = false;
			}

			if (IO.slave->PointerInterface.enabled) {
				if (!IO.slave->PointerInterface.connected)
				{
					IO.slave->Ch[0].Out = {'M'};
					IO.slave->PointerInterface.connected = true;
					IO.slave->assert_irq();
				}

				else if (must_poll)
				{
					// Convert to SLAVE response (allowed coord bounds: 54x97 to 704x679?)
					IO.slave->Ch[0].Out =
					{
						(uint8_t)((x >> 7 & 0x07) | (buttons[Button2] << 5) | (buttons[Button1] << 4) | 0x08),
						(uint8_t)(x & 0x7f),
						(uint8_t)(y >> 7 & 0x07),
						(uint8_t)(y & 0x7f)
					};
					IO.slave->assert_irq();

					if (!(this->buttons[Left] || this->buttons[Right] || this->buttons[Down] || this->buttons[Up]))
						must_poll = false;
					return;
				}
			}
		}

		else if (IO.ikat != NULL) {
			if (IO.ikat->PointerInterface.posChanged) {
				x = IO.ikat->PointerInterface.x;
				// y = IO.ikat->PointerInterface.y;
				IO.ikat->PointerInterface.posChanged = false;
			}

			if (IO.ikat->PointerInterface.connected && must_poll) {
				// Convert to IKAT response
				/*if (x < 128 && x >= -128 && y < 128 && y >= -128)
				{
					// Relative coordinates
					IO.ikat->Ch[1].Out =
					{
						(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x & 0b11000000 >> 4) | (y & 0b11000000 >> 6)),
						(uint8_t)(x & 0b00111111),
						(uint8_t)(y & 0b00111111),
						0,
					};
				}
				else*/
				{
					// Absolute coordinates
					IO.ikat->Ch[1].Out =
					{
						(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x & 0b1111000000 >> 6)),
						(uint8_t)((1 << 4) | (y & 0b1111000000 >> 6)),
						(uint8_t)(x & 0b0000111111),
						(uint8_t)(0x80 | (y & 0b0000111111)),
					};
				}
				IO.ikat->poll_packet(1);

				if (!(this->buttons[Left] || this->buttons[Right] || this->buttons[Down] || this->buttons[Up]))
					must_poll = false;
				return;
			}
		}

		must_poll = false;
	}

	void set_button(enum Buttons b, bool value)
	{
		if (b == Left || b == Right || b == Down || b == Up) this->buttons[(int)b] = value;
		if (this->buttons[Left] || this->buttons[Right] || this->buttons[Down] || this->buttons[Up])
		{
			must_poll = true;
			x = std::clamp(x + (buttons[Left] && !buttons[Right] ? POINTER_ADVANCE * -1
																 : !buttons[Left] && buttons[Right] ? POINTER_ADVANCE
																 : 0), 0, 767);
			y = std::clamp(y + (buttons[Up] && !buttons[Down] ? POINTER_ADVANCE * -1
															  : !buttons[Up] && buttons[Down] ? POINTER_ADVANCE
															  : 0), 0, 559);
			MiniCDI::Log("[PD] x=%d,y=%d", x, y);
		}

		if ((b == Button1 || b == Button2) && this->buttons[(int)b] != value)
		{
			must_poll = true;
			this->buttons[(int)b] = value;
			MiniCDI::Log("[PD] 1=%d,2=%d", this->buttons[Button1], this->buttons[Button2]);
		}
	}

	void set_coord(float x, float y)
	{
		if (x < 0 || y < 0 || x > 1 || y > 1) return;

		must_poll = true;
		this->x = (int)(x*(IO.ikat ? 16.0f : 768.0f));
		this->y = (int)(y*(IO.ikat ? 16.0f : 560.0f));
	}
};

#endif