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
			uint32_t offset;
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

			Module(std::vector<char> &rom, uint32_t loc)
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

				name = &rom[loc + M_Name];
				offset = loc;

				MiniCDI::Log("[OS9] found module \"%s\" at ROM address %08X", name.c_str(), M_Name, loc);
			}
	};
	std::vector<Module> modules; // "module directory"
	std::vector<Process> processes;

	Module* get_module(uint32_t loc) {
		for (size_t i = 0; i < modules.size(); i++) {
			if (modules[i].offset == loc)
				return &modules[i];
		}

		return NULL;
	}

	const char* get_function_name(uint32_t code) {
		switch (code) {
			default: return "undefined";

			case 0x0000: return "SS_Opt";
			case 0x0001: return "SS_Ready";
			case 0x0002: return "SS_Size";
			case 0x0003: return "SS_Reset";
			case 0x0004: return "SS_WTrk";
			case 0x0005: return "SS_Pos";
			case 0x0006: return "SS_EOF";
			case 0x0007: return "SS_Link";
			case 0x0008: return "SS_ULink";
			case 0x0009: return "SS_Feed";
			case 0x000A: return "SS_Frz";
			case 0x000B: return "SS_SPT";
			case 0x000C: return "SS_SQD";
			case 0x000D: return "SS_DCmd";
			case 0x000E: return "SS_DevNm";
			case 0x000F: return "SS_FD";
			case 0x0010: return "SS_Ticks";
			case 0x0011: return "SS_Lock";
			case 0x0012: return "SS_DStat";
			case 0x0013: return "SS_Joy";
			case 0x0014: return "SS_BlkRd";
			case 0x0015: return "SS_BlkWr";
			case 0x0016: return "SS_Reten";
			case 0x0017: return "SS_WFM";
			case 0x0018: return "SS_RFM";
			case 0x0019: return "SS_ELog";
			case 0x001A: return "SS_SSig";
			case 0x001B: return "SS_Relea";
			case 0x001C: return "SS_Attr";
			case 0x001D: return "SS_Break";
			case 0x001E: return "SS_RsBit";
			case 0x001F: return "SS_RMS";
			case 0x0020: return "SS_FDInf";
			case 0x0021: return "SS_ACRTC";
			case 0x0022: return "SS_IFC";
			case 0x0023: return "SS_OFC";
			case 0x0024: return "SS_EnRTS";
			case 0x0025: return "SS_DsRTS";
			case 0x0026: return "SS_DCOn";
			case 0x0027: return "SS_DCOff";
			case 0x0028: return "SS_Skip";
			case 0x0029: return "SS_Mode";
			case 0x002A: return "SS_Open";
			case 0x002B: return "SS_Close";
			case 0x002C: return "SS_Path";
			case 0x002D: return "SS_Play";
			case 0x002E: return "SS_HEADER";
			case 0x002F: return "SS_Raw";
			case 0x0030: return "SS_Seek";
			case 0x0031: return "SS_Abort";
			case 0x0032: return "SS_CDDA";
			case 0x0033: return "SS_Pause";
			case 0x0034: return "SS_Eject";
			case 0x0035: return "SS_Mount";
			case 0x0036: return "SS_Stop";
			case 0x0037: return "SS_Cont";
			case 0x0038: return "SS_Disable";
			case 0x0039: return "SS_Enable";
			case 0x003A: return "SS_ReadToc";
			case 0x003B: return "SS_SM";
			case 0x003C: return "SS_SD";
			case 0x003D: return "SS_SC";
			case 0x003E: return "SS_SEvent";
			case 0x003F: return "SS_Sound";
			case 0x0040: return "SS_DSize";
			case 0x0041: return "SS_Net";
			case 0x0042: return "SS_Rename";
			case 0x0043: return "SS_Free";
			case 0x0044: return "SS_VarSect";

			case 0x004C: return "SS_UCM";

			case 0x0051: return "SS_DM";
			case 0x0052: return "SS_GC";
			case 0x0053: return "SS_RG";
			case 0x0054: return "SS_DP";
			case 0x0055: return "SS_DR";
			case 0x0056: return "SS_DC";
			case 0x0057: return "SS_CO";
			case 0x0058: return "SS_VIQ";
			case 0x0059: return "SS_PT";
			case 0x005A: return "SS_SLink";
			case 0x005B: return "SS_KB";
			case 0x005C: return "SS_SL";

			case 0x006C: return "SS_Bind";
			case 0x006D: return "SS_Listen";
			case 0x006E: return "SS_Connect";
			case 0x006F: return "SS_Resv";
			case 0x0070: return "SS_Accept";
			case 0x0071: return "SS_Recv";
			case 0x0072: return "SS_Send";
			case 0x0073: return "SS_GNam";
			case 0x0074: return "SS_SOpt";
			case 0x0075: return "SS_GOpt";
			case 0x0076: return "SS_Shut";
			case 0x0077: return "SS_SendTo";
			case 0x0078: return "SS_RecvFr";
			case 0x0079: return "SS_Install";
			case 0x007A: return "SS_PCmd";

			case 0x008C: return "SS_SN";
			case 0x008D: return "SS_AR";
			case 0x008E: return "SS_MS";
			case 0x008F: return "SS_AC";
			case 0x0090: return "SS_CDFD";
			case 0x0091: return "SS_CChan";
			case 0x0092: return "SS_FG";

			case 0x0100: return "MV_Abort";
			case 0x0101: return "MV_BColor";
			case 0x0102: return "MV_ChSpeed";
			case 0x0103: return "MV_Close";
			case 0x0104: return "MV_Conceal";
			case 0x0105: return "MV_Continue";
			case 0x0106: return "MV_Freeze";
			case 0x0107: return "MV_Hide";
			case 0x0108: return "MV_ImgSize";
			case 0x0109: return "MV_Loop";
			case 0x010A: return "MV_Next";
			case 0x010B: return "MV_Off";
			case 0x010C: return "MV_Org";
			case 0x010D: return "MV_Pause";
			case 0x010E: return "MV_Play";
			case 0x010F: return "MV_Pos";
			case 0x0110: return "MV_Release";
			case 0x0111: return "MV_SelStrm";
			case 0x0112: return "MV_Show";
			case 0x0113: return "MV_Trigger";
			case 0x0114: return "MV_Window";
			case 0x0115: return "MV_SLink";
			case 0x0116: return "MV_Jump";
			case 0x0117: return "MV_ReqSync";

			case 0x011E: return "MA_Abort";
			case 0x011F: return "MA_Close";
			case 0x0120: return "MA_Cntrl";
			case 0x0121: return "MA_Continue";
			case 0x0122: return "MA_Loop";
			case 0x0123: return "MA_Pause";
			case 0x0124: return "MA_Play";
			case 0x0125: return "MA_Release";
			case 0x0126: return "MA_Trigger";
			case 0x0127: return "MA_SLink";
			case 0x0128: return "MA_Jump";
			case 0x0129: return "MA_ReqSync";
			case 0x012A: return "MA_Waste";

			case 0x0130: return "MV_Create";
			case 0x0131: return "MV_Info";
			case 0x0132: return "MV_Status";

			case 0x0138: return "MA_Create";
			case 0x0139: return "MA_Status";
			case 0x013A: return "MA_Info";
		}
	}

	public:
		void list_modules(std::vector<char> &rom) {
			modules.push_back(Module(rom, 0));

			for(uint32_t i = 0; i < rom.size(); i += 2)
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
				const uint32_t addr = m68k_get_reg(NULL, M68K_REG_PC) + 1;
				switch (memory[addr]) {
					// Should return INPUT values

					case 0x00: MiniCDI::Log("[OS9] F$Link    d0.w=%04X", m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF); return;
					case 0x06: MiniCDI::Log("[OS9] F$Exit    d1.w=%04X", m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF); return;
					// case 0x0A: MiniCDI::Log("[OS9] F$Sleep   d0.l=%08X", m68k_get_reg(NULL, M68K_REG_D0)); return;
					case 0x1D: MiniCDI::Log("[OS9] F$UnLoad  a2.l=%08X", m68k_get_reg(NULL, M68K_REG_A2)); return;
					case 0x1E: MiniCDI::Log("[OS9] F$RTE"); return;
					case 0x28: MiniCDI::Log("[OS9] F$SRqMem  d0.l=%08X", m68k_get_reg(NULL, M68K_REG_D0)); return;
					case 0x33: MiniCDI::Log("[OS9] F$IODel   a0.l=%08X", m68k_get_reg(NULL, M68K_REG_A0)); return;
					case 0x52: MiniCDI::Log("[OS9] F$SysDbg  d1.w=%04X", m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF); return;
					case 0x8E: MiniCDI::Log("[OS9] I$SetStt  d0.w=%04X d1.w=%s",
											m68k_get_reg(NULL, M68K_REG_D0) & 0xFFFF,
											get_function_name(m68k_get_reg(NULL, M68K_REG_D1) & 0xFFFF)); return;
				}
			}
		}
};

#endif