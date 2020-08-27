#include "GLTFSceneLoader.h"
#include "Texture.h"
#include "Material.h"
#include "Mesh.h"
#include "Scene.h"
#include <map>
#include <cmath>

using Microsoft::WRL::ComPtr;
using DirectX::XMVector3Normalize; 
using DirectX::XMVectorSubtract;
using DirectX::XMVectorScale;
using DirectX::XMVectorMultiply;
using DirectX::XMVector3Dot;
using DirectX::XMVectorGetX;
using DirectX::XMVector3Cross;

GLTFSceneLoader::GLTFSceneLoader(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue)
{
	m_device = device;
	m_commandQueue = commandQueue;
}

void GLTFSceneLoader::Load(std::string fileName)
{
	std::string err;
	std::string warn;

	bool ret = m_glTFLoader.LoadBinaryFromFile(&m_model, &err, &warn, fileName.c_str());
	if (!warn.empty()) { printf("Warn: %s\n", warn.c_str()); }
	if (!err.empty()) { printf("Err: %s\n", err.c_str()); }
	if (!ret) { printf("Failed to parse glTF\n"); return; }
}

void GLTFSceneLoader::GetScene(const int sceneId, std::shared_ptr<Scene>& scene)
{	
	if (sceneId >= m_model.scenes.size()) DXUtil::ThrowException("Scene index out of range");

	GPUHeapUploader gpuHeapUploader(m_device.Get(), m_commandQueue.Get());
	scene = std::make_shared<Scene>(m_device.Get());

	// Load all buffers
	for (tinygltf::Buffer buffer : m_model.buffers)
	{
		scene->AddGPUBuffer(gpuHeapUploader.Upload(buffer.data.data(), buffer.data.size()));
	}

	// Set up meshes
	int meshId = 0;
	for (tinygltf::Mesh mesh : m_model.meshes)
	{
		Mesh m;
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
			if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
			{
				tinygltf::BufferView tangentsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["TANGENT"]].bufferView];
				sm.tangentsBufferView.bufferId = tangentsBV.buffer;
				sm.tangentsBufferView.byteOffset = tangentsBV.byteOffset;
				sm.tangentsBufferView.byteLength = tangentsBV.byteLength;
				sm.tangentsBufferView.count = m_model.accessors[primitive.attributes["TANGENT"]].count;
			}
			else 
			{
				tinygltf::BufferView positionsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["POSITION"]].bufferView];
				sm.tangentsBufferView.bufferId = 1;
				sm.tangentsBufferView.byteOffset = 0;
				sm.tangentsBufferView.count = m_model.accessors[primitive.attributes["POSITION"]].count;
				sm.tangentsBufferView.byteLength = sm.tangentsBufferView.count * sizeof(DirectX::XMFLOAT4);
				ComputeTangentSpace(primitive, scene.get());
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
		scene->AddMesh(m);
	}

	// Load lights
	// Set up materials
	int lightId = 0;
	for (tinygltf::Light light : m_model.lights) {}
	if(m_model.lights.size() == 0) 
	{
		Light light = { { 0.0f, 3.0f, 0.0f, 0.1f }, { 0.5f, 0.5f, 0.5f, 0.1f } };
		scene->AddLight(0, light);
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
		scene->AddMaterial(materialId++, rmMaterial);
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
		CreateTextureFromMemory(m_device.Get(), m_commandQueue.Get(), imageBufferBegin, imageBufferView.byteLength, &pTexture);
		scene->AddTexture(textureId++, pTexture);
	}

	// Set up samplers
	int samplerId = 0;
	if(m_model.samplers.empty()) 
	{
		// Default sampler
		D3D12_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		scene->AddSampler(samplerId++, samplerDesc);
	}
	else 
	{
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
			scene->AddSampler(samplerId++, samplerDesc);
		}
	}
}

