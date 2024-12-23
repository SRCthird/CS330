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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// free up the allocated memory
	m_pShaderManager = NULL;
	if (NULL != m_basicMeshes)
	{
		delete m_basicMeshes;
		m_basicMeshes = NULL;
	}
	// clear the collection of defined materials
	m_objectMaterials.clear();
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

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 *
 *  Converted to RGBA because thats what I'm use to :)
 ***********************************************************/
void SceneManager::SetShaderColor(int r, int g, int b, int a = 255)
{
	float redColorValue = std::max(0, std::min(255, r));
	float greenColorValue = std::max(0, std::min(255, g));
	float blueColorValue = std::max(0, std::min(255, b));
	float alphaValue = std::max(0, std::min(255, a));

	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue / 255.0f;
	currentColor.g = greenColorValue / 255.0f;
	currentColor.b = blueColorValue / 255.0f;
	currentColor.a = alphaValue / 255.0f;

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

/***********************************************************
*  DefineObjectMaterials()
*
*  This method is used for configuring the various material
*  settings for all of the objects within the 3D scene.
***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

  OBJECT_MATERIAL porcelain;

  porcelain.tag = "porcelain";
  porcelain.ambientColor = glm::vec3(0.25f, 0.25f, 0.3f);
  porcelain.ambientStrength = 0.15f;
  porcelain.diffuseColor = glm::vec3(0.9f, 0.9f, 0.9f);
  porcelain.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
  porcelain.shininess = 50.0f;
  m_objectMaterials.push_back(porcelain);

  OBJECT_MATERIAL gold;
	gold.tag = "gold";
	gold.ambientColor = glm::vec3(0.247f, 0.199f, 0.074f);
	gold.ambientStrength = 0.3f;
	gold.diffuseColor = glm::vec3(0.751f, 0.606f, 0.226f);
	gold.specularColor = glm::vec3(0.628f, 0.556f, 0.366f);
	gold.shininess = 51.2f;
	m_objectMaterials.push_back(gold);

	OBJECT_MATERIAL silver;
	silver.tag = "silver";
	silver.ambientColor = glm::vec3(0.192f, 0.192f, 0.192f);
	silver.ambientStrength = 0.25f;
	silver.diffuseColor = glm::vec3(0.507f, 0.507f, 0.507f);
	silver.specularColor = glm::vec3(0.508f, 0.508f, 0.508f);
	silver.shininess = 51.2f;
	m_objectMaterials.push_back(silver);

	OBJECT_MATERIAL bronze;
	bronze.tag = "bronze";
	bronze.ambientColor = glm::vec3(0.2125f, 0.1275f, 0.054f);
	bronze.ambientStrength = 0.25f;
	bronze.diffuseColor = glm::vec3(0.714f, 0.4284f, 0.18144f);
	bronze.specularColor = glm::vec3(0.393f, 0.271f, 0.166f);
	bronze.shininess = 25.6f;
	m_objectMaterials.push_back(bronze);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

  m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
  m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.85f, 0.75f, 0.65f);
  m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.95f, 0.85f, 0.75f);
  m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.95f, 0.85f, 0.75f);
  m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
  m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

  m_pShaderManager->setVec3Value("lightSources[1].position", -5.0f, 10.0f, 5.0f);
  m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.0f, 0.0f, 0.0f);
  m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.75f, 0.75f, 0.85f);
  m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f);
  m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 1.0f);
  m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f);


  // Enable lighting in the shader
  m_pShaderManager->setBoolValue("bUseLighting", true);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
  
	// Load textures
	CreateGLTexture("Textures/floor.jpg", "floor");
	CreateGLTexture("Textures/coffee_body.jpg", "coffeeBody");
	CreateGLTexture("Textures/coffee_liquid.jpg", "coffeeLiquid");
	CreateGLTexture("Textures/laptop.jpg", "laptop");
	CreateGLTexture("Textures/mouse.jpg", "mouse");

	// Bind the textures to texture slots
	BindGLTextures();

  // add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

  
  m_basicMeshes->LoadBoxMesh();
  m_basicMeshes->LoadConeMesh();
  m_basicMeshes->LoadCylinderMesh();
  m_basicMeshes->LoadPlaneMesh();
  m_basicMeshes->LoadPrismMesh();
  m_basicMeshes->LoadSphereMesh();
  m_basicMeshes->LoadTaperedCylinderMesh();
  m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	//================================================================//
	//= Floor Plane                       						               =//
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		glm::vec3(20.0f, 1.0f, 10.0f), // XYZ Scale
		0.0f, 0.0f, 0.0f,              // XYZ Rotation
		glm::vec3(0.0f, 0.0f, 0.0f)    // XYZ Position
  );

	// set the color values into the shader
	SetShaderColor(255, 255, 255);

  // set the texture into the shader
  SetShaderTexture("floor");

  // set the material into the shader
  SetShaderMaterial("porcelain");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	//================================================================//

	//================================================================//
	//= Coffee Cup Body                   						               =//
  glEnable(GL_CULL_FACE); // Enable face culling
  glCullFace(GL_FRONT);   // Cull the front face to remove the top surface
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		glm::vec3(1.0f, 4.0f, 1.0f),   // XYZ Scale
		0.0f, 0.0f, 0.0f,              // XYZ Rotation
		glm::vec3(0.0f, 0.0f, 0.0f)    // XYZ Position
  );

	// set the color values into the shader
	SetShaderColor(221, 204, 176);

  // set the texture into the shader
	SetShaderTexture("coffeeBody");

  // set the material into the shader
  SetShaderMaterial("porcelain");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
  glDisable(GL_CULL_FACE); // Redisbale face culling
	//=================================================================//
  
	//================================================================//
	//= Coffee                             						               =//
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		glm::vec3(0.9f, 3.5f, 0.9f),   // XYZ Scale
		0.0f, 0.0f, 0.0f,              // XYZ Rotation
		glm::vec3(0.0f, 0.0f, 0.0f)    // XYZ Position
  );

	// set the color values into the shader
	SetShaderColor(108, 88, 76);

  // set the texture into the shader
	SetShaderTexture("coffeeLiquid");

  // set the material into the shader
  SetShaderMaterial("porcelain");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
	//=================================================================//

	//================================================================//
	//= Coffee Cup Handle                 						               =//
	SetTransformations(
		glm::vec3(0.6f, 1.0f, 0.4f),     // XYZ Scale
		0.0f, glm::radians(90.0f), 0.0f, // XYZ Rotation 
                                     //   (with a 90° radian for the Y)
		glm::vec3(0.8f, 2.0f, 0.0f)      // XYZ Position
  );

	// set the color values into the shader
	SetShaderColor(221, 204, 176);

  // set the material into the shader
  SetShaderMaterial("porcelain");

	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();
	//=================================================================//

	//================================================================//
	//= Laptop                            						               =//
	SetTransformations(
		glm::vec3(12.0f, 0.75f, 6.0f), // XYZ Scale
		0.0f, 35.0f, 0.0f,             // XYZ Rotation
		glm::vec3(-8.0f, 0.0f, 0.0f)   // XYZ Position
  );

	// set the color values into the shader
	SetShaderColor(206, 212, 218);

  // set the material into the shader
  SetShaderMaterial("silver");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	//=================================================================//
  
	//================================================================//
	//= Laptop Top                         						               =//
	SetTransformations(
		glm::vec3(6.0f, 0.75f, 3.0f),  // XYZ Scale
		0.0f, 35.0f, 0.0f,             // XYZ Rotation
		glm::vec3(-8.0f, 0.40f, 0.0f)  // XYZ Position
  );

	// set the color values into the shader
	SetShaderColor(206, 212, 218);

  // set the texture into the shader
  //   ( I've since stickered my laptop since taking that first picture )
	SetShaderTexture("laptop");

  // set the material into the shader
  SetShaderMaterial("silver");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	//=================================================================//

	//================================================================//
	//= Mouse                             						               =//
	SetTransformations(
		glm::vec3(1.0f, 0.75f, 2.0f),  // XYZ Scale
		0.0f, 35.0f, 0.0f,             // XYZ Rotation
		glm::vec3(4.0f, 0.0f, 0.0f)    // XYZ Position
  );

	// set the color values into the shader
	SetShaderColor(100, 100, 100);

  // set the texture into the shader
	SetShaderTexture("mouse");

  // set the material into the shader
  SetShaderMaterial("gold");

	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();
	//=================================================================//

	//================================================================//
	//= Remote                            						               =//
	SetTransformations(
		glm::vec3(1.0f, 0.45f, 4.0f),   // XYZ Scale
		0.0f, 35.0f, 0.0f,              // XYZ Rotation
		glm::vec3(-4.0f, 0.75f, -3.0f)  // XYZ Position
  );

	// set the color values into the shader
	SetShaderColor(0, 0, 0);

  // set the material into the shader
  SetShaderMaterial("bronze");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();
	//=================================================================//
}
