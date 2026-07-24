#ifndef MINICDI_POINTINGDEVICE
#define MINICDI_POINTINGDEVICE

class PointingDevice
{
	static constexpr int MAX_POINTER_X = 768;
	static constexpr int MAX_POINTER_Y = 560;

	bool buttons[6];
	bool poll_movement = false;
	bool poll_stationary = false; // only used for maneuvering devices
	bool poll_state_changed = false;
	int xR = 0, yR = 0, xA = MAX_POINTER_X/2, yA = MAX_POINTER_Y/2;
	bool absolute = false;

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

	inline void send_packet()
	{
		uint16_t x, y;

		if (IO.slave != NULL) {
			if (IO.slave->PointerInterface.posChanged) {
				xA = IO.slave->PointerInterface.x;
				yA = IO.slave->PointerInterface.y;
				IO.slave->PointerInterface.posChanged = false;
			}

			if (IO.slave->PointerInterface.enabled) {
				if (!IO.slave->PointerInterface.connected)
				{
					IO.slave->PointerInterface.connected = true;

					// Send identification byte to SLAVE
					x = MAX_POINTER_X/2;
					y = MAX_POINTER_Y/2;
					IO.slave->Ch[0].Out =
					{
						(uint8_t)((absolute ? 'T' : 'J') | 0x80),
						(uint8_t)((x >> 7 & 0x07) | (buttons[Button2] << 5) | (buttons[Button1] << 4) | 0x08),
						(uint8_t)(x & 0x7f),
						(uint8_t)(y >> 7 & 0x07),
						(uint8_t)(y & 0x7f)
					};
					IO.slave->assert_irq();
				}

				else if (poll_movement || poll_stationary || poll_state_changed)
				{
					MiniCDI::Log("[PD] x=%d,y=%d", xA, yA);
					x = xA;
					y = yA;

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

		else if (IO.ikat != NULL) {
			if (IO.ikat->PointerInterface.connected && (poll_movement || poll_stationary || poll_state_changed)) {
				if (absolute)
				{
					x = std::clamp(static_cast<int>((xA / 768.0f) * 0x3FF), 0, 0x3FF);
					y = std::clamp(static_cast<int>((yA / 560.0f) * 0x3FF), 0, 0x3FF);
				}
				else
				{
					x = xR > 0 ? std::clamp(xR, 0x01, 0x7F) : xR < 0 ? std::clamp(0x100 + xR, 0x80, 0xFF) : 0;
					y = yR > 0 ? std::clamp(yR, 0x01, 0x7F) : yR < 0 ? std::clamp(0x100 + yR, 0x80, 0xFF) : 0;
				}

				// Convert to IKAT response
				if (absolute)
				{
					// Absolute coordinates
					IO.ikat->Ch[1].Out =
					{
						(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x & 0b1111000000 >> 6)),
						(uint8_t)((poll_movement << 4) | (y & 0b1111000000 >> 6)),
						(uint8_t)(x & 0b0000111111),
						(uint8_t)(0x80 | (y & 0b0000111111)),
					};
				}
				else
				{
					// Relative coordinates
					IO.ikat->Ch[1].Out =
					{
						(uint8_t)(0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x & 0b11000000 >> 6) | (y & 0b11000000 >> 4)),
						(uint8_t)(x & 0b00111111),
						(uint8_t)(y & 0b00111111)
					};
				}
				IO.ikat->poll_packet(1);
			}
		}

		poll_stationary = false;
		poll_state_changed = false;
	}

	inline void set_button(enum Buttons b, bool value)
	{
		if (b == Left || b == Right || b == Down || b == Up) this->buttons[(int)b] = value;
		if (this->buttons[Left] || this->buttons[Right] || this->buttons[Down] || this->buttons[Up])
		{
			poll_movement = true;
			xR = buttons[Left] && !buttons[Right] ? 0 - MiniCDI::Config::PointerAdvance
			   : !buttons[Left] && buttons[Right] ? MiniCDI::Config::PointerAdvance
			   : 0;
			yR = buttons[Up] && !buttons[Down] ? 0 - MiniCDI::Config::PointerAdvance
			   : !buttons[Up] && buttons[Down] ? MiniCDI::Config::PointerAdvance
			   : 0;

			xA = std::clamp(xA + (buttons[Left] && !buttons[Right] ? 0 - MiniCDI::Config::PointerAdvance
							   : !buttons[Left] && buttons[Right] ? MiniCDI::Config::PointerAdvance
							   : 0), 0, MAX_POINTER_X);
			yA = std::clamp(yA + (buttons[Up] && !buttons[Down] ? 0 - MiniCDI::Config::PointerAdvance
							   : !buttons[Up] && buttons[Down] ? MiniCDI::Config::PointerAdvance
							   : 0), 0, MAX_POINTER_Y);
			// MiniCDI::Log("[PD] x=%d,y=%d", x, y);
		}
		else
		{
			// Susceptible to input lag ??
			poll_movement = false;
			if (xR != 0) { xR = 0; if (!absolute) { poll_stationary = true; } }
			if (yR != 0) { yR = 0; if (!absolute) { poll_stationary = true; } }
		}

		if ((b == Button1 || b == Button2) && this->buttons[(int)b] != value)
		{
			poll_state_changed = true;
			this->buttons[(int)b] = value;
			//MiniCDI::Log("[PD] B1=%d,B2=%d", this->buttons[Button1], this->buttons[Button2]);
		}
	}

	inline void set_coord(int x, int y, int w, int h)
	{
		// Convert to native CD-i highres
		x = static_cast<int>((float)x / (float)w * 768.0f);
		y = static_cast<int>((float)y / (float)h * 560.0f);

		if (x < 0 || y < 0 || x > MAX_POINTER_X || y > MAX_POINTER_Y) return;

		poll_movement = this->xA != x || this->yA != y;
		if (!absolute && (this->xR != 0 || this->yR != 0) && !poll_movement) poll_stationary = true;

		this->xR = poll_movement ? x - xA : 0;
		this->yR = poll_movement ? y - yA : 0;
		this->xA = x;
		this->yA = y;
	}
};

#endif