#ifndef MINICDI_SCC68070
#define MINICDI_SCC68070

class SCC68070
{
	uint8_t* memory;

	// On-chip peripherals
	uint8_t LIR = 0;
	uint8_t PICR[2] = {0,0};

	/** UART **/
	uint8_t UMR = 0; // Mode Register
	uint8_t USR = 0; // Status Register
	uint8_t UCS = 0; // Clock Select Register
	uint8_t UCR = 0; // Command Register
	struct {
		uint8_t HR = 0; // Transmit Holding Register
		std::vector<uint8_t> chars;
		int clock = 0;
	} UART_T;
	uint8_t URH = 0; // Receive Holding Register

	/** Timer **/
	uint8_t TSR = 0;
	uint8_t TCR = 0;
	uint16_t RR = 0;
	uint16_t T[3] = {0,0,0}; // only Timer 0 is used in practice.
	int T_cycles[3] = {0,0,0};

	/** DMA **/
	struct {
		uint8_t CSR = 0;
		uint8_t CER = 0;

		uint8_t DCR = 0;
		uint8_t OCR = 0;
		uint8_t SCR = 0;
		uint8_t CCR = 0;

		uint16_t MTC = 0;
		uint32_t MAC = 0;
		uint32_t DAC = 0;

		uint8_t CPR = 0;
	} DMA[2];

	void uart_log_tx();
	void dma_call(size_t index, uint32_t start_address);

	/** I²C **/
	uint8_t IDR = 0;
	uint8_t IAR = 0;
	uint8_t ISR = 0;
	uint8_t ICR = 0;
	uint8_t ICCR = 0;

public:
	uint8_t fc = 0; // used for FC/address space callback
	friend class PointingDevice;
	friend class CDIC;
	friend class CIAP;

	// Priority order of interrupt signal booleans
	enum IPLSignal
	{
		IPL_IN7N = 0, // NMI, should always come first.
		IPL_IN5N,
		IPL_IN4N, // CD/audio device
		IPL_IN2N, // microcontroller device
		IPL_INT1, // VDSC/video (responsible for Ev$Pulse+Ev$All syscalls)
		IPL_INT2,
		IPL_TIMER,
		IPL_UART_RX,
		IPL_UART_TX,
		IPL_I2C,
		IPL_DMA1,
		IPL_DMA2
	};
	struct
	{
		uint8_t curr = 0; // Current pending interrupt level
		uint8_t ack = 0; // Acknowledged interrupt level
		bool levels[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
		uint8_t vectors[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
	} Ipl;

	/**
	 * @brief  Sets a peripheral's (or external's) pending interrupt to true or false.
	 *         It gets the level from the corresponding onchip register and then sets the Musashi IRQ through `update_ipl()`.
	 *
	 * @param  assert:  Whether the interrupt is asserted.
	 */
	void interrupt(size_t index, bool assert);
	int interrupt_ack(int int_level);

	SCC68070(uint8_t* memory) : memory(memory) { }

	void load_rom(std::vector<char> &rom);
	void reset_internal();
	void reset();

	uint8_t read8(uint32_t addr);
	void write8(uint32_t addr, uint8_t value);

	void run(int cycles);
	void timer0_tick();
	void uart_tx_tick();
};

#endif