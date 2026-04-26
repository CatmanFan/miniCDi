/** @name os9defs.h Defintions for OS-9 emulation.
 *
 * This file contains definitions for OS-9 constants,
 * in many cases directly corresponding with the contents
 * of the Microware link libraries usr.l and sys.l or
 * the assembler definitions file oskdefs.d.
 *
 * @author CD-i Fan
 */

#ifndef OS9DEFS_H
#define OS9DEFS_H

// Define OS-9 memory modules.
#include "os9/os9mmod.hpp"

/** OS-9 System Call numbers.
 *
 * These correspond with F$Xxx and I$Xxx from usr.l and sys.l.
 */
enum EOs9SysCall
{
	// Non-I/O system calls.
	F_Link = 0x0000,
	F_Load = 0x0001,
	F_UnLink = 0x0002,
	F_Fork = 0x0003,
	F_Wait = 0x0004,
	F_Chain = 0x0005,
	F_Exit = 0x0006,
	F_Mem = 0x0007,
	F_Send = 0x0008,
	F_Icpt = 0x0009,
	F_Sleep = 0x000A,
	F_SSpd = 0x000B,
	F_ID = 0x000C,
	F_SPrior = 0x000D,
	F_STrap = 0x000E,
	F_PErr = 0x000F,
	F_PrsNam = 0x0010,
	F_CmpNam = 0x0011,
	F_SchBit = 0x0012,
	F_AllBit = 0x0013,
	F_DelBit = 0x0014,
	F_Time = 0x0015,
	F_STime = 0x0016,
	F_CRC = 0x0017,
	F_GPrDsc = 0x0018,
	F_GBlkMp = 0x0019,
	F_GModDr = 0x001A,
	F_CpyMem = 0x001B,
	F_SUser = 0x001C,
	F_UnLoad = 0x001D,
	F_RTE = 0x001E,
	F_GPrDBT = 0x001F,
	F_Julian = 0x0020,
	F_TLink = 0x0021,
	F_DFork = 0x0022,
	F_DExec = 0x0023,
	F_DExit = 0x0024,
	F_DatMod = 0x0025,
	F_SetCRC = 0x0026,
	F_SetSys = 0x0027,
	F_SRqMem = 0x0028,
	F_SRtMem = 0x0029,
	F_IRQ = 0x002A,
	F_IOQu = 0x002B,
	F_AProc = 0x002C,
	F_NProc = 0x002D,
	F_VModul = 0x002E,
	F_FindPD = 0x002F,
	F_AllPD = 0x0030,
	F_RetPD = 0x0031,
	F_SSvc = 0x0032,
	F_IODel = 0x0033,
	F_GProcP = 0x0037,
	F_Move = 0x0038,
	F_AllRAM = 0x0039,
	F_Permit = 0x003A,
	F_Protect = 0x003B,
	F_AllTsk = 0x003F,
	F_DelTsk = 0x0040,

	F_AllPrc = 0x004B,
	F_DelPrc = 0x004C,

	F_FModul = 0x004E,

	F_SysDbg = 0x0052,
	F_Event = 0x0053,
	F_Gregor = 0x0054,
	F_SysID = 0x0055,
	F_Alarm = 0x0056,
	F_SigMask = 0x0057,
	F_ChkMem = 0x0058,
	F_UAcct = 0x0059,
	F_CCtl = 0x005A,
	F_GSPUMp = 0x005B,
	F_SRqCMem = 0x005C,
	F_POSK = 0x005D,
	F_Panic = 0x005E,
	F_MBuf = 0x005F,
	F_Trans = 0x0060,

	// I/O system calls.
	I_Attach = 0x0080,
	I_Detach = 0x0081,
	I_Dup = 0x0082,
	I_Create = 0x0083,
	I_Open = 0x0084,
	I_MakDir = 0x0085,
	I_ChgDir = 0x0086,
	I_Delete = 0x0087,
	I_Seek = 0x0088,
	I_Read = 0x0089,
	I_Write = 0x008A,
	I_ReadLn = 0x008B,
	I_WritLn = 0x008C,
	I_GetStt = 0x008D,
	I_SetStt = 0x008E,
	I_Close = 0x008F,

	I_SGetSt = 0x0092,

	//* Number of system calls.
	OS9_SYSCALLS = 0x100,

	// User state SVC.
	OS9_UsrTrap = 0,

	// System state SVC.
	OS9_SysTrap = 0x8000,
};

enum EOs9Event
{
	Ev_Link = 0x0000,
	Ev_UnLnk = 0x0001,
	Ev_Creat = 0x0002,
	Ev_Delet = 0x0003,
	Ev_Wait = 0x0004,
	Ev_WaitR = 0x0005,
	Ev_Read = 0x0006,
	Ev_Info = 0x0007,
	Ev_Signl = 0x0008,
	Ev_Pulse = 0x0009,
	Ev_Set = 0x000A,
	Ev_SetR = 0x000B,

	Ev_All = 0x8000,
};

enum EOs9Alarm
{
	A_Delete = 0x0000,
	A_Set = 0x0001,
	A_Cycle = 0x0002,
	A_AtDate = 0x0003,
	A_AtJul = 0x0004,
};

/** OS-9 GetStt/SetStt function codes.
 * 
 * These correspond directly with usr.l and sys.l.
 */
