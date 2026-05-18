#include "cdi/common.hpp"

static struct
{
	uint8_t* memory;
	SCC68070* scc68070;
	SLAVE* slave;
	MCD212* mcd212;
} m_player;

#include "cdi/Musashi/m68kcpu.h"

unsigned int  m68k_read_disassembler_8(unsigned int address) { return m68k_read_memory_8(address); }
unsigned int  m68k_read_disassembler_16(unsigned int address) { return m68k_read_memory_16(address); }
unsigned int  m68k_read_disassembler_32(unsigned int address) { return m68k_read_memory_32(address); }

unsigned int  m68k_read_immediate_16(unsigned int address) { return m68k_read_memory_16(address); }
unsigned int  m68k_read_immediate_32(unsigned int address) { return m68k_read_memory_32(address); }
unsigned int  m68k_read_pcrelative_8(unsigned int address) { return m68k_read_memory_8(address); }
unsigned int  m68k_read_pcrelative_16(unsigned int address) { return m68k_read_memory_16(address); }
unsigned int  m68k_read_pcrelative_32(unsigned int address) { return m68k_read_memory_32(address); }

void scc68070_set_fc(unsigned int new_fc) {
	if (m_player.scc68070) {
		m_player.scc68070->fc = /*new_fc*/FLAG_S | ((CPU_PREF_ADDR & 0xC0000000) >> 24);
	}
}

unsigned int  m68k_read_memory_8(unsigned int address) {
	if (m_player.scc68070 && (address & 0xC0000000) == 0x80000000 && FLAG_S) {
		return m_player.scc68070->read8(address);
	} if (m_player.slave && (address & 0x00FFFF00) == 0x00310000) {
		return m_player.slave->read8(address);
	} else if (m_player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00) {
		return m_player.mcd212->read8(address);
	} else {
		return m_player.memory[address & 0x00ffffff];
	}
}

unsigned int  m68k_read_memory_16(unsigned int address) {
	if (m_player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00) {
		return m_player.mcd212->read16(address);
	} else {
		return (uint16_t)((m68k_read_memory_8(address) << 8) | m68k_read_memory_8(address+1));
	}
}

unsigned int  m68k_read_memory_32(unsigned int address) {
	return ((uint32_t)m68k_read_memory_16(address) << 16) | m68k_read_memory_16(address+2);
}

void m68k_write_memory_8(unsigned int address, unsigned int value) {
	if (m_player.scc68070 && (address & 0xC0000000) == 0x80000000 && FLAG_S) {
		m_player.scc68070->write8(address, value);
	} else if (m_player.slave && (address & 0x00FFFF00) == 0x00310000) {
		m_player.slave->write8(address, value, m_player.scc68070);
	} else {
		m_player.memory[address & 0x00ffffff] = value;
	}
}

void m68k_write_memory_16(unsigned int address, unsigned int value) {
	if (m_player.mcd212 && (address & 0x00FFFF00) == 0x004FFF00) {
		m_player.mcd212->write16(address, value);
	} else {
		m68k_write_memory_8(address, (uint8_t)(value >> 8));
		m68k_write_memory_8(address + 1, (uint8_t)value);
	}
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

		m_player =
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