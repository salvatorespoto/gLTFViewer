#pragma once

#include "DXUtil.h"

class Scene;
class SceneNode;

/** Load a Scene from a glTF file */
class GLTFSceneLoader
{
public:
	GLTFSceneLoader(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);

	/**
	 * Loads the model information from the glTF file filename, to load a scene from the model call GetScene
	 * @param fileName the .gltf or .glb file to load
	 */
	void Load(std::string fileName);
	
	/** 
	  * Load a scene from the glTF file into the Scene object 
	  *	@param sceneId (in) the id of the scene to load from the glTF file
	  * @pparame scene (out) a pointer to a scene object that will points to the loaded scene
	  */
	void GetScene(const int sceneId, std::shared_ptr<Scene>& scene);

protected:
	std::unique_ptr<SceneNode> ParseSceneNode(const int nodeId);
	void ParseSceneGraph(const int sceneId, Scene* scene);
	void LoadMeshes(Scene* scene);
	void LoadLights(Scene* scene);
	void LoadMaterials(Scene* scene);
	void LoadTextures(Scene* scene);
	void LoadSamplers(Scene* scene);
	
	virtual void ComputeTangents(tinygltf::Primitive primitive, Scene* scene);
	virtual void ComputeNormals(tinygltf::Primitive primitive, Scene* scene);

	static constexpr unsigned int MAX_PATH_SIZE = 200;
	TCHAR m_gltfFilePath[MAX_PATH_SIZE];	// The path where gltf file and its resources are stored
	tinygltf::Model m_model;
	tinygltf::TinyGLTF m_glTFLoader;
		
	Microsoft::WRL::ComPtr<ID3D12Device> m_device; 
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
};