enum EOs9GetSetStat
{
	SS_Opt = 0x0000,
	SS_Ready = 0x0001,
	SS_Size = 0x0002,
	SS_Reset = 0x0003,
	SS_WTrk = 0x0004,
	SS_Pos = 0x0005,
	SS_EOF = 0x0006,
	SS_Link = 0x0007,
	SS_ULink = 0x0008,
	SS_Feed = 0x0009,
	SS_Frz = 0x000A,
	SS_SPT = 0x000B,
	SS_SQD = 0x000C,
	SS_DCmd = 0x000D,
	SS_DevNm = 0x000E,
	SS_FD = 0x000F,
	SS_Ticks = 0x0010,
	SS_Lock = 0x0011,
	SS_DStat = 0x0012,
	SS_Joy = 0x0013,
	SS_BlkRd = 0x0014,
	SS_BlkWr = 0x0015,
	SS_Reten = 0x0016,
	SS_WFM = 0x0017,
	SS_RFM = 0x0018,
	SS_ELog = 0x0019,
	SS_SSig = 0x001A,
	SS_Relea = 0x001B,
	SS_Attr = 0x001C,
	SS_Break = 0x001D,
	SS_RsBit = 0x001E,
	SS_RMS = 0x001F,
	SS_FDInf = 0x0020,
	SS_ACRTC = 0x0021,
	SS_IFC = 0x0022,
	SS_OFC = 0x0023,
	SS_EnRTS = 0x0024,
	SS_DsRTS = 0x0025,
	SS_DCOn = 0x0026,
	SS_DCOff = 0x0027,
	SS_Skip = 0x0028,
	SS_Mode = 0x0029,
	SS_Open = 0x002A,
	SS_Close = 0x002B,
	SS_Path = 0x002C,
	SS_Play = 0x002D,
	SS_HEADER = 0x002E,
	SS_Raw = 0x002F,
	SS_Seek = 0x0030,
	SS_Abort = 0x0031,
	SS_CDDA = 0x0032,
	SS_Pause = 0x0033,
	SS_Eject = 0x0034,
	SS_Mount = 0x0035,
	SS_Stop = 0x0036,
	SS_Cont = 0x0037,
	SS_Disable = 0x0038,
	SS_Enable = 0x0039,
	SS_ReadToc = 0x003A,
	SS_SM = 0x003B,
	SS_SD = 0x003C,
	SS_SC = 0x003D,
	SS_SEvent = 0x003E,
	SS_Sound = 0x003F,
	SS_DSize = 0x0040,
	SS_Net = 0x0041,
	SS_Rename = 0x0042,
	SS_Free = 0x0043,
	SS_VarSect = 0x0044,

	SS_UCM = 0x004C,

	SS_DM = 0x0051,
	SS_GC = 0x0052,
	SS_RG = 0x0053,
	SS_DP = 0x0054,
	SS_DR = 0x0055,
	SS_DC = 0x0056,
	SS_CO = 0x0057,
	SS_VIQ = 0x0058,
	SS_PT = 0x0059,
	SS_SLink = 0x005A,
	SS_KB = 0x005B,
	SS_SL = 0x005C,

	SS_Bind = 0x006C,
	SS_Listen = 0x006D,
	SS_Connect = 0x006E,
	SS_Resv = 0x006F,
	SS_Accept = 0x0070,
	SS_Recv = 0x0071,
	SS_Send = 0x0072,
	SS_GNam = 0x0073,
	SS_SOpt = 0x0074,
	SS_GOpt = 0x0075,
	SS_Shut = 0x0076,
	SS_SendTo = 0x0077,
	SS_RecvFr = 0x0078,
	SS_Install = 0x0079,
	SS_PCmd = 0x007A,

	SS_SN = 0x008C,
	SS_AR = 0x008D,
	SS_MS = 0x008E,
	SS_AC = 0x008F,
	SS_CDFD = 0x0090,
	SS_CChan = 0x0091,
	SS_FG = 0x0092,

	MV_Abort = 0x0100,
	MV_BColor = 0x0101,
	MV_ChSpeed = 0x0102,
	MV_Close = 0x0103,
	MV_Conceal = 0x0104,
	MV_Continue = 0x0105,
	MV_Freeze = 0x0106,
	MV_Hide = 0x0107,
	MV_ImgSize = 0x0108,
	MV_Loop = 0x0109,
	MV_Next = 0x010A,
	MV_Off = 0x010B,
	MV_Org = 0x010C,
	MV_Pause = 0x010D,
	MV_Play = 0x010E,
	MV_Pos = 0x010F,
	MV_Release = 0x0110,
	MV_SelStrm = 0x0111,
	MV_Show = 0x0112,
	MV_Trigger = 0x0113,
	MV_Window = 0x0114,
	MV_SLink = 0x0115,
	MV_Jump = 0x0116,
	MV_ReqSync = 0x0117,

	MA_Abort = 0x011E,
	MA_Close = 0x011F,
	MA_Cntrl = 0x0120,
	MA_Continue = 0x0121,
	MA_Loop = 0x0122,
	MA_Pause = 0x0123,
	MA_Play = 0x0124,
	MA_Release = 0x0125,
	MA_Trigger = 0x0126,
	MA_SLink = 0x0127,
	MA_Jump = 0x0128,
	MA_ReqSync = 0x0129,
	MA_Waste = 0x012A,

	MV_Create = 0x0130,
	MV_Info = 0x0131,
	MV_Status = 0x0132,

	MA_Create = 0x0138,
	MA_Status = 0x0139,
	MA_Info = 0x013A,

	//* Limit for valid codes.
	SS_LIMIT
};

/** Mode values.
 *
 * These correspond to values in oskdefs.d.
 */
enum EOs9Mode
{
	//* Append all writes to EOF.
	OS9_Append_ = 1 << 4,

	//* Initial file size (F$Create Mode value only).
	OS9_ISize_ = 1 << 5,

	//* Non-Sharable file if set.
	OS9_Share_ = 1 << 6,

	//* Directory file if set.
	OS9_Dir_ = 1 << 7,
};

/** OS-9 Signal numbers.
 * 
 * These correspond directly to S$XXX from usr.l
 * and the OS-9 2.4 Technical Manual.
 */
enum EOs9Signal
{
	//* Kill process.
	S_Kill = 0x00,

	//* Wake process.
	S_Wake = 0x01,

	//* Keyboard quit.
	S_Abort = 0x02,

	//* Keyboard interrupt.
	S_Intrpt = 0x03,

	//* Modem hangup.
	S_HangUp = 0x04,

	//* Signals below this are deadly if not caught.
	S_Deadly = 0x20,
};

/** OS-9 Error codes.
 * 
 * These correspond directly to E$XXX from usr.l
 * and the OS-9 2.4 Technical Manual, Error codes.
 */
