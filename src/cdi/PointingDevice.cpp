#include "cdi/common.hpp"

void PointingDevice::send_packet()
{
	if (IO.slave != NULL)
	{
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
				IO.slave->Ch[0].Out =
				{
					static_cast<uint8_t>((xA >> 7 & 0x07) | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (type == PointingDevice::Absolute ? 0x08 : 0x88)),
					static_cast<uint8_t>(xA & 0x7f),
					static_cast<uint8_t>(yA >> 7 & 0x07),
					static_cast<uint8_t>(yA & 0x7f)
				};
				IO.slave->assert_irq(0);
			}
		}
	}

	else if (IO.ikat != NULL)
	{
		if (IO.ikat->PointerInterface.connected && (poll_movement || poll_stationary || poll_state_changed))
		{
			switch (type)
			{
				case PointingDevice::Absolute:
				{
					uint16_t x = std::clamp(static_cast<int>((xA / static_cast<float>(MAX_POINTER_X)) * 0x3FF), 0, 0x3FF);
					uint16_t y = std::clamp(static_cast<int>((yA / static_cast<float>(MAX_POINTER_Y)) * 0x3FF), 0, 0x3FF);

					// Convert to IKAT response (absolute coordinates)
					// Data format partially taken from CeDImu.
					IO.ikat->poll_packet(1,
						0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | (x >> 6 & 0xF),
						(poll_movement << 5) | (y >> 6 & 0xF),
						x & 0x3F,
						0x80 | (y & 0x3F)
					);
					return;
				}

				default:
				case PointingDevice::Relative:
				case PointingDevice::Maneuvering:
				{
					uint8_t x = xR > 127 ? 127 : xR < -128 ? -128 : xR;
					uint8_t y = yR > 127 ? 127 : yR < -128 ? -128 : yR;

					// Convert to IKAT response (relative coordinates)
					IO.ikat->poll_packet(1,
						0x40 | (buttons[Button2] << 5) | (buttons[Button1] << 4) | ((y >> 4) & 0x0C) | ((x >> 6) & 0x03),
						x & 0x3F,
						y & 0x3F,
						0x00
					);
					return;
				}
			}
		}
	}

	// poll_movement = false;
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

void PointingDevice::set_coord(int src_x, int src_y, int src_w, int src_h)
{
	if (src_x != source_coords.x || src_y != source_coords.y || src_w != source_coords.w || src_h != source_coords.h)
	{
		source_coords = { src_x, src_y, src_w, src_h };

		// Convert to native CD-i highres
		// Actual formula is (x / w) * 768 and (y / h) * 560 but has been optimized.
		float xF = static_cast<float>(source_coords.x) / static_cast<float>(source_coords.w) * static_cast<float>(MAX_POINTER_X);
		float yF = static_cast<float>(source_coords.y) / static_cast<float>(source_coords.h) * static_cast<float>(MAX_POINTER_Y);
		int x = std::clamp(static_cast<int>(xF), 0, MAX_POINTER_X);
		int y = std::clamp(static_cast<int>(yF), 0, MAX_POINTER_Y);

		this->xR = x - xA;
		this->yR = y - yA;
		this->xA = x;
		this->yA = y;
		poll_movement = true;
	}
	else
	{
		this->xR = 0;
		this->yR = 0;
		if (type != PointingDevice::Absolute && poll_movement) { poll_stationary = true; }
		poll_movement = false;
	}

	// MiniCDI::Log("[PD] x=%d,y=%d", xA, yA);
}