#ifndef MINICDI_POINTINGDEVICE
#define MINICDI_POINTINGDEVICE

class PointingDevice
{
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

	enum PDType
	{
		Relative = 0,
		Maneuvering,
		Absolute
	};

private:
	static constexpr int MAX_POINTER_X = 767;
	static constexpr int MAX_POINTER_Y = 559;

	bool buttons[6];
	bool poll_movement = false;
	bool poll_stationary = false; // only used for maneuvering devices
	bool poll_state_changed = false;
	int xR = 0, yR = 0, xA = MAX_POINTER_X/2, yA = MAX_POINTER_Y/2;
	enum PDType type = PointingDevice::Maneuvering;

public:
	struct
	{
		SLAVE* slave = NULL;
		IKAT* ikat = NULL;
	} IO;

	void send_packet();
	void set_button(enum Buttons b, bool value);
	void set_coord(int x, int y, int w, int h);
};

#endif