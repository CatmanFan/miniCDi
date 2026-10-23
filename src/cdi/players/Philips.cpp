#include "cdi/m68k/m68kcpu.h"
#include "cdi/common.hpp"
#include <numeric>

// static int cpu_cycles = 0;
static int sector_cycles = 0;
static int mcd212_cycles = 0;
static int timer0_cycles = 0;
static int uart_cycles = 0;

void PhilipsCDI::run_frame(bool no_draw)
{
	#ifdef MINICDI_DEBUG_OS9
		MiniCDI::OS9::scan_modules(memory);
	#endif

	// Update microcontroller
	pd.send_packet();
	if (ikat) ikat->update();

	/// Per CD-i 220 Service manual :
	/// SYSCLK (MCD212): 30 MHz (PAL), 30.2098 MHz (NTSC)
	/// SYSCLK/2 (CPU): 15 MHz (PAL), 15.1049 MHz (NTSC)
	/// UART: 4.9152 MHz

	// int cycles_per_frame = MiniCDI::Config.PAL ? 599040 : 503040;

	#define CYCLE_INCREMENT_RATE 48 /* 288 */

	#define EVENT_BEGIN(cycles, rate)	cycles += CYCLE_INCREMENT_RATE; \
										while (cycles >= (rate<<1)) \
										{
													
	#define EVENT_END(cycles, rate)			cycles -= (rate<<1); \
										}

	// for (int cycles = 0; cycles < cycles_per_frame; cycles += CYCLE_INCREMENT_RATE)
	while (1)
	{
		m68k_execute(CYCLE_INCREMENT_RATE);

		EVENT_BEGIN(timer0_cycles, 96)
			cpu.timer0_tick();
		EVENT_END(timer0_cycles, 96)

		// EVENT_BEGIN(uart_cycles, uart_rate)
			// cpu.uart_tx_tick();
		// EVENT_END(uart_cycles, uart_rate)

		EVENT_BEGIN(sector_cycles, 400000)
			if (disc.is_open())
			{
				if (cdic) cdic->tick();
				if (dsp) dsp->tick();
				if (ciap) ciap->tick();
			}
		EVENT_END(sector_cycles, 400000)

		/// SYSCLK /= PAL line frequency (15625) or NTSC line frequency (15734)
		EVENT_BEGIN(mcd212_cycles, 960)
			if (vpu != NULL)
			{
				vpu->skip_draw = no_draw;
				vpu->tick();
			}
		EVENT_END(mcd212_cycles, 960)

		if (vpu != NULL && vpu->frame_ready)
		{
			vpu->frame_ready = false;
			break;
		}
	}
}

void PhilipsCDI::reset()
{
	MiniCDI::Log("[CDI] reset");
	MiniCDI::OS9::clear_modules();

	nvram_save();
	cpu.reset();
	ftd.reset();
	vpu->reset();

	// Microcontroller
	if (slave != NULL) slave->reset();
	if (ikat != NULL) ikat->reset();

	// CD-Audio
	if (cdic != NULL) cdic->reset();
	if (dsp != NULL) dsp->reset();
	if (ciap != NULL) ciap->reset();
}

void PhilipsCDI::reset_pd()
{
	pd = PointingDevice();
	if (slave != NULL) this->pd.IO.slave = slave;
	if (ikat != NULL) this->pd.IO.ikat = ikat;
}

uint8_t PhilipsCDI::read8(int address)
{
	if (check_for_unmapped(address))
	{
		// 68070 (On-chip)
		if ((address & 0xC0000000) == 0x80000000)
			return cpu.read8(address);

		// Slave microcontroller bus
		else if ((address & 0x00FFFF00) == 0x00310000)
		{
			if (slave != NULL)
				return slave->read8(address);
			if (ikat != NULL)
				return ikat->read8(address);
		}

		// CD+Audio microcontroller bus
		else if ((address & 0x00FF0000) == 0x00300000 && dsp)
		{
			address &= 0x00FF3FFF;
			return dsp->read8(address);
		}

		// The Video Processing unit
		else if ((address & 0x00FFFF00) == 0x004FFF00 && vpu)
		{
			return vpu->read8(address);
		}

		// RAM/ROM/everything else?
		else if (address < memsize)
			return memory[address];
	}

	return 0;
}

