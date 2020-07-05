#pragma once

#include <iostream>
#include <vector>

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace vload {
	struct pt {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texcoord;
	};

	class mesh {
	public:
		std::vector<pt> pList; // position coords
		std::vector<unsigned int> elemList; // elements to dictate what to render

		mesh(std::vector<pt> inPList, std::vector<unsigned int> inElemList) : pList(inPList), elemList(inElemList) { }
		mesh() : pList(), elemList() { }
	};

	class vloader {
	public:
		vloader(std::string path);
		std::vector<mesh> meshList;
	private:
		bool processNode(aiNode* node, const aiScene* scene);
		mesh processMesh(aiMesh* inMesh, const aiScene* scene);
	};
}
