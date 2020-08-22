#pragma once

#include <iostream>
#include <vector>

#include "glm_wrapper.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace vload {
	struct vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texcoord;
	};

	class mesh {
	public:
		std::vector<vertex> verts;
		std::vector<unsigned int> indices; // elements to dictate what to render

		mesh(std::vector<vertex> inPList, std::vector<unsigned int> inElemList) : verts(inPList), indices(inElemList) { }
		mesh() : verts(), indices() { }
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
