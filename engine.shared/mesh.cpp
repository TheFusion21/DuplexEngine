
// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "mesh.h"
#include "vertex.h"
#include "math/mathutils.h"
#include <fstream>
#include <sstream>
#include "utils/exceptions.h"
#include <map>
#include <string>
#include "objloader.h"
using namespace std;
using namespace Engine::Math;
using namespace Engine::Utils;
using namespace Engine::Resources;

Mesh Mesh::GenerateQuad()
{
	SubMesh m;
	//Create the verices required to represent a quad
	m.vertices =
	{
		{ Vec3{-0.5f, -0.5f, 0.0f}, Vec3{0.f, 0.f, -1.f}, Vec2{0.f,1.f} },
		{ Vec3{0.5f, -0.5f, 0.0f}, Vec3{0.f, 0.f, -1.f}, Vec2{1.f,1.f} },
		{ Vec3{-0.5f, 0.5f, 0.0f}, Vec3{0.f, 0.f, -1.f}, Vec2{0.f,0.f}},
		{ Vec3{0.5f, 0.5f, 0.0f}, Vec3{0.f, 0.f, -1.f}, Vec2{1.f,0.f} }
	};
	//index the verices counterclock wise
	//Counterclock wise because D3D uses it by default
	m.indices =
	{
		0,1,2,
		1,3,2
	};
	Mesh mesh;
	mesh.subMeshes.push_back(m);
	return mesh;
}

