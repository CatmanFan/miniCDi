#include "cdi/common.hpp"

static struct
{
	uint8_t* memory;
	SCC68070* scc68070;
	SLAVE* slave;
	MCD212* mcd212;
} m_currentPlayer;

#ifdef MINICDI_MUSASHI
	#define MINICDI_MEMADDRESS address
	#include "cdi/Musashi/m68kcpu.h"

	unsigned int  m68k_read_disassembler_8(unsigned int address) { return m68k_read_memory_8(address); }
	unsigned int  m68k_read_disassembler_16(unsigned int address) { return m68k_read_memory_16(address); }
	unsigned int  m68k_read_disassembler_32(unsigned int address) { return m68k_read_memory_32(address); }

	unsigned int  m68k_read_memory_8(unsigned int address) {
#else
	#define MINICDI_MEMADDRESS addr

	static uint8_t MonoI_read8(M68kCpu* cpu, uint32_t addr) {
#endif
		if (m_currentPlayer.scc68070 && (MINICDI_MEMADDRESS & 0xF0000000) == 0x80000000)
			return m_currentPlayer.scc68070->read8(MINICDI_MEMADDRESS);

		if (m_currentPlayer.slave && (MINICDI_MEMADDRESS & 0xFFFF00) == 0x310000)
			return m_currentPlayer.slave->read8(MINICDI_MEMADDRESS);

		if (m_currentPlayer.mcd212 && (MINICDI_MEMADDRESS & 0xFFFF00) == 0x4FFF00)
			return m_currentPlayer.mcd212->read8(MINICDI_MEMADDRESS);

		return m_currentPlayer.memory[MINICDI_MEMADDRESS & 0x00ffffff];
	}

#ifdef MINICDI_MUSASHI
	unsigned int  m68k_read_memory_16(unsigned int address) {
#else
	static uint16_t MonoI_read16(M68kCpu* cpu, uint32_t addr) {
#endif
		if (m_currentPlayer.mcd212 && (MINICDI_MEMADDRESS & 0xFFFF00) == 0x4FFF00)
			return m_currentPlayer.mcd212->read16(MINICDI_MEMADDRESS);

#ifdef MINICDI_MUSASHI
		return (uint16_t)((m68k_read_memory_8(MINICDI_MEMADDRESS) << 8) | m68k_read_memory_8(MINICDI_MEMADDRESS+1));
#else
		return (uint16_t)((MonoI_read8(cpu, MINICDI_MEMADDRESS) << 8) | MonoI_read8(cpu, MINICDI_MEMADDRESS+1));
#endif
	}

#ifdef MINICDI_MUSASHI
	unsigned int  m68k_read_memory_32(unsigned int address) {
		return ((uint32_t)m68k_read_memory_16(MINICDI_MEMADDRESS) << 16) | m68k_read_memory_16(MINICDI_MEMADDRESS+2);
#else
	static uint32_t MonoI_read32(M68kCpu* cpu, uint32_t addr) {
		return ((uint32_t)MonoI_read16(cpu, MINICDI_MEMADDRESS) << 16) | MonoI_read16(cpu, MINICDI_MEMADDRESS+2);
#endif
	}

#ifdef MINICDI_MUSASHI
	void m68k_write_memory_8(unsigned int address, unsigned int value) {
#else
	static void MonoI_write8(M68kCpu* cpu, uint32_t addr, uint8_t value) {
#endif
		if (m_currentPlayer.scc68070 && (MINICDI_MEMADDRESS & 0xF0000000) == 0x80000000) {
			m_currentPlayer.scc68070->write8(MINICDI_MEMADDRESS, value);
			return;
		}

		if (m_currentPlayer.slave && (MINICDI_MEMADDRESS & 0xFFFF00) == 0x310000) {
			m_currentPlayer.slave->write8(MINICDI_MEMADDRESS, value);
			return;
		}

		m_currentPlayer.memory[MINICDI_MEMADDRESS & 0x00ffffff] = value;
	}

#ifdef MINICDI_MUSASHI
	void m68k_write_memory_16(unsigned int address, unsigned int value) {
#else
	static void MonoI_write16(M68kCpu* cpu, uint32_t addr, uint16_t value) {
#endif
		if (m_currentPlayer.mcd212 && (MINICDI_MEMADDRESS & 0xFFFF00) == 0x4FFF00) {
			m_currentPlayer.mcd212->write16(MINICDI_MEMADDRESS, value);
			return;
		}

#ifdef MINICDI_MUSASHI
		m68k_write_memory_8(MINICDI_MEMADDRESS, (uint8_t)(value >> 8));
		m68k_write_memory_8(MINICDI_MEMADDRESS + 1, (uint8_t)value);
#else
		MonoI_write8(cpu, addr, (uint8_t)(value >> 8));
		MonoI_write8(cpu, addr + 1, (uint8_t)value);
#endif
	}

#ifdef MINICDI_MUSASHI
	void m68k_write_memory_32(unsigned int address, unsigned int value) {
		m68k_write_memory_16(MINICDI_MEMADDRESS, (uint16_t)(value >> 16));
		m68k_write_memory_16(MINICDI_MEMADDRESS + 2, (uint16_t)value);
	}
#else
	static void MonoI_write32(M68kCpu* cpu, uint32_t addr, uint32_t value) {
		MonoI_write16(cpu, addr, (uint16_t)(value >> 16));
		MonoI_write16(cpu, addr + 2, (uint16_t)value);
	}
#endif

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

		#ifndef MINICDI_MUSASHI
		m68k_set_read8_callback(&this->cpu.context, MonoI_read8);
		m68k_set_read16_callback(&this->cpu.context, MonoI_read16);
		m68k_set_read32_callback(&this->cpu.context, MonoI_read32);
		m68k_set_write8_callback(&this->cpu.context, MonoI_write8);
		m68k_set_write16_callback(&this->cpu.context, MonoI_write16);
		m68k_set_write32_callback(&this->cpu.context, MonoI_write32);
		#endif

		return true;
	}

	return false;
}