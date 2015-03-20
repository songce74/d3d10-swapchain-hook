
/* Method prototypes */
typedef HRESULT (STDMETHODCALLTYPE *SWAPCHAIN_PRESENT)(void * pSwapChain, RECT* pSourceRect, RECT* pDestRect,HWND hDestWindowOverride, RGNDATA* pDirtyRegion, DWORD dwFlags);
static SWAPCHAIN_PRESENT g_swapChain_Present = NULL;


// D3D 10
typedef HRESULT (STDMETHODCALLTYPE *SWAPCHAIN_PRESENTD3D10)(void * pSwapChain, DWORD syncInterval, DWORD flags);//void * pSwapChain, RECT* pSourceRect, RECT* pDestRect,HWND hDestWindowOverride, RGNDATA* pDirtyRegion, DWORD dwFlags);
static SWAPCHAIN_PRESENTD3D10 g_swapChain_PresentD3D10 = NULL;
