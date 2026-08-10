#include "cdi/common.hpp"
#include <chrono>
#include <thread>
#include <numeric>

void PhilipsCDI::run(bool no_draw)
{
	#ifndef MINICDI_RAW_68K_MODE
	pd.send_packet();
	#endif

	// Benchmark
	const auto t1 = std::chrono::steady_clock::now();

	#ifdef MINICDI_DEBUG_OS9
	MiniCDI::OS9::scan_modules(memory);
	#endif

	while (1)
	{
		// A cycle rate of 240 is large enough that it doesn't break CDi_BadApple, but small enough that it also doesn't break the 2nd player shell.
		// On embedded consoles this also affects the speed of the emulator.
		const int cycles = std::min({240, event_rates[0], event_rates[1], event_rates[2]});
		cpu.run(cycles);

		for (int i = 0; i < EVTNUM; i++)
		{
			event_cycles[i] -= cycles;
			while (event_cycles[i] <= 0)
			{
				event_cycles[i] += event_rates[i];
				switch (i)
				{
					case SECTOR:
						if (!disc.is_open()) continue;
						if (cdic != NULL) cdic->tick();
						else if (dsp != NULL) dsp->tick();
						else if (ciap != NULL) ciap->tick();
						break;

					case VPU:
						if (vpu == NULL) continue;
						vpu->skip_draw = no_draw;
						if (vpu->tick()) goto frame_end;
						break;

					case UART_TX:
						cpu.uart_tx_tick();
						break;
				}
			}
		}
	}

	frame_end:
	// Update microcontroller
	if (ikat != NULL) ikat->update();

	// Benchmark end
	// integral duration: requires duration_cast
	const auto t2 = std::chrono::steady_clock::now();
	const std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
	#ifdef MINICDI_BENCHMARKING
	MiniCDI::Log("[CDI] %s frame in %.2f ms", no_draw ? "Executed" : "Executed and drawn", fp_ms.count());
	#endif

	#ifdef MINICDI_NO_THROTTLING
	return;
	#endif

	// Throttling
	#if defined(_WIN32) || defined(__APPLE__)
	if (fp_ms.count() < (1000.0f/60.0f) && !MiniCDI::Config::NoFrameLimit) {
		const int wait_ms = (int)((1000.0f/60.0f) - fp_ms.count());
		std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
	}
	#endif
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

	for (int i = 0; i < EVTNUM; i++) { event_cycles[i] = event_rates[i]; }
}

uint8_t PhilipsCDI::read8(int address)
{
	return !check_for_unmapped(address) ? 0
		 : (address & 0xC0000000) == 0x80000000 ? cpu.read8(address)
		 : (address & 0x00FFFF00) == 0x00310000 ? (ikat != NULL ? ikat->read8(address) : slave ? slave->read8(address) : 0)
		 : dsp != NULL && address >= 0x00300000 && address < 0x00303FFF ? dsp->read8(address)
		 : vpu != NULL && (address & 0x00FFFF00) == 0x004FFF00 ? vpu->read8(address)
		 : address < memsize ? memory[address]
		 : 0;
}

uint16_t PhilipsCDI::read16(int address)
{
	return !check_for_unmapped(address) ? 0
		 : vpu != NULL && (address & 0x00FFFF00) == 0x004FFF00 ? vpu->read16(address)
		 : address >= 0x00300000 && address < 0x00303FFF ? (cdic != NULL ? cdic->read16(address) : ciap != NULL ? ciap->read16(address) : 0)
		 : (uint16_t)(read8(address) << 8 | read8(address+1));
}

uint32_t PhilipsCDI::read32(int address)
{
	return !check_for_unmapped(address) ? 0
		 : cdic != NULL && address >= 0x00300000 && address < 0x00303FFF ? cdic->read32(address)
		 : (uint32_t)(read16(address) << 16 | read16(address+2));
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
		ikat->write8(address, value, ciap);
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
	if (cdic != NULL) {
		if (cdic->touched_disc) reset();
		else cdic->reset();
	}

	disc.eject();
	if (!disc.open(path)) return;

	if (slave != NULL) slave->send_disc_status(true);
	if (ikat != NULL) ikat->send_disc_status(true);
}