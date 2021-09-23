#include "lstd/common.h"

#if OS == WINDOWS

#define LSTD_JUST_DX
#include "lstd/platform/windows.h"
#undef LSTD_JUST_DX

#include <d3d11.h>
#include <dxgi.h>

#include "lstd/fmt/fmt.h"
#include "lstd_graphics/graphics/api.h"
#include "lstd_graphics/graphics/texture.h"
#include "lstd_graphics/video.h"

import lstd.os;

LSTD_BEGIN_NAMESPACE

// @TODO: These are so useful that we should include them in the API
string utf16_to_utf8(const wchar *str, allocator alloc = {});

void d3d_init(graphics *g) {
    IDXGIFactory *factory;
    DX_CHECK(CreateDXGIFactory(__uuidof(IDXGIFactory), (void **) &factory));
    defer(COM_SAFE_RELEASE(factory));

    IDXGIAdapter *adapter;
    DX_CHECK(factory->EnumAdapters(0, &adapter));
    defer(COM_SAFE_RELEASE(adapter));

    IDXGIOutput *adapterOutput;
    defer(COM_SAFE_RELEASE(adapterOutput));
    DX_CHECK(adapter->EnumOutputs(0, &adapterOutput));

    u32 numModes = 0;
    DX_CHECK(adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, null));
    assert(numModes);

    DXGI_MODE_DESC *displayModeList = malloc<DXGI_MODE_DESC>({.Count = numModes, .Alloc = TemporaryAllocator});
    DX_CHECK(adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList));

    DXGI_ADAPTER_DESC adapterDesc;
    DX_CHECK(adapter->GetDesc(&adapterDesc));

    string adapterStr = platform_utf16_to_utf8(adapterDesc.Description);
    print("{!YELLOW}----------------------------------\n");
    print(" Direct3D 11:\n");
    print("    {}\n", adapterStr);
    print("    VRAM: {} MB\n", adapterDesc.DedicatedVideoMemory / 1024 / 1024);
    print("----------------------------------\n\n{!}");

    // In order to support Direct2D, add D3D11_CREATE_DEVICE_BGRA_SUPPORT to _creationFlags_
    u32 creationFlags = 0;
#if defined DEBUG || defined DEBUG_OPTIMIZED
    // If the project is not in Dist configuration, enable the debug layer.
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D11CreateDevice(null, D3D_DRIVER_TYPE_HARDWARE, null, creationFlags, null, 0, D3D11_SDK_VERSION, &g->D3D.Device, null, &g->D3D.DeviceContext);

    // @TODO: g->D3D.Device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, sampleCount, &sampleQuality);

    D3D11_BLEND_DESC blendDest;
    zero_memory(&blendDest, sizeof(blendDest));
    {
        blendDest.AlphaToCoverageEnable  = false;
        blendDest.IndependentBlendEnable = false;

        blendDest.RenderTarget[0].BlendEnable           = true;
        blendDest.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        blendDest.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        blendDest.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        blendDest.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;  // @TODO: Provide more flexibility for choosing the blend function and factors
        blendDest.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
        blendDest.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        blendDest.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        // @TODO blendDest.RenderTarget is an array of 8 targets
    }
    DX_CHECK(g->D3D.Device->CreateBlendState(&blendDest, &g->D3D.BlendStates[0]));

    zero_memory(&blendDest, sizeof(blendDest));
    {
        blendDest.RenderTarget[0].BlendEnable           = false;
        blendDest.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }
    DX_CHECK(g->D3D.Device->CreateBlendState(&blendDest, &g->D3D.BlendStates[1]));

    D3D11_DEPTH_STENCIL_DESC stencilDesc;
    zero_memory(&stencilDesc, sizeof(stencilDesc));
    {
        stencilDesc.DepthEnable    = true;
        stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        stencilDesc.DepthFunc      = D3D11_COMPARISON_LESS;

        stencilDesc.StencilEnable    = true;
        stencilDesc.StencilReadMask  = 0xff;
        stencilDesc.StencilWriteMask = 0xff;

        stencilDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        stencilDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        stencilDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;

        stencilDesc.BackFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        stencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        stencilDesc.BackFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        stencilDesc.BackFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
    }
    DX_CHECK(g->D3D.Device->CreateDepthStencilState(&stencilDesc, &g->D3D.DepthStencilStates[0]));

    zero_memory(&stencilDesc, sizeof(stencilDesc));
    {
        stencilDesc.DepthEnable    = false;
        stencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        stencilDesc.DepthFunc      = D3D11_COMPARISON_ALWAYS;

        stencilDesc.StencilEnable = false;

        stencilDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        stencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        stencilDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        stencilDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
        stencilDesc.BackFace                     = stencilDesc.FrontFace;
    }
    DX_CHECK(g->D3D.Device->CreateDepthStencilState(&stencilDesc, &g->D3D.DepthStencilStates[1]));
}