void GLTFSceneLoader::ComputeTangentSpace(tinygltf::Primitive primitive, Scene* scene)
{
	tinygltf::BufferView positionsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["POSITION"]].bufferView];
	DirectX::XMFLOAT3* vertices = (DirectX::XMFLOAT3*)(m_model.buffers[0].data.data() + positionsBV.byteOffset);
	size_t verticesCount = m_model.accessors[primitive.attributes["POSITION"]].count;

	tinygltf::BufferView normalsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["NORMAL"]].bufferView];
	DirectX::XMFLOAT3* normals = (DirectX::XMFLOAT3*)(m_model.buffers[0].data.data() + normalsBV.byteOffset);
	
	//tinygltf::BufferView tangentsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["TANGENT"]].bufferView];
	//DirectX::XMFLOAT4* tangents = (DirectX::XMFLOAT4*)(m_model.buffers[0].data.data() + tangentsBV.byteOffset);

	tinygltf::BufferView texCoord0BV = m_model.bufferViews[m_model.accessors[primitive.attributes["TEXCOORD_0"]].bufferView];
	DirectX::XMFLOAT2* texCoords = (DirectX::XMFLOAT2*)(m_model.buffers[0].data.data() + texCoord0BV.byteOffset);

	tinygltf::BufferView indicesBV = m_model.bufferViews[m_model.accessors[primitive.indices].bufferView];
	uint16_t* indexes = (uint16_t*)(m_model.buffers[0].data.data() + indicesBV.byteOffset);
	size_t indexesCount = m_model.accessors[primitive.indices].count;

	std::unique_ptr<DirectX::XMFLOAT3[]> tan1(new DirectX::XMFLOAT3[verticesCount]);
	ZeroMemory(tan1.get(), verticesCount * sizeof(DirectX::XMFLOAT3));
	std::unique_ptr<DirectX::XMFLOAT3[]> tan2(new DirectX::XMFLOAT3[verticesCount]);
	ZeroMemory(tan2.get(), verticesCount * sizeof(DirectX::XMFLOAT3));
	//std::unique_ptr<DirectX::XMVECTOR[]> tan1(new DirectX::XMVECTOR[verticesCount]);
	//std::unique_ptr<DirectX::XMVECTOR[]> tan2(new DirectX::XMVECTOR[verticesCount]);
	std::unique_ptr<DirectX::XMFLOAT4[]> tangent(new DirectX::XMFLOAT4[verticesCount]);
	ZeroMemory(tangent.get(), verticesCount * sizeof(DirectX::XMFLOAT3));

	for(size_t i=0; i<indexesCount; i+=3)
	{
		long i1 = indexes[i];
		long i2 = indexes[i+1];
		long i3 = indexes[i+2];
		
		DirectX::XMFLOAT3 v1 = vertices[i1];
		DirectX::XMFLOAT3 v2 = vertices[i2];
		DirectX::XMFLOAT3 v3 = vertices[i3];
		
		DirectX::XMFLOAT2 w1 = texCoords[i1];
		DirectX::XMFLOAT2 w2 = texCoords[i2];
		DirectX::XMFLOAT2 w3 = texCoords[i3];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;        
		float y2 = v3.y - v1.y;        
		float z1 = v2.z - v1.z;        
		float z2 = v3.z - v1.z;        
		float s1 = w2.x - w1.x;        
		float s2 = w3.x - w1.x;        
		float t1 = w2.y - w1.y;        
		float t2 = w3.y - w1.y;        
		float r = 1.0f / (s1 * t2 - s2 * t1);        
		DirectX::XMFLOAT3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		DirectX::XMFLOAT3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
		tan1[i1] = { tan1[i1].x + sdir.x, tan1[i1].y + sdir.y, tan1[i1].z + sdir.z };
		tan1[i2] = { tan1[i2].x + sdir.x, tan1[i2].y + sdir.y, tan1[i2].z + sdir.z };
		tan1[i3] = { tan1[i3].x + sdir.x, tan1[i3].y + sdir.y, tan1[i3].z + sdir.z };
		tan2[i1] = { tan2[i1].x + sdir.x, tan2[i1].y + sdir.y, tan2[i1].z + sdir.z };
		tan2[i2] = { tan2[i2].x + sdir.x, tan2[i2].y + sdir.y, tan2[i2].z + sdir.z };
		tan2[i3] = { tan2[i3].x + sdir.x, tan2[i3].y + sdir.y, tan2[i3].z + sdir.z };
	}

	for (size_t i = 0; i < verticesCount; i++)
	{

		DirectX::XMVECTOR n = DirectX::XMLoadFloat3(&normals[i]);
		DirectX::XMVECTOR t = DirectX::XMLoadFloat3(&tan1[i]);

		// Gram-Schmidt orthogonalize        
		DirectX::XMVECTOR xmTangent = XMVector3Normalize(XMVectorSubtract(t, XMVectorScale(n, XMVectorGetX(XMVector3Dot(n, t)))));

		// Calculate handedness        
		DirectX::XMVECTOR t2 = DirectX::XMLoadFloat3(&tan2[i]);
		float handedness = (XMVectorGetX(XMVector3Dot(XMVector3Cross(n, t), t2)) < 0.0f) ? -1.0f : 1.0f;
		xmTangent = DirectX::XMVectorSetW(xmTangent, handedness);
		DirectX::XMStoreFloat4(&tangent[i], xmTangent);
	}

	GPUHeapUploader gpuHeapUploader(m_device, m_commandQueue);
	scene->AddGPUBuffer(gpuHeapUploader.Upload(tangent.get(), verticesCount * sizeof(DirectX::XMFLOAT4)));
}