enum EOs9Error
{
	//* @section Miscellaneous errors.

	E_IllFnc = 0x40,
	E_FmtErr = 0x41,
	E_NotNum = 0x42,
	E_IllArg = 0x43,

	//* @section Processor Exception errors.

	E_BusErr = 0x66,
	E_AdrErr = 0x67,
	E_IllIns = 0x68,
	E_ZerDiv = 0x69,
	E_Chk = 0x6A,
	E_TrapV = 0x6B,
	E_Violat = 0x6C,
	E_Trace = 0x6D,
	E_1010 = 0x6E,
	E_1111 = 0x6F,
	E_Resrvd = 0x70,
	E_Trap = 0x85,

	//* @section Miscellaneous errors.

	E_Permit = 0xA4,
	E_Differ = 0xA5,
	E_StkOvf = 0xA6,
	E_EvntID = 0xA7,
	E_EvNF = 0xA8,
	E_EvBusy = 0xA9,
	E_EvParm = 0xAA,
	E_Damage = 0xAB,
	E_BadRev = 0xAC,
	E_PthLost = 0xAD,
	E_BadPart = 0xAE,
	E_Hardware = 0xAF,
	E_SectSize = 0xB0,

	//* @section Operating System errors.

	E_PthFul = 0xC8,
	E_BPNum = 0xC9,
	E_Poll = 0xCA,
	E_BMode = 0xCB,
	E_DevOvf = 0xCC,
	E_BMID = 0xCD,
	E_DirFul = 0xCE,
	E_MemFul = 0xCF,
	E_UnkSvc = 0xD0,
	E_ModBsy = 0xD1,
	E_BPAddr = 0xD2,
	E_EOF = 0xD3,
	E_VctBsy = 0xD4,
	E_NES = 0xD5,
	E_FNA = 0xD6,
	E_BPNam = 0xD7,
	E_PNNF = 0xD8,
	E_SLF = 0xD9,
	E_CEF = 0xDA,
	E_IBA = 0xDB,
	E_HangUp = 0xDC,
	E_MNF = 0xDD,
	E_NoClk = 0xDE,
	E_DelSP = 0xDF,
	E_IPrcID = 0xE0,
	E_Param = 0xE1,
	E_NoChld = 0xE2,
	E_ITrap = 0xE3,
	E_PrcAbt = 0xE4,
	E_PrcFul = 0xE5,
	E_IForkP = 0xE6,
	E_KwnMod = 0xE7,
	E_BMCRC = 0xE8,
	E_Signal = 0xE9,
	E_NEMod = 0xEA,
	E_BNam = 0xEB,
	E_BMHP = 0xEC,
	E_NoRAM = 0xED,
	E_DNE = 0xEE,
	E_NoTask = 0xEF,

	//* @section I/O errors.

	E_Unit = 0xF0,
	E_Sect = 0xF1,
	E_WP = 0xF2,
	E_CRC = 0xF3,
	E_Read = 0xF4,
	E_Write = 0xF5,
	E_NotRdy = 0xF6,
	E_Seek = 0xF7,
	E_Full = 0xF8,
	E_BTyp = 0xF9,
	E_DevBsy = 0xFA,
	E_DIDC = 0xFB,
	E_Lock = 0xFC,
	E_Share = 0xFD,
	E_DeadLk = 0xFE,
	E_Format = 0xFF,

	//* @section CD-RTOS errors.

	E_IllPrm = 0x0600,
	E_IdFull = 0x0601,
	E_BadSiz = 0x0602,
	E_RgFull = 0x0603,
	E_UnID = 0x0604,
	E_NullRg = 0x0605,
	E_BadMod = 0x0606,
	E_NoFont = 0x0607,
	E_NoDM = 0x0608,
	E_NoPlay = 0x0609,
	E_Abort = 0x060A,
	E_QFull = 0x060B,
	E_Busy = 0x060C,
};

/** OS-9 User's Register Stack offsets.
 *
 * These mostly correspond to R$xx from sys.l;
 * the _w and _b suffixes have been added.
 */
enum EOs9UserRegStack
{
	//* Data register 0 (long).
	R_d0 = 0x00,

	//* Data register 0 (word).
	R_d0_w = 0x02,

	//* Data register 0 (byte).
	R_d0_b = 0x03,

	//* Data register 1 (long).
	R_d1 = 0x04,

	//* Data register 1 (word).
	R_d1_w = 0x06,

	//* Data register 1 (byte).
	R_d1_b = 0x07,

	//* Data register 2 (long).
	R_d2 = 0x08,

	//* Data register 2 (word).
	R_d2_w = 0x0A,

	//* Data register 2 (byte).
	R_d2_b = 0x0B,

	//* Data register 3 (long).
	R_d3 = 0x0C,

	//* Data register 3 (word).
	R_d3_w = 0x0E,

	//* Data register 3 (byte).
	R_d3_b = 0x0F,

	//* Data register 4 (long).
	R_d4 = 0x10,

	//* Data register 4 (word).
	R_d4_w = 0x12,

	//* Data register 4 (byte).
	R_d4_b = 0x13,

	//* Data register 5 (long).
	R_d5 = 0x14,

	//* Data register 5 (word).
	R_d5_w = 0x16,

	//* Data register 5 (byte).
	R_d5_b = 0x17,

	//* Data register 6 (long).
	R_d6 = 0x18,

	//* Data register 6 (word).
	R_d6_w = 0x1A,

	//* Data register 6 (byte).
	R_d6_b = 0x1B,

	//* Data register 7 (long).
	R_d7 = 0x1C,

	//* Data register 7 (word).
	R_d7_w = 0x1E,

	//* Data register 7 (byte).
	R_d7_b = 0x1F,

	//* Address register 0 (long).
	R_a0 = 0x20,

	//* Address register 1 (long).
	R_a1 = 0x24,

	//* Address register 2 (long).
	R_a2 = 0x28,

	//* Address register 3 (long).
	R_a3 = 0x2C,

	//* Address register 4 (long).
	R_a4 = 0x30,

