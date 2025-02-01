#pragma once

#include <Scene.hpp>
#include <TriangleMeshD3D12.hpp>
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using Microsoft::WRL::ComPtr;

class SceneGraphViewerApp; // Forward declaration

class RayTracingUtils
{
public:

  RayTracingUtils(ComPtr<ID3D12Device5> device);
  ~RayTracingUtils() = default;

  static RayTracingUtils createRayTracingUtils(ComPtr<ID3D12Device5> device, Scene& scene,
                                               ComPtr<ID3D12GraphicsCommandList4> commandList,
                                               ComPtr<ID3D12CommandAllocator>     commandAllocator,
                                               ComPtr<ID3D12CommandQueue> commandQueue, SceneGraphViewerApp& app);

  // Acceleration structure
  ComPtr<ID3D12Resource>              m_topLevelAS;
  std::vector<ComPtr<ID3D12Resource>> m_bottomLevelAS;

  bool isRayTracingSupported(ComPtr<ID3D12Device5> device);
  void createAccelerationStructures(ComPtr<ID3D12Device5> device, Scene& scene,
                                    ComPtr<ID3D12GraphicsCommandList4> commandList,
                                    ComPtr<ID3D12CommandAllocator>     commandAllocator,
                                    ComPtr<ID3D12CommandQueue> commandQueue, SceneGraphViewerApp& app);
};
