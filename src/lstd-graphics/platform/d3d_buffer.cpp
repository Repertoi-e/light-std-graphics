#include "lstd/common.h"

#if OS == WINDOWS

#pragma warning(disable : 4005)
#pragma warning(disable : 5105)
#pragma warning(disable : 5106)

#define LSTD_JUST_DX
#include "lstd/platform/windows.h"
#undef LSTD_JUST_DX

import lstd.context;

#include <d3d11.h>

#include "lstd_graphics/graphics/api.h"
#include "lstd_graphics/graphics/buffer.h"
#include "lstd_graphics/graphics/shader.h"

void d3d_buffer_init(gbuffer *b, const char *data) {
    if (b->Usage == gbuffer_usage::Immutable) {
        assert(data && "Immutable buffers must be created with initial data");
    }

    D3D11_BUFFER_DESC desc;
    memset0(&desc, sizeof(desc));
    {
        assert(b->Size <= numeric<u32>::max());
        desc.ByteWidth = (u32) b->Size;

        if (b->Usage == gbuffer_usage::Immutable) desc.Usage = D3D11_USAGE_IMMUTABLE;
        if (b->Usage == gbuffer_usage::Dynamic) desc.Usage = D3D11_USAGE_DYNAMIC;
        if (b->Usage == gbuffer_usage::Staging) desc.Usage = D3D11_USAGE_STAGING;

        if (b->Type == gbuffer_type::Vertex_Buffer) desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        if (b->Type == gbuffer_type::Index_Buffer) desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        if (b->Type == gbuffer_type::Shader_Uniform_Buffer) desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

        if (b->Usage == gbuffer_usage::Dynamic) desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (b->Usage == gbuffer_usage::Staging) desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    }

    D3D11_SUBRESOURCE_DATA srData;
    srData.pSysMem          = data;
    srData.SysMemPitch      = 0;
    srData.SysMemSlicePitch = 0;

    auto *srDataPtr = &srData;
    if (!data) srDataPtr = null;
    DX_CHECK(b->Graphics->D3D.Device->CreateBuffer(&desc, srDataPtr, &b->D3D.Buffer));
}

DXGI_FORMAT gtype_and_count_to_dxgi_format(gtype type, s64 count, bool normalized) {
    switch (type) {
        case gtype::BOOL:
            assert(count == 1);
            assert(!normalized);
            return DXGI_FORMAT_R1_UNORM;
        case gtype::U8:
            assert(count == 1);
            return normalized ? DXGI_FORMAT_R8_UNORM : DXGI_FORMAT_R8_UINT;
        case gtype::S8:
            assert(count == 1);
            return normalized ? DXGI_FORMAT_R8_SNORM : DXGI_FORMAT_R8_SINT;
        case gtype::U16:
            assert(count == 1);
            return normalized ? DXGI_FORMAT_R8G8_UNORM : DXGI_FORMAT_R8G8_UINT;
        case gtype::S16:
            assert(count == 1);
            return normalized ? DXGI_FORMAT_R8G8_SNORM : DXGI_FORMAT_R8G8_SINT;
        case gtype::U32:
            assert(count <= 4);
            if (count == 1) return normalized ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UINT;
            if (count == 2) return normalized ? DXGI_FORMAT_R16G16B16A16_UNORM : DXGI_FORMAT_R16G16B16A16_UINT;
            if (count == 3) {
                assert(!normalized);
                return DXGI_FORMAT_R32G32B32_UINT;
            }
            if (count == 4) {
                assert(!normalized);
                return DXGI_FORMAT_R32G32B32A32_UINT;
            }
        case gtype::S32:
            assert(count <= 4);
            if (count == 1) return normalized ? DXGI_FORMAT_R8G8B8A8_SNORM : DXGI_FORMAT_R8G8B8A8_SINT;
            if (count == 2) return normalized ? DXGI_FORMAT_R16G16B16A16_SNORM : DXGI_FORMAT_R16G16B16A16_SINT;
            if (count == 3) {
                assert(!normalized);
                return DXGI_FORMAT_R32G32B32_SINT;
            }
            if (count == 4) {
                assert(!normalized);
                return DXGI_FORMAT_R32G32B32A32_SINT;
            }
            return DXGI_FORMAT_UNKNOWN;
        case gtype::F32:
            assert(count <= 4);
            if (count == 1) return DXGI_FORMAT_R32_FLOAT;
            if (count == 2) return DXGI_FORMAT_R32G32_FLOAT;
            if (count == 3) return DXGI_FORMAT_R32G32B32_FLOAT;
            if (count == 4) return DXGI_FORMAT_R32G32B32A32_FLOAT;
            return DXGI_FORMAT_UNKNOWN;
        default:
            assert(false);
            return DXGI_FORMAT_UNKNOWN;
    }
}

