/** @name os9trap.def Defines OS9 traps.
 *
 * This file contains definitions of the OS9 traps for tracing.
 * This file is #include'd in {@c m68kemu.cpp} to fill the trap table.
 *
 * Each trap definition consists of a single {@c DEFINE_TRAP} line.
 */

#ifndef DEFINE_TRAP
/** Defines trap.
 *
 * @param wCode Trap code word.
 * @param pszTrap Trap name.
 * @param pszInput Input parameter format.
 * @param pszOutput Output parameter format.
 */
#define DEFINE_TRAP(wCode, pszTrap, pszInput, pszOutput)
#endif

#ifndef DEFINE_SUBTRAP1
/** Defines subtrap using d1.
 *
 * @param wCode Trap subcode word from d1.w.
 * @param pszInput Input parameter format.
 * @param pszOutput Output parameter format.
 */
#define DEFINE_SUBTRAP1(wSubCode, pszInput, pszOutput)
#endif

#ifndef DEFINE_SUBTRAP2
/** Defines subtrap using d2.
 *
 * @param wCode Trap subcode word from d2.w.
 * @param pszInput Input parameter format.
 * @param pszOutput Output parameter format.
 */
#define DEFINE_SUBTRAP2(wSubCode, pszInput, pszOutput)
#endif

//* @section Define OS9 traps.

#define OS9_TRAP	0

DEFINE_TRAP(0x0000, "F$Link", "(a0)s d0.w", "(a0)s a1 a2 d0.w d1.w")
DEFINE_TRAP(0x0001, "F$Load", "(a0)s d0.b d1.l", "(a0)s a1 a2 d0.w d1.w")
DEFINE_TRAP(0x0002, "F$UnLink", "a2", "")
DEFINE_TRAP(0x0003, "F$Fork", "(a0)s a1 d0.w d1.l d2.l d3.w d4.w", "(a0)s d0.w")
DEFINE_TRAP(0x0004, "F$Wait", "", "d0.w d1.w")
DEFINE_TRAP(0x0005, "F$Chain", "(a0)s a1 d0.w d1.l d2.l d3.w d4.w", "")
DEFINE_TRAP(0x0006, "F$Exit", "d1.w", "")
DEFINE_TRAP(0x0007, "F$Mem", "d0.l", "d0.l a1")
DEFINE_TRAP(0x0008, "F$Send", "d0.w d1.i", "")
DEFINE_TRAP(0x0009, "F$Icpt", "a0 a6", "")
DEFINE_TRAP(0x000A, "F$Sleep", "d0.l", "d0.l")
DEFINE_TRAP(0x000B, "F$SSpd", "d0.w", "")
DEFINE_TRAP(0x000C, "F$ID", "", "d0.w d1.l d2.w")
DEFINE_TRAP(0x000D, "F$SPrior", "d0.w d1.w", "")
DEFINE_TRAP(0x000E, "F$STrap", "a0 a1", "")
DEFINE_TRAP(0x000F, "F$PErr", "d0.w d1.w", "")
DEFINE_TRAP(0x0010, "F$PrsNam", "(a0)s", "a0 d0.b d1.w a1")
DEFINE_TRAP(0x0011, "F$CmpNam", "(a0)s (a1)s d1.w", "")
DEFINE_TRAP(0x0012, "F$SchBit", "d0.w d1.w a0 a1", "d0.w d1.w")
DEFINE_TRAP(0x0013, "F$AllBit", "a0 d0.w d1.w", "")
DEFINE_TRAP(0x0014, "F$DelBit", "a0 d0.w d1.w", "")
DEFINE_TRAP(0x0015, "F$Time", "d0.w", "d0.l d1.l d2.w d3.l")
DEFINE_TRAP(0x0016, "F$STime", "d0.l d1.l", "")
DEFINE_TRAP(0x0017, "F$CRC", "a0 d0.l d1.l", "d1.l")
DEFINE_TRAP(0x0018, "F$GPrDsc", "d0.w d1.w a0", "")
DEFINE_TRAP(0x0019, "F$GBlkMp", "d0.l d1.l a0", "d0.l d1.l d2.l d3.l a0")
DEFINE_TRAP(0x001A, "F$GModDr", "a0 d1.l", "d1.l")
DEFINE_TRAP(0x001B, "F$CpyMem", "a0 a1 d0.w d1.l", "")
DEFINE_TRAP(0x001C, "F$SUser", "d1.l", "")
DEFINE_TRAP(0x001D, "F$UnLoad", "(a0)s d0.w", "(a0)s")
DEFINE_TRAP(0x001E, "F$RTE", "", "")
DEFINE_TRAP(0x001F, "F$GPrDBT", "d1.l a0", "d1.l")
DEFINE_TRAP(0x0020, "F$Julian", "d0.l d1.l", "d0.l d1.l")
DEFINE_TRAP(0x0021, "F$TLink", "(a0)s d0.w", "(a0)s d0.w d1.w a1 a2")
DEFINE_TRAP(0x0022, "F$DFork", "(a0)s a1 a2 d0.w d1.l d2.l d3.w d4.w", "(a0)s a2 d0.w")
DEFINE_TRAP(0x0023, "F$DExec", "d0.w d1.l d2.w a0", "d0.l d1.l d2.w d3.w d4.l d5.w")
DEFINE_TRAP(0x0024, "F$DExit", "d0.w", "")
DEFINE_TRAP(0x0025, "F$DatMod", "(a0)s d0.l d1.w d2.w d3.w d4.l", "(a0)s d0.w d1.w a1 a2")
DEFINE_TRAP(0x0026, "F$SetCRC", "a0", "")
DEFINE_TRAP(0x0027, "F$SetSys", "d0.w d1.l d2.l", "d2.l")
DEFINE_TRAP(0x0028, "F$SRqMem", "d0.l", "d0.l a2")
DEFINE_TRAP(0x0029, "F$SRtMem", "a2 d0.l", "")
DEFINE_TRAP(0x002A, "F$IRQ", "a0 a2 a3 d0.b d1.b", "")
DEFINE_TRAP(0x002B, "F$IOQu", "d0.w", "")
DEFINE_TRAP(0x002C, "F$AProc", "a0", "")
DEFINE_TRAP(0x002D, "F$NProc", "", "")
DEFINE_TRAP(0x002E, "F$VModul", "a0 d0.l d1.l", "a2")
DEFINE_TRAP(0x002F, "F$FindPD", "a0 d0.w", "a1")
DEFINE_TRAP(0x0030, "F$AllPD", "a0", "a1 d0.w")
DEFINE_TRAP(0x0031, "F$RetPD", "a0 d0.w", "")
DEFINE_TRAP(0x0032, "F$SSvc", "a1 a3", "")
DEFINE_TRAP(0x0033, "F$IODel", "a0", "")
DEFINE_TRAP(0x0037, "F$GProcP", "d0.w", "a1")
DEFINE_TRAP(0x0038, "F$Move", "a0 a2 d2.l", "")
DEFINE_TRAP(0x0039, "F$AllRAM", "xxx", "")
DEFINE_TRAP(0x003A, "F$Permit", "d0.l d1.b a2", "")
DEFINE_TRAP(0x003B, "F$Protect", "d0.l d1.b a2", "")
DEFINE_TRAP(0x003F, "F$AllTsk", "", "")
DEFINE_TRAP(0x0040, "F$DelTsk", "", "")
DEFINE_TRAP(0x004B, "F$AllPrc", "", "a2")
DEFINE_TRAP(0x004C, "F$DelPrc", "d0.w", "")
DEFINE_TRAP(0x004E, "F$FModul", "xxx", "")
DEFINE_TRAP(0x0052, "F$SysDbg", "", "")

