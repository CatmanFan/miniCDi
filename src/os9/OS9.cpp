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
			const std::vector<Module> prevModules = modules;
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

					bool exists = false;
					for (size_t j = 0; j < prevModules.size(); j++) {
						if (prevModules[j].size == m.size && prevModules[j].address == m.address) {
							exists = true;
						}
					}

					if (!exists)
						MiniCDI::Log("[OS9] found new module \"%s\" (address: $%X, size: $%X)", m.name.c_str(), name_addr, m.address, m.size);

					// Skip module area
					i += m.size - 2;
				}
			}

			//MiniCDI::Log("[OS9] memory contains %d modules", modules.size());
		}

		void log(uint8_t* memory)
		{
			if (!memory) return;
			return; ///!\\\

			// ************************************
			// Should return INPUT values
			// ************************************
			switch (memory[m68k_get_reg(NULL, M68K_REG_PC) + 1])
			{
				case 0x00: {
					std::string name;
					for (uint32_t i = m68k_get_reg(NULL, M68K_REG_A0); i < m68k_get_reg(NULL, M68K_REG_A0)+40; i++) {
						char c = memory[i];
						if (c == 0)
							break;
						name += c;
					}
					MiniCDI::Log("[OS9] F$Link    d0.w=%X (a0)=$%X(%s)",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_A0),
										name.c_str());
					break;
				}

				case 0x01: {
					std::string name;
					for (uint32_t i = m68k_get_reg(NULL, M68K_REG_A0); i < m68k_get_reg(NULL, M68K_REG_A0)+40; i++) {
						char c = memory[i];
						if (c == 0)
							break;
						name += c;
					}
					MiniCDI::Log("[OS9] F$Load    d0.b=%X d1.l=%X (a0)=$%X(%s)",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFF,
										m68k_get_reg(NULL, M68K_REG_D1),
										m68k_get_reg(NULL, M68K_REG_A0),
										name.c_str());
					break;
				}

				case 0x02:
					MiniCDI::Log("[OS9] F$UnLink  (a2)=$%X",
										m68k_get_reg(NULL, M68K_REG_A2));
					break;

				case 0x03: {
					std::string name;
					for (uint32_t i = m68k_get_reg(NULL, M68K_REG_A0); i < m68k_get_reg(NULL, M68K_REG_A0)+40; i++) {
						char c = memory[i];
						if (c == 0)
							break;
						name += c;
					}
					MiniCDI::Log("[OS9] F$Load    d0.w=%X d1.l=%X d2.l=%X d3.w=%X d4.w=%X (a0)=$%X(%s) (a1)=$%X",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D1),
										m68k_get_reg(NULL, M68K_REG_D2),
										m68k_get_reg(NULL, M68K_REG_D3) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D4) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_A0),
										name.c_str(),
										m68k_get_reg(NULL, M68K_REG_A1));
					break;
				}

				case 0x04:
					MiniCDI::Log("[OS9] F$Wait");
					break;

				case 0x06:
					MiniCDI::Log("[OS9] F$Exit    d1.w=%X",
										m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF);
					break;

				case 0x08:
					MiniCDI::Log("[OS9] F$Send    d0.w=%d d1.w=$%X <%s>",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF,
										get_signal_name(m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF));
					break;

				case 0x0A:
					MiniCDI::Log("[OS9] F$Sleep   d0.l=%d",
										m68k_get_reg(NULL, M68K_REG_D0));
					break;

				case 0x1D: {
					std::string name;
					for (uint32_t i = m68k_get_reg(NULL, M68K_REG_A0); i < m68k_get_reg(NULL, M68K_REG_A0)+40; i++) {
						char c = memory[i];
						if (c == 0)
							break;
						name += c;
					}
					MiniCDI::Log("[OS9] F$UnLoad  d0.w=%d (a0)=$%X(%s)",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_A0),
										name.c_str());
					break;
				}

				case 0x1E:
					MiniCDI::Log("[OS9] F$RTE");
					break;

				case 0x28:
					MiniCDI::Log("[OS9] F$SRqMem  d0.l=%X",
										m68k_get_reg(NULL, M68K_REG_D0));
					break;

				case 0x29:
					MiniCDI::Log("[OS9] F$SRtMem  d0.l=%X (a2)=$%X",
										m68k_get_reg(NULL, M68K_REG_D0),
										m68k_get_reg(NULL, M68K_REG_A2));
					break;

				case 0x2A:
					MiniCDI::Log("[OS9] F$IRQ     d0.b=%X d1.b=%X (a0)=$%X (a2)=$%X (a3)=$%X",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFF,
										m68k_get_reg(NULL, M68K_REG_D1) & 0xFF,
										m68k_get_reg(NULL, M68K_REG_A0),
										m68k_get_reg(NULL, M68K_REG_A2),
										m68k_get_reg(NULL, M68K_REG_A3));
					break;

				case 0x33:
					MiniCDI::Log("[OS9] F$IODel   a0.l=%X",
										m68k_get_reg(NULL, M68K_REG_A0));
					break;

				case 0x52:
					MiniCDI::Log("[OS9] F$SysDbg  d1.w=%X",
										m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF);
					break;

				case 0x53:
					MiniCDI::Log("[OS9] F$Event   d0.l=$%X d1.w=%s d2.l=%X d3.l=%X",
										m68k_get_reg(NULL, M68K_REG_D0),
										get_event_name(m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF),
										m68k_get_reg(NULL, M68K_REG_D2),
										m68k_get_reg(NULL, M68K_REG_D3));
					break;

				case 0x54:
					MiniCDI::Log("[OS9] F$Gregor  d0.l=%X d1.l=%X",
										m68k_get_reg(NULL, M68K_REG_D0),
										m68k_get_reg(NULL, M68K_REG_D1));
					break;

				case 0x56:
					MiniCDI::Log("[OS9] F$Alarm   d0.l=%X d1.w=%X d2.l=%X d3.l=%X d4.l=%X",
										m68k_get_reg(NULL, M68K_REG_D0),
										m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D2),
										m68k_get_reg(NULL, M68K_REG_D3),
										m68k_get_reg(NULL, M68K_REG_D4));
					break;

				case 0x81:
					MiniCDI::Log("[OS9] I$Detach  (a2)=$%X",
										m68k_get_reg(NULL, M68K_REG_A2));
					break;

				case 0x83: {
					std::string name;
					for (uint32_t i = m68k_get_reg(NULL, M68K_REG_A0); i < m68k_get_reg(NULL, M68K_REG_A0)+40; i++) {
						char c = memory[i];
						if (c == 0)
							break;
						name += c;
					}
					MiniCDI::Log("[OS9] I$Create  d0.b=%d d1.w=%x d2.l=%x (a0)=$%X(%s)",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFF,
										m68k_get_reg(NULL, M68K_REG_D1) & 0xFF,
										m68k_get_reg(NULL, M68K_REG_D2),
										m68k_get_reg(NULL, M68K_REG_A0),
										name.c_str());
					break;
				}

				case 0x84: {
					std::string name;
					for (uint32_t i = m68k_get_reg(NULL, M68K_REG_A0); i < m68k_get_reg(NULL, M68K_REG_A0)+40; i++) {
						char c = memory[i];
						if (c == 0)
							break;
						name += c;
					}
					MiniCDI::Log("[OS9] I$Open    d0.w=%d (a0)=$%X(%s)",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_A0),
										name.c_str());
					break;
				}

				case 0x87: {
					std::string name;
					for (uint32_t i = m68k_get_reg(NULL, M68K_REG_A0); i < m68k_get_reg(NULL, M68K_REG_A0)+40; i++) {
						char c = memory[i];
						if (c == 0)
							break;
						name += c;
					}
					MiniCDI::Log("[OS9] I$Delete  d0.b=%d (a0)=$%X(%s)",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFF,
										m68k_get_reg(NULL, M68K_REG_A0),
										name.c_str());
					break;
				}

				case 0x88:
					MiniCDI::Log("[OS9] I$Seek    d0.w=%d d1.l=%X",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D1));
					break;

				case 0x89:
					MiniCDI::Log("[OS9] I$Read    d0.w=%d d1.l=$%X (a0)=$%X",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D1),
										m68k_get_reg(NULL, M68K_REG_A0));
					break;

				case 0x8A:
					MiniCDI::Log("[OS9] I$Write   d0.w=%d d1.l=%X (a0)=$%X",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D1),
										m68k_get_reg(NULL, M68K_REG_A0));
					break;

				case 0x8B:
					MiniCDI::Log("[OS9] I$ReadLn  d0.w=%d d1.l=$%X (a0)=$%X",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D1),
										m68k_get_reg(NULL, M68K_REG_A0));
					break;

				case 0x8C: {
					std::string name;
					for (uint32_t i = 0; i < m68k_get_reg(NULL, M68K_REG_D1); i++) {
						char c = memory[i+m68k_get_reg(NULL, M68K_REG_A0)];
						name += c;
					}
					MiniCDI::Log("[OS9] I$WritLn  d0.w=%d d1.l=$%X (a0)=\"%s\"",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D1),
										name.c_str());
					break;
				}

				case 0x8D:
					MiniCDI::Log("[OS9] I$GetStt  d0.w=%d d1.l=$%X",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
										m68k_get_reg(NULL, M68K_REG_D1));
					break;

				case 0x8E:
					switch (m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF)
					{
						default:
							MiniCDI::Log("[OS9] I$SetStt  d0.w=%d d1.w=%s",
												m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
												get_function_name(m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF));
							break;

						case 0x56:
							MiniCDI::Log("[OS9] I$SetStt  d0.w=%d d1.w=SS_DC d2.w=%s d3.w=%d d4.w=%d d5.w=$%X d2.l=$%X",
												m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
												get_SSDC_sub_name(m68k_get_reg(NULL, M68K_REG_D2) & 0xFFFF),
												m68k_get_reg(NULL, M68K_REG_D3) & 0xFFFF,
												m68k_get_reg(NULL, M68K_REG_D4) & 0xFFFF,
												m68k_get_reg(NULL, M68K_REG_D5) & 0xFFFF,
												m68k_get_reg(NULL, M68K_REG_D6));
							break;
					}
					break;

				case 0x8F:
					MiniCDI::Log("[OS9] I$Close   d0.w=%d",
										m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF);
					break;
			}
		}
	};
};