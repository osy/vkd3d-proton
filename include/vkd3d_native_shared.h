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

#ifndef _WIN32
/* The shared-resource descriptors (dxn_shared_texture) and the embedder API
 * are directx-native's: the contract with the other native D3D backends and
 * the Neptune render server. */
#include <directx-native/dxn_shared.h>
#include <directx-native/dxn_embedder.h>

/*
 * Native-only vendor D3D12_HEAP_FLAGS bit for CreateCommittedResource.
 * Combined with D3D12_HEAP_FLAG_SHARED it forces the exported image to
 * VK_IMAGE_TILING_LINEAR (drm_modifier = DRM_FORMAT_MOD_LINEAR),
 * so the dmabuf can be consumed without modifier negotiation.  The bit
 * is stripped from the resource's public heap flags.
 */
#define VKD3D_HEAP_FLAG_EXPORT_LINEAR_DMABUF DXN_MISC_LINEAR_EXPORT
#endif /* !_WIN32 */

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