void d3d_init_target_window(graphics *g, graphics::target_window *targetWindow) {
    assert(targetWindow);

    auto win = targetWindow->Window;
    assert(win);

    vec2<s32> windowSize = win.get_size();

    DXGI_SWAP_CHAIN_DESC desc;
    zero_memory(&desc, sizeof(desc));
    {
        desc.BufferCount       = 1;
        desc.BufferDesc.Width  = windowSize.x;
        desc.BufferDesc.Height = windowSize.y;
        desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferDesc.RefreshRate.Numerator =
            win.get_flags() & window::VSYNC ? os_monitor_from_window(win)->CurrentMode.RefreshRate : 0;
        desc.BufferDesc.RefreshRate.Denominator = 1;
        desc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
        desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow                       = (HWND) win.ID;
        desc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
        desc.SampleDesc.Count                   = 1;
        desc.Windowed                           = !win.is_fullscreen();
    }

    IDXGIDevice *device;
    DX_CHECK(g->D3D.Device->QueryInterface(__uuidof(IDXGIDevice), (void **) &device));
    defer(COM_SAFE_RELEASE(device));

    IDXGIAdapter *adapter;
    device->GetAdapter(&adapter);
    defer(COM_SAFE_RELEASE(adapter));

    IDXGIFactory *factory = null;
    adapter->GetParent(__uuidof(IDXGIFactory), (void **) &factory);
    defer(COM_SAFE_RELEASE(factory));

    DX_CHECK(factory->CreateSwapChain(g->D3D.Device, &desc, &targetWindow->D3D.SwapChain));
}

void d3d_release_target_window(graphics *g, graphics::target_window *targetWindow) {
    assert(targetWindow);

    targetWindow->D3D.SwapChain->SetFullscreenState(false, null);
    COM_SAFE_RELEASE(targetWindow->D3D.SwapChain);
    COM_SAFE_RELEASE(targetWindow->D3D.BackBuffer);
    COM_SAFE_RELEASE(targetWindow->D3D.DepthStencilBuffer);
    COM_SAFE_RELEASE(targetWindow->D3D.DepthStencilView);
    COM_SAFE_RELEASE(targetWindow->D3D.RasterStates[0]);
    COM_SAFE_RELEASE(targetWindow->D3D.RasterStates[1]);
    COM_SAFE_RELEASE(targetWindow->D3D.RasterStates[2]);
}

void d3d_target_window_resized(graphics *g, graphics::target_window *targetWindow, s32 width, s32 height) {
    assert(targetWindow);

    COM_SAFE_RELEASE(targetWindow->D3D.BackBuffer);
    COM_SAFE_RELEASE(targetWindow->D3D.DepthStencilView);
    COM_SAFE_RELEASE(targetWindow->D3D.DepthStencilBuffer);
    COM_SAFE_RELEASE(targetWindow->D3D.RasterStates[0]);
    COM_SAFE_RELEASE(targetWindow->D3D.RasterStates[1]);
    COM_SAFE_RELEASE(targetWindow->D3D.RasterStates[2]);

    auto *oldTargetWindow = g->CurrentTargetWindow;
    defer(g->set_target_window(oldTargetWindow->Window));

    g->set_target_window({});
    g->D3D.DeviceContext->Flush();

    DX_CHECK(targetWindow->D3D.SwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

    ID3D11Texture2D *swapChainBackBuffer;
    DX_CHECK(targetWindow->D3D.SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **) &swapChainBackBuffer));
    DX_CHECK(g->D3D.Device->CreateRenderTargetView(swapChainBackBuffer, null, &targetWindow->D3D.BackBuffer));
    COM_SAFE_RELEASE(swapChainBackBuffer);

    D3D11_TEXTURE2D_DESC textureDesc;
    zero_memory(&textureDesc, sizeof(textureDesc));
    {
        textureDesc.Width            = width;
        textureDesc.Height           = height;
        textureDesc.MipLevels        = 1;
        textureDesc.ArraySize        = 1;
        textureDesc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
        textureDesc.SampleDesc.Count = 1;
        textureDesc.Usage            = D3D11_USAGE_DEFAULT;
        textureDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
    }

    DX_CHECK(g->D3D.Device->CreateTexture2D(&textureDesc, null, &targetWindow->D3D.DepthStencilBuffer));
    DX_CHECK(g->D3D.Device->CreateDepthStencilView(targetWindow->D3D.DepthStencilBuffer, null, &targetWindow->D3D.DepthStencilView));

    D3D11_RASTERIZER_DESC rDesc;
    zero_memory(&rDesc, sizeof(rDesc));
    {
        rDesc.FillMode        = D3D11_FILL_SOLID;
        rDesc.CullMode        = D3D11_CULL_NONE;
        rDesc.ScissorEnable   = true;
        rDesc.DepthClipEnable = true;
    }
    DX_CHECK(g->D3D.Device->CreateRasterizerState(&rDesc, &targetWindow->D3D.RasterStates[(s64) cull::None]));
    {
        rDesc.FillMode        = D3D11_FILL_SOLID;
        rDesc.CullMode        = D3D11_CULL_FRONT;
        rDesc.ScissorEnable   = true;
        rDesc.DepthClipEnable = true;
    }
    DX_CHECK(g->D3D.Device->CreateRasterizerState(&rDesc, &targetWindow->D3D.RasterStates[(s64) cull::Front]));
    {
        rDesc.FillMode        = D3D11_FILL_SOLID;
        rDesc.CullMode        = D3D11_CULL_BACK;
        rDesc.ScissorEnable   = true;
        rDesc.DepthClipEnable = true;
    }
    DX_CHECK(g->D3D.Device->CreateRasterizerState(&rDesc, &targetWindow->D3D.RasterStates[(s64) cull::Back]));
}

