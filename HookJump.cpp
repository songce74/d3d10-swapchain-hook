#include "StdAfx.h"
#include "HookJump.h"

/* CHookJump modified from taksi project: BSD license */
bool CHookJump::InstallHook( LPVOID pFunc, LPVOID pFuncNew )
{
    if (pFunc == NULL)
		return false;
    
	if (IsHookInstalled() && ! memcmp(pFunc, m_jump, sizeof(m_jump)))
        return true;

    m_jump[0] = 0;

    DWORD dwNewProtection = PAGE_EXECUTE_READWRITE;
    if (!::VirtualProtect(pFunc, 8, dwNewProtection, &m_dwOldProtection))
            return false;

    /* unconditional JMP to relative address is 5 bytes */
    m_jump[0] = 0xE9;
    DWORD dwAddr = (DWORD)((UINT_PTR)pFuncNew - (UINT_PTR)pFunc) - sizeof(m_jump);
    memcpy(m_jump+1, &dwAddr, sizeof(dwAddr));
    memcpy(m_oldCode, pFunc, sizeof(m_oldCode));
    memcpy(pFunc, m_jump, sizeof(m_jump));

	// added flushing CPU cache so if it was in cache old code isnt executed
	// jk not needed on x86/64 as they have transparent cache
	/*According to the Intel 64 and IA-32 Architectures Software Deverloper's Manual, Volume 3A: System Programming Guide, Part 1, 
	a jump instruction is sufficient to serialize the instruction prefetch queue when executing self-modifying code.*/

	/*Cross-Modifying Code: Intel defines cross-modifying code as a scenario where Thread 1 on Processor 1 
	modifies instructions that are then immediately executed by Thread 2 on Processor 2.   
	The execution of a barrier instruction to serialize the data prefetch queue (LOCK prefix, XCHG, MFENCE, etc) 
	will not serialize the instruction prefetch queue.  To serialize the instruction prefetch queue the other 
	processors must do a context switch (e.g., SuspendThread followed by ResumeThread) or execute a serializing 
	instruction such as CPUID.  See the topic "Handling Self- and Cross-Modifying Code", in the Intel Architecture manual.*/
	//FlushInstructionCache()
    return true;
}

void CHookJump::RemoveHook( LPVOID pFunc )
{
    if (pFunc == NULL)
		return;

	/* was never set! */
    if (!IsHookInstalled())       
        return;

    try
    {
        memcpy(pFunc, m_oldCode, sizeof(m_oldCode));    // SwapOld(pFunc)
        DWORD dwOldProtection = 0;
        ::VirtualProtect(pFunc, 8, m_dwOldProtection, &dwOldProtection ); // restore protection.
        m_jump[0] = 0;
    }
    catch (...)
    {}
}
