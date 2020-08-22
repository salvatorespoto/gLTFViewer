#include "GLTFScene.h"
#include "Texture.h"
#include "Material.h"
#include "AssetsManager.h"
#include <map>

using Microsoft::WRL::ComPtr;

void GLTFScene::LoadScene(std::shared_ptr<Renderer> renderer, std::shared_ptr<AssetsManager> assetsManager, const tinygltf::Model& model, unsigned int sceneId)
{
	m_renderer = renderer;
	m_model = model;
	if (sceneId >= m_model.scenes.size()) DXUtil::ThrowException("Scene index out of range");

	GPUHeapUploader gpuHeapUploader(m_renderer->GetDevice().Get(), m_renderer->GetCommandQueue().Get());
	
	// Load all buffers
	for(tinygltf::Buffer buffer : m_model.buffers)
	{
		assetsManager->AddGPUBuffer(gpuHeapUploader.Upload(buffer.data.data(), buffer.data.size()));
	}

	// Set up meshes
	int meshId = 0;
	for (tinygltf::Mesh mesh : m_model.meshes)
	{
		Mesh m(assetsManager);
		m.SetId(meshId++);

		// Create a submesh for each primitive
		for (tinygltf::Primitive primitive : mesh.primitives)
		{
			SubMesh sm;
			// Attribute (es. "POSITION") -> Accessors -> BufferView -> Buffer
			if(primitive.attributes.find("POSITION") != primitive.attributes.end()) 
			{
				tinygltf::BufferView positionsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["POSITION"]].bufferView];
				sm.verticesBufferView.bufferId = positionsBV.buffer;
				sm.verticesBufferView.byteOffset = positionsBV.byteOffset;
				sm.verticesBufferView.byteLength = positionsBV.byteLength;
				sm.verticesBufferView.count = m_model.accessors[primitive.attributes["POSITION"]].count;
			}
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) 
			{
				tinygltf::BufferView normalsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["NORMAL"]].bufferView];
				sm.normalsBufferView.bufferId = normalsBV.buffer;
				sm.normalsBufferView.byteOffset = normalsBV.byteOffset;
				sm.normalsBufferView.byteLength = normalsBV.byteLength;
				sm.normalsBufferView.count = m_model.accessors[primitive.attributes["NORMAL"]].count;
			}
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
			{
				tinygltf::BufferView texCoord0BV = m_model.bufferViews[m_model.accessors[primitive.attributes["TEXCOORD_0"]].bufferView];
				sm.texCoord0BufferView.bufferId = texCoord0BV.buffer;
				sm.texCoord0BufferView.byteOffset = texCoord0BV.byteOffset;
				sm.texCoord0BufferView.byteLength = texCoord0BV.byteLength;
				sm.texCoord0BufferView.count = m_model.accessors[primitive.attributes["TEXCOORD_0"]].count;
			}
			if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
			{
				tinygltf::BufferView texCoord1BV = m_model.bufferViews[m_model.accessors[primitive.attributes["TEXCOORD_1"]].bufferView];
				sm.texCoord1BufferView.bufferId = texCoord1BV.buffer;
				sm.texCoord1BufferView.byteOffset = texCoord1BV.byteOffset;
				sm.texCoord1BufferView.byteLength = texCoord1BV.byteLength;
				sm.texCoord1BufferView.count = m_model.accessors[primitive.attributes["TEXCOORD_1"]].count;
			}
			if (primitive.indices != -1)
			{
				tinygltf::BufferView indicesBV = m_model.bufferViews[m_model.accessors[primitive.indices].bufferView];
				sm.indicesBufferView.bufferId = indicesBV.buffer;
				sm.indicesBufferView.byteOffset = indicesBV.byteOffset;
				sm.indicesBufferView.byteLength = indicesBV.byteLength;
				sm.indicesBufferView.count = m_model.accessors[primitive.indices].count;
			}
			sm.materialId = primitive.material;
			m.AddSubMesh(sm);
		}
		m_meshes.push_back(m);
	}

	// Set up materials
	int materialId = 0;
	for (tinygltf::Material material : m_model.materials)
	{
		RoughMetallicMaterial rmMaterial;
		rmMaterial.baseColorFactor =
		{
			static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[0]),
			static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[1]),
			static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[2]),
			static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[3])
		};
		rmMaterial.roughnessFactor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);
		rmMaterial.metallicFactor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
		rmMaterial.baseColorTA.textureId = material.pbrMetallicRoughness.baseColorTexture.index;
		rmMaterial.baseColorTA.texCoordId = material.pbrMetallicRoughness.baseColorTexture.texCoord;
		rmMaterial.roughMetallicTA.textureId = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
		rmMaterial.roughMetallicTA.texCoordId = material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;
		rmMaterial.normalTA.textureId = material.normalTexture.index;
		rmMaterial.normalTA.texCoordId = material.normalTexture.texCoord;
		rmMaterial.occlusionTA.textureId = material.occlusionTexture.index;
		rmMaterial.occlusionTA.texCoordId = material.occlusionTexture.texCoord;
		rmMaterial.emissiveTA.textureId = material.emissiveTexture.index;
		rmMaterial.emissiveTA.texCoordId = material.emissiveTexture.texCoord;
		assetsManager->AddMaterial(materialId++, rmMaterial);
	}

	// Set up textures
	int textureId = 0;
	for (tinygltf::Texture texture : m_model.textures)
	{
		tinygltf::Image image = m_model.images[texture.source];
		tinygltf::BufferView imageBufferView = m_model.bufferViews[image.bufferView];
		tinygltf::Buffer imageBuffer = m_model.buffers[imageBufferView.buffer];
		uint8_t* imageBufferBegin = imageBuffer.data.data() + imageBufferView.byteOffset;
		Microsoft::WRL::ComPtr<ID3D12Resource> pTexture;
		CreateTextureFromMemory(m_renderer.get(), imageBufferBegin, imageBufferView.byteLength, &pTexture);
		assetsManager->AddTexture(textureId++, pTexture);
	}

	// Set up samplers
	int samplerId = 0;
	for (tinygltf::Sampler sampler : m_model.samplers)
	{
		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		
		if (sampler.wrapS == REPEAT) samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		else if (sampler.wrapS == CLAMP_TO_EDGE) samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		else if (sampler.wrapS == MIRRORED_REPEAT) samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

		if (sampler.wrapT == REPEAT) samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		else if (sampler.wrapT == CLAMP_TO_EDGE) samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		else if (sampler.wrapT == MIRRORED_REPEAT) samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		
		if (sampler.wrapR == REPEAT) samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		else if (sampler.wrapR == CLAMP_TO_EDGE) samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		else if (sampler.wrapR == MIRRORED_REPEAT) samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
	
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		assetsManager->AddSampler(samplerId++, samplerDesc);
	}	
}

std::vector<Mesh> GLTFScene::getMeshes()
{
	return m_meshes;
}