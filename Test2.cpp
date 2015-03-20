#include <Windows.h>
#include <atlbase.h>
#include <atlwin.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <d3dcompiler.h>

#include <memory>
#include <iostream>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef UNICODE
typedef std::wstring    tstring;
typedef wchar_t         tchar;
#else
typedef std::string     tstring;
typedef char            tchar;
#endif

struct Vertex {
    float position[4];
    float color[4];
};

class MainWindow : public CWindowImpl<MainWindow, CWindow, CFrameWinTraits> {
public:
    MainWindow() {
        RECT bounds = { 0, 0, 800, 600 };
        AdjustWindowRect(&bounds, WS_OVERLAPPEDWINDOW, false);

        bounds = { 0, 0, bounds.right - bounds.left, bounds.bottom - bounds.top };
        Create(nullptr, bounds, _T("D3DSample Window"), WS_OVERLAPPEDWINDOW);

        ShowWindow(SW_SHOW);

        // A traditional text. For a traditional time.
        m_text = _T("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    }

    bool ProcessMessages() {
        MSG msg;

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) != 0) {
            if (msg.message == WM_QUIT)
                return false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return true;
    }

    void Present() {
        static float clearColor[] = { 0, 0, 0, 1 };

        {
            m_deviceContext->OMSetRenderTargets(1, &m_backBufferRTV.p, nullptr);
            m_deviceContext->IASetInputLayout(m_inputLayout);
            m_deviceContext->VSSetShader(m_vertexShader, nullptr, 0);
            m_deviceContext->PSSetShader(m_pixelShader, nullptr, 0);
            m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            size_t stride = sizeof(Vertex);
            size_t offsets = 0;
            m_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer.p, &stride, &offsets);
        }
        {
            m_deviceContext->ClearRenderTargetView(m_backBufferRTV, clearColor);
            // Draw our triangle first
            m_deviceContext->Draw(3, 0);

            // Then render our text over it.
            m_d2dRenderTarget->BeginDraw();
            m_d2dRenderTarget->DrawText(m_text.c_str(), m_text.length(), m_dwFormat, D2D1::RectF(0, 0, 512, 512), m_d2dSolidBrush);
            m_d2dRenderTarget->EndDraw();

            m_swapChain->Present(0, 0);
        }
    }
public:
    BEGIN_MSG_MAP(MainWindow)
        MESSAGE_HANDLER(WM_DESTROY, [](unsigned msg, WPARAM wParam, LPARAM lParam, BOOL & bHandled) {
            PostQuitMessage(0);
            return 0;
        });
        MESSAGE_HANDLER(WM_SIZE, OnSize);
        MESSAGE_HANDLER(WM_CREATE, OnCreate);
    END_MSG_MAP()

private:
    HRESULT CreateD3DVertexAndShaders() {
        // Hard coded shaders, not a great idea, but works for the sample.
        std::string vertexShader = "struct VS_IN { float4 pos : POSITION; float4 col : COLOR; }; struct PS_IN { float4 pos : SV_POSITION; float4 col : COLOR; }; PS_IN main( VS_IN input ) { PS_IN output = (PS_IN)0; output.pos = input.pos; output.col = input.col; return output; }";
        std::string pixelShader = "struct VS_IN { float4 pos : POSITION; float4 col : COLOR; }; struct PS_IN { float4 pos : SV_POSITION; float4 col : COLOR; }; float4 main( PS_IN input ) : SV_Target { return input.col; }";

        // If compilation fails, we don't report the errors, just that it failed.
        CComPtr<ID3DBlob> vsBlob;
        CComPtr<ID3DBlob> vsError;
        auto result = D3DCompile(vertexShader.c_str(), vertexShader.length() * sizeof(tchar), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vsBlob, &vsError);
        if (FAILED(result)) {
            std::cout << "Failed to compile vertex shader." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        // If compilation fails, we don't report the errors, just that it failed.
        CComPtr<ID3DBlob> psBlob;
        CComPtr<ID3DBlob> psError;
        result = D3DCompile(pixelShader.c_str(), pixelShader.length() * sizeof(tchar), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psBlob, &psError);
        if (FAILED(result)) {
            std::cout << "Failed to compile pixel shader." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        CComPtr<ID3DBlob> inputLayoutBlob;
        result = D3DGetInputSignatureBlob(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayoutBlob);
        if (FAILED(result)) {
            std::cout << "Failed to get input layout." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        // Hard coded triangle. Tis a silly idea, but works for the sample.
        Vertex vertices[] = {
            { 0.0, 0.5, 0.5, 1.0, 1.0, 0.0, 0.0, 1.0 },
            { 0.5f, -0.5f, 0.5f, 1.0, 0.0, 1.0, 0.0, 1.0 },
            { -0.5f, -0.5f, 0.5f, 1.0, 0.0, 0.0, 1.0, 1.0 }
        };

        D3D11_BUFFER_DESC desc = {
            sizeof(vertices),
            D3D11_USAGE_DEFAULT,
            D3D11_BIND_VERTEX_BUFFER
        };

        D3D11_SUBRESOURCE_DATA data = {
            vertices
        };

        result = m_device->CreateBuffer(&desc, &data, &m_vertexBuffer);
        if (FAILED(result)) {
            std::cout << "Failed to create vertex buffer." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16 }
        };

        result = m_device->CreateInputLayout(inputElementDesc, sizeof(inputElementDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC), inputLayoutBlob->GetBufferPointer(), inputLayoutBlob->GetBufferSize(), &m_inputLayout);
        if (FAILED(result)) {
            std::cout << "Failed to create input layout." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        result = m_device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &m_vertexShader);
        if (FAILED(result)) {
            std::cout << "Failed to create vertex shader." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        result = m_device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &m_pixelShader);
        if (FAILED(result)) {
            std::cout << "Failed to create pixel shader." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        return S_OK;
    }
    HRESULT CreateD3DResources() {
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };

        // We only want to draw to the portion of the window that is the client rect.
        // This will also work for dialog / borderless windows.
        RECT clientRect;
        GetClientRect(&clientRect);

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {
            { clientRect.right, clientRect.bottom, { 60, 1 }, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED, DXGI_MODE_SCALING_UNSPECIFIED },
            { 1, 0 },
            DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT,
            1,
            m_hWnd,
            true,
            DXGI_SWAP_EFFECT_DISCARD,
            DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
        };

        // At the moment we don't actually care about what feature level we got back, so we don't keep this around just yet.
        D3D_FEATURE_LEVEL featureLevel;

        auto result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            // BGRA Support is necessary for D2D functionality.
            D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
            // D2D works with all of our feature levels, so we don't actually care which oen we get. 
            featureLevels, sizeof(featureLevels) / sizeof(D3D_FEATURE_LEVEL),
            D3D11_SDK_VERSION,
            &swapChainDesc,
            &m_swapChain,
            &m_device,
            &featureLevel,
            &m_deviceContext
            );

        if (FAILED(result)) {
            std::cout << "Failed to create D3D device and DXGI swap chain." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        // And lets create our D2D factory and DWrite factory at this point as well, that way if any of them fail we'll fail out completely.
        auto options = D2D1_FACTORY_OPTIONS();
        options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
        result = D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, options, &m_d2dFactory);
        if (FAILED(result)) {
            std::cout << "Failed to create multithreaded D2D factory." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown * *>(&m_dwFactory));
        if (FAILED(result)) {
            std::cout << "Failed to create DirectWrite Factory." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        return S_OK;
    }

    HRESULT CreateBackBufferTarget() {
        CComPtr<ID3D11Texture2D> backBuffer;

        // Get a pointer to our back buffer texture.
        auto result = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
        if (FAILED(result)) {
            std::cout << "Failed to get back buffer." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        // We acquire a render target view to the entire surface (no parameters), with nothing special about it.
        result = m_device->CreateRenderTargetView(backBuffer, nullptr, &m_backBufferRTV);
        if (FAILED(result)) {
            std::cout << "Failed to create render target view for back buffer." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        return S_OK;
    }

    HRESULT CreateD2DResources() {
        CComPtr<IDXGISurface> backBufferSurface;

        // Get a DXGI surface for D2D use.
        auto result = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferSurface));
        if (FAILED(result)) {
            std::cout << "Failed to get DXGI surface for back buffer." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        // Proper DPI support is very important. Most applications do stupid things like hard coding this, which is why you,
        // can't use proper DPI on most monitors in Windows yet.
        float dpiX;
        float dpiY;
        m_d2dFactory->GetDesktopDpi(&dpiX, &dpiY);

        // DXGI_FORMAT_UNKNOWN will cause it to use the same format as the back buffer (R8G8B8A8_UNORM)
        auto d2dRTProps = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED), dpiX, dpiY);

        // Wraps up our DXGI surface in a D2D render target.
        result = m_d2dFactory->CreateDxgiSurfaceRenderTarget(backBufferSurface, &d2dRTProps, &m_d2dRenderTarget);
        if (FAILED(result)) {
            std::cout << "Failed to create D2D DXGI Render Target." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        // This is the brush we will be using to render our text, it does not need to be a solid color,
        // we could use any brush we wanted. In this case we chose a nice solid red brush.
        result = m_d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LimeGreen), &m_d2dSolidBrush);
        if (FAILED(result)) {
            std::cout << "Failed to create solid color brush." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        return S_OK;
    }

    HRESULT CreateDWriteResources() {
        auto result = m_dwFactory->CreateTextFormat(L"Consolas", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14.0f, L"", &m_dwFormat);
        if (FAILED(result)) {
            std::cout << "Failed to create DirectWrite text format." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return result;
        }

        return S_OK;
    }

private:
    LRESULT OnSize(unsigned msg, WPARAM wParam, LPARAM lParam, BOOL & bHandled) {
        // We need to release everything that may be holding a reference to the back buffer.
        // This includes D2D interfaces as well, as they hold a reference to the DXGI surface.
        m_backBufferRTV.Release();
        m_d2dRenderTarget.Release();
        m_d2dSolidBrush.Release();

        // And we make sure that we do not have any render tarvets bound either, which could
        // also be holding references to the back buffer.
        m_deviceContext->ClearState();

        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        auto result = m_swapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
        if (FAILED(result)) {
            std::cout << "Failed to resize swap chain." << std::endl;
            std::cout << "Error was: " << std::hex << result << std::endl;
            return -1;
        }

        // We need to recreate those resources we disposed of above, including our D2D interfaces
        if (FAILED(CreateBackBufferTarget()))
            return -1;

        if (FAILED(CreateD2DResources())) {
            return -1;
        }

        D3D11_VIEWPORT viewport = {
            0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f
        };

        // We setup our viewport here as the size of the viewport is known at this point, WM_SIZE will be sent after a WM_CREATE.
        m_deviceContext->RSSetViewports(1, &viewport);
        return 0;
    }

    LRESULT OnCreate(unsigned msg, WPARAM wParam, LPARAM lParam, BOOL & bHandled) {
        if (FAILED(CreateD3DResources()))
            return -1;

        if (FAILED(CreateBackBufferTarget()))
            return -1;

        if (FAILED(CreateD3DVertexAndShaders()))
            return -1;

        if (FAILED(CreateD2DResources()))
            return -1;

        if (FAILED(CreateDWriteResources()))
            return -1;

        return 0;
    }

private:
    CComPtr<IDXGISwapChain>         m_swapChain;

    CComPtr<ID3D11Device>           m_device;
    CComPtr<ID3D11DeviceContext>    m_deviceContext;

    CComPtr<ID3D11RenderTargetView> m_backBufferRTV;
    CComPtr<ID3D11Buffer>           m_vertexBuffer;
    CComPtr<ID3D11InputLayout>      m_inputLayout;
    CComPtr<ID3D11VertexShader>     m_vertexShader;
    CComPtr<ID3D11PixelShader>      m_pixelShader;

    CComPtr<ID2D1Factory>           m_d2dFactory;
    CComPtr<ID2D1RenderTarget>      m_d2dRenderTarget;
    CComPtr<ID2D1SolidColorBrush>   m_d2dSolidBrush;

    CComPtr<IDWriteFactory>         m_dwFactory;
    CComPtr<IDWriteTextLayout>      m_dwLayout;
    CComPtr<IDWriteTextFormat>      m_dwFormat;

    tstring                         m_text;
};

int main() {
    MainWindow window;
    float clearColor[] = { 0, 0, 0, 0 };

    while (true) {
        if (!window.ProcessMessages())
            break;

        window.Present();
    }
    return 0;
}