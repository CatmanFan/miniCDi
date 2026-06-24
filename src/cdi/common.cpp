#include "cdi/Musashi/m68kcpu.h"
#include "cdi/common.hpp"

namespace MiniCDI
{
	namespace Config
	{
		bool TestPlug = false;
		bool PAL = true;
		bool ShowLCD = false;
		bool HasDisc = false;
		size_t FrameSkip = 0;

		FILE* LogFile = nullptr;
		std::string NvramFile = "";
	}

	static struct
	{
		uint8_t* memory;
		SCC68070* scc68070;
		MCD212* mcd212;
		SLAVE* slave;
		IKAT* ikat;
		CDIC* cdic;
		CIAP* ciap;
	} Player;
}

unsigned int  m68k_read_disassembler_8(unsigned int address) { return m68k_read_memory_8(address); }
unsigned int  m68k_read_disassembler_16(unsigned int address) { return m68k_read_memory_16(address); }
unsigned int  m68k_read_disassembler_32(unsigned int address) { return m68k_read_memory_32(address); }

unsigned int  m68k_read_memory_8(unsigned int address)
{
	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	if (MiniCDI::Player.scc68070 && (address & 0xC0000000) == 0x80000000)		return MiniCDI::Player.scc68070->read8(address);
	if (MiniCDI::Player.slave && (address & 0x00FFFF00) == 0x00310000)			return MiniCDI::Player.slave->read8(address);
	if (MiniCDI::Player.ikat && (address & 0x00FFFF00) == 0x00310000)			return MiniCDI::Player.ikat->read8(address);
	if (MiniCDI::Player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00)			return MiniCDI::Player.mcd212->read8(address);
	return MiniCDI::Player.memory[address & 0x00ffffff];
}

unsigned int  m68k_read_memory_16(unsigned int address)
{
	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }
	
	if (MiniCDI::Player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00)			return MiniCDI::Player.mcd212->read16(address);
	if (MiniCDI::Player.cdic && address >= 0x00300000 && address < 0x00303FFF)	return MiniCDI::Player.cdic->read16(address);
	if (MiniCDI::Player.ciap && address >= 0x00300000 && address < 0x00303FFF)	return MiniCDI::Player.ciap->read16(address);
	return (uint16_t)((m68k_read_memory_8(address) << 8) | m68k_read_memory_8(address+1));
}

unsigned int  m68k_read_memory_32(unsigned int address)
{
	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	if (MiniCDI::Player.cdic && address >= 0x00300000 && address < 0x00303FFF)	return MiniCDI::Player.cdic->read32(address);
	return ((uint32_t)m68k_read_memory_16(address) << 16) | m68k_read_memory_16(address+2);
}

void m68k_write_memory_8(unsigned int address, unsigned int value)
{
	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	#ifdef MINICDI_DEADNVRAM
	if ((address & 0x00FFFF00) == 0x00320000) return;
	#endif

	if (MiniCDI::Player.scc68070 && (address & 0xC0000000) == 0x80000000)		MiniCDI::Player.scc68070->write8(address, value);
	else if (MiniCDI::Player.slave && (address & 0x00FFFF00) == 0x00310000)		MiniCDI::Player.slave->write8(address, value);
	else if (MiniCDI::Player.ikat && (address & 0x00FFFF00) == 0x00310000)		MiniCDI::Player.ikat->write8(address, value, MiniCDI::Player.ciap);
	else MiniCDI::Player.memory[address & 0x00ffffff] = value;
}

void m68k_write_memory_16(unsigned int address, unsigned int value)
{
	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	if (MiniCDI::Player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00) MiniCDI::Player.mcd212->write16(address, value);
	else if (MiniCDI::Player.cdic && address >= 0x00300000 && address < 0x00303FFF) MiniCDI::Player.cdic->write16(address, value);
	else if (MiniCDI::Player.ciap && address >= 0x00300000 && address < 0x00303FFF) MiniCDI::Player.ciap->write16(address, value);
	else {
		m68k_write_memory_8(address, (uint8_t)(value >> 8 & 0x00FF));
		m68k_write_memory_8(address + 1, (uint8_t)(value & 0x00FF));
	}
}

