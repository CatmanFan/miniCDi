#include "cdi/m68k/m68kcpu.h"
#include "cdi/common.hpp"
#include <fstream>

// #define MINICDI_USE_SWITCH_FOR_BUS

namespace MiniCDI
{
	namespace Config
	{
		bool TestPlug = false;
		bool PAL = true;
		bool ShowFPS = false;
		bool ShowFTD = false;
		bool AnalogColors = false;
		size_t FrameSkip = 0;
		bool NoFrameLimit = false;
		int PointerAdvance = 1;

		FILE* LogFile = nullptr;
		std::string NvramFile = "";
	}

	static struct
	{
		uint8_t* memory;
		enum CDi::BoardType board;

		SCC68070* scc68070;
		MCD212* mcd212;
		SLAVE* slave;
		IKAT* ikat;
		CDIC* cdic;
		DRVDSP* dsp;
		CIAP* ciap;
	} Player;
}

unsigned int  m68k_read_disassembler_8(unsigned int address) { return m68k_read_memory_8(address); }
unsigned int  m68k_read_disassembler_16(unsigned int address) { return m68k_read_memory_16(address); }
unsigned int  m68k_read_disassembler_32(unsigned int address) { return m68k_read_memory_32(address); }

unsigned int  m68k_read_memory_8(unsigned int address)
{
	// Bus error on unmapped memory
	bool bus_error = (address >= 0x080000 && address <= 0x1fffff) || (address >= 0x500000 && address <= 0xcfffff);
	if (bus_error) { m68k_pulse_bus_error(); return 0; }

	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	// Redirect bus
	#ifdef MINICDI_USE_SWITCH_FOR_BUS
		switch (MiniCDI::Player.board)
		{
			case CDi::MonoI:
				if ((address & 0xC0000000) == 0x80000000) return MiniCDI::Player.scc68070->read8(address);
				if ((address & 0x00FFFF00) == 0x00310000) return MiniCDI::Player.slave->read8(address);
				if ((address & 0x00FFFF00) == 0x004FFF00) return MiniCDI::Player.mcd212->read8(address);
				break;

			case CDi::MonoII:
				if ((address & 0xC0000000) == 0x80000000) return MiniCDI::Player.scc68070->read8(address);
				if ((address & 0x00FFFF00) == 0x00310000) return MiniCDI::Player.slave->read8(address);
				if ((address & 0x00FFFF00) == 0x004FFF00) return MiniCDI::Player.mcd212->read8(address);
				if (address >= 0x00300000 && address < 0x00303FFF) return MiniCDI::Player.dsp->read8(address);
				break;

			case CDi::MonoIII:
			case CDi::MonoIV:
				if ((address & 0xC0000000) == 0x80000000) return MiniCDI::Player.scc68070->read8(address);
				if ((address & 0x00FFFF00) == 0x00310000) return MiniCDI::Player.ikat->read8(address);
				if ((address & 0x00FFFF00) == 0x004FFF00) return MiniCDI::Player.mcd212->read8(address);
				break;

			default:
				break;
		}
		return address < 8*1024*1024 ? MiniCDI::Player.memory[address & 0xFFFFFF] : 0;
	#else
		return MiniCDI::Player.scc68070 && (address & 0xC0000000) == 0x80000000 ? MiniCDI::Player.scc68070->read8(address)
			 : MiniCDI::Player.slave && (address & 0x00FFFF00) == 0x00310000 ? MiniCDI::Player.slave->read8(address)
			 : MiniCDI::Player.ikat && (address & 0x00FFFF00) == 0x00310000 ? MiniCDI::Player.ikat->read8(address)
			 : MiniCDI::Player.dsp && address >= 0x00300000 && address < 0x00303FFF ? MiniCDI::Player.dsp->read8(address)
			 : MiniCDI::Player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00 ? MiniCDI::Player.mcd212->read8(address)
			 : address < 8*1024*1024 ? MiniCDI::Player.memory[address & 0xFFFFFF] : 0;
	#endif
}

