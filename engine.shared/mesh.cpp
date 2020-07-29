
// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "mesh.h"
#include "vertex.h"
#include "math/mathutils.h"

using namespace Engine::Math;
using namespace Engine::Utils;
using namespace Engine::Resources;

Mesh Mesh::GenerateQuad()
{
	Mesh m;
	//Create the verices required to represent a quad
	m.vertices =
	{
		{ Vec3{-0.5f, -0.5f, 0.0f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f, -1.f}, Vec2{0.f,1.f} },
		{ Vec3{0.5f, -0.5f, 0.0f}, FloatColor{0.f, 1.f, 0.f, 1.f}, Vec3{0.f, 0.f, -1.f}, Vec2{1.f,1.f} },
		{ Vec3{-0.5f, 0.5f, 0.0f}, FloatColor{0.f, 0.f, 1.f, 1.f}, Vec3{0.f, 0.f, -1.f}, Vec2{0.f,0.f}},
		{ Vec3{0.5f, 0.5f, 0.0f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f, -1.f}, Vec2{1.f,0.f} }
	};
	//index the verices counterclock wise
	//Counterclock wise because D3D uses it by default
	m.indices =
	{
		0,1,2,
		1,3,2
	};
	return m;
}

Mesh Mesh::GenerateFlatCube()
{
	Mesh m;
	m.vertices =
	{
		//BACK FACE
		{ Vec3{-0.5f,  0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f,-1.f}, Vec2{1.f,1.f} },
		{ Vec3{ 0.5f,  0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f,-1.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f,-1.f}, Vec2{0.f,1.f} },
		{ Vec3{ 0.5f, -0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f,-1.f}, Vec2{1.f,0.f} },

		//BACK FACE
		{ Vec3{-0.5f,  0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f,  0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f, -0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		
		//RIGHT FACE
		{ Vec3{ 0.5f,  0.5f,  -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{1.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f,  0.5f,   0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{1.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f, -0.5f,  -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{1.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f, -0.5f,   0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{1.f, 0.f, 1.f}, Vec2{1.f,0.f} },
		
		//LEFT FACE
		{ Vec3{-0.5f,  0.5f,  -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{-1.f, 0.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f,  0.5f,   0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{-1.f, 0.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f,  -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{-1.f, 0.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f,   0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{-1.f, 0.f, 0.f}, Vec2{1.f,0.f} },
		
		//BOTTOM
		{ Vec3{-0.5f, -0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, -1.f, 0.f}, Vec2{1.f,1.f} },
		{ Vec3{ 0.5f, -0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, -1.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, -1.f, 0.f}, Vec2{0.f,1.f} },
		{ Vec3{ 0.5f, -0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, -1.f, 0.f}, Vec2{1.f,0.f} },
		
		//TOP
		{ Vec3{-0.5f,  0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 1.f, 0.f}, Vec2{1.f,1.f} },
		{ Vec3{ 0.5f,  0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 1.f, 0.f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f,  0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 1.f, 0.f}, Vec2{0.f,1.f} },
		{ Vec3{ 0.5f,  0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{0.f, 1.f, 0.f}, Vec2{1.f,0.f} }
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
	return m;
}

Mesh Mesh::GenerateSmoothCube()
{
	Mesh m;
	m.vertices =
	{
		{ Vec3{-0.5f,  0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{-0.5f,  0.5f, -0.5f}, Vec2{1.f,1.f} },
		{ Vec3{ 0.5f,  0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{ 0.5f,  0.5f, -0.5f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{-0.5f, -0.5f, -0.5f}, Vec2{0.f,1.f} },
		{ Vec3{ 0.5f, -0.5f, -0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{ 0.5f, -0.5f, -0.5f}, Vec2{1.f,0.f} },

		{ Vec3{-0.5f,  0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{-0.5f,  0.5f,  0.5f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f,  0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{ 0.5f,  0.5f,  0.5f}, Vec2{1.f,0.f} },
		{ Vec3{-0.5f, -0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{-0.5f, -0.5f,  0.5f}, Vec2{1.f,0.f} },
		{ Vec3{ 0.5f, -0.5f,  0.5f}, FloatColor{1.f, 0.f, 0.f, 1.f}, Vec3{ 0.5f, -0.5f,  0.5f}, Vec2{1.f,0.f} }
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
	return m;
}
//void Mesh::GenerateRoundedCube(float cornerRadius, int edgeCount)
//{
//	int vertexCount = (4 + edgeCount * 4);
//	int indexCount = (6 + edgeCount * 6);
//
//
//}

Mesh Mesh::GenerateSphere(int rings, int segments)
{
	Mesh m;
	rings = Max(rings, 3);
	segments = Max(segments, 3);

	float sectorStep = 2 * PI / segments;
	float stackStep = PI / rings;
	float sectorAngle, stackAngle;

	int vertexCount = (rings - 1) * segments + 2;
	int indexCount = (vertexCount - 2) * 6;
	//this->indexCount = indexCount;
	int v = 0;
	m.vertices = std::vector<Vertex>(vertexCount);
	m.indices = std::vector<int>(indexCount);
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
			m.vertices[v].color = FloatColor{ 1.0, 0.0, 0.0, 1.0 };
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
	return m;
}
