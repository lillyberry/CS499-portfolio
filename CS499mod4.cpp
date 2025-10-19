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

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
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

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
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

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
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

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
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

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/

void SceneManager::LoadTextures()
{
	bool bReturn = false;

	// Plank texture (brown wood)
	bReturn = CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "tabletop");

	// Cylinder texture (gold)
	bReturn = CreateGLTexture("../../Utilities/textures/gold-seamless-texture.jpg", "legs");

	bReturn = CreateGLTexture("../../Utilities/textures/stainedglass.jpg","torus");

	bReturn = CreateGLTexture("../../Utilities/textures/abstract.jpg","centerpiece");

	// Bind all loaded textures to texture slots
	BindGLTextures();
}


void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.4f, 0.3f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.2f, 0.1f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 0.3;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL rugMaterial;
	rugMaterial.ambientColor = glm::vec3(0.6f, 0.2f, 0.2f);       
	rugMaterial.ambientStrength = 0.3f;                           
	rugMaterial.diffuseColor = glm::vec3(0.7f, 0.3f, 0.3f);       
	rugMaterial.specularColor = glm::vec3(0.05f, 0.05f, 0.05f);   
	rugMaterial.shininess = 0.1f;                                 
	rugMaterial.tag = "rug";

	m_objectMaterials.push_back(rugMaterial);

	// Floor material
	OBJECT_MATERIAL floorMaterial;
	floorMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	floorMaterial.ambientStrength = 0.3f;
	floorMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	floorMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	floorMaterial.shininess = 0.2f;
	floorMaterial.tag = "floor";

	m_objectMaterials.push_back(floorMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.3f, 0.1f, 0.1f);
	metalMaterial.ambientStrength = 0.4f;
	metalMaterial.diffuseColor = glm::vec3(0.8f, 0.3f, 0.1f);
	metalMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	metalMaterial.shininess = 1.0f;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.3f, 0.4f, 0.6f);
	glassMaterial.ambientStrength = 0.1f;
	glassMaterial.diffuseColor = glm::vec3(0.5f, 0.8f, 1.0f);
	glassMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
	glassMaterial.shininess = 1.5f;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);


	OBJECT_MATERIAL plateMaterial;
	plateMaterial.ambientColor = glm::vec3(0.8f, 0.8f, 0.8f); 
	plateMaterial.ambientStrength = 0.2f;
	plateMaterial.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f); 
	plateMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	plateMaterial.shininess = 0.8f; 
	plateMaterial.tag = "plate";

	m_objectMaterials.push_back(plateMaterial);


}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Light 0 - Overhead Right (White)
	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.4f, 0.4f, 0.4f);  // White Light
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Light 1 - Overhead Left (White)
	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.4f, 0.4f, 0.4f);  // White Light
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	// Light 2 - Front Fill (Blue)
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.3f, 0.3f, 1.0f);  // Blue Light
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.3f, 0.3f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.5f);

	// Light 3 - Front Fill (Red)
	m_pShaderManager->setVec3Value("lightSources[3].position", 0.6f, 5.0f, -6.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.01f, 0.01f, 0.01f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 1.0f, 0.3f, 0.3f);  // Red Light
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 1.0f, 0.3f, 0.3f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.5f);

	// Tell the shader we want to use lighting
	m_pShaderManager->setBoolValue("bUseLighting", true);
}

void SceneManager::PrepareScene()
{
	// Load all meshes and textures into memory
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadPlaneMesh();

	// Load all textures
	LoadTextures();
	DefineObjectMaterials();
	SetupSceneLights();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("floor");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************/
	// Sphere object (e.g., glass ball)
	scaleXYZ = glm::vec3(1.0f);
	positionXYZ = glm::vec3(2.5f, 5.2f, -1.5f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("glass");
	SetShaderColor(0.5f, 0.8f, 1.0f, 0.7f); // transparent blue
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/

	// Draw the tabletop (box mesh with texture)
	scaleXYZ = glm::vec3(10.0f, 0.5f, 6.0f);
	positionXYZ = glm::vec3(0.0f, 4.5f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderMaterial("wood");
	SetShaderTexture("tabletop"); // Use the 'tabletop' texture
	SetTextureUVScale(1.0f, 1.0f); // Texture scaling
	m_basicMeshes->DrawBoxMesh();

	// Draw placemats on the table (4)
	scaleXYZ = glm::vec3(1.0f, 0.05f, 1.0f); 
	for (float x = -3.0f; x <= 3.0f; x += 6.0f) { 
		for (float z = -2.0f; z <= 2.0f; z += 4.0f) { 
			positionXYZ = glm::vec3(x, 4.8f, z); 
			SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ); 
			SetShaderMaterial("plate"); 
			SetShaderTexture("rug");
			SetTextureUVScale(1.0f, 1.0f); 
			m_basicMeshes->DrawPlaneMesh(); 
		}
	}

	// Draw glass cups on the table (4)
	scaleXYZ = glm::vec3(0.3f, 0.5f, 0.3f);  
	for (float x = -3.0f; x <= 3.0f; x += 6.0f) {  
		for (float z = -2.0f; z <= 2.0f; z += 4.0f) {  
			positionXYZ = glm::vec3(x, 5.3f, z); 

			// Apply transformations
			SetTransformations(scaleXYZ, 90.0f, 90.0f, 90.0f, positionXYZ); 

			// Set shader material for glass (make sure "glass" material is defined)
			SetShaderMaterial("glass");

			// Draw the cup using a tapered cylinder mesh
			m_basicMeshes->DrawTaperedCylinderMesh();
		}
	}

	// Draw torus object on the table
	scaleXYZ = glm::vec3(1.0f, 0.2f, 1.0f);  
	positionXYZ = glm::vec3(0.0f, 15.0f, 0.0f); 
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("torus");  
	SetShaderMaterial("rug");
	SetTextureUVScale(1.0f, 1.0f);  
	m_basicMeshes->DrawTorusMesh();


	/****************************************************************/
	// Draw the rug just above the floor
	scaleXYZ = glm::vec3(12.0f, 1.0f, 6.0f);
	positionXYZ = glm::vec3(0.0f, 1.01f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("rug");
	SetShaderMaterial("rug");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawPlaneMesh();

	// --- Centerpiece on the Table ---
	scaleXYZ = glm::vec3(0.25f, 0.25f, 0.25f);  
	positionXYZ = glm::vec3(0.0f, 6.0f, 0.0f);  
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("centerpiece");
	SetShaderMaterial("glass");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh();


	// Draw table legs (4)
	scaleXYZ = glm::vec3(0.5f, 3.0f, 0.5f);
	for (float x = -4.5f; x <= 4.5f; x += 9.0f) {
		for (float z = -2.5f; z <= 2.5f; z += 5.0f) {
			positionXYZ = glm::vec3(x, 1.5f, z);
			SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
			SetShaderTexture("legs"); 
			SetShaderMaterial("wood");
			SetTextureUVScale(1.0f, 1.0f); 
			m_basicMeshes->DrawTaperedCylinderMesh();
		}
	}

}
