#ifndef MINICDI_POINTINGDEVICE
#define MINICDI_POINTINGDEVICE

class PointingDevice
{
	int MIN_POINTER_X = 0;
	int MIN_POINTER_Y = 0;
	int MAX_POINTER_X = 768;
	int MAX_POINTER_Y = 560;
	static constexpr int POINTER_ADVANCE = 1;

	bool buttons[6];
	bool has_packet = false;
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
		if (!has_packet) return;

		if (IO.slave != NULL)
		{
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

				else
				{
					// Convert to SLAVE response
					IO.slave->Ch[0].Out =
					{
						(uint8_t)((x >> 7 & 0x07) | (buttons[Button2] << 5) | (buttons[Button1] << 4) | 0x08),
						(uint8_t)(x & 0x7f),
						(uint8_t)(y >> 7 & 0x07),
						(uint8_t)(y & 0x7f)
					};
					IO.slave->assert_irq();
				}
			}
		}

		else if (IO.ikat != NULL)
		{
			if (IO.ikat->PointerInterface.posChanged) {
				x = IO.ikat->PointerInterface.x;
				// y = IO.ikat->PointerInterface.y;
				IO.ikat->PointerInterface.posChanged = false;
			}

			if (IO.ikat->PointerInterface.connected)
			{
				/*// Convert to IKAT response

				// Absolute coordinates (absolute device)
				if (IO.ikat->PointerInterface.absolute || x >= 128 || y >= 128)
				{
					MAX_POINTER_X = 16;
					MAX_POINTER_Y = 18;

					IO.ikat->Ch[1].Out =
					{
						(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x & 0b1111000000 >> 6)),
						(uint8_t)((1 << 4) | (y & 0b1111000000 >> 6)),
						(uint8_t)(x & 0b0000111111),
						(uint8_t)(0x80 | (y & 0b0000111111)),
					};
					IO.ikat->poll_packet(1);
				}

				// Relative coordinates (relative or maneuvering device)
				else if (!IO.ikat->PointerInterface.absolute)
				{
					MAX_POINTER_X = MAX_POINTER_Y = 128;
					MIN_POINTER_X = MIN_POINTER_Y = -128;

					IO.ikat->Ch[1].Out =
					{
						(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x & 0b11000000 >> 4) | (y & 0b11000000 >> 6)),
						(uint8_t)(x & 0b00111111),
						(uint8_t)(y & 0b00111111),
						0,
					};
					IO.ikat->poll_packet(1);
				}*/
			}
		}

		has_packet = false;
	}

	void set_button(enum Buttons b, bool value)
	{
		/*if (this->buttons[(int)b] != value)
			MiniCDI::Log("[PD] set pointer button %d = %d", (int)b, value);*/

		if (b == Left || b == Right || b == Down || b == Up) {
			this->buttons[(int)b] = value;

			if (IO.ikat != NULL && !IO.ikat->PointerInterface.absolute)
			{
				// Relative coordinates
				x = buttons[Left] && !buttons[Right] ? POINTER_ADVANCE * -1
				  : !buttons[Left] && buttons[Right] ? POINTER_ADVANCE
				  : 0;
				y = buttons[Up] && !buttons[Down] ? POINTER_ADVANCE * -1
				  : !buttons[Up] && buttons[Down] ? POINTER_ADVANCE
				  : 0;
			}
			else
			{
				// Absolute coordinates
				x = std::clamp(x + (buttons[Left] && !buttons[Right] ? POINTER_ADVANCE * -1
																	 : !buttons[Left] && buttons[Right] ? POINTER_ADVANCE
																	 : 0), MIN_POINTER_X, MAX_POINTER_X-1);
				y = std::clamp(y + (buttons[Up] && !buttons[Down] ? POINTER_ADVANCE * -1
																  : !buttons[Up] && buttons[Down] ? POINTER_ADVANCE
																  : 0), MIN_POINTER_Y, MAX_POINTER_Y-1);
			}

			//MiniCDI::Log("[PD] x=%d,y=%d", x, y);
		}

		has_packet = this->buttons[Left]
				  || this->buttons[Right]
				  || this->buttons[Down]
				  || this->buttons[Up];

		if ((b == Button1 || b == Button2) && this->buttons[(int)b] != value) {
			this->buttons[(int)b] = value;
			has_packet = true;
		}

	}

	void set_coord(float x, float y)
	{
		if (x < 0 || y < 0 || x > 1 || y > 1) return;

		this->x = (int)(x*(float)MAX_POINTER_X) - MIN_POINTER_X;
		this->y = (int)(y*(float)MAX_POINTER_Y) - MIN_POINTER_Y;
		has_packet = true;
	}
};

#endif