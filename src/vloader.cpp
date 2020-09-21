#include <iostream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "vloader.h"
using namespace std;

namespace vload {

	vloader::vloader(std::string path) : meshList() {
		Assimp::Importer imp;
		// make everything triangles, generate normals if they aren't there, calcuate tangents, and join identical vertices together.
		const aiScene* scene = imp.ReadFile(path, aiProcess_Triangulate 
												| aiProcess_CalcTangentSpace
												| aiProcess_GenNormals
												| aiProcess_JoinIdenticalVertices
												);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) { // check if import failed and exit
			cerr << "Assimp scene import failed: " << imp.GetErrorString() << endl;
			throw "file error";
		}

		processNode(scene->mRootNode, scene);
	}

	// recursively visit nodes
	void vloader::processNode(aiNode* node, const aiScene* scene) {
		for (size_t i = 0; i < node->mNumMeshes; i++) {
			aiMesh* temp = scene->mMeshes[node->mMeshes[i]];
			meshList.push_back(processMesh(temp, scene));
		}

		for (size_t i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene); // process children
		}
	}

	mesh vloader::processMesh(aiMesh* inMesh, const aiScene* scene) {
		vector<vertex> vList;
		vector<uint32_t> indices;
		
		for (size_t i = 0; i < inMesh->mNumVertices; i++) { // extract position information
			glm::vec3 position;
			glm::vec3 normal;

			position.x = inMesh->mVertices[i].x;
			position.y = inMesh->mVertices[i].y;
			position.z = inMesh->mVertices[i].z;

			normal.x = inMesh->mNormals[i].x;
			normal.y = inMesh->mNormals[i].y;
			normal.z = inMesh->mNormals[i].z;

			vertex temppoint = { position, normal };
			vList.push_back(temppoint);
		}

		for (size_t i = 0; i < inMesh->mNumFaces; i++) { // extract element information
			aiFace f = inMesh->mFaces[i];
			for (size_t j = 0; j < f.mNumIndices; j++) {
				indices.push_back(f.mIndices[j]);
			}
		}

		if (!inMesh->mTextureCoords[0]) { // extract texture information, if available
			cout << "Mesh has no texture coordinates." << endl;
		}
		else {
			for (size_t i = 0; i < inMesh->mNumVertices; i++) {
				glm::vec2 coord;
				coord.s = inMesh->mTextureCoords[0][i].x;
				coord.t = inMesh->mTextureCoords[0][i].y;
				vList[i].texcoord = coord;
			}
		}
		
		// note that calculating tangent vectors requires knowing both position and uv coords.
		if (!inMesh->HasTangentsAndBitangents()) { // extract tangent vector information, if possible
			cout << "Mesh has no tangents." << endl;
		} else {
			for (size_t i = 0; i < inMesh->mNumVertices; i++) {
				glm::vec3 tan;	
				tan.x = inMesh->mTangents[i].x;
				tan.y = inMesh->mTangents[i].y;
				tan.z = inMesh->mTangents[i].z;
				vList[i].tangent = tan;
			}
		}
		
		return mesh(vList, indices);
	}
}