void d3d_set_viewport(graphics *g, rect viewport) {
    D3D11_VIEWPORT rect;
    rect.TopLeftX = (f32) viewport.Top;
    rect.TopLeftY = (f32) viewport.Left;
    rect.Width    = (f32) viewport.width();
    rect.Height   = (f32) viewport.height();
    rect.MinDepth = 0.0f;
    rect.MaxDepth = 1.0f;

    g->D3D.DeviceContext->RSSetViewports(1, &rect);
}

void d3d_set_scissor_rect(graphics *g, rect scissorRect) {
    D3D11_RECT rect;
    rect.left   = scissorRect.Left;
    rect.top    = scissorRect.Top;
    rect.right  = scissorRect.Right;
    rect.bottom = scissorRect.Bot;

    g->D3D.DeviceContext->RSSetScissorRects(1, &rect);
}

void d3d_set_render_target(graphics *g, texture_2D *target) {
    auto *renderTarget = g->CurrentTargetWindow->D3D.BackBuffer;
    auto *depthStencil = g->CurrentTargetWindow->D3D.DepthStencilView;
    if (target) {
        renderTarget = target->D3D.RenderTargetView;
        depthStencil = target->D3D.DepthStencilView;
    }
    g->D3D.DeviceContext->OMSetRenderTargets(1, &renderTarget, depthStencil);
}

void d3d_set_blend(graphics *g, bool enabled) {
    g->D3D.DeviceContext->OMSetBlendState(enabled ? g->D3D.BlendStates[0] : g->D3D.BlendStates[1], null, 0xffffffff);
}

void d3d_set_depth_testing(graphics *g, bool enabled) {
    g->D3D.DeviceContext->OMSetDepthStencilState(enabled ? g->D3D.DepthStencilStates[0] : g->D3D.DepthStencilStates[1],
                                                 0);
}

void d3d_set_cull_mode(graphics *g, cull mode) {
    g->D3D.DeviceContext->RSSetState(g->CurrentTargetWindow->D3D.RasterStates[(s64) mode]);
}

void d3d_clear_color(graphics *g, v4 color) {
    auto *renderTarget = g->CurrentTargetWindow->D3D.BackBuffer;
    auto *depthStencil = g->CurrentTargetWindow->D3D.DepthStencilView;
    if (g->CurrentTargetWindow->CustomRenderTarget) {
        renderTarget = g->CurrentTargetWindow->CustomRenderTarget->D3D.RenderTargetView;
        depthStencil = g->CurrentTargetWindow->CustomRenderTarget->D3D.DepthStencilView;
    }
    g->D3D.DeviceContext->ClearRenderTargetView(renderTarget, &color.x);
    g->D3D.DeviceContext->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void d3d_draw(graphics *g, u32 vertices, u32 startVertexLocation) {
    g->D3D.DeviceContext->Draw(vertices, startVertexLocation);
}

void d3d_draw_indexed(graphics *g, u32 indices, u32 startIndex, u32 baseVertexLocation) {
    g->D3D.DeviceContext->DrawIndexed(indices, startIndex, baseVertexLocation);
}

void d3d_swap(graphics *g) {
    g->CurrentTargetWindow->D3D.SwapChain->Present(g->CurrentTargetWindow->Window.get_flags() & window::VSYNC ? 1 : 0, 0);
}

void d3d_release(graphics *g) {
    COM_SAFE_RELEASE(g->D3D.Device);
    COM_SAFE_RELEASE(g->D3D.DeviceContext);

    COM_SAFE_RELEASE(g->D3D.BlendStates[0]);
    COM_SAFE_RELEASE(g->D3D.BlendStates[1]);
    COM_SAFE_RELEASE(g->D3D.DepthStencilStates[0]);
    COM_SAFE_RELEASE(g->D3D.DepthStencilStates[1]);
}

graphics::impl g_D3DImpl = {d3d_init,
                            d3d_init_target_window,
                            d3d_release_target_window,
                            d3d_target_window_resized,
                            d3d_set_viewport,
                            d3d_set_scissor_rect,
                            d3d_set_render_target,
                            d3d_set_blend,
                            d3d_set_depth_testing,
                            d3d_set_cull_mode,
                            d3d_clear_color,
                            d3d_draw,
                            d3d_draw_indexed,
                            d3d_swap,
                            d3d_release};

LSTD_END_NAMESPACE

#endif
