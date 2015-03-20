#pragma unmanaged
#include "stdafx.h"
#include "HookJump.h"
#include "Dll.h"
#include <d3d9.h>
#include <d3d10.h>
#include <windows.h>
#include <atlbase.h>

CHookJump g_swapChainHookD3D9;
CHookJump g_swapChainHookD3D10;
IDirect3DDevice9 * g_direct3DDevice9 = NULL;
ID3D10Device * g_direct3DDevice10 = NULL;

#pragma unmanaged
HRESULT WINAPI SwapChain_PresentHooked(void * pSwapChain, 
									   RECT* pSourceRect, 
									   RECT* pDestRect,
									   HWND hDestWindowOverride,
									   RGNDATA* pDirtyRegion, 
									   DWORD dwFlags)
{
	HRESULT hr = S_OK;

	/* Restore old method */
	g_swapChainHookD3D9.SwapOld((unsigned int*)g_swapChain_Present);

	/* Modify the data before going into the original */
	//pDestRect->bottom = pDestRect->bottom /2;
	//pDestRect->right = pDestRect->right /2;

	/* We don't need to worry about QueryInterface here
	   because the pointer is already of the correct interface. */
	IDirect3DSwapChain9 * swapChain = (IDirect3DSwapChain9*)pSwapChain;
	
	/* Our temp device pointer */
	IDirect3DDevice9 * d3dDevice;

	/* Get the device.  This does an AddRef() */
 	hr = swapChain->GetDevice(&d3dDevice);
	
	/* If we got a device pointer AND it's a different device */
	if(d3dDevice != g_direct3DDevice9 && hr == S_OK)
	{
		if(g_direct3DDevice9)
			g_direct3DDevice9->Release();

		/* Store ref to the device */
		g_direct3DDevice9 = d3dDevice;
	}

	g_direct3DDevice9->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	/* Call the original method */
	hr = g_swapChain_Present(pSwapChain, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

	
	/* Restore the hook */
	g_swapChainHookD3D9.SwapReset(g_swapChain_Present);

	return hr;
}

HRESULT WINAPI SwapChain_PresentHookedD3D10(void * pSwapChain, DWORD syncInterval, DWORD flags)
{
	//MessageBox(NULL, "SwapChain_PresentHookedD3D10", "caption", 0);
	OutputDebugStringA("SwapChain_PresentHookedD3D10 executed\n");

	g_swapChainHookD3D10.SwapOld((unsigned int*)g_swapChain_PresentD3D10);

	IDXGISwapChain* swapChain = (IDXGISwapChain*)pSwapChain;
	if (swapChain)
	{
		CComPtr<IDXGISurface> backBufferSurface;
		//CComPtr<ID3D10Device> device;
		ID3D10Device *device = nullptr;
		HRESULT hr = swapChain->GetDevice(__uuidof(device), reinterpret_cast<void**>( &device ));
		if (hr != S_OK)
		{
			OutputDebugStringA("SwapChain_PresentHookedD3D10: failed to get device\n");
		}
		else 
		{	
			swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferSurface));
			//auto d2dRTProps = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);
			//D3DXCreateTextureFromFileInMemory()
			/*device->ClearState();
			D3D10_RASTERIZER_DESC rdesc;
			ZeroMemory(&rdesc, sizeof(rdesc));
			rdesc.FillMode = D3D10_FILL_WIREFRAME;
			rdesc.CullMode = D3D10_CULL_BACK;
			ID3D10RasterizerState *rs;
			hr = device->CreateRasterizerState(&rdesc, &rs);
			if (hr == S_OK)
			{
				device->RSSetState(rs);
				OutputDebugStringA("SwapChain_PresentHookedD3D10: set wireframe OK\n");
			}
			else 
				OutputDebugStringA("SwapChain_PresentHookedD3D10: failed to create rasterizer\n");

			g_swapChain_PresentD3D10(pSwapChain, syncInterval, flags);

			device->ClearState();
			device->RSSetState(rs);*/
			/*
			D3DPRESENT_PARAMETERS d3dpp = {0};
			d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
			d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
			d3dpp.Windowed = TRUE;
			d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
			err = d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);
			assert(err == S_OK);
			D3DXCreateTextureFromFileInMemory(d3ddev, img, size, &d3dtex);
			D3DLOCKED_RECT lock;
			D3DSURFACE_DESC d3ddesc;
			d3dtex->GetLevelDesc(0, &d3ddesc);
			pix = new unsigned int[d3ddesc.Width * d3ddesc.Height];
			d3dtex->LockRect(0, &lock, NULL, D3DLOCK_READONLY);
			unsigned int *dst, *src;
			src = (unsigned int *)lock.pBits;
			dst = pix;
			for (unsigned int y = 0; y < d3ddesc.Height; y++)
			{
				memcpy(dst, src, d3ddesc.Width * sizeof(unsigned int));
				dst += d3ddesc.Width;
				src += lock.Pitch/sizeof(unsigned int);
			}
			d3dtex->UnlockRect(0);
			d3dtex->Release();
			d3ddev->Release();
			d3d9->Release();*/
			device->Draw(3, 0);
		}
	}
	else 
		OutputDebugStringA("SwapChain_PresentHookedD3D10: bad swap chain\n");

	g_swapChain_PresentD3D10(pSwapChain, syncInterval, flags);
	g_swapChainHookD3D10.SwapReset(g_swapChain_PresentD3D10);
	return S_OK;
}

