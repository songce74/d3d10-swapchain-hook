#define HOOK_JUMP_LEN 5

/* CHookJump modified from taksi project: BSD license */
struct CHookJump
{
public:
        CHookJump() : m_dwOldProtection(0)
        {
			m_jump[0] = 0;
        }

        bool IsHookInstalled()
        {
			return(m_jump[0] != 0);
        }

        bool InstallHook(LPVOID pFunc, LPVOID pFuncNew );
        void RemoveHook(LPVOID pFunc);

		//Copy back saved code fragment
        void SwapOld( LPVOID pFunc )
        {
			memcpy(pFunc, m_oldCode, sizeof(m_oldCode));
        }

		//Copy back JMP instruction again
        void SwapReset( LPVOID pFunc )
        {
			/* hook has since been destroyed! */
			if (!IsHookInstalled())
				return;

			memcpy(pFunc, m_jump, sizeof(m_jump));
        }
private:
        DWORD m_dwOldProtection;	   // used by VirtualProtect()
        BYTE m_oldCode[HOOK_JUMP_LEN]; // what was there previous.
        BYTE m_jump[HOOK_JUMP_LEN];    // what do i want to replace it with.
};