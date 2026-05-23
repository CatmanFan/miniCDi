#include "cdi/Musashi/m68kcpu.h"
#include "cdi/common.hpp"

namespace MiniCDI
{
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

void scc68070_set_fc(unsigned int new_fc) {
	if (MiniCDI::Player.scc68070) {
		MiniCDI::Player.scc68070->fc = /*new_fc*/FLAG_S | ((CPU_PREF_ADDR & 0xC0000000) >> 24);
	}
}

unsigned int  m68k_read_memory_8(unsigned int address) {
	if (MiniCDI::Player.scc68070 && (address & 0xC0000000) == 0x80000000 && FLAG_S)	return MiniCDI::Player.scc68070->read8(address);
	if (MiniCDI::Player.slave && (address & 0x00FFFF00) == 0x00310000)				return MiniCDI::Player.slave->read8(address);
	if (MiniCDI::Player.ikat && (address & 0x00FFFF00) == 0x00310000)				return MiniCDI::Player.ikat->read8(address);
	if (MiniCDI::Player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00)				return MiniCDI::Player.mcd212->read8(address);
	return MiniCDI::Player.memory[address & 0x00ffffff];
}

unsigned int  m68k_read_memory_16(unsigned int address) {
	if (MiniCDI::Player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00)	return MiniCDI::Player.mcd212->read16(address);
	if (MiniCDI::Player.cdic && (address & 0x00FF0000) == 0x00300000)	return MiniCDI::Player.cdic->read16(address);
	if (MiniCDI::Player.ciap && (address & 0x00FF0000) == 0x00300000)	return MiniCDI::Player.ciap->read16(address);
	return (uint16_t)((m68k_read_memory_8(address) << 8) | m68k_read_memory_8(address+1));
}

unsigned int  m68k_read_memory_32(unsigned int address) {
	return ((uint32_t)m68k_read_memory_16(address) << 16) | m68k_read_memory_16(address+2);
}

void m68k_write_memory_8(unsigned int address, unsigned int value) {
	if (MiniCDI::Player.scc68070 && (address & 0xC0000000) == 0x80000000 && FLAG_S) MiniCDI::Player.scc68070->write8(address, value);
	else if (MiniCDI::Player.slave && (address & 0x00FFFF00) == 0x00310000) MiniCDI::Player.slave->write8(address, value);
	else if (MiniCDI::Player.ikat && (address & 0x00FFFF00) == 0x00310000) MiniCDI::Player.ikat->write8(address, value);
	else MiniCDI::Player.memory[address & 0x00ffffff] = value;
}

void m68k_write_memory_16(unsigned int address, unsigned int value) {
	if (MiniCDI::Player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00) MiniCDI::Player.mcd212->write16(address, value);
	else if (MiniCDI::Player.cdic && (address & 0x00FF0000) == 0x00300000) MiniCDI::Player.cdic->write16(address, value, MiniCDI::Player.scc68070);
	else if (MiniCDI::Player.ciap && (address & 0x00FF0000) == 0x00300000) MiniCDI::Player.ciap->write16(address, value, MiniCDI::Player.scc68070);
	else {
		m68k_write_memory_8(address, (uint8_t)(value >> 8));
		m68k_write_memory_8(address + 1, (uint8_t)value);
	}
}

void m68k_write_memory_32(unsigned int address, unsigned int value) {
	m68k_write_memory_16(address, (uint16_t)(value >> 16));
	m68k_write_memory_16(address + 2, (uint16_t)value);
}

/** @brief Contains initialization functions for boards. **/

bool MonoI::Init(const char* bios)
{
	if (CDi::Init(bios)) {
		this->cpu = SCC68070(this->memory);
		this->vpu = new MCD212(&this->cpu, this->memory);
		this->slave = new SLAVE(&this->cpu, this->memory, 0x00310000);
		this->cdic = new CDIC(this->memory);

		MiniCDI::Player =
		{
			.memory = this->memory,
			.scc68070 = &this->cpu,
			.mcd212 = this->vpu,
			.slave = this->slave,
			.cdic = this->cdic
		};

		this->cpu.load_rom(bios, 0x400000);
		this->cpu.reset();
		for (int i = 0; i < 15; i++) { m68k_set_reg((m68k_register_t)i, 0xffffffff); }

		this->pd.IO.slave = this->slave;

		return true;
	}

	return false;
}

bool MonoIV::Init(const char* bios)
{
	if (CDi::Init(bios)) {
		this->cpu = SCC68070(this->memory);
		this->vpu = new MCD212(&this->cpu, this->memory);
		this->ikat = new IKAT(&this->cpu);
		this->ciap = new CIAP(this->memory);

		MiniCDI::Player =
		{
			.memory = this->memory,
			.scc68070 = &this->cpu,
			.mcd212 = this->vpu,
			.ikat = this->ikat,
			.ciap = this->ciap
		};

		this->cpu.load_rom(bios, 0x400000);
		this->cpu.reset();
		for (int i = 0; i < 15; i++) { m68k_set_reg((m68k_register_t)i, 0xffffffff); }

		this->pd.IO.ikat = this->ikat;

		return true;
	}

	return false;
}