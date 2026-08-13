#include "cdi/common.hpp"

void PointingDevice::send_packet()
{
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
				switch (type)
				{
					case PointingDevice::Relative:
						IO.slave->Ch[0].Out = { static_cast<uint8_t>(0x80 | 'M') };
						IO.slave->assert_irq(0);
						return;

					case PointingDevice::Maneuvering:
						IO.slave->Ch[0].Out = { static_cast<uint8_t>(0x80 | 'J') };
						IO.slave->assert_irq(0);
						return;

					case PointingDevice::Absolute:
						IO.slave->Ch[0].Out = { static_cast<uint8_t>(0x80 | 'T') };
						IO.slave->assert_irq(0);
						return;

					default:
						assert(0 && "Invalid pointing device type.");
						break;
				}
			}

			else if (poll_movement || poll_stationary || poll_state_changed)
			{
				// Convert to SLAVE response
				switch (type)
				{
					case PointingDevice::Absolute:
						IO.slave->Ch[0].Out =
						{
							static_cast<uint8_t>((xA >> 7 & 0x07) | (buttons[Button2] << 5) | (buttons[Button1] << 4) | 0x08),
							static_cast<uint8_t>(xA & 0x7f),
							static_cast<uint8_t>(yA >> 7 & 0x07),
							static_cast<uint8_t>(yA & 0x7f)
						};
						IO.slave->assert_irq(0);
						return;

					default:
					case PointingDevice::Relative:
					case PointingDevice::Maneuvering:
					{
						IO.slave->Ch[0].Out =
						{
							static_cast<uint8_t>((xA >> 7 & 0x07) | (buttons[Button2] << 5) | (buttons[Button1] << 4) | 0x88),
							static_cast<uint8_t>(xA & 0x7f),
							static_cast<uint8_t>(yA >> 7 & 0x07),
							static_cast<uint8_t>(yA & 0x7f)
						};
						//MiniCDI::Log("[PD] Sending to SLAVE: %02X %02X %02X %02X", IO.slave->Ch[0].Out[0], IO.slave->Ch[0].Out[1], IO.slave->Ch[0].Out[2], IO.slave->Ch[0].Out[3]);
						IO.slave->assert_irq(0);
						return;
					}
				}
			}
		}
	}

	else if (IO.ikat != NULL) {
		if (IO.ikat->PointerInterface.connected && (poll_movement || poll_stationary || poll_state_changed)) {
			uint16_t x = 0, y = 0;

			switch (type)
			{
				case PointingDevice::Absolute:
					x = std::clamp(static_cast<int>((xA / static_cast<float>(MAX_POINTER_X)) * 0x3FF), 0, 0x3FF);
					y = std::clamp(static_cast<int>((yA / static_cast<float>(MAX_POINTER_Y)) * 0x3FF), 0, 0x3FF);

					// Convert to IKAT response (absolute coordinates)
					// Data format partially taken from CeDImu.
					IO.ikat->poll_packet(1,
						0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x >> 6 & 0xF),
						(poll_movement << 5) | (y >> 6 & 0xF),
						x & 0x3F,
						0x80 | (y & 0x3F)
					);
					return;

				default:
				case PointingDevice::Relative:
				case PointingDevice::Maneuvering:
					x = xR > 0 ? std::clamp(xR, 0x01, 0x7F) : xR < 0 ? std::clamp(0x100 + xR, 0x80, 0xFF) : 0;
					y = yR > 0 ? std::clamp(yR, 0x01, 0x7F) : yR < 0 ? std::clamp(0x100 + yR, 0x80, 0xFF) : 0;

					// Convert to IKAT response (relative coordinates)
					// Data format partially taken from CeDImu.
					IO.ikat->poll_packet(1,
						0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x & 0b11000000 >> 6) | (y & 0b11000000 >> 4),
						x & 0x3F,
						y & 0x3F
					);
					return;
			}
		}
	}

	poll_movement = false;
	poll_stationary = false;
	poll_state_changed = false;
}

void PointingDevice::set_button(enum PointingDevice::Buttons b, bool value)
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

		xA = std::clamp(xA + xR, 0, MAX_POINTER_X);
		yA = std::clamp(yA + yR, 0, MAX_POINTER_Y);
		// MiniCDI::Log("[PD] x=%d,y=%d", x, y);
	}
	else
	{
		// Susceptible to input lag ??
		poll_movement = false;
		if (xR != 0) { xR = 0; if (type != PointingDevice::Absolute) { poll_stationary = true; } }
		if (yR != 0) { yR = 0; if (type != PointingDevice::Absolute) { poll_stationary = true; } }
	}

	if ((b == Button1 || b == Button2) && this->buttons[(int)b] != value)
	{
		poll_state_changed = true;
		this->buttons[(int)b] = value;
		//MiniCDI::Log("[PD] B1=%d,B2=%d", this->buttons[Button1], this->buttons[Button2]);
	}
}

void PointingDevice::set_coord(int x, int y, int w, int h)
{
	// Convert to native CD-i highres
	// Actual formula is (x / w) * 768 and (y / h) * 560 but has been optimized.
	float new_x = static_cast<float>(x) / static_cast<float>(w) * static_cast<float>(MAX_POINTER_X);
	float new_y = static_cast<float>(y) / static_cast<float>(h) * static_cast<float>(MAX_POINTER_Y);
	x = static_cast<int>(new_x);
	y = static_cast<int>(new_y);

	if (x < 0 || y < 0 || x > MAX_POINTER_X || y > MAX_POINTER_Y) return;

	poll_movement = this->xA != x || this->yA != y;
	if (type != PointingDevice::Absolute && (this->xR != 0 || this->yR != 0) && !poll_movement) poll_stationary = true;

	this->xR = poll_movement ? x - xA : 0;
	this->yR = poll_movement ? y - yA : 0;
	this->xA = x;
	this->yA = y;
	// MiniCDI::Log("[PD] x=%d,y=%d", xA, yA);
}