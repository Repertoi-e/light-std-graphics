#include "lstd/common.h"

#if OS == WINDOWS

#define LSTD_JUST_DX
#include "lstd/platform/windows.h"
#undef LSTD_JUST_DX

#include <d3d11.h>
#include <d3dcompiler.h>

#include "lstd_graphics/graphics/api.h"
#include "lstd_graphics/graphics/texture.h"
#include "lstd_graphics/memory/pixel_buffer.h"

LSTD_BEGIN_NAMESPACE

void d3d_texture_2D_init(texture_2D *t) {
    D3D11_TEXTURE2D_DESC textureDesc;
    zero_memory(&textureDesc, sizeof(textureDesc));
    {
        textureDesc.Width              = t->Width;
        textureDesc.Height             = t->Height;
        textureDesc.MipLevels          = 1;
        textureDesc.ArraySize          = 1;
        textureDesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Usage              = t->RenderTarget ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
        textureDesc.CPUAccessFlags     = textureDesc.Usage == D3D11_USAGE_DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0;
        textureDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE | (t->RenderTarget ? D3D11_BIND_RENDER_TARGET : 0);
        textureDesc.SampleDesc.Count   = 1;
        textureDesc.SampleDesc.Quality = 0;
    }
    DX_CHECK(t->Graphics->D3D.Device->CreateTexture2D(&textureDesc, null, &t->D3D.Texture));

    D3D11_SHADER_RESOURCE_VIEW_DESC rvDesc;
    zero_memory(&rvDesc, sizeof(rvDesc));
    {
        rvDesc.Format              = textureDesc.Format;
        rvDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
        rvDesc.Texture2D.MipLevels = 1;
    }
    DX_CHECK(t->Graphics->D3D.Device->CreateShaderResourceView(t->D3D.Texture, &rvDesc, &t->D3D.ResourceView));

    if (t->RenderTarget) {
        D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
        zero_memory(&rtvDesc, sizeof(rtvDesc));
        {
            rtvDesc.Format        = textureDesc.Format;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        }
        DX_CHECK(t->Graphics->D3D.Device->CreateRenderTargetView(t->D3D.Texture, &rtvDesc, &t->D3D.RenderTargetView));

        D3D11_TEXTURE2D_DESC dsbDesc;
        zero_memory(&dsbDesc, sizeof(dsbDesc));
        {
            dsbDesc.Width            = t->Width;
            dsbDesc.Height           = t->Height;
            dsbDesc.MipLevels        = 1;
            dsbDesc.ArraySize        = 1;
            dsbDesc.Format           = DXGI_FORMAT_D24_UNORM_S8_UINT;
            dsbDesc.SampleDesc.Count = 1;
            dsbDesc.Usage            = D3D11_USAGE_DEFAULT;
            dsbDesc.BindFlags        = D3D11_BIND_DEPTH_STENCIL;
        }

        DX_CHECK(t->Graphics->D3D.Device->CreateTexture2D(&dsbDesc, null, &t->D3D.DepthStencilBuffer));
        DX_CHECK(
            t->Graphics->D3D.Device->CreateDepthStencilView(t->D3D.DepthStencilBuffer, null, &t->D3D.DepthStencilView));
    }

    D3D11_TEXTURE_ADDRESS_MODE addressMode;
    if (t->Wrap == texture_wrap::Clamp) {
        addressMode = D3D11_TEXTURE_ADDRESS_CLAMP;
    } else if (t->Wrap == texture_wrap::Mirrored_Repeat) {
        addressMode = D3D11_TEXTURE_ADDRESS_MIRROR;
    } else if (t->Wrap == texture_wrap::Repeat) {
        addressMode = D3D11_TEXTURE_ADDRESS_WRAP;
    } else if (t->Wrap == texture_wrap::Clamp_To_Border) {
        addressMode = D3D11_TEXTURE_ADDRESS_BORDER;
    } else {
        assert(false);
        addressMode = (D3D11_TEXTURE_ADDRESS_MODE) -1;
    }

    D3D11_SAMPLER_DESC samplerDesc;
    zero_memory(&samplerDesc, sizeof(samplerDesc));
    {
        samplerDesc.Filter =
            t->Filter == texture_filter::Linear ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_POINT;
        samplerDesc.AddressU       = addressMode;
        samplerDesc.AddressV       = addressMode;
        samplerDesc.AddressW       = addressMode;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
        samplerDesc.MaxAnisotropy  = 1;
        samplerDesc.MinLOD         = 0;
        samplerDesc.MaxLOD         = D3D11_FLOAT32_MAX;
    }
    DX_CHECK(t->Graphics->D3D.Device->CreateSamplerState(&samplerDesc, &t->D3D.SamplerState));
}

void d3d_texture_2D_set_data(texture_2D *t, pixel_buffer data) {
    // We have a very strict image format and don't support anything else at the moment...
    assert(t->Width == data.Width && t->Height == data.Height && data.BPP == 4);

    D3D11_MAPPED_SUBRESOURCE mappedData;
    zero_memory(&mappedData, sizeof(mappedData));

    DX_CHECK(t->Graphics->D3D.DeviceContext->Map(t->D3D.Texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

    s64 sourceRow = t->Width * data.BPP;

    auto *dest = (u8 *) mappedData.pData, *p = data.Pixels;
    For(range(data.Height)) {
        copy_memory(dest, p, sourceRow);
        dest += mappedData.RowPitch;
        p += sourceRow;
    }
    t->Graphics->D3D.DeviceContext->Unmap(t->D3D.Texture, 0);
}

void d3d_texture_2D_bind(texture_2D *t) {
    assert(t->BoundSlot != (u32) -1);

    t->Graphics->D3D.DeviceContext->PSSetShaderResources(t->BoundSlot, 1, &t->D3D.ResourceView);
    t->Graphics->D3D.DeviceContext->PSSetSamplers(t->BoundSlot, 1, &t->D3D.SamplerState);
}

void d3d_texture_2D_unbind(texture_2D *t) {
    assert(t->BoundSlot != (u32) -1);

    ID3D11ShaderResourceView *rv = null;
    t->Graphics->D3D.DeviceContext->PSSetShaderResources(t->BoundSlot, 1, &rv);
}

void d3d_texture_2D_release(texture_2D *t) {
    COM_SAFE_RELEASE(t->D3D.Texture);
    COM_SAFE_RELEASE(t->D3D.ResourceView);
    COM_SAFE_RELEASE(t->D3D.SamplerState);
    COM_SAFE_RELEASE(t->D3D.RenderTargetView);
    COM_SAFE_RELEASE(t->D3D.DepthStencilBuffer);
    COM_SAFE_RELEASE(t->D3D.DepthStencilView);
}

texture_2D::impl g_D3DTexture2DImpl = {d3d_texture_2D_init, d3d_texture_2D_set_data, d3d_texture_2D_bind, d3d_texture_2D_unbind, d3d_texture_2D_release};

LSTD_END_NAMESPACE

#endif