DEFINE_TRAP(0x0053, "F$Event", "d1.w d1.w (a0)s", "")
	DEFINE_SUBTRAP1(0x0000, "d0.l d1.w=Ev$Link (a0)s", "(a0)s d0.l ")
	DEFINE_SUBTRAP1(0x0001, "d0.l d1.w=Ev$UnLnk", "")
	DEFINE_SUBTRAP1(0x0002, "d0.l d1.w=Ev$Creat (a0)s d2.w d3.w", "(a0)s d0.l")
	DEFINE_SUBTRAP1(0x0003, "d0.l d1.w=Ev$Delet (a0)s", "(a0)s")
	DEFINE_SUBTRAP1(0x0004, "d0.l d1.w=Ev$Wait d2.l d3.l", "d1.l d2.l")
	DEFINE_SUBTRAP1(0x0005, "d0.l d1.w=Ev$WaitR d2.l d3.l", "d1.l d2.l d3.l d2.l")
	DEFINE_SUBTRAP1(0x0006, "d0.l d1.w=Ev$Read", "d1.l")
	DEFINE_SUBTRAP1(0x0007, "d0.l d1.w=Ev$Info a0", "d0.l a0")
	DEFINE_SUBTRAP1(0x0008, "d0.l d1.w=Ev$Signl", "")
	DEFINE_SUBTRAP1(0x8008, "d0.l d1.w=Ev$Signl", "")
	DEFINE_SUBTRAP1(0x0009, "d0.l d1.w=Ev$Pulse d2.l", "")
	DEFINE_SUBTRAP1(0x8009, "d0.l d1.w=Ev$Pulse+Ev$All d2.l", "")
	DEFINE_SUBTRAP1(0x000A, "d0.l d1.w=Ev$Set d2.l", "")
	DEFINE_SUBTRAP1(0x800A, "d0.l d1.w=Ev$Set+Ev$All d2.l", "d1.l")
	DEFINE_SUBTRAP1(0x000B, "d0.l d1.w=Ev$SetR d2.l", "d1.l")
	DEFINE_SUBTRAP1(0x800B, "d0.l d1.w=Ev$SetR+Ev$All d2.l", "d1.l")

