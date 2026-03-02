///////////////////////////////////////////////////////////////////////////////
// shadermanager.h
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"

#include <string>
#include <vector>

class SceneManager
{
public:
	SceneManager(ShaderManager *pShaderManager);
	~SceneManager();

	struct TEXTURE_INFO
	{
		std::string tag;
		uint32_t ID;
	};

	struct OBJECT_MATERIAL
	{
		float ambientStrength;
		glm::vec3 ambientColor;
		glm::vec3 diffuseColor;
		glm::vec3 specularColor;
		float shininess;
		std::string tag;
	};

private:
	ShaderManager* m_pShaderManager;
	ShapeMeshes* m_basicMeshes;
	int m_loadedTextures;
	TEXTURE_INFO m_textureIDs[16];
	std::vector<OBJECT_MATERIAL> m_objectMaterials;

	bool CreateGLTexture(const char* filename, std::string tag);
	void BindGLTextures();
	void DestroyGLTextures();
	int FindTextureID(std::string tag);
	int FindTextureSlot(std::string tag);
	bool FindMaterial(std::string tag, OBJECT_MATERIAL& material);

	void SetTransformations(
		glm::vec3 scaleXYZ,
		float XrotationDegrees,
		float YrotationDegrees,
		float ZrotationDegrees,
		glm::vec3 positionXYZ);

	void SetShaderColor(
		float redColorValue,
		float greenColorValue,
		float blueColorValue,
		float alphaValue);

	void SetShaderTexture(std::string textureTag);
	void SetTextureUVScale(float u, float v);
	void SetShaderMaterial(std::string materialTag);
	void DefineObjectMaterials();
	void SetupSceneLights();

public:
	void PrepareScene();
	void RenderScene();
};