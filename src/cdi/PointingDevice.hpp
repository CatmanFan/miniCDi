#ifndef MINICDI_POINTINGDEVICE
#define MINICDI_POINTINGDEVICE

class PointingDevice
{
	static constexpr int MAX_POINTER_X = 768;
	static constexpr int MAX_POINTER_Y = 560;
	static constexpr int POINTER_ADVANCE = 1;

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
		if (IO.slave) {
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

				else if (changed_Face || changed_DPad)
				{
					if (changed_Face) { changed_Face = false; }

					// Convert to SLAVE response (allowed coord bounds: 54x97 to 704x679?)
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

		else if (IO.ikat) {
			if (IO.ikat->PointerInterface.posChanged) {
				x = IO.ikat->PointerInterface.x;
				// y = IO.ikat->PointerInterface.y;
				IO.ikat->PointerInterface.posChanged = false;
			}

			if (IO.ikat->PointerInterface.connected) {
				/*if (changed_Face || changed_DPad)
				{
					if (changed_Face) { changed_Face = false; }

					// Convert to IKAT response
					if (x < 128 && x >= -128 && y < 128 && y >= -128) {
						// Relative coordinates
						IO.ikat->Ch[1].Out =
						{
							(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x & 0b11000000 >> 4) | (y & 0b11000000 >> 6)),
							(uint8_t)(x & 0b00111111),
							(uint8_t)(y & 0b00111111),
							0,
						};
					} else {
						// Absolute coordinates
						IO.ikat->Ch[1].Out =
						{
							(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x & 0b1111000000 >> 6)),
							(uint8_t)(((changed_DPad || changed_Face ? 0x01 : 0x00) << 4) | (y & 0b1111000000 >> 6)),
							(uint8_t)(x & 0b0000111111),
							(uint8_t)(0x80 | (y & 0b0000111111)),
						};
					}
					IO.ikat->poll_packet(1);
				}*/
			}
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

		if (changed_DPad)
		{
			x = std::clamp(x + (buttons[Left] && !buttons[Right] ? POINTER_ADVANCE * -1
																 : !buttons[Left] && buttons[Right] ? POINTER_ADVANCE
																 : 0), 0, 767);
			y = std::clamp(y + (buttons[Up] && !buttons[Down] ? POINTER_ADVANCE * -1
															  : !buttons[Up] && buttons[Down] ? POINTER_ADVANCE
															  : 0), 0, 559);
			//MiniCDI::Log("[PD] x=%d,y=%d", x, y);
		}

		if ((b == Button1 || b == Button2) && this->buttons[(int)b] != value) {
			this->buttons[(int)b] = value;
			changed_Face = true;
		}
	}

	void set_coord(float x, float y)
	{
		if (x < 0 || y < 0 || x > 1 || y > 1) return;

		this->x = (int)(x*(IO.ikat ? 16.0f : 768.0f));
		this->y = (int)(y*(IO.ikat ? 16.0f : 560.0f));
		changed_DPad = true;
	}
};

#endif