	//* Address register 5 (long).
	R_a5 = 0x34,

	//* Address register 6 (long).
	R_a6 = 0x38,

	//* Address register 7 (long).
	R_a7 = 0x3C,

	//* User stack pointer (long).
	R_sp = 0x3C,

	//* Status register (word).
	R_sr = 0x40,

	//* Condition codes (byte).
	R_cc = 0x41,

	//* Program counter register (long).
	R_pc = 0x42,

	//* Exception format and vector (word).
	R_fmt = 0x46,

	//* Total Register Stack size.
	R_Size = 0x48
};

/** OS-9 System Global Variables offset.
 *
 * These correspond directly to sys.l.
 */
enum EOs9GlobalVars
{
	//* Set to modsync code after coldstart has finished.
	D_ID = 0x0000,

	//* Set to non-zero to prevent sysproc from sleeping.
	D_NoSleep = 0x0002,

	//* Initialization module ptr ("init").
	D_Init = 0x0020,

	//* Address of system tick routine.
	D_Clock = 0x0024,

	//* Number of ticks per second.
	D_TckSec = 0x0028,

	//* Year.
	D_Year = 0x002A,

	//* Month.
	D_Month = 0x002C,

	//* Day.
	D_Day = 0x002D,

	//* Reserved for version compatability problems (#1).
	D_Compat = 0x002E,

	//* FPU Type (68020/030/040 systems).
	D_68881 = 0x002F,

	//* Julian day number.
	D_Julian = 0x0030,

	//* System time: seconds left until midnight.
	D_Second = 0x0034,

	//* IRQ flag.
	D_IRQFlag = 0x003A,

	//* Number of times unknown IRQ occurred in a row.
	D_UnkIRQ = 0x003B,

	//* Module directory (start & end ptrs).
	D_ModDir = 0x003C,

	//* Process Descriptor Block Table ptr.
	D_PrcDBT = 0x0044,

	//* Path Descriptor Block Table ptr.
	D_PthDBT = 0x0048,

	//* Current Process descriptor ptr.
	D_Proc = 0x004C,

	//* System Process Descriptor ptr.
	D_SysPrc = 0x0050,

	//* Ever-increment system tick.
	D_Ticks = 0x0054,

	//* Process whose context is in FPU registers.
	D_FProc = 0x0058,

	//* System state bus trap panic abort sp, return pc.
	D_AbtStk = 0x005C,

	//* System IRQ stack ptr.
	D_SysStk = 0x0060,

	//* Bootstrap ROM execution entry point.
	D_SysROM = 0x0064,

	//* Exception Jump Table ptr.
	D_ExcJmp = 0x0068,

	//* Total RAM found by BootROM at startup.
	D_TotRAM = 0x006C,

	//* Process minimum allocatable block size.
	D_MinBlk = 0x0070,

	//* System free memory list head.
	D_FreMem = 0x0074,

	//* System minimum allocatable block size.
	D_BlkSiz = 0x007C,

	//* I/O Device Table ptr.
	D_DevTbl = 0x0080,

	//* 68070 on-chip I/O AutoVector Polling table heads.
	D_AutIRQ2 = 0x0088,

	//* Vectored Interrupt device tbl ptrs.
	D_VctIRQ = 0x00A4,

	//* System Service dispatch table ptr.
	D_SysDis = 0x03A4,

	//* User Service dispatch table ptr.
	D_UsrDis = 0x03A8,

	//* Active process queue head.
	D_ActivQ = 0x03AC,

	//* Sleeping process queue head.
	D_SleepQ = 0x03B4,

	//* Sleeping process queue head.
	D_WaitQ = 0x03BC,

	//* Active queue age.
	D_ActAge = 0x03C4,

	//* MPU type (68000/68010/68020/68030/68070/68300/68040).
	D_MPUTyp = 0x03C8,

	//* Ptr to system Event table start, end.
	D_EvTbl = 0x03CC,

	//* Next (incrementing) event ID number.
	D_EvID = 0x03D4,

	//* Ptr to SPU global variables (null == not enabled).
	D_SPUMem = 0x03D8,

	//* Highest address found during startup.
	D_AddrLim = 0x03DC,

	//* Reserved for version compatibility problems (#2).
	D_Compat2 = 0x03E0,

	//* All data caches are coherent/snoopy (if non-zero).
	D_SnoopD = 0x03E1,

	//* Size of a process descriptor.
	D_ProcSz = 0x03E2,

	//* I/O AutoVector Polling table heads.
	D_PolTbl = 0x03E4,

	//* Head of system free memory color node list.
	D_FreeMem = 0x0404,

	//* Interprocessor identification number.
	D_IPID = 0x040C,

	//* Ptr to array of cpu descriptor list heads.
	D_CPUs = 0x0410,

	//* Head of inter-processor command queue.
	D_IPCmd = 0x0414,

	//* 68020/68030/68040 CACR mode.
	D_CachMode = 0x0764,

	//* Instruction cache disable depth.
	D_DisInst = 0x0768,

	//* Data cache disable depth.
	D_DisData = 0x076C,

	//* Ptr to clock tick thief’s static storage.
	D_ClkMem = 0x0770,

	//* Current tick.
	D_Tick = 0x0774,

	//* Ticks per slice.
	D_TSlice = 0x0776,

	//* Current time slice remaining (ticks).
	D_Slice = 0x0778,

	//* Time (ticks) to elapse before sys proc is awakened.
	D_Elapse = 0x077C,

	//* System Thread queue head (immediate, or at absolute time).
	D_Thread = 0x0780,

	//* System timed alarm threads (relative times).
	D_AlarTh = 0x0788,

	//* System IRQ stack low bound.
	D_SStkLm = 0x0790,

	//* Number of actively forked processes..
	D_Forks = 0x0794,

	//* Ram found during bootrom search (for integrity check).
	D_BootRAM = 0x0798,

	//* FPU (max) state frame size.
	D_FPUSize = 0x079C,

	//* FPU Emulator global data.
	D_FPUMem = 0x07A0,

