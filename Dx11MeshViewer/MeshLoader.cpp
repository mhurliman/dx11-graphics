//
// MeshLoader.cpp
//

#include "pch.h"
#include "MeshLoader.h"

#include <unordered_map>

#pragma warning(push)
#pragma warning(disable : 26495 26451 26498 26812)

// Source - https://github.com/tinyobjloader/tinyobjloader
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#pragma warning(pop)

using namespace DirectX;

// Helpers for determining unique vertices
struct Vertex
{
    Vertex(float* v, float* n)
        : Position{ v[0], v[1], v[2] }
        , Normal{ n[0], n[1], n[2] }
    { }

    float Position[3];
    float Normal[3];
};

template <class T>
inline void hash_combine(size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<>
struct std::hash<Vertex>
{
    size_t operator()(const Vertex& a) const noexcept
    {
        size_t h = 0;
        for (int i = 0; i < 3; ++i)
        {
            hash_combine(h, a.Position[i]);
            hash_combine(h, a.Normal[i]);
        }
        return h;
    }
};

template<>
struct std::equal_to<Vertex>
{
    bool operator()(const Vertex& a, const Vertex& b) const noexcept
    {
        return std::memcmp(&a, &b, sizeof(a)) == 0;
    }
};


HRESULT LoadMesh(const char* filename, Mesh& outMesh)
{
    const char* ext = strstr(filename, ".obj");
    if (!ext)
    {
        return E_FAIL; // Only supports .obj files
    }

    // Leverage TinyObjLoader to load the mesh
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warnings;
    std::string errors;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warnings, &errors, filename))
    {
        OutputDebugStringA(warnings.c_str());
        OutputDebugStringA(errors.c_str());

        return E_FAIL;
    }

    // Normals are expected from the model file in this sample
    if (attrib.normals.size() == 0)
    {
        return E_FAIL;
    }

    // Map used to de-duplicate vertices and generate an index buffer
    std::unordered_map<Vertex, size_t> uniqueVertexMap;
    
    // Process the OBJ mesh into a vertex and index buffer
    for (size_t f = 0; f < shapes[0].mesh.indices.size() / 3; f++)
    {
        tinyobj::index_t idx0 = shapes[0].mesh.indices[3 * f + 0];
        tinyobj::index_t idx1 = shapes[0].mesh.indices[3 * f + 1];
        tinyobj::index_t idx2 = shapes[0].mesh.indices[3 * f + 2];

        float v[3][3] {};
        for (int k = 0; k < 3; k++)
        {
            size_t f0 = idx0.vertex_index;
            size_t f1 = idx1.vertex_index;
            size_t f2 = idx2.vertex_index;

            v[0][k] = attrib.vertices[3 * f0 + k];
            v[1][k] = attrib.vertices[3 * f1 + k];
            v[2][k] = attrib.vertices[3 * f2 + k];
        }

        float n[3][3] {};
        {
            size_t nf0 = idx0.normal_index;
            size_t nf1 = idx1.normal_index;
            size_t nf2 = idx2.normal_index;

            for (int k = 0; k < 3; k++) 
            {
                n[0][k] = attrib.normals[3 * nf0 + k];
                n[1][k] = attrib.normals[3 * nf1 + k];
                n[2][k] = attrib.normals[3 * nf2 + k];
            }
        }

        for (int k = 0; k < 3; k++)
        {
            Vertex m (v[k], n[k]);

            // Check our unique vertex map for identical vertices
            auto it = uniqueVertexMap.find(m);

            if (it == uniqueVertexMap.end())
            {
                // A new vertex! Add it to the vertex buffer
                outMesh.VertexBuffer.push_back(v[k][0]);
                outMesh.VertexBuffer.push_back(v[k][1]);
                outMesh.VertexBuffer.push_back(v[k][2]);

                outMesh.VertexBuffer.push_back(n[k][0]);
                outMesh.VertexBuffer.push_back(n[k][1]);
                outMesh.VertexBuffer.push_back(n[k][2]);

                // Toss it into the hash map
                uniqueVertexMap.insert(std::make_pair(m, uniqueVertexMap.size()));

                it = uniqueVertexMap.find(m);
            }

            // Keep building our index list
            outMesh.IndexBuffer.push_back(static_cast<uint32_t>(it->second));
        }
    }


    // Find spatial bounds of the mesh
    auto boundsMin = DirectX::XMVectorReplicate(FLT_MAX);
    auto boundsMax = DirectX::XMVectorReplicate(-FLT_MAX);

    for (int i = 0; i < outMesh.VertexBuffer.size(); i += 6)
    {
        auto v = reinterpret_cast<XMFLOAT3*>(&outMesh.VertexBuffer[i]);
        auto position = XMLoadFloat3(v);

        boundsMax = XMVectorMax(boundsMax, position);
        boundsMin = XMVectorMin(boundsMin, position);
    }

    DirectX::XMStoreFloat3(&outMesh.BoundsMin, boundsMin);
    DirectX::XMStoreFloat3(&outMesh.BoundsMax, boundsMax);

    return S_OK;
}

