#include "cdi/common.hpp"
#ifndef MINICDI_CHRONO_ENABLED
#define MINICDI_CHRONO_ENABLED (!defined(MINICDI_DISABLE_THROTTLING) && (defined(MINICDI_BENCHMARKING) \
																	  || defined(_WIN32) \
																	  || defined(__APPLE__) \
																	  || defined(HW_RVL) \
																	  || defined(__WIIU__)))
#endif

#define USE_WHILE_TRUE_LOOP

#if MINICDI_CHRONO_ENABLED == 1
#include <chrono>
#include <thread>
#endif
#include <numeric>

// Scheduler values
#define SECTOR 0
#define VPU 1
#define UART_TX 2
#define TIMER 3
#define PD 4

#define EVENTS_USED 3
#define EVENTS_TOTAL 5

static int event_rates[EVENTS_TOTAL] =
{
	/* SECTOR */ (MiniCDI::Config::PAL ? 15000000 : 15104900) / 75,
	/* VPU */ (MiniCDI::Config::PAL ? 15000000 : 15104900) / 15625,
	/* UART_TX */ 4915200,
	/* TIMER */ 96*2,
	/* PD */ (MiniCDI::Config::PAL ? 15000000 : 15104900) / 30
};

static int event_cycles[EVENTS_TOTAL] =
{
	event_rates[SECTOR],
	event_rates[VPU],
	event_rates[UART_TX],
	event_rates[TIMER],
	event_rates[PD]
};

void PhilipsCDI::run(bool no_draw)
{
	#if (PD >= EVENTS_USED)
		#pragma message "note: Pointing Device not included in scheduler (Philips.cpp)"
		pd.send_packet();
	#endif

	#if MINICDI_CHRONO_ENABLED == 1
	// Benchmark
	const auto t1 = std::chrono::steady_clock::now();
	#endif

	#ifdef MINICDI_DEBUG_OS9
	MiniCDI::OS9::scan_modules(memory);
	#endif

	#ifdef USE_WHILE_TRUE_LOOP
	while (1)
	#else
	for (int total_cycles = 0; total_cycles < event_rates[VPU] * (MiniCDI::Config::PAL ? 312 : 262);)
	#endif
	{
		// A cycle rate of 240 is large enough that it doesn't break CDi_BadApple, but small enough that it also doesn't break the 2nd player shell.
		// On embedded consoles this also affects the speed of the emulator.
		const int cycles = std::min({
		#if (TIMER >= EVENTS_USED)
			240,
		#endif
		#if (EVENTS_USED >= 5)
			event_rates[0],
			event_rates[1],
			event_rates[2],
			event_rates[3],
			event_rates[4]
		#elif (EVENTS_USED == 4)
			event_rates[0],
			event_rates[1],
			event_rates[2],
			event_rates[3]
		#elif (EVENTS_USED == 3)
			event_rates[0],
			event_rates[1],
			event_rates[2]
		#else
			event_rates[0],
			event_rates[1]
		#endif
		});

		#if (TIMER >= EVENTS_USED)
			#pragma message "note: Timer0 not included in scheduler (Philips.cpp)"
			cpu.run(cycles, true);
		#else
			cpu.run(cycles, false);
		#endif
		#ifndef USE_WHILE_TRUE_LOOP
			total_cycles += cycles;
		#endif

		for (int i = 0; i < EVENTS_USED; i++)
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

					#if (TIMER < EVENTS_USED)
					case TIMER:
						cpu.timer0_tick();
						break;
					#endif

					#if (UART_TX < EVENTS_USED)
					case UART_TX:
						cpu.uart_tx_tick();
						break;
					#endif

					#if (PD < EVENTS_USED)
					case PD:
						pd.send_packet();
						break;
					#endif
				}
			}
		}
	}

	frame_end:
	// Update microcontroller
	if (ikat != NULL) ikat->update();

	#if MINICDI_CHRONO_ENABLED == 1
	// Benchmark end
	const auto t2 = std::chrono::steady_clock::now();
	const std::chrono::duration<uint_fast32_t, std::nano> t_duration = t2 - t1;
	#endif

	#if MINICDI_CHRONO_ENABLED == 1 && defined(MINICDI_BENCHMARKING)
	MiniCDI::Log("[CDI] %s frame in %d ns", no_draw ? "Executed" : "Executed and drawn", t_duration.count());
	#endif

	#ifdef MINICDI_NO_THROTTLING
	return;
	#endif

	// Throttling
	#if MINICDI_CHRONO_ENABLED == 1 && (defined(_WIN32) || defined(__APPLE__) || defined(HW_RVL) || defined(__WIIU__))
	if (t_duration.count() < 16'666'667 && !MiniCDI::Config::NoFrameLimit) {
		const int wait_ms = 16'666'667 - t_duration.count();
		std::this_thread::sleep_for(std::chrono::nanoseconds(wait_ms));
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

	for (int i = 0; i < EVENTS_TOTAL; i++) { event_cycles[i] = event_rates[i]; }
}

void PhilipsCDI::reset_pd()
{
	pd = PointingDevice();
	if (slave != NULL) this->pd.IO.slave = slave;
	if (ikat != NULL) this->pd.IO.ikat = ikat;
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
		 : static_cast<uint16_t>(read8(address) << 8 | read8(address+1));
}

uint32_t PhilipsCDI::read32(int address)
{
	return !check_for_unmapped(address) ? 0
		 : cdic != NULL && address >= 0x00300000 && address < 0x00303FFF ? cdic->read32(address)
		 : static_cast<uint32_t>(read16(address) << 16 | read16(address+2));
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
	if (cdic != NULL) cdic->reset();

	disc.eject();
	if (!disc.open(path)) return;

	if (slave != NULL) slave->send_disc_status(true);
	if (ikat != NULL) ikat->send_disc_status(true);
}