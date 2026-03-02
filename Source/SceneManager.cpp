///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
}

SceneManager::~SceneManager()
{
	DestroyGLTextures();
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

unsigned char* DownscaleImage(unsigned char* input, int inputWidth, int inputHeight,
	int outputWidth, int outputHeight, int channels)
{
	unsigned char* output = (unsigned char*)malloc(outputWidth * outputHeight * channels);

	if (!output)
	{
		return nullptr;
	}

	float xRatio = (float)(inputWidth - 1) / (float)outputWidth;
	float yRatio = (float)(inputHeight - 1) / (float)outputHeight;

	for (int y = 0; y < outputHeight; y++)
	{
		for (int x = 0; x < outputWidth; x++)
		{
			float srcX = x * xRatio;
			float srcY = y * yRatio;

			int x1 = (int)srcX;
			int y1 = (int)srcY;
			int x2 = x1 + 1;
			int y2 = y1 + 1;

			if (x2 >= inputWidth) x2 = inputWidth - 1;
			if (y2 >= inputHeight) y2 = inputHeight - 1;

			float xWeight = srcX - x1;
			float yWeight = srcY - y1;

			for (int c = 0; c < channels; c++)
			{
				int idx11 = (y1 * inputWidth + x1) * channels + c;
				int idx21 = (y1 * inputWidth + x2) * channels + c;
				int idx12 = (y2 * inputWidth + x1) * channels + c;
				int idx22 = (y2 * inputWidth + x2) * channels + c;

				float p11 = input[idx11];
				float p21 = input[idx21];
				float p12 = input[idx12];
				float p22 = input[idx22];

				float top = p11 * (1.0f - xWeight) + p21 * xWeight;
				float bottom = p12 * (1.0f - xWeight) + p22 * xWeight;

				float value = top * (1.0f - yWeight) + bottom * yWeight;

				int outIdx = (y * outputWidth + x) * channels + c;
				output[outIdx] = (unsigned char)value;
			}
		}
	}

	return output;
}

bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	stbi_set_flip_vertically_on_load(true);

	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	if (image)
	{
		const int MAX_TEXTURE_SIZE = 2048;
		unsigned char* processedImage = image;
		int finalWidth = width;
		int finalHeight = height;
		bool needsDownscale = false;

		if (width > MAX_TEXTURE_SIZE || height > MAX_TEXTURE_SIZE)
		{
			needsDownscale = true;

			float aspectRatio = (float)width / (float)height;

			if (width > height)
			{
				finalWidth = MAX_TEXTURE_SIZE;
				finalHeight = (int)(MAX_TEXTURE_SIZE / aspectRatio);
			}
			else
			{
				finalHeight = MAX_TEXTURE_SIZE;
				finalWidth = (int)(MAX_TEXTURE_SIZE * aspectRatio);
			}

			if (finalWidth < 1) finalWidth = 1;
			if (finalHeight < 1) finalHeight = 1;

			processedImage = DownscaleImage(image, width, height, finalWidth, finalHeight, colorChannels);

			if (!processedImage)
			{
				stbi_image_free(image);
				return false;
			}
		}

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, finalWidth, finalHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, processedImage);
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, finalWidth, finalHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, processedImage);
		else
		{
			if (needsDownscale) free(processedImage);
			stbi_image_free(image);
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		if (needsDownscale) free(processedImage);
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0);

		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	return false;
}

void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glDeleteTextures(1, &m_textureIDs[i].ID);
	}
}

int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;

	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	scale = glm::scale(scaleXYZ);
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

void SceneManager::SetShaderTexture(std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		int textureSlot = FindTextureSlot(textureTag);
		if (textureSlot < 0) return;

		m_pShaderManager->setIntValue(g_UseTextureName, true);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureSlot);
	}
}

void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

void SceneManager::SetShaderMaterial(std::string materialTag)
{
}

void SceneManager::PrepareScene()
{
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadPyramid3Mesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadTorusMesh();

	CreateGLTexture("tarmac.jpg", "tarmac");
	BindGLTextures();
}

void SceneManager::RenderScene()
{
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	scaleXYZ = glm::vec3(30.0f, 1.0f, 100.0f);
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetTextureUVScale(10.0f, 10.0f);
	SetShaderTexture("tarmac");
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(0.5f, 1.0f, 100.0f);
	positionXYZ = glm::vec3(0.0f, 0.02f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(1.0f, 0.9f, 0.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	scaleXYZ = glm::vec3(1.0f, 1.0f, 8.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(5.0f, 0.9f, 0.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.9f, 0.9f, 0.95f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(1.0f, 1.0f, 2.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(5.0f, 0.9f, 5.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.9f, 0.9f, 0.95f, 1.0f);
	m_basicMeshes->DrawConeMesh();

	scaleXYZ = glm::vec3(8.0f, 0.15f, 1.5f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.0f, 0.95f, -1.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.9f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(3.0f, 0.1f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.0f, 1.0f, -6.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.9f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.1f, 1.2f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.0f, 1.6f, -6.2f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.85f, 0.85f, 0.9f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	scaleXYZ = glm::vec3(0.3f, 0.3f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(8.0f, 0.3f, -1.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.3f, 0.3f, 0.35f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.3f, 0.3f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(2.0f, 0.3f, -1.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.3f, 0.3f, 0.35f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();

	scaleXYZ = glm::vec3(0.7f, 0.35f, 1.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(5.0f, 1.3f, 3.5f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.2f, 0.5f, 0.8f, 1.0f);
	m_basicMeshes->DrawBoxMesh();
}