unsigned int  m68k_read_memory_16(unsigned int address)
{
	// Bus error on unmapped memory
	bool bus_error = (address >= 0x080000 && address <= 0x1fffff) || (address >= 0x500000 && address <= 0xcfffff);
	if (bus_error) { m68k_pulse_bus_error(); return 0; }

	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	// Redirect bus
	#ifdef MINICDI_USE_SWITCH_FOR_BUS
		switch (MiniCDI::Player.board)
		{
			case CDi::MonoI:
				if ((address & 0x00FFFF00) == 0x004FFF00) return MiniCDI::Player.mcd212->read16(address);
				if (address >= 0x00300000 && address < 0x00303FFF) return MiniCDI::Player.cdic->read16(address);
				break;

			case CDi::MonoII:
				if ((address & 0x00FFFF00) == 0x004FFF00) return MiniCDI::Player.mcd212->read16(address);
				break;

			case CDi::MonoIII:
			case CDi::MonoIV:
				if ((address & 0x00FFFF00) == 0x004FFF00) return MiniCDI::Player.mcd212->read16(address);
				if (address >= 0x00300000 && address < 0x00303FFF) return MiniCDI::Player.ciap->read16(address);
				break;

			default:
				break;
		}
		return (m68k_read_memory_8(address) << 8) | m68k_read_memory_8(address+1);
	#else
		return MiniCDI::Player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00 ? MiniCDI::Player.mcd212->read16(address)
			 : MiniCDI::Player.cdic && address >= 0x00300000 && address < 0x00303FFF ? MiniCDI::Player.cdic->read16(address)
			 : MiniCDI::Player.ciap && address >= 0x00300000 && address < 0x00303FFF ? MiniCDI::Player.ciap->read16(address)
			 : (uint16_t)((m68k_read_memory_8(address) << 8) | m68k_read_memory_8(address+1));
	#endif
}

unsigned int  m68k_read_memory_32(unsigned int address)
{
	// Bus error on unmapped memory
	bool bus_error = (address >= 0x080000 && address <= 0x1fffff) || (address >= 0x500000 && address <= 0xcfffff);
	if (bus_error) { m68k_pulse_bus_error(); return 0; }

	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	// Redirect bus
	return MiniCDI::Player.board == CDi::MonoI && address >= 0x00300000 && address < 0x00303FFF ? MiniCDI::Player.cdic->read32(address)
		 : (uint32_t)((m68k_read_memory_16(address) << 16) | m68k_read_memory_16(address+2));
}

void m68k_write_memory_8(unsigned int address, unsigned int value)
{
	// Bus error on unmapped memory
	bool bus_error = (address >= 0x080000 && address <= 0x1fffff) || (address >= 0x500000 && address <= 0xcfffff);
	if (bus_error) { m68k_pulse_bus_error(); return; }

	// ROM is not supposed to be writable
	if ((address & 0x00FFFFFF) >= 0x400000 && (address & 0x00FFFFFF) < 0x480000) return;

	#ifdef MINICDI_DEADNVRAM
	// Dead Timekeeper/NVRAM will not allow writing
	if ((address & 0x00FFFF00) == 0x00320000) return;
	#endif

	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	// Redirect bus
	#ifdef MINICDI_USE_SWITCH_FOR_BUS
		switch (MiniCDI::Player.board)
		{
			case CDi::MonoI:
				if ((address & 0xC0000000) == 0x80000000) MiniCDI::Player.scc68070->write8(address, value);
				else if ((address & 0x00FFFF00) == 0x00310000) MiniCDI::Player.slave->write8(address, value);
				else if (address < 8*1024*1024) MiniCDI::Player.memory[address] = value;
				break;

			case CDi::MonoII:
				if ((address & 0xC0000000) == 0x80000000) MiniCDI::Player.scc68070->write8(address, value);
				else if ((address & 0x00FFFF00) == 0x00310000) MiniCDI::Player.slave->write8(address, value);
				else if (address >= 0x00300000 && address < 0x00303FFF) MiniCDI::Player.dsp->write8(address, value);
				else if (address < 8*1024*1024) MiniCDI::Player.memory[address] = value;
				break;

			case CDi::MonoIII:
			case CDi::MonoIV:
				if ((address & 0xC0000000) == 0x80000000) MiniCDI::Player.scc68070->write8(address, value);
				else if ((address & 0x00FFFF00) == 0x00310000) MiniCDI::Player.ikat->write8(address, value, MiniCDI::Player.ciap);
				else if (address < 8*1024*1024) MiniCDI::Player.memory[address] = value;
				break;

			default:
				if (address < 8*1024*1024) MiniCDI::Player.memory[address] = value;
				break;
		}
	#else
		if (MiniCDI::Player.scc68070 && (address & 0xC0000000) == 0x80000000)			MiniCDI::Player.scc68070->write8(address, value);
		else if (MiniCDI::Player.slave && (address & 0x00FFFF00) == 0x00310000)			MiniCDI::Player.slave->write8(address, value);
		else if (MiniCDI::Player.ikat && (address & 0x00FFFF00) == 0x00310000)			MiniCDI::Player.ikat->write8(address, value, MiniCDI::Player.ciap);
		else if (MiniCDI::Player.dsp && address >= 0x00300000 && address < 0x00303FFF)	MiniCDI::Player.dsp->write8(address, value);
		else if (address < 8*1024*1024) MiniCDI::Player.memory[address] = value;
	#endif
}