Mesh Mesh::GenerateFlatCube()
{
	SubMesh m;
	m.vertices =
	{
		//BACK FACE
		{ Vec3{-0.5f,  0.5f, -0.5f}, Vec3{0.f, 0.f,-1.f}, Vec2{1.f,1.f} },
		{ Vec3{ 0.5f,  0.5f, -0.5f}, Vec3{0.f, 0.f,-1.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f, -0.5f}, Vec3{0.f, 0.f,-1.f}, Vec2{0.f,1.f} },
		{ Vec3{ 0.5f, -0.5f, -0.5f}, Vec3{0.f, 0.f,-1.f}, Vec2{1.f,0.f} },

		//BACK FACE
		{ Vec3{-0.5f,  0.5f,  0.5f}, Vec3{0.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f,  0.5f,  0.5f}, Vec3{0.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f,  0.5f}, Vec3{0.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f, -0.5f,  0.5f}, Vec3{0.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		
		//RIGHT FACE
		{ Vec3{ 0.5f,  0.5f,  -0.5f}, Vec3{1.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f,  0.5f,   0.5f}, Vec3{1.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f, -0.5f,  -0.5f}, Vec3{1.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f, -0.5f,   0.5f}, Vec3{1.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		
		//LEFT FACE
		{ Vec3{-0.5f,  0.5f,  -0.5f}, Vec3{-1.f, 0.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f,  0.5f,   0.5f}, Vec3{-1.f, 0.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f,  -0.5f}, Vec3{-1.f, 0.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f,   0.5f}, Vec3{-1.f, 0.f, 0.f}, Vec2{1.f,0.f} },
		
		//BOTTOM
		{ Vec3{-0.5f, -0.5f,  0.5f}, Vec3{0.f, -1.f, 0.f}, Vec2{1.f,1.f} },
		{ Vec3{ 0.5f, -0.5f,  0.5f}, Vec3{0.f, -1.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f, -0.5f}, Vec3{0.f, -1.f, 0.f}, Vec2{0.f,1.f} },
		{ Vec3{ 0.5f, -0.5f, -0.5f}, Vec3{0.f, -1.f, 0.f}, Vec2{1.f,0.f} },
		
		//TOP
		{ Vec3{-0.5f,  0.5f,  0.5f}, Vec3{0.f, 1.f, 0.f}, Vec2{1.f,1.f} },
		{ Vec3{ 0.5f,  0.5f,  0.5f}, Vec3{0.f, 1.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f,  0.5f, -0.5f}, Vec3{0.f, 1.f, 0.f}, Vec2{0.f,1.f} },
		{ Vec3{ 0.5f,  0.5f, -0.5f}, Vec3{0.f, 1.f, 0.f}, Vec2{1.f,0.f} }
	};
	//index the verices counterclock wise
	//Counterclock wise because D3D uses it by default
	m.indices =
	{
		0,1,2,2,1,3,
		5,4,7,4,6,7,
		8,9,10,9,11,10,
		13,12,14,13,14,15,
		16,18,17,18,19,17,
		20,21,22,21,23,22
	};

	for (int i = 0; i < m.vertices.size(); i += 3)
	{
		auto& v0 = m.vertices[i];
		auto& v1 = m.vertices[i+1];
		auto& v2 = m.vertices[i+2];

		Vec3 deltaPos1 = v1.position - v0.position;
		Vec3 deltaPos2 = v2.position - v0.position;

		Vec2 detalUV1 = v1.texCoords - v0.texCoords;
		Vec2 detalUV2 = v2.texCoords - v0.texCoords;

		float r = 1.0f / (detalUV1.x * detalUV2.y - detalUV1.y * detalUV2.x);
		v0.tangent = v1.tangent = v2.tangent = (deltaPos1 * detalUV2.y - deltaPos2 * detalUV1.y) * r;
		v0.bitTangent = v1.bitTangent = v2.bitTangent = (deltaPos2 * detalUV1.x - deltaPos1 * detalUV2.x) * r;


	}
	Mesh mesh;
	mesh.subMeshes.push_back(m);
	return mesh;
}

Mesh Mesh::GenerateSmoothCube()
{
	SubMesh m;
	m.vertices =
	{
		{ Vec3{-0.5f,  0.5f, -0.5f}, Vec3{-0.5f,  0.5f, -0.5f}, Vec2{1.f,1.f} },
		{ Vec3{ 0.5f,  0.5f, -0.5f}, Vec3{ 0.5f,  0.5f, -0.5f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f, -0.5f}, Vec3{-0.5f, -0.5f, -0.5f}, Vec2{0.f,1.f} },
		{ Vec3{ 0.5f, -0.5f, -0.5f}, Vec3{ 0.5f, -0.5f, -0.5f}, Vec2{1.f,0.f} },

		{ Vec3{-0.5f,  0.5f,  0.5f}, Vec3{-0.5f,  0.5f,  0.5f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f,  0.5f,  0.5f}, Vec3{ 0.5f,  0.5f,  0.5f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f,  0.5f}, Vec3{-0.5f, -0.5f,  0.5f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f, -0.5f,  0.5f}, Vec3{ 0.5f, -0.5f,  0.5f}, Vec2{1.f,0.f} }
	};

	//index the verices counterclock wise
	//Counterclock wise because D3D uses it by default
	m.indices =
	{
		0,1,2,2,1,3,
		4,0,6,6,0,2,
		1,5,3,3,5,7,
		5,4,7,4,6,7,
		5,0,4,1,0,5,
		2,3,7,6,2,7
	};

	for (int i = 0; i < m.vertices.size(); i += 3)
	{
		auto& v0 = m.vertices[i];
		auto& v1 = m.vertices[i + 1];
		auto& v2 = m.vertices[i + 2];

		Vec3 deltaPos1 = v1.position - v0.position;
		Vec3 deltaPos2 = v2.position - v0.position;

		Vec2 detalUV1 = v1.texCoords - v0.texCoords;
		Vec2 detalUV2 = v2.texCoords - v0.texCoords;

		float r = 1.0f / (detalUV1.x * detalUV2.y - detalUV1.y * detalUV2.x);
		v0.tangent = v1.tangent = v2.tangent = (deltaPos1 * detalUV2.y - deltaPos2 * detalUV1.y) * r;
		v0.bitTangent = v1.bitTangent = v2.bitTangent = (deltaPos2 * detalUV1.x - deltaPos1 * detalUV2.x) * r;


	}
	Mesh mesh;
	mesh.subMeshes.push_back(m);
	return mesh;
}

Mesh Mesh::GenerateSphere(int rings, int segments)
{
	SubMesh m;
	rings = Max(rings, 3);
	segments = Max(segments, 3);

	float sectorStep = 2 * PI / segments;
	float stackStep = PI / rings;
	float sectorAngle, stackAngle;

	int vertexCount = (rings - 1) * segments + 2;
	int indexCount = (vertexCount - 2) * 6;
	//this->indexCount = indexCount;
	int v = 0;
	m.vertices = vector<Vertex>(vertexCount);
	m.indices = vector<ui32>(indexCount);
	for (int i = 0; i <= rings; ++i)
	{
		stackAngle = PI_2 - i * stackStep;
		float xy = 1.0f * cosf(stackAngle);
		float z = 1.0f * sinf(stackAngle);

		for (int j = 0; j < segments; ++j)
		{
			sectorAngle = j * sectorStep;

			float x = xy * cosf(sectorAngle);
			float y = xy * sinf(sectorAngle);

			m.vertices[v].position = Vec3(x, y, z) * static_cast<real>(0.5);

			float nx = x;
			float ny = y;
			float nz = z;

			m.vertices[v].normal = Vec3(nx, ny, nz);
			v++;
			if (i == 0 || i == rings)
				break;
		}
	}
	int f = 0;
	ui32 k1, k2;
	for (int i = 0; i < rings; ++i)
	{
		k1 = (i - 1) * segments + 1;
		k2 = k1 + segments;

		for (int j = 0; j <= segments; ++j, ++k1, ++k2)
		{
			if (i == 0)
			{
				if (j != 0)
				{
					m.indices[f++] = 0;
					m.indices[f++] = j;
					m.indices[f++] = (j % segments) + 1;
				}
			}
			else if (i == (rings - 1))
			{
				if (j == segments)break;
				int offset = vertexCount - segments;
				m.indices[f++] = vertexCount - 1;
				m.indices[f++] = offset + ((j + 1) % segments) - 1;
				m.indices[f++] = offset + j - 1;
			}
			else
			{
				if (j != 0)
				{
					m.indices[f++] = k1 - 1;
					m.indices[f++] = k2 - 1;
					m.indices[f++] = k1;
				}
				if (j < segments)
				{
					m.indices[f++] = k1;
					m.indices[f++] = k2 - 1;
					m.indices[f++] = k2;
				}
			}
		}
	}
	Mesh mesh;
	mesh.subMeshes.push_back(m);
	return mesh;
}

Mesh Engine::Resources::Mesh::LoadFBX(const char* filename)
{
	ifstream file;

	int bufferSize = 1 << 16;
	char* buffer = new char[bufferSize];
	file.rdbuf()->pubsetbuf(buffer, bufferSize);

	file.open(filename, ios::in | ios::binary);
	if(!file.is_open())
		throw new FileLoadException(filename);
	file.close();
	return Mesh();
}

Mesh Engine::Resources::Mesh::LoadOBJ(const char* filename)
{
	Mesh m;

	objl::Loader loader;
	if (!loader.LoadFile(filename))
	{
		throw new FileLoadException(filename);
	}
	for (objl::Mesh lMesh : loader.LoadedMeshes)
	{
		SubMesh sm = {};
		sm.vertices = lMesh.Vertices;
		sm.indices = lMesh.Indices;
		for (int i = 0; i < sm.indices.size(); i += 3)
		{
			Vertex& v0 = sm.vertices[sm.indices[i]];
			Vertex& v1 = sm.vertices[sm.indices[i + 1]];
			Vertex& v2 = sm.vertices[sm.indices[i + 2]];

			Vec3 deltaPos1 = v1.position - v0.position;
			Vec3 deltaPos2 = v2.position - v0.position;

			Vec2 detalUV1 = v1.texCoords - v0.texCoords;
			Vec2 detalUV2 = v2.texCoords - v0.texCoords;

			float r = 1.0f / (detalUV1.x * detalUV2.y - detalUV1.y * detalUV2.x);
			Vec3 tangent = (deltaPos1 * detalUV2.y - deltaPos2 * detalUV1.y) * r;

			Vec3 bitangent = (deltaPos2 * detalUV1.x - deltaPos1 * detalUV2.x) * r;

			v0.tangent += tangent;
			v1.tangent += tangent;
			v2.tangent += tangent;

			v0.bitTangent += bitangent;
			v1.bitTangent += bitangent;
			v2.bitTangent += bitangent;
		}
		for (int i = 0; i < sm.vertices.size(); i ++)
		{
			sm.vertices[i].tangent.Normalize();
			sm.vertices[i].bitTangent.Normalize();
		}
		m.subMeshes.push_back(sm);
	}

	return m;
}