	//* System Hardware dependent I/O flags.
	D_IOGlob = 0x07A4,

	//* ???.
	D_DMAReq = 0x07A4,

	//* ???.
	D_DuImr = 0x07C4,

	//* ???.
	D_IntSel = 0x07C4,

	//* ???.
	D_MotOn = 0x07C4,

	//* ???.
	D_Start = 0x07C4,

	//* ???.
	D_Buzzer = 0x07C5,

	//* ???.
	D_MotCnt = 0x07C5,

	//* ???.
	D_Cursor = 0x07C6,

	//* ???.
	D_ExpIRQ = 0x07C6,

	//* ???.
	D_Col = 0x07C8,

	//* ???.
	D_DskMot = 0x07C8,

	//* ???.
	D_Row = 0x07C9,

	//* ???.
	D_DskSta = 0x07CC,

	//* System minimum process priority.
	D_MinPty = 0x08A6,

	//* System priority maximum age limit.
	D_MaxAge = 0x08A8,

	//* Process ID of process that has siezed cpu.
	D_Sieze = 0x08AA,

	//* Gross estimate of sysglob, process, and module defs.
	D_Cigar = 0x08AC,

	//* Former minimum memory address allocatable.
	D_MMinLim = 0x08B0,

	//* Former maximum memory address allocatable (+1).
	D_MMaxLim = 0x08B4,

	//* Last user changable global.
	D_SysLst = 0x08E4,

	//* System debugger entry pt address.
	D_SysDbg = 0x08EC,

	//* System debugger memory ptr.
	D_DbgMem = 0x08F0,

	//* System debugger active flag.
	D_DbgFlg = 0x08F4,

	//* Disk cache buffer head.
	D_Cache = 0x08F8,

	//* End of system global RAM.
	D_End = 0x1000
};

/** OS-9 Device Table offsets.
 *
 * These correspond to V$XXX from sys.l and
 * the struct devicetbl from sysio.h.
 */
enum Os9DevTbl
{
	//* Device driver module ptr.
	V_DRIV = 0x0000,

	//* Device driver static storage base.
	V_STAT = 0x0004,

	//* Device descriptor module ptr.
	V_DESC = 0x0008,

	//* File manager module ptr.
	V_FMGR = 0x000C,

	//* Use count.
	V_USRS = 0x0010,

	//* End of device table entry.
	V_END = 0x0012
};

/** OS-9 IRQ Polling Table offsets.
 *
 * These correspond to Q$XXX from sys.l.
 */
enum Os9PolTbl
{
	//* Pointer to next entry in linked list.
	Q_LINK = 0x0000,

	//* Address of interrupt handler routine.
	Q_SERV = 0x0004,

	//* Interrupt handler static storage address (A2).
	Q_STAT = 0x0008,

	//* Interrupt handler port address (A3).
	Q_POLL = 0x000C,

	//* Interrupt polling priority.
	Q_PRTY = 0x0010,

	//* End of IRQ polling table entry.
	Q_END = 0x0012
};

/** OS-9 Free Memory List offset.
 *
 * These correspond to M_XXX from sys.l but
 * are not considered documented.
 */
enum Os9MemBlk
{
	M_NxtPtr = 0x0000,
	M_PrvPtr = 0x0004,
	M_BlkSiz = 0x0008,

	/* Size of memory block */
	M_SIZE = 0x000C
};


/** OS-9 Path Descriptor offsets.
 *
 * These correspond to PD_XXX from sys.l and the
 * OS-9 2.4 Technical I/O Manual, pages 1-14 to 1-16.
 */
enum EOs9PathDesc
{
	//* Path Number.
	PD_PD = 0x00,

	//* Access Mode (R W E S D).
	PD_MOD = 0x02,

	//* Number of Paths using this PD (obsolete).
	PD_CNT = 0x03,

	//* Address of Related Device Table Entry.
	PD_DEV = 0x04,

	//* Requester's Process ID.
	PD_CPR = 0x08,

	//* Address of Caller's MPU Register Stack.
	PD_RGS = 0x0A,

	//* Address of Data Buffer.
	PD_BUF = 0x0E,

	//* Group/User ID of Original path Owner
	PD_USER = 0x12,

	//* Kernel List of Open Paths on Device.
	PD_PATHS = 0x16,

	//* Kernel Number of Paths using this PD.
	PD_COUNT = 0x1A,

	//* Kernel Last Active Process ID.
	PD_LProc = 0x1C,

	//* File Manager Global "errno" for C language file managers.
	PD_ErrNo = 0x20,

	//* File Manager System global pointer for C language file managers.
	PD_SysGlob = 0x24,

	//* File Manager Working Storage.
	PD_FST = 0x2A,

	//* Driver/File Man. Option Table.
	PD_OPT = 0x80,

	//* Device Type (used by many drivers).
	PD_DTP = 0x80,

	//* Size of Path Descriptor.
	PD_SIZE = 0x100
};

/** OS-9 Device Type codes.
 *
 * These correspond to DT_XXX from the
 * OS-9 2.4 Technical I/O Manual, page 1-12.
 */
/*enum EOs9DevType
{
	DT_SCF = 0x00,
	DT_RBF = 0x01,
	DT_Pipe = 0x02,
	DT_SBF = 0x03,
	DT_NFM = 0x04,
	DT_CDFM = 0x05,
	DT_UCM = 0x06,
	DT_SOCK = 0x07,
	DT_PTTY = 0x08,
	DT_INET = 0x09,
	DT_NRF = 0x0A,
	DT_GFM = 0x0B,
};*/

/** OS-9 SCF Path Descriptor offsets.
 *
 * These correspond to PD_XXX from sys.l and the
 * OS-9 2.4 Technical I/O Manual, page 3-12.
 */
