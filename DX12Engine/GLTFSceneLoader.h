#pragma once

#include "DXUtil.h"

class Scene;

/** Load a Scene from a glTF file */
class GLTFSceneLoader
{

public:
	GLTFSceneLoader(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
	void Load(std::string fileName);
	
	/** 
	  * Load a scene from the glTF file into the Scene object 
	  *	@param sceneId (in) the id of the scene to load from the glTF file
	  * @pparame scene (out) a pointer to a scene object that will points to the loaded scene
	  */
	void GetScene(const int sceneId, std::shared_ptr<Scene>& scene);

protected:
	virtual void ComputeTangentSpace(tinygltf::Primitive primitive, Scene* scene);
	
	tinygltf::Model m_model;
	tinygltf::TinyGLTF m_glTFLoader;
		
	Microsoft::WRL::ComPtr<ID3D12Device> m_device; 
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	ID3D12GraphicsCommandList* m_cmdList;	
};