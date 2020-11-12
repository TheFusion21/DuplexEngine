#pragma once
// EXTERNAL INCLUDES
#include <vector>
// INTERNAL INCLUDES
#include "vertex.h"

namespace DUPLEX_NS_RESOURCES
{
	class Mesh
	{
	public:
		struct SubMesh
		{
			std::vector<Vertex> vertices = std::vector<Vertex>();
			std::vector<ui32> indices = std::vector<ui32>();
		};
		std::vector<SubMesh> subMeshes;
		static Mesh GenerateQuad();
		static Mesh GenerateFlatCube();
		static Mesh GenerateSmoothCube();
		//void GenerateRoundedCube(float cornerRadius, int edgeCount);
		static Mesh GenerateSphere(int rings, int segments);

		static Mesh LoadFBX(const char* filename);
		static Mesh LoadOBJ(const char* filename);
	};
}