enum EOs9ScfPathDesc
{
	// PD_DTP = 0x80,
	PD_UPC = 0x81,
	PD_BSO = 0x82,
	PD_DLO = 0x83,
	PD_EKO = 0x84,
	PD_ALF = 0x85,
	PD_NUL = 0x86,
	PD_PAU = 0x87,
	PD_PAG = 0x88,
	PD_BSP = 0x89,
	PD_DEL = 0x8A,
	PD_EOR = 0x8B,
	PD_EOF = 0x8C,
	PD_RPR = 0x8D,
	PD_DUP = 0x8E,
	PD_PSC = 0x8F,
	PD_INT = 0x90,
	PD_QUT = 0x91,
	PD_BSE = 0x92,
	PD_OVF = 0x93,
	PD_PAR = 0x94,
	PD_BAU = 0x95,
	PD_D2P = 0x65,
	PD_XON = 0x98,
	PD_XOFF = 0x99,
	PD_Tab = 0x9A,
	PD_Tabs = 0x9B,
	PD_TBL = 0x9C,
	PD_Col = 0xA0,
	PD_ERR = 0xA2,
};

/** OS-9 RBF Path Descriptor offsets.
 *
 * These correspond to PD_XXX from sys.l and the
 * OS-9 2.4 Technical I/O Manual, page 2-16 to 2-18.
 */
enum EOs9RbfPathDesc
{
	// PD_DTP = 0x80,
	PD_DRV = 0x81,
	PD_STP = 0x82,
	PD_TYP = 0x83,
	PD_DNS = 0x84,
	PD_CYL = 0x86,
	PD_SID = 0x88,
	PD_VFY = 0x89,
	PD_SCT = 0x8A,
	PD_T0S = 0x8C,
	PD_SAS = 0x8E,
	PD_ILV = 0x90,
	PD_TFM = 0x91,
	PD_TOffs = 0x92,
	PD_SOffs = 0x93,
	PD_SSize = 0x94,
	PD_Cntl = 0x96,
	PD_Trys = 0x98,
	PD_LUN = 0x99,
	PD_WPC = 0x9A,
	PD_RWR = 0x9C,
	PD_Park = 0x9E,
	PD_LSNOffs = 0xA0,
	PD_TotCyls = 0xA4,
	PD_CtrlrID = 0xA6,
	PD_Rate = 0xA7,
	PD_ScsiOpt = 0xA8,
	PD_MaxCnt = 0xAC,
	PD_ATT = 0xB5,
	PD_FD = 0xB6,
	PD_DFD = 0xBA,
	PD_DCP = 0xBE,
	PD_DVT = 0xC2,
	PD_SctSiz = 0xC8,
	PD_NAME = 0xE0,
};

enum EOs9CdfmPathDesc
{
	// PD_DTP = 0x80,
	PD_CDFC = 0x81,
	PD_ErrSz = 0x82,
	PD_NDscs = 0x83,
	PD_DNum = 0x84,
	PD_BFctr = 0x85,
	PD_Did = 0x86,
	PD_Track = 0x8A,
	PD_XAR = 0x8C,
	PD_CNUM = 0x8E,
	PD_TOK = 0x92,
	PD_PTNUM = 0xB2,
	PD_PTOFF = 0xB4,
};

enum EOs9UcmPathDesc
{
	// PD_DTP = 0x80,
	PD_FC = 0x9C,
	PD_D2N = 0x9E,
	PD_D3N = 0xA0,
	PD_CNum = 0xA2,
	PD_Errs = 0xA4,
	PD_D4N = 0xA6,
};

// Moved to os9nrf.h
#ifdef notdef

enum EOs9NrfPathDesc
{
	// PD_DTP = 0x80,
	PD_CONT = 0x81,
	PD_RSIZ = 0x82,
	PD_Name = 0xE0,
};

#endif /*notdef*/

enum EOs9XxxPathDesc
{
	PD_CTLCH = 0x08,
	PD_BufSz = 0x82,
	PD_IOBuf = 0x86,
};

/** OS-9 Process Descriptor offsets.
 *
 * These correspond to P$XXX from sys.l.
 */
enum EOs9ProcDesc
{
	//* Process ID.
	P_ID = 0x0000,

	//* Parent’s ID.
	P_PID = 0x0002,

	//* Sibling’s ID.
	P_SID = 0x0004,

	//* Child’s ID.
	P_CID = 0x0006,

	//* System stack ptr.
	P_sp = 0x0008,

	//* User stack ptr.
	P_usp = 0x000C,

	//* Total combined non-module memory consumption.
	P_MemSiz = 0x0010,

	//* User index (group, user).
	P_User = 0x0014,

	//* Priority.
	P_Prior = 0x0018,

	//* Age.
	P_Age = 0x001A,

	//* Status.
	P_State = 0x001C,

	//* Process task number.
	P_Task = 0x001E,

	//* Current queue; what process is doing.
	P_QueuID = 0x0020,

	//* Last system call executed.
	P_SCall = 0x0021,

	//* Non-zero if process was created by Fork.
	P_Baked = 0x0022,

	//* Dominant Process ID if I/O locked.
	P_DeadLk = 0x0024,

	//* Signal code.
	P_Signal = 0x0026,

	//* Signal intercept vector.
	P_SigVec = 0x0028,

	//* Signal intercept data address.
	P_SigDat = 0x002C,

	//* Next process queue ptr.
	P_QueueN = 0x0030,

	//* Previous process queue ptr.
	P_QueueP = 0x0034,

	//* Primary module.
	P_PModul = 0x0038,

	//* program error exception vectors.
	P_Except = 0x003C,

	//* Program error exception stack frame ptrs.
	P_ExStk = 0x0064,

	//* User’s TRAP vectorltable.
	P_Traps = 0x008C,

	//* Trap handler static memory block ptrs.
	P_TrpMem = 0x00C8,

	//* Trap handler static memory block sizes.
	P_TrpSiz = 0x0104,

	//* System state exception recovery stack.
	P_ExcpSP = 0x0140,

	//* System state exception recovery program counter.
	P_ExcpPC = 0x0144,

	//* Default I/O data.
	P_DIO = 0x0148,

	//* I/O path table.
	P_Path = 0x0168,

	//* Allocated memory block ptrs.
	P_MemImg = 0x01A8,

	//* Size of each allocated memory block.
	P_BlkSiz = 0x0228,

