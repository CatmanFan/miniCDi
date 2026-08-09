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
	bool absolute = true;

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

	void send_packet();
	void set_button(enum Buttons b, bool value);
	void set_coord(int x, int y, int w, int h);
};

#endif