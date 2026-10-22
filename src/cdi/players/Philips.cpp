#include "cdi/common.hpp"
#include <numeric>

// static int cpu_cycles = 0;
static int timer_cycles = 192;
static int sector_cycles = 0;
static int mcd212_cycles = 0;

void PhilipsCDI::run_frame(bool no_draw)
{
	#ifdef MINICDI_DEBUG_OS9
		MiniCDI::OS9::scan_modules(memory);
	#endif

	// Update microcontroller
	pd.send_packet();
	if (ikat != NULL) ikat->update();

	int sector_rate, mcd212_rate, timer_rate = 192;
	if (MiniCDI::Config.PAL)
	{
		sector_rate = 400000;
		mcd212_rate = 1920;
	}
	else
	{
		sector_rate = 402797;
		mcd212_rate = 1933;
	}

	for (int cycles = 0; cycles < (mcd212_rate * (MiniCDI::Config.PAL ? 312 : 262));)
	{
		int executed_cycles = std::min({240, sector_cycles, mcd212_cycles, timer_cycles});
		m68k_execute(executed_cycles);
		// cpu_cycles += m68k_execute(1);

		timer_cycles -= executed_cycles;
		while (timer_cycles <= 0)
		{
			timer_cycles += timer_rate;
			cpu.timer0_tick();
		}

		sector_cycles -= executed_cycles;
		while (sector_cycles <= 0)
		{
			sector_cycles += sector_rate;
			if (disc.is_open())
			{
				if (cdic) cdic->tick();
				if (dsp) dsp->tick();
				if (ciap) ciap->tick();
			}
		}

		mcd212_cycles -= executed_cycles;
		while (mcd212_cycles <= 0)
		{
			mcd212_cycles += mcd212_rate;
			if (vpu != NULL)
			{
				vpu->skip_draw = no_draw;
				vpu->tick();
			}
		}

		cycles += executed_cycles;
	}
}

void PhilipsCDI::reset()
{
	MiniCDI::Log("[CDI] reset");
	MiniCDI::OS9::clear_modules();

	nvram_save();
	cpu.reset();
	vpu->reset();
	if (ftd != NULL) ftd->reset();

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
		if ((address & 0x00FFFF00) == 0x00310000)
		{
			if (slave != NULL)
				return slave->read8(address);
			if (ikat != NULL)
				return ikat->read8(address);
		}

		// CD+Audio microcontroller bus
		if ((address & 0x00FF0000) == 0x00300000 && dsp)
		{
			address &= 0x00FF3FFF;
			return dsp->read8(address);
		}

		// The Video Processing unit
		if ((address & 0x00FFFF00) == 0x004FFF00 && vpu)
		{
			return vpu->read8(address);
		}

		// RAM/ROM/everything else?
		if (address < memsize)
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
		if ((address & 0x00FFFF00) == 0x00310000)
		{
			if (slave)
				return slave->read8(address);
			if (ikat)
				return ikat->read8(address);
		}

		// CD+Audio microcontroller bus
		if ((address & 0x00FF0000) == 0x00300000)
		{
			address &= 0x00FF3FFF;
			if (cdic)
				return cdic->read16(address);
			if (ciap)
				return ciap->read16(address);
		}

		// The Video Processing unit
		if ((address & 0x00FFFF00) == 0x004FFF00 && vpu)
		{
			return vpu->read16(address);
		}

		return (read8(address) << 8) | read8(address+1);
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

		return (read16(address) << 16) | read16(address+2);
	}

	return 0;
}

void PhilipsCDI::write8(int address, uint8_t value)
{
	if (!check_for_unmapped(address))
		return;
	#ifdef MINICDI_DEADNVRAM
	// Dead Timekeeper/NVRAM will not allow writing
	if ((address & 0x00FFFF00) == 0x00320000)
		return;
	#endif
	else if ((address & 0xC0000000) == 0x80000000)
		cpu.write8(address, value);
	else if (slave != NULL && (address & 0x00FFFF00) == 0x00310000)
		slave->write8(address, value);
	else if (ikat != NULL && (address & 0x00FFFF00) == 0x00310000)
		ikat->write8(address, value);
	else if (dsp != NULL && address >= 0x00300000 && address < 0x00303FFF)
		dsp->write8(address, value);
	else if (address < memsize)
		memory[address] = value;
}

void PhilipsCDI::write16(int address, uint16_t value)
{
	if (!check_for_unmapped(address))
		return;
	if (vpu != NULL && (address & 0x00FFFF00) == 0x004FFF00)
		vpu->write16(address, value);
	else if (cdic != NULL && address >= 0x00300000 && address < 0x00303FFF)
		cdic->write16(address, value);
	else if (ciap != NULL && address >= 0x00300000 && address < 0x00303FFF)
		ciap->write16(address, value);
	else
	{
		write8(address, value >> 8 & 0xFF);
		write8(address+1, value & 0xFF);
	}
}

void PhilipsCDI::write32(int address, uint32_t value)
{
	if (!check_for_unmapped(address))
		return;
	if (cdic != NULL && address >= 0x00300000 && address < 0x00303FFF)
		return cdic->write32(address, value);
	else
	{
		write16(address, value >> 16 & 0xFFFF);
		write16(address+2, value & 0xFFFF);
	}
}

void PhilipsCDI::play_disc()
{
	if (slave != NULL) slave->send_play_button();
}

void PhilipsCDI::swap_disc(const std::string &path)
{
	// Clear CD+Audio controller attributes of current disc
	if (cdic != NULL)
		cdic->reset();

	disc.eject();

	// Read new disc
	if (!disc.open(path)) return;
	if (slave != NULL) slave->send_disc_status(true);
	if (ikat != NULL) ikat->send_disc_status(true);
}