#pragma once

#include <iostream>
#include <vector>

#include "glm_wrapper.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace vload {
	struct vertex {
		// aligning things is a little less space-efficient, but makes offsets easier.
		alignas(4) glm::vec3 pos;
		alignas(4) glm::vec3 normal;
		alignas(4) glm::vec3 tangent;
		alignas(4) glm::vec2 texcoord;
	};

	class mesh {
	public:
		std::vector<vertex> verts;
		std::vector<uint32_t> indices; // elements to dictate what to render

		mesh(std::vector<vertex> inPList, std::vector<uint32_t> inElemList) : verts(inPList), indices(inElemList) { }
		mesh() : verts(), indices() { }
	};

	class vloader {
	public:
		vloader(std::string path);
		std::vector<mesh> meshList;
	private:
		void processNode(aiNode* node, const aiScene* scene);
		mesh processMesh(aiMesh* inMesh, const aiScene* scene);
	};
}
