#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include "Material.h"
#include <string>
#include <vector>
#include <map>
#include <wrl.h>

class GLTFSceneLoader;
class Camera;
class Mesh;
struct MeshConstants;
class SkyBox;

/* A SceneNode is a node in the scene graph */
struct SceneNode 
{
	int meshId;													// Id of the scene mesh associated with this node, -1 for no mesh
	std::vector<std::unique_ptr<SceneNode>> children;			// Children of this node
	DirectX::XMFLOAT4X4 transformMtx = DXUtil::IdentityMtx();	// Node tranformation relative to its parent
};

class Scene
{
public:
	Scene(Microsoft::WRL::ComPtr<ID3D12Device> device);
	~Scene();

	void AddGPUBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> buffer);
	void AddMaterial(unsigned int materialId, RoughMetallicMaterial material);
	void AddTexture(unsigned int textureId, Microsoft::WRL::ComPtr<ID3D12Resource> texture);
	void AddSampler(unsigned int samplerId, D3D12_SAMPLER_DESC samplerDesc);
	void AddLight(unsigned int lightId, Light light);
	void AddMesh(Mesh mesh);
	
	void SetCamera(const Camera& camera);
	void SetMeshConstants(unsigned int meshId, MeshConstants meshConstant);
	void SetLight(unsigned int lightId, Light light);
	void SetCubeMapTexture(Microsoft::WRL::ComPtr<ID3D12Resource> cubeMapTexture);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> CreateRootSignature();
	void Draw(ID3D12GraphicsCommandList* commandList);
	void DrawNode(SceneNode* node, ID3D12GraphicsCommandList* commandList, DirectX::XMFLOAT4X4 parentMtx);
	void DrawMesh(const Mesh& mesh, ID3D12GraphicsCommandList* commandList);

protected:
	void AddFrameConstantsBuffer();
	void SetRootSignature(ID3D12GraphicsCommandList* commandList, unsigned int meshId);
	void UpdateConstants(FrameConstants frameConstants);

	const unsigned int MESH_CONSTANTS_N_DESCRIPTORS = 15;	// Mesh constants descriptors goes from 0 to 15 in the CBV_SRV_UAV descriptor heap (maximum 15 mesh)
	const unsigned int MATERIALS_N_DESCRIPTORS = 15;		// Materials descriptors goes from 15 to 29 in the CBV_SRV_UAV descriptor heap (maximum 15 materials)
	const unsigned int TEXTURES_N_DESCRIPTORS = 15;			// Texture resource view descriptors go from 30 to 44 in the CBV_SRV_UAV descriptor heap (maximum 15 textures)
	const unsigned int SAMPLERS_N_DESCRIPTORS = 15;			// Number of samplers descriptors in the samplers descriptor heap
	

	Microsoft::WRL::ComPtr<ID3D12Device> m_device;	
	UINT m_CBVSRVDescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CBVSRVDescriptorHeap;
	UINT m_samplersDescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplersDescriptorHeap;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_buffersGPU;
	FrameConstants m_frameConstants;
	std::unique_ptr<UploadBuffer<FrameConstants>> m_frameConstantsBuffer;
	std::map<unsigned int, RoughMetallicMaterial> m_materials;
	std::map<unsigned int, std::unique_ptr<UploadBuffer<RoughMetallicMaterial>>> m_materialsBuffer;
	std::map<unsigned int, Microsoft::WRL::ComPtr<ID3D12Resource>> m_textures;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_cubeMapTexture;
	std::map<unsigned int, Mesh> m_meshes;
	std::map<unsigned int, std::unique_ptr<UploadBuffer<MeshConstants>>> m_meshConstantsBuffer;
	std::map<unsigned int, Light> m_lights;
	std::map<unsigned int, std::unique_ptr<UploadBuffer<Light>>> m_lightsConstantsBuffer;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

	std::unique_ptr<SceneNode> m_sceneRoot;
	bool m_isInitialized = false;

private:
	friend class GLTFSceneLoader;
};