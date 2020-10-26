#include <iostream>
#include <stdexcept> // for std::runtime_error

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "vloader.h"
using std::cout;
using std::endl;

namespace vload {

	vloader::vloader(std::string_view path) : meshList() {
		Assimp::Importer imp;
		// make everything triangles, generate normals/tangents if they aren't there, and join identical vertices together.
		// NOTE: if JoinIdenticalVertices isn't specified, an index buffer isn't required.
		const aiScene* scene = imp.ReadFile(path.data(), aiProcess_Triangulate 
												| aiProcess_CalcTangentSpace
												| aiProcess_GenNormals
												| aiProcess_JoinIdenticalVertices
												| aiProcess_OptimizeMeshes
												| aiProcess_OptimizeGraph
												);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::string err = imp.GetErrorString();
			throw std::runtime_error(err);
		}

		processNode(scene->mRootNode, scene);
	}

	// recursively visit nodes
	void vloader::processNode(aiNode* node, const aiScene* scene) {
		for (size_t i = 0; i < node->mNumMeshes; i++) {
			aiMesh* temp = scene->mMeshes[node->mMeshes[i]];
			meshList.push_back(processMesh(temp));
		}

		for (size_t i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene); // process children
		}
	}

	mesh vloader::processMesh(aiMesh* inMesh) {
		std::vector<vertex> vList;
		std::vector<uint32_t> indices;

		vList.resize(inMesh->mNumVertices);
		
		for (size_t i = 0; i < inMesh->mNumVertices; i++) { // extract position information
			auto& v = vList[i];
			v.pos.x = inMesh->mVertices[i].x;
			v.pos.y = inMesh->mVertices[i].y;
			v.pos.z = inMesh->mVertices[i].z;

			v.normal.x = inMesh->mNormals[i].x;
			v.normal.y = inMesh->mNormals[i].y;
			v.normal.z = inMesh->mNormals[i].z;
		}

		// can't call resize here - might want to import point clouds with 1 vertex per face
		for (size_t i = 0; i < inMesh->mNumFaces; i++) { // extract element information
			aiFace f = inMesh->mFaces[i];
			for (size_t j = 0; j < f.mNumIndices; j++) {
				indices.push_back(f.mIndices[j]);
			}
		}

		if (inMesh->mTextureCoords[0]) {
			for (size_t i = 0; i < inMesh->mNumVertices; i++) {
				glm::vec2 coord;
				coord.s = inMesh->mTextureCoords[0][i].x;
				coord.t = inMesh->mTextureCoords[0][i].y;
				vList[i].uv = coord;
			}
		} else {
			cout << "mesh has no texture coordinates" << endl;
		}
		
		if (inMesh->HasTangentsAndBitangents()) {
			if (inMesh->mTextureCoords[0]) {
				for (size_t i = 0; i < inMesh->mNumVertices; i++) {
					glm::vec3 tangent;	
					tangent.x = inMesh->mTangents[i].x;
					tangent.y = inMesh->mTangents[i].y;
					tangent.z = inMesh->mTangents[i].z;
					vList[i].tangent = tangent;
				}
			} else {
				cout << "mesh has no texture coordinates, cannot calculate tangents" << endl;
			}
		} else {
			cout << "mesh has no tangents" << endl;
		}
		
		return mesh(vList, indices);
	}
}
