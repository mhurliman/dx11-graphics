//--------------------------------------------------------------------------------------
// MeshLoader.h
//
// EA Runtime Technology Group (RTG)
// Copyright (C) Electronic Arts. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <DirectXMath.h>
#include <vector>

struct Mesh
{
    std::vector<float>    VertexBuffer;
    std::vector<uint32_t> IndexBuffer;

    DirectX::XMFLOAT3     BoundsMin {};
    DirectX::XMFLOAT3     BoundsMax {};
};

HRESULT LoadMesh(const char* filename, Mesh& outMesh); // Currently only supports .obj format
