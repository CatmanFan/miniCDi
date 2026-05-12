#include "cdi/common.hpp"

static struct
{
	uint8_t* memory;
	SCC68070* scc68070;
	MCD212* mcd212;
} m_currentPlayer;

#ifdef MINICDI_MUSASHI
	#include "cdi/m68k/m68kcpu.h"

	unsigned int  m68k_read_memory_8(unsigned int address) {
		if (m_currentPlayer.scc68070 && (address & 0xF0000000) == 0x80000000) {
			return m_currentPlayer.scc68070->read8(address);
		}

		else if (m_currentPlayer.mcd212 && (address & 0xFFFF00) == 0x4FFF00)
			return m_currentPlayer.mcd212->read8(address);

		else
			return m_currentPlayer.memory[address & 0x00ffffff];
	}

	unsigned int  m68k_read_memory_16(unsigned int address) {
		if (m_currentPlayer.mcd212 && (address & 0xFFFF00) == 0x4FFF00)
			return m_currentPlayer.mcd212->read16(address);

		else
			return (uint16_t)((m68k_read_memory_8(address) << 8) | m68k_read_memory_8(address+1));
	}

	unsigned int  m68k_read_memory_32(unsigned int address) {
		return ((uint32_t)m68k_read_memory_16(address) << 16) | m68k_read_memory_16(address+2);
	}

	void m68k_write_memory_8(unsigned int address, unsigned int value) {
		if (m_currentPlayer.scc68070 && (address & 0xF0000000) == 0x80000000)
			m_currentPlayer.scc68070->write8(address, value);

		else
			m_currentPlayer.memory[address & 0x00ffffff] = value;
	}

	void m68k_write_memory_16(unsigned int address, unsigned int value) {
		if (m_currentPlayer.mcd212 && (address & 0xFFFF00) == 0x4FFF00)
			m_currentPlayer.mcd212->write16(address, value);

		else {
			m68k_write_memory_8(address, (uint8_t)(value >> 8));
			m68k_write_memory_8(address + 1, (uint8_t)value);
		}
	}

	void m68k_write_memory_32(unsigned int address, unsigned int value) {
		m68k_write_memory_16(address, (uint16_t)(value >> 16));
		m68k_write_memory_16(address + 2, (uint16_t)value);
	}

#else

	static uint8_t MonoI_read8(M68kCpu* cpu, uint32_t addr) {
		if (m_currentPlayer.scc68070 && (addr & 0xF0000000) == 0x80000000)
			return m_currentPlayer.scc68070->read8(addr);

		else if (m_currentPlayer.mcd212 && (addr & 0xFFFF00) == 0x4FFF00)
			return m_currentPlayer.mcd212->read8(addr);

		else
			return m_currentPlayer.memory[addr & 0x00ffffff];
	}

	static uint16_t MonoI_read16(M68kCpu* cpu, uint32_t addr) {
		if (m_currentPlayer.mcd212 && (addr & 0xFFFF00) == 0x4FFF00)
			return m_currentPlayer.mcd212->read16(addr);

		else
			return (uint16_t)((MonoI_read8(cpu, addr) << 8) | MonoI_read8(cpu, addr + 1));
	}

	static uint32_t MonoI_read32(M68kCpu* cpu, uint32_t addr) {
		return ((uint32_t)MonoI_read16(cpu, addr) << 16) | MonoI_read16(cpu, addr + 2);
	}

	static void MonoI_write8(M68kCpu* cpu, uint32_t addr, uint8_t value) {
		if (m_currentPlayer.scc68070 && (addr & 0xF0000000) == 0x80000000)
			m_currentPlayer.scc68070->write8(addr, value);

		else
			m_currentPlayer.memory[addr & 0x00ffffff] = value;
	}

	static void MonoI_write16(M68kCpu* cpu, uint32_t addr, uint16_t value) {
		if (m_currentPlayer.mcd212 && (addr & 0xFFFF00) == 0x4FFF00)
			m_currentPlayer.mcd212->write16(addr, value);

		else {
			MonoI_write8(cpu, addr, (uint8_t)(value >> 8));
			MonoI_write8(cpu, addr + 1, (uint8_t)value);
		}
	}

	static void MonoI_write32(M68kCpu* cpu, uint32_t addr, uint32_t value) {
		MonoI_write16(cpu, addr, (uint16_t)(value >> 16));
		MonoI_write16(cpu, addr + 2, (uint16_t)value);
	}

	static int MonoI_int_ack(M68kCpu* cpu, int level) {
		return M68K_INT_ACK_AUTOVECTOR;
	}

#endif

bool MonoIPlayer::Init(const char* bios, MiniCDIConfig *config)
{
	if (CDIPlayer::Init(bios, config)) {
		// Load system ROM
		this->cpu.init(this->memory, this->memSize, bios, this->romAddr);

		// Init OS-9
		this->os9.init(&this->cpu, &this->memory[romAddr], romSize);

		// Init slave processor (MC68HC)
		this->slave = new IKAT(this->memory, &this->config);

		// Init video processor (MCD212)
		this->vpu = new MCD212(&this->cpu, this->memory, this->vdscAddr, &this->config);

		m_currentPlayer =
		{
			.memory = this->memory,
			.scc68070 = &this->cpu,
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
		m68k_set_int_ack_callback(&this->cpu.context, MonoI_int_ack);
#endif

		return true;
	}

	return false;
}