void d3d_buffer_set_input_layout(gbuffer *b, gbuffer_layout layout) {
    assert(b->Graphics->CurrentlyBoundShader);
    assert(b->Graphics->CurrentlyBoundShader->D3D.VSBlob);

    COM_SAFE_RELEASE(b->D3D.Layout);

    auto *desc = malloc<D3D11_INPUT_ELEMENT_DESC>({.Count = layout.Count, .Alloc = TemporaryAllocator});
    auto *p    = desc;

    u32 accumSize = 0;

    For(layout) {
        auto bits = it.SizeInBits;
        if (bits == 1) bits = 8;  // bools have 7 bits of padding

        assert(it.Count > 0);

        const char *name = string_to_c_string(it.Name, TemporaryAllocator);

        *p++ = {name,
                0,
                gtype_and_count_to_dxgi_format(it.Type, it.Count, it.Normalized),
                0,
                accumSize,
                D3D11_INPUT_PER_VERTEX_DATA,
                0};

        accumSize += (u32) ((bits / 8) * it.Count);
    }

    b->Stride = accumSize;

    auto *vs = (ID3DBlob *) b->Graphics->CurrentlyBoundShader->D3D.VSBlob;
    DX_CHECK(b->Graphics->D3D.Device->CreateInputLayout(desc, (u32) layout.Count, vs->GetBufferPointer(), vs->GetBufferSize(), &b->D3D.Layout));
}

void *d3d_buffer_map(gbuffer *b, gbuffer_map_access access) {
    D3D11_MAP d3dMap;
    if (access == gbuffer_map_access::Read) {
        d3dMap = D3D11_MAP_READ;
    } else if (access == gbuffer_map_access::Read_Write) {
        d3dMap = D3D11_MAP_READ_WRITE;
    } else if (access == gbuffer_map_access::Write) {
        d3dMap = D3D11_MAP_WRITE;
    } else if (access == gbuffer_map_access::Write_Discard_Previous) {
        d3dMap = D3D11_MAP_WRITE_DISCARD;
    } else if (access == gbuffer_map_access::Write_Unsynchronized) {
        d3dMap = D3D11_MAP_WRITE_NO_OVERWRITE;
    } else {
        assert(false);
        d3dMap = (D3D11_MAP) -1;
    }

    DX_CHECK(b->Graphics->D3D.DeviceContext->Map(b->D3D.Buffer, 0, d3dMap, 0, (D3D11_MAPPED_SUBRESOURCE *) &b->D3D.MappedData));
    return ((D3D11_MAPPED_SUBRESOURCE *) (&b->D3D.MappedData))->pData;
}

void d3d_buffer_unmap(gbuffer *b) { b->Graphics->D3D.DeviceContext->Unmap(b->D3D.Buffer, 0); }

void d3d_buffer_bind(gbuffer *b, primitive_topology topology, u32 offset, u32 stride, shader_type shaderType, u32 position) {
    if (b->Type == gbuffer_type::Vertex_Buffer) {
        if (stride == 0) stride = (u32) b->Stride;

        D3D_PRIMITIVE_TOPOLOGY d3dTopology = (D3D_PRIMITIVE_TOPOLOGY) 0;
        if (topology == primitive_topology::LineList) d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        if (topology == primitive_topology::LineStrip) d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        if (topology == primitive_topology::PointList) d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
        if (topology == primitive_topology::TriangleList) d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        if (topology == primitive_topology::TriangleStrip) d3dTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        assert((s32) d3dTopology);

        b->Graphics->D3D.DeviceContext->IASetPrimitiveTopology(d3dTopology);

        b->Graphics->D3D.DeviceContext->IASetInputLayout(b->D3D.Layout);
        b->Graphics->D3D.DeviceContext->IASetVertexBuffers(0, 1, &b->D3D.Buffer, &stride, &offset);
    } else if (b->Type == gbuffer_type::Index_Buffer) {
        b->Graphics->D3D.DeviceContext->IASetIndexBuffer(b->D3D.Buffer, DXGI_FORMAT_R32_UINT, offset);
    } else if (b->Type == gbuffer_type::Shader_Uniform_Buffer) {
        if (shaderType == shader_type::Vertex_Shader) {
            b->Graphics->D3D.DeviceContext->VSSetConstantBuffers(position, 1, &b->D3D.Buffer);
        } else if (shaderType == shader_type::Fragment_Shader) {
            b->Graphics->D3D.DeviceContext->PSSetConstantBuffers(position, 1, &b->D3D.Buffer);
        }
    } else {
        assert(false);
    }
}

void d3d_buffer_unbind(gbuffer *b) {
    ID3D11Buffer *buffer = null;
    if (b->Type == gbuffer_type::Vertex_Buffer) {
        u32 stride = 0, offset = 0;
        b->Graphics->D3D.DeviceContext->IASetInputLayout(null);
        b->Graphics->D3D.DeviceContext->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
    } else if (b->Type == gbuffer_type::Index_Buffer) {
        b->Graphics->D3D.DeviceContext->IASetIndexBuffer(null, DXGI_FORMAT_R32_UINT, 0);
    } else if (b->Type == gbuffer_type::Shader_Uniform_Buffer) {
    } else {
        assert(false);
    }
}

void d3d_buffer_release(gbuffer *b) {
    COM_SAFE_RELEASE(b->D3D.Buffer);
    COM_SAFE_RELEASE(b->D3D.Layout);
}

gbuffer::impl g_D3DBufferImpl = {d3d_buffer_init, d3d_buffer_set_input_layout, d3d_buffer_map, d3d_buffer_unmap, d3d_buffer_bind, d3d_buffer_unbind, d3d_buffer_release};

#endif
