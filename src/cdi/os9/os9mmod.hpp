/** @name os9mmod.h Defines OS-9 memory modules.
 *
 * This file contains the definition of the OS9 memory modules.
 *
 * @author CD-i Fan
 */

#ifndef OS9MMOD_H
#define OS9MMOD_H

/** OS9 Memory Module constants.
 *
 * These are taken from Microware's usr.l file.
 */
enum EOs9Module
{
	//* @section Header offsets for all module types.
	
	M_ID            = 0x0000,
	M_SysRev        = 0x0002,
	M_Size          = 0x0004,
	M_Owner         = 0x0008,
	M_Name          = 0x000C,
	M_Accs          = 0x0010,
	M_Type          = 0x0012,
	M_Lang          = 0x0013,
	M_Attr          = 0x0014,
	M_Revs          = 0x0015,
	M_Edit          = 0x0016,
	M_Usage         = 0x0018,
	M_Symbol        = 0x001C,
	M_Parity        = 0x002E,

	M_IDSize        = 0x0030,
	
	//* @section Header constants for all module types.

	M_ID12          = 0x4AFC,
	M_Rev           = 0x0001,
	M_IPID          = 0x004A,
	
	//* @section Header offsets for program, handler, driver, manager, system.
	
	M_Exec          = 0x0030,
	M_Excpt         = 0x0034,
	
	//* @section Header offsets for program, handler, driver.
	
	M_Mem           = 0x0038,
	
	//* @section Header offsets for program, handler.
	
	M_Stack         = 0x003C,
	M_IData         = 0x0040,
	M_IRefs         = 0x0044,

	//* @section Header offsets for handler.
	
	M_Init          = 0x0048,
	M_Term          = 0x004C,
	
	//* @section Header offsets for init descriptor.
	
	M_PollSz        = 0x0034,
	M_DevCnt        = 0x0036,
	M_Mode          = 0x0037,
	M_Procs         = 0x0038,
	M_Paths         = 0x003A,
	M_SParam        = 0x003C,
	M_Sysgo         = 0x003E,
	M_SysDev        = 0x0040,
	M_Consol        = 0x0042,
	M_Extens        = 0x0044,
	M_Clock         = 0x0046,
	M_Slice         = 0x0048,
	M_Site          = 0x004C,
	M_Instal        = 0x0050,
	M_CPUTyp        = 0x0052,
	M_OS9Lvl        = 0x0056,
	M_OS9Rev        = 0x005A,
	M_SysPri        = 0x005C,
	M_MinPty        = 0x005E,
	M_MaxAge        = 0x0060,
	M_MDirSz        = 0x0062,
	M_Events        = 0x0066,
	M_Compat        = 0x0068,
	M_MemList       = 0x006A,
	M_IRQStk        = 0x006C,
	M_ColdTrys      = 0x006E,
	
	//* @section Headers offset for device descriptor.
	
	M_Port          = 0x0030,
	M_Vector        = 0x0034,
	M_IRQLvl        = 0x0035,
	M_Prior         = 0x0036,
	M_FMgr          = 0x0038,
	M_PDev          = 0x003A,
	M_DevCon        = 0x003C,
	M_Opt           = 0x0046,
	M_DTyp          = 0x0048,
};

enum EOs9ModuleType
{
	//* Program Module.
	OS9_Prgrm = 1,

	//* Subroutine Module.
	OS9_Sbrtn = 2,

	//* Multi-Module.
	OS9_Multi = 3,

	//* Data Module.
	OS9_Data = 4,

	//* Configuration Status Descriptor.
	OS9_CSDData = 5,

	//* Trap handler library.
	OS9_TrapLib = 11,

	//* System.
	OS9_Systm = 12,

	//* File Manager.
	OS9_FlMgr = 13,

	//* Device Driver.
	OS9_Drivr = 14,

	//* Device Descriptor.
	OS9_Devic = 15,
};

enum EOs9Language
{
	//* Object Code Module.
	OS9_Objct = 1,

	//* Basic I-code.
	OS9_ICode = 2,

	//* Pascal P-code.
	OS9_PCode = 3,

	//* C I-code.
	OS9_CCode = 4,

	//* Cobol I-code.
	OS9_CblCode = 5,

	//* Fortran I-code.
	OS9_FrtnCode = 6,
};

enum EOs9ModuleAttr
{
	//* Module is re-entrant.
	OS9_ReEnt = 0x80,

	//* Module remains in memory when not in use.
	OS9_Ghost = 0x40,

	//* Module must execute in supervisor state.
	OS9_SupStat = 0x20,

	//* Re-entrant module bit number.
	OS9_ReEntBit = 7,

	//* Ghost module bit number.
	OS9_GhostBit = 6,

	//* Supervisor state bit number.
	OS9_SupStBit = 5,
};

/** Access Permissions.
 *
 * These correspond to values in oskdefs.d.
 */
enum EOs9AccessMode
{
	OS9_Read_ = 1 << 0,

	OS9_Write_ = 1 << 1,

	OS9_Exec_ = 1 << 2,

	OS9_Updat_ = OS9_Read_ | OS9_Write_,

	OS9_PRead_ = 1 << 3,

	OS9_PWrit_ = 1 << 4,

	OS9_PExec_ = 1 << 5,

	OS9_PUpdat_ = OS9_PRead_ | OS9_PWrit_,
};

#endif /* OS9MMOD_H */