DEFINE_TRAP(0x0054, "F$Gregor", "d0.l d1.l", "d0.l d1.l")
DEFINE_TRAP(0x0055, "F$SysID", "d0.l a0 a1 a2 a3", "d0.l d1.l d2.l d3.l d4.l d5.l d6.l d7.l (a0)s (a1)s (a2) (a3)")

DEFINE_TRAP(0x0056, "F$Alarm", "d0.l d1.w d2.l d3.l d4.l sa0", "d0.l")
	DEFINE_SUBTRAP1(0x0000, "d0.l d1.w=A$Delete", "")
	DEFINE_SUBTRAP1(0x0001, "d0.l d1.w=A$Set d2.w d3.l sa0", "d0.l")
	DEFINE_SUBTRAP1(0x0002, "d0.l d1.w=A$Cycle d2.w d3.l sa0", "d0.l")
	DEFINE_SUBTRAP1(0x0003, "d0.l d1.w=A$AtDate d2.w d3.l d4.l sa0", "d0.l")
	DEFINE_SUBTRAP1(0x0004, "d0.l d1.w=A$AtJul d2.w d3.l d4.l sa0", "d0.l")

DEFINE_TRAP(0x0057, "F$SigMask", "d0.l d1.l", "")
DEFINE_TRAP(0x0058, "F$ChkMem", "d0.l d1.b a2", "")
DEFINE_TRAP(0x0059, "F$UAcct", "d0.w a0", "")
DEFINE_TRAP(0x005A, "F$CCtl", "d0.l", "")
DEFINE_TRAP(0x005B, "F$GSPUMp", "d0.w d2.l a0", "d2.l (a0)")
DEFINE_TRAP(0x005C, "F$SRqCMem", "d0.l d1.l", "d0.l a2")
DEFINE_TRAP(0x005D, "F$POSK", "xxx", "")
DEFINE_TRAP(0x005E, "F$Panic", "d0.l", "")
DEFINE_TRAP(0x005F, "F$MBuf", "xxx", "")
DEFINE_TRAP(0x0060, "F$Trans", "d0.l d1.l a0", "d0.l a0")

DEFINE_TRAP(0x0080, "I$Attach", "(a0)s d0.b", "a2")
DEFINE_TRAP(0x0081, "I$Detach", "a2", "")
DEFINE_TRAP(0x0082, "I$Dup", "d0.w", "d0.w")
DEFINE_TRAP(0x0083, "I$Create", "(a0)s d0.b d1.w d2.l", "(a0)s d0.w")
DEFINE_TRAP(0x0084, "I$Open", "(a0)s d0.b", "d0.w a0")
DEFINE_TRAP(0x0085, "I$MakDir", "(a0)s d0.b d1.w d2.l", "(a0)s")
DEFINE_TRAP(0x0086, "I$ChgDir", "(a0)s d0.b", "(a0)s")
DEFINE_TRAP(0x0087, "I$Delete", "(a0)s d0.b", "(a0)s")
DEFINE_TRAP(0x0088, "I$Seek", "d0.w d1.h", "")
DEFINE_TRAP(0x0089, "I$Read", "d0.w d1.l a0", "d1.l")
DEFINE_TRAP(0x008A, "I$Write", "d0.w d1.l (a0)s", "d1.l")
DEFINE_TRAP(0x008B, "I$ReadLn", "d0.w d1.l a0", "d1.l")
DEFINE_TRAP(0x008C, "I$WritLn", "d0.w d1.l (a0)s", "d1.l")
DEFINE_TRAP(0x008D, "I$GetStt", "d0.w d1.w", "")
DEFINE_TRAP(0x008E, "I$SetStt", "d0.w d1.w", "")
DEFINE_TRAP(0x008F, "I$Close", "d0.w", "")


//* @section Remove macros.

#undef DEFINE_TRAP
#undef DEFINE_SUBTRAP1
#undef DEFINE_SUBTRAP2