uint16_t PhilipsCDI::read16(int address)
{
	if (check_for_unmapped(address))
	{
		// 68070 (On-chip)
		if ((address & 0xC0000000) == 0x80000000)
			return cpu.read8(address);

		// Slave microcontroller bus
		else if ((address & 0x00FFFF00) == 0x00310000)
		{
			if (slave)
				return slave->read8(address);
			if (ikat)
				return ikat->read8(address);
		}

		// CD+Audio microcontroller bus
		else if ((address & 0x00FF0000) == 0x00300000)
		{
			address &= 0x00FF3FFF;
			if (cdic)
				return cdic->read16(address);
			if (ciap)
				return ciap->read16(address);
		}

		// The Video Processing unit
		else if ((address & 0x00FFFF00) == 0x004FFF00 && vpu)
			return vpu->read16(address);

		else return (read8(address) << 8) | read8(address+1);
	}

	return 0;
}

uint32_t PhilipsCDI::read32(int address)
{
	if (check_for_unmapped(address))
	{
		// CD+Audio microcontroller bus
		if ((address & 0x00FF0000) == 0x00300000 && cdic)
		{
			address &= 0x00FF3FFF;
			return cdic->read32(address);
		}

		else return (read16(address) << 16) | read16(address+2);
	}

	return 0;
}

void PhilipsCDI::write8(int address, uint8_t value)
{
	if (check_for_unmapped(address))
	{
		// 68070 (On-chip)
		if ((address & 0xC0000000) == 0x80000000)
			cpu.write8(address, value);

		// Slave microcontroller bus
		else if ((address & 0x00FFFF00) == 0x00310000)
		{
			if (slave)
				slave->write8(address, value);
			if (ikat)
				ikat->write8(address, value);
		}

		// CD+Audio microcontroller bus
		else if ((address & 0x00FF0000) == 0x00300000 && dsp)
		{
			address &= 0x00FF3FFF;
			dsp->write8(address, value);
		}

		// Dead Timekeeper/NVRAM will not allow writing
		// else if ((address & 0x00FFFF00) != 0x00320000)
		else if (address < memsize)
			memory[address] = value;
	}
}

void PhilipsCDI::write16(int address, uint16_t value)
{
	if (check_for_unmapped(address))
	{
		// CD+Audio microcontroller bus
		if ((address & 0x00FF0000) == 0x00300000)
		{
			address &= 0x00FF3FFF;
			if (cdic)
				cdic->write16(address, value);
			if (ciap)
				ciap->write16(address, value);
		}

		// The Video Processing unit
		else if ((address & 0x00FFFF00) == 0x004FFF00 && vpu)
			vpu->write16(address, value);

		else
		{
			write8(address, value >> 8 & 0xFF);
			write8(address+1, value & 0xFF);
		}
	}
}

void PhilipsCDI::write32(int address, uint32_t value)
{
	if (check_for_unmapped(address))
	{
		// CD+Audio microcontroller bus
		if ((address & 0x00FF0000) == 0x00300000 && cdic)
		{
			address &= 0x00FF3FFF;
			cdic->write32(address, value);
		}

		else
		{
			write16(address, value >> 16 & 0xFFFF);
			write16(address+2, value & 0xFFFF);
		}
	}
}

void PhilipsCDI::play_disc()
{
	if (slave)
		slave->send_play_button();
}

void PhilipsCDI::swap_disc(const std::string &path)
{
	// Clear CD+Audio controller attributes of current disc prior to ejecting
	if (cdic) cdic->reset();

	disc.eject();

	// Read new disc
	if (!disc.open(path)) return;
	if (slave) slave->send_disc_status(true);
	if (ikat) ikat->send_disc_status(true);
}