void m68k_write_memory_32(unsigned int address, unsigned int value)
{
	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	if (MiniCDI::Player.cdic && address >= 0x00300000 && address < 0x00303FFF) MiniCDI::Player.cdic->write32(address, value);
	else {
		m68k_write_memory_16(address, (uint16_t)(value >> 16 & 0x0000FFFF));
		m68k_write_memory_16(address + 2, (uint16_t)(value & 0x0000FFFF));
	}
}

static int MiniCDI_op_trap_handler(int trap)
{
	#ifdef MINICDI_DEBUG_OS9
	if (trap == 0) MiniCDI::OS9::log(MiniCDI::Player.memory);
	#endif

	return 0; // unhandled, generate exception.
}

static void MiniCDI_reset_handler()
{
	if (MiniCDI::Player.scc68070) MiniCDI::Player.scc68070->reset_internal();
}

static void MiniCDI_set_fc(unsigned int new_fc)
{
	if (MiniCDI::Player.scc68070) {
		MiniCDI::Player.scc68070->fc = /*new_fc*/FLAG_S | (CPU_PREF_ADDR >> 24 & 0xC0);
	}
}

static int MiniCDI_int_ack_handler(int int_level)
{
	m68k_set_irq(0); // resets IRQ

	if (MiniCDI::Player.scc68070)
	{
		//MiniCDI::Log("[SCC68070:IPL] acknowledge lvl=%X", int_level);
		if (MiniCDI::Player.scc68070->InterruptManager.vectors[MiniCDI::Player.scc68070->InterruptManager.cur_index])
			return MiniCDI::Player.scc68070->InterruptManager.vectors[MiniCDI::Player.scc68070->InterruptManager.cur_index];
	}

	return M68K_INT_ACK_AUTOVECTOR;
}

/** @brief Contains initialization functions for boards. **/

void CDi::shutdown()
{
	this->nvram_save();

	MiniCDI::Log("[CDI] shutting down");

	if (MiniCDI::Config::LogFile)
		fclose(MiniCDI::Config::LogFile);

	// Avoid exception crash
	MiniCDI::Player = {NULL};
	m68k_pulse_halt();
	if (memory) delete[] memory;
}

bool MonoI::init(const std::string &bios)
{
	if (CDi::init(bios)) {
		// Load system ROM data
		std::ifstream romStream(bios);
		std::vector<char> rom((std::istreambuf_iterator<char>(romStream)),(std::istreambuf_iterator<char>()));
		romStream.close();

		// Setup peripherals
		this->cpu = SCC68070(this->memory);
		this->vpu = new MCD212(&this->cpu, this->memory);

		MiniCDI::Player = {NULL};
		MiniCDI::Player.memory = this->memory;
		MiniCDI::Player.scc68070 = &this->cpu;
		MiniCDI::Player.mcd212 = this->vpu;

		switch (this->board) {
			default:
			case CDi::MonoI:
				this->cdic = new CDIC(&this->cpu, this->memory, &this->disc);
				this->slave = new SLAVE(&this->cpu, this->memory, 0x00310000);
				this->pd.IO.slave = this->slave;
				this->nvram = 0x00320000;
				MiniCDI::Player.slave = this->slave;
				MiniCDI::Player.cdic = this->cdic;
				break;

			case CDi::MonoIV:
				this->ciap = new CIAP(&this->cpu, &this->disc, this->memory);
				this->ikat = new IKAT(&this->cpu, this->memory);
				this->pd.IO.ikat = this->ikat;
				this->nvram = 0x00320000;
				MiniCDI::Player.ikat = this->ikat;
				MiniCDI::Player.ciap = this->ciap;
				break;
		}

		m68k_init();
		m68k_set_cpu_type(M68K_CPU_TYPE_SCC68070);
		// m68k_pulse_reset();

		// Set callbacks
		m68k_set_int_ack_callback(MiniCDI_int_ack_handler);
		m68k_set_reset_instr_callback(MiniCDI_reset_handler);
		m68k_set_trap_instr_callback(MiniCDI_op_trap_handler);
		m68k_set_fc_callback(MiniCDI_set_fc);

		this->cpu.load_rom(rom);
		this->cpu.reset();
		this->nvram_load();

		return true;
	}

	return false;
}