	//* Debugged process register stack frame.
	P_DbgReg = 0x02A8,

	//* Debugged process parent Proc Desc ptr.
	P_DbgPar = 0x02AC,

	//* Debugged process instruction count.
	P_DbgIns = 0x02B0,

	//* User state ticks elapsed.
	P_UTicks = 0x02B4,

	//* System state ticks elapsed.
	P_STicks = 0x02B8,

	//* Fork system julian date.
	P_DatBeg = 0x02BC,

	//* Fork julian time when forked.
	P_TimBeg = 0x02C0,

	//* Number of function calls executed.
	P_FCalls = 0x02C4,

	//* Number of I/O system calls executed.
	P_ICalls = 0x02C8,

	//* Number of bytes read.
	P_RBytes = 0x02CC,

	//* Number of bytes written.
	P_WBytes = 0x02D0,

	//* Previous I/O queue (process ID) link.
	P_IOQP = 0x02D4,

	//* Next I/O queue (process ID) link.
	P_IOQN = 0x02D6,

	//* Free memory fragment list head.
	P_Frags = 0x02D8,

	//* Active queue scheduling constant.
	P_Sched = 0x02E0,

	//* Ptr to process’ SPU data.
	P_SPUMem = 0x02E4,

	//* Number of breakpoints set.
	P_BkPtCnt = 0x02E8,

	//* Reserved for contents of breakpoint instructions.
	P_BkPts = 0x02EC,

	//* Reserved for user accounting module.
	P_Acct = 0x030C,

	//* Ptr to process primary data area.
	P_Data = 0x032C,

	//* Size of process primary data area.
	P_DataSz = 0x0330,

	//* Ptr to FPU save area.
	P_FPUSave = 0x0334,

	//* Floating point error exception vectors.
	P_FPExcpt = 0x0338,

	//* Floating point error exception stack frame ptrs.
	P_FPExStk = 0x0354,

	//* Signal interrupt level.
	P_SigLvl = 0x0370,

	//* Signal flag.
	P_SigFlg = 0x0371,

	//* Number of excess signal blocks allocated.
	P_Sigxs = 0x0372,

	//* Mask to disable signals 2-31.
	P_SigMask = 0x0374,

	//* Number of signals pending.
	P_SigCnt = 0x0378,

	//* Ptr to head of signal queue.
	P_SigQue = 0x037C,

	//* Default initial signal queue element.
	P_DefSig = 0x0380,

	//* Doubly linked thread queue.
	P_Thread = 0x0390,

	//* New-style memcolor lists.
	P_frag = 0x0398,

	//* Original owner of primary module (for security).
	P_MOwn = 0x03A0,

	//* Reserved for more stack space.
	P_ExpStk = 0x03A4,

	//* Non-stack data requirement.
	P_Last = 0x03AC,

	//* Top of Stack.
	P_Stack = 0x0800,

	//* Size of Process Descriptor.
	P_ProcSz = 0x0800
};

/** OS-9 File Manager Subroutines.
 *
 * These are the entry numbers for the word offset table
 * pointed to by the module header M$Exec offset.
 *
 * @see OS-9 2.4 Technical I/O Manual, page 1-18.
 */
enum EOs9FileManagerSubroutine
{
	//* Create file (I$Creat).
	FMAN_Create,

	//* Open file (I$Open).
	FMAN_Open,

	//* Make directory (I$MakDir).
	FMAN_MakDir,

	//* Change directory (I$ChgDir).
	FMAN_ChgDir,

	//* Delete file or directory (I$Delete).
	FMAN_Delete,

	//* Seek in file (I$Seek).
	FMAN_Seek,

	//* Read from file (I$Read).
	FMAN_Read,

	//* Write to file (I$Write).
	FMAN_Write,

	//* Read line from file (I$ReadLn).
	FMAN_ReadLn,

	//* Write line to file (I$WritLn).
	FMAN_WriteLn,

	//* Get status of file (I$GetStt.
	FMAN_GetStat,

	//* Set status of file (I$SetStt).
	FMAN_SetStat,

	//* Close file (I$Close).
	FMAN_Close,

	//* Number of File Manager subroutines.
	FMAN_COUNT
};

/** OS-9 Device Driver Subroutines.
 *
 * These are the entry numbers for the word offset table pointed to by the module header M$Exec offset.
 *
 * @see OS-9 2.4 Technical I/O Manual, page 1-21.
 */
enum EOs9DeviceDriverSubroutine
{
	//* Initialize device (I$Attach).
	DRV_Init,

	//* Read from device (I$Read).
	DRV_Read,

	//* Write to device (I$Write).
	DRV_Write,

	//* Get status of device (I$GetStt.
	DRV_GetStat,

	//* Set status of device (I$SetStt).
	DRV_SetStat,

	//* Terminate device (I$Detach).
	DRV_Term,

	//* Handle errors (not used).
	DRV_Trap,

	//* Number of Device Driver subroutines.
	DRV_COUNT,

	//* Interrupt (IRQ).
	DRV_IRQ = 0x100
};

/** OS-9 Compact Disc File Manager SS_SM Subfunctions.
 */
enum EOs9SoundMap
{
	SM_Creat = 0x0000,
	SM_Info = 0x0000,
	SM_Out = 0x0001,
	SM_Stat = 0x0001,
	SM_Off = 0x0002,
	SM_Cncl = 0x0003,
	SM_Close = 0x0004,
	SM_Loop = 0x0005,
	SM_In = 0x0006,
};

/** OS-9 Compact Disc File Manager SS_SD Subfunctions.
 */
enum EOs9SoundData
{
	SD_MMix = 0x0000,
	SD_SMix = 0x0001,
	SD_Loop = 0x0002,
};

/** OS-9 Compact Disc File Manager SS_SC Subfunctions.
 */
enum EOs9SoundControl
{
	SC_Atten = 0x0000,
};

/** OS-9 User Communication Manager SS_DM Subfunctions.
 */
