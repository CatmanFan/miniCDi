#ifndef MINICDI_OS9
#define MINICDI_OS9

#include "cdi/common.hpp"

namespace MiniCDI
{
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

		/** OS-9 Module Header data. **/
		struct Module
		{
			uint32_t address;
			uint32_t size;
			std::string name;
		};
		// std::vector<Process> processes;

		static inline const char* get_SSDC_sub_name(uint32_t code) {
			switch (code & 0x000F) {
				default: return "undefined";

				case 0x0000: return "DC_CrFCT";
				case 0x0001: return "DC_RdFCT";
				case 0x0002: return "DC_WrFCT";
				case 0x0003: return "DC_RdFI";
				case 0x0004: return "DC_WrFI";
				case 0x0005: return "DC_DlFCT";
				case 0x0006: return "DC_CrLCT";
				case 0x0007: return "DC_RdLCT";
				case 0x0008: return "DC_WrLCT";
				case 0x0009: return "DC_RdLI";
				case 0x000A: return "DC_WrLI";
				case 0x000B: return "DC_DlLCT";
				case 0x000C: return "DC_FLnk";
				case 0x000D: return "DC_LLnk";
				case 0x000E: return "DC_Exec";
				case 0x000F: return "DC_Intl";
				case 0x0010: return "DC_NOP";
				case 0x0011: return "DC_SSig";
				case 0x0012: return "DC_Relea";
				case 0x0013: return "DC_SetCmp";
				case 0x0014: return "DC_DsplSiz";
				case 0x0015: return "DC_GetClut";
				case 0x0016: return "DC_GetCluts";
				case 0x0017: return "DC_SetClut";
				case 0x0018: return "DC_SetCluts";
				case 0x0019: return "DC_MapDM";
				case 0x001A: return "DC_Off";
				case 0x001B: return "DC_PRdLCT";
				case 0x001C: return "DC_PWrLCT";
				case 0x0020: return "DC_SetAR";
			}
		}

		static inline const char* get_signal_name(uint32_t code) {
			switch (code & 0x000F) {
				default: return "undefined";

				case 0x0000: return "S$Kill";
				case 0x0001: return "S$Wake";
				case 0x0002: return "S$Abort";
				case 0x0003: return "S$Intrpt";
				case 0x0004: return "S$HangUp";

				case 0x0020: return "S$Deadly";
			}
		}

		static inline const char* get_event_name(uint32_t code) {
			switch (code & 0x000F) {
				default: return "undefined";

				case 0x0000: return code & 0x8000 ? "Ev$Link+Ev$All" : "Ev$Link";
				case 0x0001: return code & 0x8000 ? "Ev$UnLnk+Ev$All" : "Ev$UnLnk";
				case 0x0002: return code & 0x8000 ? "Ev$Creat+Ev$All" : "Ev$Creat";
				case 0x0003: return code & 0x8000 ? "Ev$Delet+Ev$All" : "Ev$Delet";
				case 0x0004: return code & 0x8000 ? "Ev$Wait+Ev$All" : "Ev$Wait";
				case 0x0005: return code & 0x8000 ? "Ev$WaitR+Ev$All" : "Ev$WaitR";
				case 0x0006: return code & 0x8000 ? "Ev$Read+Ev$All" : "Ev$Read";
				case 0x0007: return code & 0x8000 ? "Ev$Info+Ev$All" : "Ev$Info";
				case 0x0008: return code & 0x8000 ? "Ev$Signl+Ev$All" : "Ev$Signl";
				case 0x0009: return code & 0x8000 ? "Ev$Pulse+Ev$All" : "Ev$Pulse";
				case 0x000A: return code & 0x8000 ? "Ev$Set+Ev$All" : "Ev$Set";
				case 0x000B: return code & 0x8000 ? "Ev$SetR+Ev$All" : "Ev$SetR";
			}
		}

		static inline const char* get_function_name(uint32_t code) {
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

		Module* get_module(uint32_t addr);

		void clear_modules();
		void scan_modules(uint8_t* memory);
		void log(uint8_t* memory);
	};
};

#endif