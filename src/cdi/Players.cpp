#include "cdi/common.hpp"

static struct
{
	uint8_t* memory;
	SCC68070* scc68070;
	SLAVE* slave;
	MCD212* mcd212;
} m_currentPlayer;

#include "cdi/Musashi/m68kcpu.h"

unsigned int  m68k_read_disassembler_8(unsigned int address) { return m68k_read_memory_8(address); }
unsigned int  m68k_read_disassembler_16(unsigned int address) { return m68k_read_memory_16(address); }
unsigned int  m68k_read_disassembler_32(unsigned int address) { return m68k_read_memory_32(address); }

void scc68070_set_fc(unsigned int new_fc) {
	if (m_currentPlayer.scc68070)
		m_currentPlayer.scc68070->fc = new_fc;
}

unsigned int  m68k_read_memory_8(unsigned int address) {
	if (m_currentPlayer.scc68070 && (address & 0xFFFF0000) == 0x80000000
		&& m_currentPlayer.scc68070->fc == FUNCTION_CODE_SUPERVISOR_PROGRAM)
		return m_currentPlayer.scc68070->read8(address);

	if (m_currentPlayer.slave && (address & 0xFFFFFF00) == 0x00310000)
		return m_currentPlayer.slave->read8(address);

	if (m_currentPlayer.mcd212 && (address & 0xFFFFFF00) == 0x004FFF00)
		return m_currentPlayer.mcd212->read8(address);

	return m_currentPlayer.memory[address & 0x00ffffff];
}

unsigned int  m68k_read_memory_16(unsigned int address) {
	if (m_currentPlayer.mcd212 && (address & 0xFFFFFF00) == 0x004FFF00)
		return m_currentPlayer.mcd212->read16(address);

	return (uint16_t)((m68k_read_memory_8(address) << 8) | m68k_read_memory_8(address+1));
}

unsigned int  m68k_read_memory_32(unsigned int address) {
	return ((uint32_t)m68k_read_memory_16(address) << 16) | m68k_read_memory_16(address+2);
}

void m68k_write_memory_8(unsigned int address, unsigned int value) {
	if (m_currentPlayer.scc68070 && m_currentPlayer.scc68070->fc == FUNCTION_CODE_SUPERVISOR_PROGRAM) {
		m_currentPlayer.scc68070->write8(address, value);
		return;
	}

	if (m_currentPlayer.slave && (address & 0xFFFFFF00) == 0x00310000) {
		m_currentPlayer.slave->write8(address, value);
		return;
	}

	m_currentPlayer.memory[address & 0x00ffffff] = value;
}

void m68k_write_memory_16(unsigned int address, unsigned int value) {
	if (m_currentPlayer.mcd212 && (address & 0xFFFFFF00) == 0x004FFF00) {
		m_currentPlayer.mcd212->write16(address, value);
		return;
	}

	m68k_write_memory_8(address, (uint8_t)(value >> 8));
	m68k_write_memory_8(address + 1, (uint8_t)value);
}

void m68k_write_memory_32(unsigned int address, unsigned int value) {
	m68k_write_memory_16(address, (uint16_t)(value >> 16));
	m68k_write_memory_16(address + 2, (uint16_t)value);
}

bool MonoIPlayer::Init(const char* bios, MiniCDIConfig *config)
{
	if (CDIPlayer::Init(bios, config)) {
		this->cpu = SCC68070(this->memory, this->memSize, config);
		this->cpu.set_bios(bios, this->romAddr);
		this->slave = new SLAVE(this->memory, config);
		this->vpu = new MCD212(&this->cpu, this->memory, this->vdscAddr, config);

		m_currentPlayer =
		{
			.memory = this->memory,
			.scc68070 = &this->cpu,
			.slave = this->slave,
			.mcd212 = this->vpu,
		};

		this->cpu.reset();

		return true;
	}

	return false;
}