void m68k_write_memory_16(unsigned int address, unsigned int value)
{
	// Bus error on unmapped memory
	bool bus_error = (address >= 0x080000 && address <= 0x1fffff) || (address >= 0x500000 && address <= 0xcfffff);
	if (bus_error) { m68k_pulse_bus_error(); return; }

	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	// Redirect bus
	if (MiniCDI::Player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00) MiniCDI::Player.mcd212->write16(address, value);
	else if (MiniCDI::Player.cdic && address >= 0x00300000 && address < 0x00303FFF) MiniCDI::Player.cdic->write16(address, value);
	else if (MiniCDI::Player.ciap && address >= 0x00300000 && address < 0x00303FFF) MiniCDI::Player.ciap->write16(address, value);
	else {
		m68k_write_memory_8(address, value >> 8 & 0xFF);
		m68k_write_memory_8(address+1, value & 0xFF);
	}
}

void m68k_write_memory_32(unsigned int address, unsigned int value)
{
	// Bus error on unmapped memory
	bool bus_error = (address >= 0x080000 && address <= 0x1fffff) || (address >= 0x500000 && address <= 0xcfffff);
	if (bus_error) { m68k_pulse_bus_error(); return; }

	// Supervisor mode mask
	if (!(FLAG_S && (address >> 30) == 0x2)) { address &= 0xFFFFFF; }

	// Redirect bus
	if (MiniCDI::Player.cdic && address >= 0x00300000 && address < 0x00303FFF)
		MiniCDI::Player.cdic->write32(address, value);
	else {
		m68k_write_memory_16(address, value >> 16 & 0xFFFF);
		m68k_write_memory_16(address+2, value & 0xFFFF);
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
	if (MiniCDI::Player.scc68070) MiniCDI::Player.scc68070->fc = /*new_fc*/FLAG_S | (CPU_PREF_ADDR >> 24 & 0xC0);
}

static int MiniCDI_int_ack_handler(int int_level)
{
	m68k_set_irq(0); // resets IRQ

	if (MiniCDI::Player.scc68070)
		return MiniCDI::Player.scc68070->interrupt_ack(int_level);

	return M68K_INT_ACK_AUTOVECTOR;
}

/** @brief Contains initialization functions for boards. **/

MonoI::~MonoI()
{
	this->nvram_save();
	MiniCDI::OS9::clear_modules();

	// Musashi end
	m68k_set_irq(0);
	m68k_end_timeslice();
	m68k_set_int_ack_callback(NULL);
	m68k_set_reset_instr_callback(NULL);
	m68k_set_trap_instr_callback(NULL);
	m68k_set_fc_callback(NULL);

	// Free peripherals and player structure
	if (this->ftd != NULL) {
		delete this->ftd;
		this->ftd = NULL;
	}
	switch (this->board) {
		default:
		case CDi::MonoI:
			if (this->cdic != NULL) {
				delete this->cdic;
				this->cdic = NULL;
			}
			if (this->slave != NULL) {
				delete this->slave;
				this->slave = NULL;
			}
			break;

		case CDi::MonoII:
			if (this->dsp != NULL) {
				delete this->dsp;
				this->dsp = NULL;
			}
			if (this->slave != NULL) {
				delete this->slave;
				this->slave = NULL;
			}
			break;

		case CDi::MonoIII:
		case CDi::MonoIV:
			if (this->ciap != NULL) {
				delete this->ciap;
				this->ciap = NULL;
			}
			if (this->ikat != NULL) {
				delete this->ikat;
				this->ikat = NULL;
			}
			break;
	}
	if (this->vpu != NULL) {
		delete this->vpu;
		this->vpu = NULL;
	}
	if (this->memory != NULL) {
		#ifdef _WIN32
		_aligned_free(this->memory);
		#else
		free(this->memory);
		#endif
		this->memory = NULL;
	}
	MiniCDI::Player = {NULL};
	this->board = CDi::Invalid;

	// Stop logging
	MiniCDI::Log("[CDI] shutdown");
	if (MiniCDI::Config::LogFile != NULL) {
		fclose(MiniCDI::Config::LogFile);
		MiniCDI::Config::LogFile = NULL;
	}
}

bool MonoI::init(const std::string &bios, enum BoardType board)
{
	if (CDi::init(bios, board)) {
		if (this->board != 0) return true;
		this->board = board;

		// Prepare CPU and video chip
		this->cpu = SCC68070(this->memory);
		this->vpu = new MCD212(&this->cpu, this->memory);

		// Load system ROM data and memory map
		std::ifstream romStream(bios, std::ios::binary);
		std::vector<char> rom((std::istreambuf_iterator<char>(romStream)),(std::istreambuf_iterator<char>()));
		romStream.close();
		this->cpu.load_rom(rom);
		this->nvram_load();

		// Setup remaining peripherals and player structure
		MiniCDI::Player =
		{
			.memory = this->memory,
			.board = board,

			.scc68070 = &this->cpu,
			.mcd212 = this->vpu
		};

		switch (this->board) {
			default:
			case CDi::MonoI:
				this->ftd = new FTD(FTD::FTD_220_20);
				this->cdic = new CDIC(&this->cpu, this->memory, &this->disc);
				this->slave = new SLAVE(&this->cpu, this->memory, 0x00310000);
				this->slave->set_ftd(this->ftd);
				this->pd.IO.slave = this->slave;
				this->nvram = 0x00320000;
				MiniCDI::Player.slave = this->slave;
				MiniCDI::Player.cdic = this->cdic;
				break;

			case CDi::MonoII:
				this->ftd = new FTD(FTD::FTD_220_40);
				this->dsp = new DRVDSP(&this->cpu, this->memory, &this->disc);
				this->slave = new SLAVE(&this->cpu, this->memory, 0x00310000);
				this->slave->set_ftd(this->ftd);
				this->pd.IO.slave = this->slave;
				this->nvram = 0x00320000;
				MiniCDI::Player.slave = this->slave;
				MiniCDI::Player.dsp = this->dsp;
				break;

			case CDi::MonoIII:
			case CDi::MonoIV:
				this->ftd = new FTD(FTD::FTD_470);
				this->ciap = new CIAP(&this->cpu, this->memory, &this->disc);
				this->ikat = new IKAT(&this->cpu, this->memory);
				this->ikat->set_ftd(this->ftd);
				this->pd.IO.ikat = this->ikat;
				this->nvram = 0x00320000;
				MiniCDI::Player.ikat = this->ikat;
				MiniCDI::Player.ciap = this->ciap;
				break;
		}

		// Init Musashi last (expects memory to already be setup in player struct)
		m68k_init();
		m68k_set_cpu_type(M68K_CPU_TYPE_SCC68070);
		this->cpu.reset();
		m68k_set_int_ack_callback(MiniCDI_int_ack_handler);
		m68k_set_reset_instr_callback(MiniCDI_reset_handler);
		m68k_set_trap_instr_callback(MiniCDI_op_trap_handler);
		m68k_set_fc_callback(MiniCDI_set_fc);

		MiniCDI::Log("[CDI] Created %s machine", this->board == CDi::MonoIV ? "Mono-IV"
											   : this->board == CDi::MonoIII ? "Mono-III"
											   : this->board == CDi::MonoII ? "Mono-II"
											   : "Mono-I");
		return true;
	}

	return false;
}

bool CDi::nvram_save()
{
	if (MiniCDI::Config::NvramFile.empty() || memory == NULL || this->nvram == 0) return false;

	uint32_t nvram_size = board == CDi::MonoIV ? 32*1024 : 8*1024;
	MiniCDI::OS9::Module *module = MiniCDI::OS9::get_module_from_name("nvr");
	if (module != NULL && module->size < nvram_size) {
		nvram_size = (module->size > 12*1024 ? 16 : 8) * 1024;
		MiniCDI::Log("[NVRAM] detected %dKB", nvram_size);
	} else {
		MiniCDI::Log("[NVRAM] \"nvr\" system module not found, defaulting to max %dKB", nvram_size/1024);
	}

	FILE *file = fopen(MiniCDI::Config::NvramFile.c_str(), "wb");
	if (!file) {
		MiniCDI::Log("[NVRAM] failed to write to %s", MiniCDI::Config::NvramFile.c_str());
		return false;
	}
	fwrite(&memory[this->nvram], sizeof(memory[0]), nvram_size, file);
	fclose(file);

	MiniCDI::Log("[NVRAM] saved to %s", MiniCDI::Config::NvramFile.c_str());
	return true;
}

void CDi::nvram_load()
{
	/*if (this->memory != NULL)
	{
		// Write a default value to where the M48T08 (8KB NVRAM) registers should be stored.
		memory[this->nvram + 0x1FFF] = 0x01; // (BCD) yy: 2001
		memory[this->nvram + 0x1FFE] = 0x01; // (BCD) mm: 1
		memory[this->nvram + 0x1FFD] = 0x01; // (BCD) dd: 1
		memory[this->nvram + 0x1FFC] = 0x01; // (BCD) dd: Monday + normal clock operation
		memory[this->nvram + 0x1FFB] = 0x12; // (BCD) HH: 12
		memory[this->nvram + 0x1FFA] = 0x00; // (BCD) MM: 00
		memory[this->nvram + 0x1FF9] = 0x00; // (BCD) SS: 00
		memory[this->nvram + 0x1FF8] = 0; // control

		// Ditto for DS1216 (32KB NVRAM)
		memory[this->nvram + 0x0000] = 0x00; // (BCD) centiseconds
		memory[this->nvram + 0x0001] = 0x00; // (BCD) SS: 00
		memory[this->nvram + 0x0002] = 0x00; // (BCD) MM: 00
		memory[this->nvram + 0x0005] = 0x01; // (BCD) dd: 1
		memory[this->nvram + 0x0006] = 0x01; // (BCD) mm: 1
		memory[this->nvram + 0x0007] = 0x01; // (BCD) yy: 2001
	}*/
	if (!MiniCDI::Config::NvramFile.empty() && access(MiniCDI::Config::NvramFile.c_str(), F_OK) == 0 && this->nvram > 0) {
		MiniCDI::Log("[NVRAM] loading %s to memory", MiniCDI::Config::NvramFile.c_str());
		std::ifstream nvrStream(MiniCDI::Config::NvramFile, std::ios::binary);
		std::vector<char> nvr((std::istreambuf_iterator<char>(nvrStream)),(std::istreambuf_iterator<char>()));
		nvrStream.close();

		memcpy(&memory[this->nvram], &nvr[0], nvr.size());
	}
}