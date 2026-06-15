#include "cdi/common.hpp"

namespace MiniCDI
{
	namespace OS9
	{
		Module* get_module(uint32_t addr)
		{
			for (size_t i = 0; i < modules.size(); i++) {
				if (addr >= modules[i].address && addr < modules[i].address + modules[i].size)
					return &modules[i];
			}

			return NULL;
		}

		/// Algorithm taken from Slamy's CDi_MiSTer OS9 function, itself based on that of cdiemu:
		/// https://github.com/Slamy/CDi_MiSTer/blob/11ee3eeceeec0f26c50afd392743eb79bc9152db/sim2/sim_top.cpp#L565
		void scan_modules(uint8_t* memory)
		{
			modules.clear();

			uint32_t offsets[][2] = 
			{
				{0x000000, 0x080000},
				{0x200000, 0x280000},
				// {0xe40000, 0xe60000},
				{0x400000, 0x4ffc00}
			};

			for (int o = 0; o < 3; o++) {
				for (uint32_t i = offsets[o][0]; i < offsets[o][1]; i += 2)
				{
					// Check for module ID
					if (!(memory[i] == 0x4A && memory[i+1] == 0xFC))
						continue;

					// Check for module sysrev
					if (!(memory[i+2] == 0x00 && memory[i+3] == 0x01))
						continue;

					// Check for module parity, read for module size in bytes
					uint16_t parity = 0xffff;
					for (uint32_t j = 0; j <= 0x30; j += 2) {
						parity ^= ((memory[i+j] << 8) | memory[i+j+1]);
					}
					if (parity != 0x0000)
						continue;

					Module m;
					m.address = i;

					// ID = 4A FC
					// SysRev = xx xx
					// Size = xx xx xx xx
					m.size = (uint32_t)((memory[i+4] << 24) | (memory[i+5] << 16) | (memory[i+6] << 8) | memory[i+7]);

					uint32_t name_addr = (uint32_t)((memory[i+0xC] << 24) | (memory[i+0xC+1] << 16) | (memory[i+0xC+2] << 8) | memory[i+0xC+3]);
					for (uint32_t j = i+name_addr; j < i+name_addr+40; j++) {
						char c = memory[j];
						if (c == 0)
							break;
						m.name += c;
					}

					modules.push_back(m);
					//MiniCDI::Log("[OS9] found module \"%s\" (name address: %08X) at address %08X of size %X bytes", m.name.c_str(), name_addr, m.address, m.size);

					// Skip module area
					i += m.size - 2;
				}
			}

			//MiniCDI::Log("[OS9] memory contains %d modules", modules.size());
		}

		void log(uint8_t* memory)
		{
			if (memory) {
				const uint32_t addr = m68k_get_reg(NULL, M68K_REG_PC) + 1;
				switch (memory[addr]) {
					// Should return INPUT values

					default:
						return;

					// case 0x00:
						// MiniCDI::Log("[OS9] F$Link    d0.w=%X", m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF);
						// break;

					case 0x06:
						MiniCDI::Log("[OS9] F$Exit    d1.w=%X", m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF);
						break;

					case 0x08:
						MiniCDI::Log("[OS9] F$Send    d0.w=%d d1.w=%d <%s>",
											m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
											m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF,
											get_signal_name(m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF));
						break;

					// case 0x0A:
						// MiniCDI::Log("[OS9] F$Sleep   d0.l=%d", m68k_get_reg(NULL, M68K_REG_D0));
						// break;

					// case 0x1D:
						// MiniCDI::Log("[OS9] F$UnLoad  a2.l=%X", m68k_get_reg(NULL, M68K_REG_A2));
						// break;

					case 0x1E:
						MiniCDI::Log("[OS9] F$RTE");
						break;

					// case 0x28:
						// MiniCDI::Log("[OS9] F$SRqMem  d0.l=%X", m68k_get_reg(NULL, M68K_REG_D0));
						// break;

					// case 0x33:
						// MiniCDI::Log("[OS9] F$IODel   a0.l=%X", m68k_get_reg(NULL, M68K_REG_A0));
						// break;

					// case 0x52:
						// MiniCDI::Log("[OS9] F$SysDbg  d1.w=%X", m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF);
						// break;

					case 0x53:
						MiniCDI::Log("[OS9] F$Event   d0.l=$%X d1.w=%s d2.l=%X d3.l=%X",
											m68k_get_reg(NULL, M68K_REG_D1),
											get_event_name(m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF),
											m68k_get_reg(NULL, M68K_REG_D2),
											m68k_get_reg(NULL, M68K_REG_D3));
						break;

					case 0x84:
						MiniCDI::Log("[OS9] I$Open    d0.w=%d a0=$%X",
											m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
											m68k_get_reg(NULL, M68K_REG_A0));
						break;

					case 0x89:
						MiniCDI::Log("[OS9] I$Read    d0.w=%d d1.l=$%X a0=$%X",
											m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
											m68k_get_reg(NULL, M68K_REG_D1),
											m68k_get_reg(NULL, M68K_REG_A0));
						break;

					case 0x8E:
						MiniCDI::Log("[OS9] I$SetStt  d0.w=%d d1.w=%s",
											m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
											get_function_name(m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF));
						break;
				}
			}
		}
	};
};