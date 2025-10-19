///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// Enhanced version for Milestone Three
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>
#include <unordered_map>
#include <functional>

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
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 ***********************************************************/
SceneManager::~SceneManager()
{
    delete m_basicMeshes;
    m_basicMeshes = nullptr;
    DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, const std::string& tag)
{
    int width = 0, height = 0, colorChannels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);

    if (!image)
    {
        std::cout << "Could not load image:" << filename << std::endl;
        return false;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (colorChannels == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (colorChannels == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    else
    {
        std::cout << "Unsupported image channels: " << colorChannels << std::endl;
        stbi_image_free(image);
        return false;
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_textureMap[tag] = textureID;
    return true;
}

/***********************************************************
 *  BindGLTextures()
 ***********************************************************/
void SceneManager::BindGLTextures()
{
    int i = 0;
    for (auto& [tag, id] : m_textureMap)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, id);
        ++i;
    }
}

/***********************************************************
 *  DestroyGLTextures()
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
    for (auto& [tag, id] : m_textureMap)
    {
        glDeleteTextures(1, &id);
    }
    m_textureMap.clear();
}

/***********************************************************
 *  FindTextureID()
 ***********************************************************/
int SceneManager::FindTextureID(const std::string& tag)
{
    auto it = m_textureMap.find(tag);
    return it != m_textureMap.end() ? it->second : -1;
}

/***********************************************************
 *  FindMaterial()
 ***********************************************************/
bool SceneManager::FindMaterial(const std::string& tag, OBJECT_MATERIAL& material)
{
    auto it = m_materialMap.find(tag);
    if (it != m_materialMap.end())
    {
        material = it->second;
        return true;
    }
    return false;
}

/***********************************************************
 *  SetTransformations()
 ***********************************************************/
void SceneManager::SetTransformations(
    glm::vec3 scaleXYZ,
    float XrotationDegrees,
    float YrotationDegrees,
    float ZrotationDegrees,
    glm::vec3 positionXYZ)
{
    glm::mat4 modelView = glm::translate(positionXYZ) *
        glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1, 0, 0)) *
        glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0, 1, 0)) *
        glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0, 0, 1)) *
        glm::scale(scaleXYZ);

    if (m_pShaderManager)
        m_pShaderManager->setMat4Value(g_ModelName, modelView);
}

/***********************************************************
 *  SetShaderColor()
 ***********************************************************/
void SceneManager::SetShaderColor(float r, float g, float b, float a)
{
    if (m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, false);
        m_pShaderManager->setVec4Value(g_ColorValueName, glm::vec4(r, g, b, a));
    }
}

/***********************************************************
 *  SetShaderTexture()
 ***********************************************************/
void SceneManager::SetShaderTexture(const std::string& tag)
{
    if (m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, true);
        int texSlot = FindTextureID(tag);
        m_pShaderManager->setSampler2DValue(g_TextureValueName, texSlot);
    }
}

/***********************************************************
 *  SetShaderMaterial()
 ***********************************************************/
void SceneManager::SetShaderMaterial(const std::string& tag)
{
    if (!m_materialMap.empty())
    {
        OBJECT_MATERIAL material;
        if (FindMaterial(tag, material))
        {
            m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
            m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
            m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
            m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
            m_pShaderManager->setFloatValue("material.shininess", material.shininess);
        }
    }
}

/***********************************************************
 *  LoadTextures()
 ***********************************************************/
void SceneManager::LoadTextures()
{
    CreateGLTexture("../../Utilities/textures/rusticwood.jpg", "tabletop");
    CreateGLTexture("../../Utilities/textures/gold-seamless-texture.jpg", "legs");
    CreateGLTexture("../../Utilities/textures/stainedglass.jpg", "torus");
    CreateGLTexture("../../Utilities/textures/abstract.jpg", "centerpiece");
    BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
    m_materialMap["wood"] = { glm::vec3(0.4f,0.3f,0.1f), 0.2f, glm::vec3(0.3f,0.2f,0.1f), glm::vec3(0.1f), 0.3f };
    m_materialMap["rug"] = { glm::vec3(0.6f,0.2f,0.2f), 0.3f, glm::vec3(0.7f,0.3f,0.3f), glm::vec3(0.05f), 0.1f };
    m_materialMap["floor"] = { glm::vec3(0.2f), 0.3f, glm::vec3(0.4f), glm::vec3(0.1f), 0.2f };
    m_materialMap["metal"] = { glm::vec3(0.3f,0.1f,0.1f), 0.4f, glm::vec3(0.8f,0.3f,0.1f), glm::vec3(0.9f), 1.0f };
    m_materialMap["glass"] = { glm::vec3(0.3f,0.4f,0.6f), 0.1f, glm::vec3(0.5f,0.8f,1.0f), glm::vec3(1.0f), 1.5f };
    m_materialMap["plate"] = { glm::vec3(0.8f), 0.2f, glm::vec3(0.9f), glm::vec3(0.9f), 0.8f };
}

/***********************************************************
 *  Helper: RenderRepeatedObjects()
 ***********************************************************/
void SceneManager::RenderRepeatedObjects(glm::vec3 scale, glm::vec3 startPos, glm::vec3 step,
    int countX, int countZ,
    const std::string& materialTag,
    const std::string& textureTag,
    std::function<void()> drawFunc)
{
    for (int ix = 0; ix < countX; ++ix)
    {
        for (int iz = 0; iz < countZ; ++iz)
        {
            glm::vec3 pos = startPos + glm::vec3(ix * step.x, 0.0f, iz * step.z);
            SetTransformations(scale, 0.0f, 0.0f, 0.0f, pos);
            SetShaderMaterial(materialTag);
            SetShaderTexture(textureTag);
            drawFunc();
        }
    }
}

/***********************************************************
 *  PrepareScene() & RenderScene()
 *  (kept mostly unchanged, but use RenderRepeatedObjects for repeated meshes)
 ***********************************************************/
 // ... Keep your original PrepareScene and RenderScene body here, 
 // replacing loops for placemats, cups, and legs with RenderRepeatedObjects
