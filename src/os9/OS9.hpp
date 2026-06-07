#ifndef MINICDI_OS9
#define MINICDI_OS9

#include "cdi/common.hpp"
#include "os9/os9defs.hpp"
#include "os9/os9mmod.hpp"
#include "os9/os9trap.hpp"

class OS9
{
	enum ModuleType : uint8_t
	{
		Type_NotUsed = 0,
		Type_Program = 1,
		Type_Subroutine,
		Type_Multi,
		Type_Data,
		Type_CSDData,
		Type_TrapLib = 11,
		Type_System,
		Type_FileManager,
		Type_Driver,
		Type_Device
	};

	class Process
	{
	};

	/** OS-9 Module Header data. **/
	class Module
	{
		public:
			std::string name;

			// standard module header fields
			uint16_t M_SysRev;
			uint32_t M_Size;
			uint32_t M_Owner;
			uint32_t M_Name; // address of module name
			uint16_t M_Accs;
			enum ModuleType M_Type;
			uint8_t M_Lang;
			uint8_t M_Attr;
			uint8_t M_Revs;
			uint16_t M_Edit; // Software version

			// additional module header fields
			uint32_t M_Exec; // starting address of program
			uint32_t M_Excpt; // relative address of executable routine for uninit user trap
			uint32_t M_Mem; // program's data size for variables
			uint32_t M_Stack; // min stack size
			uint32_t M_IData; // starting address of initialization data area
			uint32_t M_IRefs; // address to table of values in data area

			Module(uint8_t *rom, uint32_t loc)
			{
				M_SysRev = (rom[loc+0x02] << 8) | rom[loc+0x02+1];
				M_Size = (rom[loc+0x04] << 24) | (rom[loc+0x04+1] << 16) | (rom[loc+0x04+2] << 8) | rom[loc+0x04+3];
				M_Owner = (rom[loc+0x08] << 24) | (rom[loc+0x08+1] << 16) | (rom[loc+0x08+2] << 8) | rom[loc+0x08+3];
				M_Name = (rom[loc+0x0C] << 24) | (rom[loc+0x0C+1] << 16) | (rom[loc+0x0C+2] << 8) | rom[loc+0x0C+3];
				M_Accs = (rom[loc+0x10] << 8) | rom[loc+0x10+1];
				M_Type = (enum ModuleType)(rom[loc+0x12]);
				M_Lang = rom[loc+0x13];
				M_Attr = rom[loc+0x14];
				M_Revs = rom[loc+0x15];
				M_Edit = (rom[loc+0x16] << 8) | rom[loc+0x16+1];

				M_Exec = (rom[loc+0x30] << 24) | (rom[loc+0x30+1] << 16) | (rom[loc+0x30+2] << 8) | rom[loc+0x30+3];
				M_Excpt = (rom[loc+0x34] << 24) | (rom[loc+0x34+1] << 16) | (rom[loc+0x34+2] << 8) | rom[loc+0x34+3];
				M_Mem = (rom[loc+0x38] << 24) | (rom[loc+0x38+1] << 16) | (rom[loc+0x38+2] << 8) | rom[loc+0x38+3];
				M_Stack = (rom[loc+0x3C] << 24) | (rom[loc+0x3C+1] << 16) | (rom[loc+0x3C+2] << 8) | rom[loc+0x3C+3];
				M_IData = (rom[loc+0x40] << 24) | (rom[loc+0x40+1] << 16) | (rom[loc+0x40+2] << 8) | rom[loc+0x40+3];
				M_IRefs = (rom[loc+0x44] << 24) | (rom[loc+0x44+1] << 16) | (rom[loc+0x44+2] << 8) | rom[loc+0x44+3];

				name = reinterpret_cast<const char *>(&rom[M_Name]);
				MiniCDI::Log("[OS9] found module \"%s\" at ROM address %08X", name, loc);
			}
	};
	std::vector<Module> modules; // "module directory"
	std::vector<Process> processes;

	public:
		void list_modules(uint8_t *rom, size_t rom_size) {
			modules.push_back(Module(rom, 0));

			/*MiniCDI::Log("[header data]");
			MiniCDI::Log("M$SysRev: %8x  M$Size: %8x\nM$Owner:  %8x  M$Name: %8x", modules[0].M_SysRev, modules[0].M_Size, modules[0].M_Owner, modules[0].M_Name);
			MiniCDI::Log("M$Accs:   %8x  M$Type: %8x\nM$Lang:   %8x  M$Attr: %8x", modules[0].M_Accs, modules[0].M_Type, modules[0].M_Lang, modules[0].M_Attr);
			MiniCDI::Log("M$Revs:   %8x  M$Edit: %8x", modules[0].M_Revs, modules[0].M_Edit);*/

			for(uint32_t i = 0; i < rom_size; i += 2)
			{
				if (rom[i] == 0x4A && rom[i+1] == 0xFC)
				{
					// check module parity
					uint16_t parity = 0;
					for (int j = 0; j < 24; j++)
						parity ^= (rom[i+(j*2)] << 8) | rom[i+(j*2)+1];

					if (parity == 0xFFFF) {
						modules.push_back(Module(rom, i));
					}
				}
			}

			MiniCDI::Log("[OS9] ROM contains %d modules", modules.size());
		}

		void log(uint8_t* memory)
		{
			if (memory) {
				switch (memory[m68k_get_reg(NULL, M68K_REG_PC)]) {
					default: MiniCDI::Log("[OS9] %X", memory[m68k_get_reg(NULL, M68K_REG_PC)]); return;
					// case 0x00: MiniCDI::Log("[OS9] F$SRqMem  d0.l=%08X d1.w=%04X", m68k_get_reg(NULL, M68K_REG_D0), m68k_get_reg(NULL, M68K_REG_D1)); return;
					// case 0x28: MiniCDI::Log("[OS9] F$SRqMem  d0.l=%08X d1.w=%04X", m68k_get_reg(NULL, M68K_REG_D0), m68k_get_reg(NULL, M68K_REG_D1)); return;
					case 0x52: MiniCDI::Log("[OS9] F$SysDbg  d1.w=%04X", m68k_get_reg(NULL, M68K_REG_D1)); return;
				}
			}
		}
};

#endif