extern "C" __declspec(dllexport) HRESULT WINAPI InitializeD3D9Hook()
{
	OutputDebugStringA("InitializeD3D9Hook executed");
	HRESULT hr;
	HMODULE mod = LoadLibrary("D3D9.DLL");

	IDirect3D9Ex * pD3D9;
	IDirect3DDevice9 *pD3D9Device;
	IDirect3DSwapChain9 * pSwapChain;

	/* Nothing out of the ordinary */
	Direct3DCreate9Ex( D3D_SDK_VERSION, &pD3D9 );

	/* Standard present parameters for D3D9 */
	D3DPRESENT_PARAMETERS		d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed				= TRUE;
	d3dpp.SwapEffect			= D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow			= GetDesktopWindow();
	d3dpp.PresentationInterval	= D3DPRESENT_INTERVAL_IMMEDIATE;

	/* Create the IDirect3DDevice9 */
	hr = pD3D9->CreateDevice(D3DADAPTER_DEFAULT,
							 D3DDEVTYPE_HAL,
							 GetDesktopWindow(),
							 D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
							 &d3dpp,
							 &pD3D9Device);
	
	/* Create the swap chain */
	hr = pD3D9Device->CreateAdditionalSwapChain(&d3dpp, &pSwapChain);

	/* Get the vtable for the swap chain */
	UINT_PTR* pSwapChainVTable = (UINT_PTR*)(/*Indirection ->*/*((UINT_PTR*)pSwapChain));

	/* The third address is the swap chain Present(...) */
	unsigned* pfSwapChain_Present = (unsigned*)(pSwapChainVTable[3]);

	g_swapChain_Present = (SWAPCHAIN_PRESENT)pfSwapChain_Present;

	/* Hook into the method */
	g_swapChainHookD3D9.InstallHook(g_swapChain_Present, SwapChain_PresentHooked);
	
	/* Free our temporary resources */
	pSwapChain->Release();
	pD3D9Device->Release();
	pD3D9->Release();
	
	return S_OK;
}

extern "C" __declspec(dllexport) HRESULT WINAPI D3D10Hook()
{
	OutputDebugStringA("D3D10Hook executed");
	HRESULT result;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	IDXGISwapChain *swapChain;
	ID3D10Device *device;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = 100;
	swapChainDesc.BufferDesc.Height = 100;

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = GetDesktopWindow();
	swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	
	result = D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, 
									       &swapChainDesc, &swapChain, &device);

	// D3D10: Present is 8, ResizeTarget = 14
	// see IDXGISwapChainVtbl in dxgi.h
	UINT_PTR* pSwapChainVTable = (UINT_PTR*)(/*Indirection ->*/*((UINT_PTR*)swapChain));
	unsigned* pfSwapChain_Present = (unsigned*)(pSwapChainVTable[8]);

	g_swapChain_PresentD3D10 = (SWAPCHAIN_PRESENTD3D10)pfSwapChain_Present;


	g_swapChainHookD3D10.InstallHook(g_swapChain_PresentD3D10, SwapChain_PresentHookedD3D10); 
	device->Release(); 
	swapChain->Release();
	return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule,
					  DWORD   dwReason,
					  LPVOID  lpReserved)
{

	//if (dwReason == DLL_THREAD_ATTACH)
	//	InitializeD3D9Hook();
	return TRUE;
}