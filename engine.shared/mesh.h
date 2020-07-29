#pragma once
// EXTERNAL INCLUDES
#include <vector>
// INTERNAL INCLUDES
#include "vertex.h"

namespace Engine::Resources
{
	class Mesh
	{
	public:
		std::vector<Vertex> vertices = std::vector<Vertex>();
		std::vector<int> indices = std::vector<int>();

		static Mesh GenerateQuad();
		static Mesh GenerateFlatCube();
		static Mesh GenerateSmoothCube();
		//void GenerateRoundedCube(float cornerRadius, int edgeCount);
		static Mesh GenerateSphere(int rings, int segments);
	};
}