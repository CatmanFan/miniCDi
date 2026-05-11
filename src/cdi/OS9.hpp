#ifndef MINICDI_OS9
#define MINICDI_OS9

#include "cdi/common.hpp"
#include "os9/os9defs.hpp"
#include "os9/os9mmod.hpp"
#include "os9/os9trap.hpp"

namespace OS9
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
				M_SysRev = READ16(rom, loc + 0x02);
				M_Size = READ32(rom, loc + 0x04);
				M_Owner = READ32(rom, loc + 0x08);
				M_Name = READ32(rom, loc + 0x0C);
				M_Accs = READ16(rom, loc + 0x10);
				M_Type = (enum ModuleType)(rom[loc + 0x12]);
				M_Lang = rom[loc + 0x13];
				M_Attr = rom[loc + 0x14];
				M_Revs = rom[loc + 0x15];
				M_Edit = READ16(rom, loc + 0x16);

				M_Exec = READ32(rom, loc + 0x30);
				M_Excpt = READ32(rom, loc + 0x34);
				M_Mem = READ32(rom, loc + 0x38);
				M_Stack = READ32(rom, loc + 0x3C);
				M_IData = READ32(rom, loc + 0x40);
				M_IRefs = READ32(rom, loc + 0x44);

				name = reinterpret_cast<const char *>(&rom[M_Name]);
			}
	};

	class System
	{
		private:
			SCC68070 *cpu;

			std::vector<Module> modules; // "module directory"
			std::vector<Process> processes;

		public:
			void init(SCC68070 *cpu, uint8_t *rom, size_t rom_size) {
				this->cpu = cpu;
				modules.push_back(Module(rom, 0));

				/*printf("[header data]\n");
				printf("M$SysRev: %8x  M$Size: %8x\nM$Owner:  %8x  M$Name: %8x\n", modules[0].M_SysRev, modules[0].M_Size, modules[0].M_Owner, modules[0].M_Name);
				printf("M$Accs:   %8x  M$Type: %8x\nM$Lang:   %8x  M$Attr: %8x\n", modules[0].M_Accs, modules[0].M_Type, modules[0].M_Lang, modules[0].M_Attr);
				printf("M$Revs:   %8x  M$Edit: %8x\n", modules[0].M_Revs, modules[0].M_Edit);*/

				for(uint32_t i = 0; i < rom_size; i += 2)
				{
					if (READ16(rom, i) == 0x4AFC)
					{
						// check module parity
						uint16_t parity = 0;
						for (int j = 0; j < 24; j++)
							parity ^= READ16(rom, i + (j * 2));

						if (parity == 0xFFFF) {
							modules.push_back(Module(rom, i));
						}
					}
				}

				/*printf("loaded %d modules\n", modules.size());*/
			}

			void execute()
			{
				if (cpu == nullptr || cpu->context.exception_thrown == 0) return;

				if (cpu->context.exception_thrown == 32) {
					printf("OS9 !!!!!!! %x\n", cpu->context.d_regs[0].w);
					// stop
					// assert(0);
					/*switch ((enum EOs9SysCall)m68k_read_16(&cpu->context, cpu->context.pc)) {
						default:
							break;

						case F_Link:
							{
								OS9::ModuleType type = (OS9::ModuleType)((cpu->context.d_regs[0].w & 0xFF00) >> 8u);
								uint8_t lang = (cpu->context.d_regs[0].w & 0x00FF);
								uint32_t name = cpu->context.a_regs[0].l;

								for (size_t i = 0; i < modules.size(); i++) {
									if (modules[i].M_Type == type && modules[i].M_Name == name && modules[i].M_Lang == lang) {
										cpu->context.d_regs[0].w = (((uint8_t)(modules[i].M_Type) << 8u) | modules[i].M_Lang);
										cpu->context.d_regs[1].w = ((modules[i].M_Attr << 8u) | modules[i].M_Revs);
										// TO-DO:
										// a0.l = Updated past the module name.
										// a2.l = Address of the module directory entry.
									}
								}
							}
							break;
					}*/
				}
			}
	};
};

#endif