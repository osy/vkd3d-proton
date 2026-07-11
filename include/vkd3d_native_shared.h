/*
 * Copyright 2026 the vkd3d-proton contributors
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __VKD3D_NATIVE_SHARED_H
#define __VKD3D_NATIVE_SHARED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Public C contract for vkd3d-proton's native shared resources.
 *
 * On native Linux builds, ID3D12Device::CreateSharedHandle() for a
 * committed texture created with D3D12_HEAP_FLAG_SHARED returns (as the
 * HANDLE) a pointer to a struct DxvkSharedTextureDescriptor.  The layout
 * is byte-compatible with DXVK's include/native/dxvk_shared_resource.h
 * (dmabuf-presenter contract), so consumers validating magic / version /
 * structSize can accept descriptors from either implementation.
 *
 * Ownership rules (matching DXVK's exporter contract):
 *
 * - The returned descriptor and its fd are owned by the exporting
 *   ID3D12Resource and stay valid until that resource is destroyed.
 *   Callers must not free the descriptor or close the fd; dup() the fd
 *   to extend its lifetime beyond the exporter's.
 *
 * The structs below intentionally keep DXVK's names.  If DXVK's
 * dxvk_shared_resource.h is included first, its (identical) definitions
 * are used instead.
 */

#ifndef DXVK_SHARED_DESCRIPTOR_TEXTURE

#define DXVK_SHARED_DESCRIPTOR_TEXTURE 0x74584444u /* 'DDXt' */
#define DXVK_SHARED_DESCRIPTOR_FENCE   0x66584444u /* 'DDXf' */
#define DXVK_SHARED_DESCRIPTOR_VERSION 1u

#define DXVK_SHARED_MAX_PLANES 4u

/*
 * D3D11-level description of a shared texture.  Fields mirror
 * D3D11_TEXTURE2D_DESC plus the texture layout; fixed-width types keep
 * the struct usable from C consumers that do not include D3D headers.
 */
struct DxvkSharedTextureMetadata {
  uint32_t Width;
  uint32_t Height;
  uint32_t MipLevels;
  uint32_t ArraySize;
  uint32_t Format;              /* DXGI_FORMAT */
  struct {
    uint32_t Count;
    uint32_t Quality;
  } SampleDesc;                 /* DXGI_SAMPLE_DESC */
  uint32_t Usage;               /* D3D11_USAGE */
  uint32_t BindFlags;
  uint32_t CPUAccessFlags;
  uint32_t MiscFlags;
  uint32_t TextureLayout;       /* D3D11_TEXTURE_LAYOUT */
};

/*
 * Shared-texture descriptor: D3D11-level metadata plus the dmabuf-level
 * facts an importer needs to reconstruct the image.
 */
struct DxvkSharedTextureDescriptor {
  uint32_t magic;               /* DXVK_SHARED_DESCRIPTOR_TEXTURE */
  uint32_t version;             /* DXVK_SHARED_DESCRIPTOR_VERSION */
  uint32_t structSize;          /* sizeof(struct DxvkSharedTextureDescriptor) */
  struct DxvkSharedTextureMetadata meta;
  /* dmabuf-level description */
  uint64_t drmFormatModifier;
  uint32_t planeCount;
  struct {
    uint64_t offset;
    uint64_t pitch;
  } planes[DXVK_SHARED_MAX_PLANES];
  uint64_t allocationSize;      /* exporter's memory size; import validation */
  int      fd;                  /* dma-buf; see ownership rules above */
};

/*
 * Shared-fence descriptor.  The fd is an opaque fd exported from a
 * timeline semaphore.
 */
struct DxvkSharedFenceDescriptor {
  uint32_t magic;               /* DXVK_SHARED_DESCRIPTOR_FENCE */
  uint32_t version;             /* DXVK_SHARED_DESCRIPTOR_VERSION */
  uint32_t structSize;          /* sizeof(struct DxvkSharedFenceDescriptor) */
  int      fd;                  /* opaque fd, timeline semaphore */
};

#endif /* DXVK_SHARED_DESCRIPTOR_TEXTURE */

/*
 * Native-only vendor D3D12_HEAP_FLAGS bit for CreateCommittedResource.
 * Combined with D3D12_HEAP_FLAG_SHARED it forces the exported image to
 * VK_IMAGE_TILING_LINEAR (drmFormatModifier = DRM_FORMAT_MOD_LINEAR),
 * so the dmabuf can be consumed without modifier negotiation.  The bit
 * is stripped from the resource's public heap flags.
 */
#define VKD3D_HEAP_FLAG_EXPORT_LINEAR_DMABUF 0x40000000u

/*
 * Native-only export: dmabuf-import twin of
 * ID3D12Device13::OpenExistingHeapFromAddress1.  Creates an
 * ALLOW_ONLY_BUFFERS CUSTOM/WRITE_BACK heap whose VkDeviceMemory is a
 * VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT import of dmabuf_fd
 * (e.g. a udmabuf wrapping memfd pages, which RADV's host-pointer
 * userptr path refuses).  `size` must equal the dmabuf's size and be
 * 64 KiB-aligned.  The fd is borrowed: it is dup()ed internally and
 * the caller keeps ownership.
 *
 * Exported (with this exact name) from both the d3d12core module and
 * the d3d12 loader, which forwards via dlsym.  Declared only when the
 * D3D12 types are in scope (include vkd3d_d3d12.h or d3d12.h first).
 */
#ifdef __ID3D12Device_INTERFACE_DEFINED__
HRESULT vkd3d_open_existing_heap_from_dmabuf(ID3D12Device *device,
        int dmabuf_fd, UINT64 size, REFIID iid, void **heap);
typedef HRESULT (*PFN_vkd3d_open_existing_heap_from_dmabuf)(ID3D12Device *device,
        int dmabuf_fd, UINT64 size, REFIID iid, void **heap);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* __VKD3D_NATIVE_SHARED_H */