enum EOs9DrawmapControl
{
	DM_Creat = 0x0000,
	DM_Org = 0x0001,
	DM_Copy = 0x0002,
	DM_Exch = 0x0003,
	DM_TCpy = 0x0004,
	DM_TExc = 0x0005,
	DM_Write = 0x0006,
	DM_IrWr = 0x0007,
	DM_Read = 0x0008,
	DM_WrPix = 0x0009,
	DM_RdPix = 0x000A,
	DM_Cncl = 0x000B,
	DM_Close = 0x000C,
	DM_DMDup = 0x000D,
};

/** OS-9 User Communication Manager SS_GC Subfunctions.
 */
enum EOs9GraphicsCursor
{
	GC_Pos = 0x0000,
	GC_Show = 0x0001,
	GC_Hide = 0x0002,
	GC_Ptn = 0x0003,
	GC_Col = 0x0004,
	GC_Blnk = 0x0005,
	GC_Org = 0x0006,
};

/** OS-9 User Communication Manager SS_RG Subfunctions.
 */
enum EOs9Region
{
	RG_Creat = 0x0000,
	RG_Isect = 0x0001,
	RG_Union = 0x0002,
	RG_Diff = 0x0003,
	RG_XOR = 0x0004,
	RG_Move = 0x0005,
	RG_Del = 0x0006,
};

/** OS-9 User Communication Manager SS_DP Subfunctions.
 */
enum EOs9DrawingParameter
{
	DP_Ptn = 0x0000,
	DP_PAln = 0x0001,
	DP_SCMM = 0x0002,
	DP_SCR = 0x0003,
	DP_GFnt = 0x0004,
	DP_AFnt = 0x0005,
	DP_DFnt = 0x0006,
	DP_RFnt = 0x0007,
	DP_Clip = 0x0008,
	DP_PnSz = 0x0009,
	DP_PStyl = 0x000A,
	DP_TCol = 0x000B,
};

/** OS-9 User Communication Manager SS_DR Subfunctions.
 */
enum EOs9GraphicsDrawing
{
	DR_Dot = 0x0000,
	DR_Line = 0x0001,
	DR_PLin = 0x0002,
	DR_CArc = 0x0003,
	DR_EArc = 0x0004,
	DR_Rect = 0x0005,
	DR_ERect = 0x0006,
	DR_PGon = 0x0007,
	DR_Circ = 0x0008,
	DR_CWdg = 0x0009,
	DR_Elps = 0x000A,
	DR_EWdg = 0x000B,
	DR_DRgn = 0x000C,
	DR_BFil = 0x000D,
	DR_FFil = 0x000E,
	DR_Copy = 0x000F,
	DR_Text = 0x0010,
	DR_JTxt = 0x0011,
};

/** OS-9 User Communication Manager SS_DC Subfunctions.
 */
enum EOs9DisplayControl
{
	DC_CrFCT = 0x0000,
	DC_RdFCT = 0x0001,
	DC_WrFCT = 0x0002,
	DC_RdFI = 0x0003,
	DC_WrFI = 0x0004,
	DC_DlFCT = 0x0005,
	DC_CrLCT = 0x0006,
	DC_RdLCT = 0x0007,
	DC_WrLCT = 0x0008,
	DC_RdLI = 0x0009,
	DC_WrLI = 0x000A,
	DC_DlLCT = 0x000B,
	DC_FLnk = 0x000C,
	DC_LLnk = 0x000D,
	DC_Exec = 0x000E,
	DC_Intl = 0x000F,
	DC_NOP = 0x0010,
	DC_SSig = 0x0011,
	DC_Relea = 0x0012,
	DC_SetCmp = 0x0013,
	DC_DsplSiz = 0x0014,
	DC_GetClut = 0x0015,
	DC_GetCluts = 0x0016,
	DC_SetClut = 0x0017,
	DC_SetCluts = 0x0018,
	DC_MapDM = 0x0019,
	DC_Off = 0x001A,
	DC_PRdLCT = 0x001B,
	DC_PWrLCT = 0x001C,
	DC_SetAR = 0x0020,
};

/** OS-9 User Communication Manager SS_CO Subfunctions.
 */
enum EOs9CharOutput
{
	CO_COD = 0x0000,
	CO_SCMM = 0x0001,
	CO_AFnt = 0x0002,
	CO_DFnt = 0x0003,
};

/** OS-9 User Communication Manager SS_VIQ Subfunctions.
 */
enum EOs9VideoInquiry
{
	VIQ_TxtL = 0x0000,
	VIQ_CPos = 0x0001,
	VIQ_JCPs = 0x0002,
	VIQ_FDta = 0x0003,
	VIQ_GDta = 0x0004,
	VIQ_PntR = 0x0005,
	VIQ_RLoc = 0x0006,
	VIQ_RInfo = 0x0007, // From library
	VIQ_RgInfo = 0x0007, // From FFGB
	VIQ_DMInfo = 0x0008,
};

/** OS-9 User Communication Manager SS_PT Subfunctions.
 */
enum EOs9Pointer
{
	PT_Coord = 0x0000,
	PT_SSig = 0x0001,
	PT_Relea = 0x0002,
	PT_Pos = 0x0003,
	PT_Org = 0x0004,
};

/** OS-9 User Communication Manager SS_KB Subfunctions.
 */
enum EOs9Keyboard
{
	KB_Rdy = 0x0000,
	KB_Read = 0x0001,
	KB_Rel = 0x0002,
	KB_Repeat = 0x0003,
	KB_SSig = 0x0004,
	KB_Stat = 0x0005,

	KB_Avail = 0x001E, //* @todo Find out real value.
	KB_NrAvail = 0x001F,  //* @todo Find out real value.
};

/** OS-9 Memory Type Codes.
 */
enum EOs9MemoryType
{
	//* System RAM memory.
	SYSRAM = 0x01,

	//* Video memory for plane A.
	VIDEO1 = 0x80,

	//* Video memory for plane B.
	VIDEO2 = 0x81,

	//* MPEG video buffer memory.
	MPEGRAM = 0x90
};

#endif /* OS9DEFS_H */
