#include "GLTFSceneLoader.h"
#include "Texture.h"
#include "Material.h"
#include "Mesh.h"
#include "Scene.h"
#include <map>
#include <cmath>
#include <Pathcch.h>
#include <atlstr.h>

#include "UsingDirectives.h"

GLTFSceneLoader::GLTFSceneLoader(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue)
{
	m_device = device;
	m_commandQueue = commandQueue;
}

void GLTFSceneLoader::Load(std::string fileName)
{
	// Save the gltf file resources path
	_tcscpy_s(m_gltfFilePath, CA2T(fileName.c_str()));		
	PathCchRemoveFileSpec(m_gltfFilePath, MAX_PATH_SIZE);

	std::string err, warn;
	bool ret = false;
	if(fileName.find(".glb") != std::string::npos)
	{
		ret = m_glTFLoader.LoadBinaryFromFile(&m_model, &err, &warn, fileName.c_str());
	}
	else if(fileName.find(".gltf") != std::string::npos)
	{
		TCHAR currentPath[MAX_PATH_SIZE];
		GetCurrentDirectory(MAX_PATH_SIZE, currentPath);
		SetCurrentDirectory(m_gltfFilePath);	// Change director to the one that contains the glTF resources
		ret = m_glTFLoader.LoadASCIIFromFile(&m_model, &err, &warn, fileName.c_str());
		SetCurrentDirectory(currentPath);		// Ripristinate the previous working directory
	}

	if (!warn.empty()) { DEBUG_LOG(warn.c_str()) }
	if (!err.empty()) { DXUtil::ThrowException(err); }
	if (!ret) { DXUtil::ThrowException("Failed to load glTF file"); }
}

void GLTFSceneLoader::GetScene(const int sceneId, std::shared_ptr<Scene>& scene)
{	
	if (sceneId >= m_model.scenes.size()) { DXUtil::ThrowException("Scene index out of range"); }
	scene = std::make_shared<Scene>(m_device);
	GPUHeapUploader gpuHeapUploader(m_device.Get(), m_commandQueue.Get());

	// Change director to the one that contains the glTF resources
	TCHAR currentPath[MAX_PATH_SIZE];
	GetCurrentDirectory(MAX_PATH_SIZE, currentPath);
	SetCurrentDirectory(m_gltfFilePath);
		
	ParseSceneGraph(sceneId, scene.get());
	LoadMeshes(scene.get());
	LoadLights(scene.get());
	LoadMaterials(scene.get());
	LoadTextures(scene.get());
	LoadSamplers(scene.get());

	// Ripristinate the previous working directory
	SetCurrentDirectory(currentPath);
	scene->m_isInitialized = true;
}

void GLTFSceneLoader::ParseSceneGraph(const int sceneId, Scene* scene)
{
	scene->m_sceneRoot = std::make_unique<SceneNode>();
	scene->m_sceneRoot->meshId = -1;
	scene->m_sceneRoot->transformMtx = DXUtil::IdentityMtx();
	for (int childId : m_model.scenes[sceneId].nodes) { scene->m_sceneRoot->children.push_back(ParseSceneNode(childId)); }
};

std::unique_ptr<SceneNode> GLTFSceneLoader::ParseSceneNode(const int nodeId)
{
	tinygltf::Node currentNode = m_model.nodes[nodeId];

	std::unique_ptr<SceneNode> sceneNode = std::make_unique<SceneNode>();
	sceneNode->meshId = currentNode.mesh;

	// Load node transformations
	XMMATRIX M, T, R, S;
	M = T = R = S = DirectX::XMMatrixIdentity();

	std::vector<double> t = currentNode.translation;
	std::vector<double> r = currentNode.rotation;
	std::vector<double> s = currentNode.scale;
	
	if (!t.empty()) { T = XMMatrixTranslation(static_cast<float>(t[0]), static_cast<float>(t[1]), static_cast<float>(t[2])); }
	if (!r.empty()) { R = XMMatrixRotationQuaternion(XMLoadFloat4(&XMFLOAT4(static_cast<float>(r[0]), static_cast<float>(r[1]), static_cast<float>(r[2]), static_cast<float>(r[3])))); }
	if (!s.empty()) { S = XMMatrixScaling(static_cast<float>(s[0]), static_cast<float>(s[1]), static_cast<float>(s[2])); }
	M = XMMatrixMultiply(S, XMMatrixMultiply(R, T));
	
	if (!currentNode.matrix.empty())
	{
		DirectX::XMFLOAT4X4 m;
		for (size_t i = 0; i < 4; i++) for (size_t j = 0; j < 4; j++) { m(i, j) = static_cast<float>(currentNode.matrix[4 * i + j]); }
		M = DirectX::XMLoadFloat4x4(&m);
	}
	
	DirectX::XMStoreFloat4x4(&sceneNode->transformMtx, M);

	for (int childId : currentNode.children) { sceneNode->children.push_back(ParseSceneNode(childId)); }

	return sceneNode;
};

void GLTFSceneLoader::LoadMeshes(Scene* scene)
{
	GPUHeapUploader gpuHeapUploader(m_device.Get(), m_commandQueue.Get());
	
	int meshId = 0;
	for (tinygltf::Mesh mesh : m_model.meshes)
	{
		Mesh m;
		m.SetId(meshId++);
		m.SetModelMtx(DXUtil::IdentityMtx());
		m.SetNodeMtx(DXUtil::IdentityMtx());

		// Create a submesh for each primitive
		for (tinygltf::Primitive primitive : mesh.primitives)
		{
			SubMesh sm;
			sm.verticesBufferView.bufferId = -1;
			sm.colorsBufferView.bufferId = -1;
			sm.normalsBufferView.bufferId = -1;
			sm.tangentsBufferView.bufferId = -1;
			sm.texCoord0BufferView.bufferId = -1;
			sm.texCoord1BufferView.bufferId = -1;

			// Attribute (es. "POSITION") -> Accessors -> BufferView -> Buffer
			if (primitive.attributes.find("POSITION") != primitive.attributes.end())
			{
				tinygltf::Accessor positionAccessor = m_model.accessors[primitive.attributes["POSITION"]];
				tinygltf::BufferView positionsBV = m_model.bufferViews[positionAccessor.bufferView];
				sm.verticesBufferView.bufferId = positionsBV.buffer;
				sm.verticesBufferView.byteOffset = positionsBV.byteOffset + positionAccessor.byteOffset; // Accessors defines an additional offset
				sm.verticesBufferView.byteStride = positionsBV.byteStride;
				sm.verticesBufferView.count = positionAccessor.count;
				sm.verticesBufferView.elemType = BUFFER_ELEM_VEC3;
				sm.verticesBufferView.componentType = BUFFER_ELEM_TYPE_FLOAT;
				sm.verticesBufferView.byteLength = positionAccessor.count * sizeof(XMFLOAT3);

				DirectX::XMFLOAT3* vp = (DirectX::XMFLOAT3*)(m_model.buffers[0].data.data() + positionsBV.byteOffset);
				for (int i = 0; i < sm.verticesBufferView.count; i++, vp++) 
				{ 
					vp->z *= -1;	// Flip the z-coordinate becouse we are converting from right handed to left handed
					// Compute the radius of the scene
					if (abs(vp->x) > scene->m_sceneRadius.x) scene->m_sceneRadius.x = abs(vp->x);
					if (abs(vp->y) > scene->m_sceneRadius.y) scene->m_sceneRadius.y = abs(vp->y);
					if (abs(vp->z) > scene->m_sceneRadius.z) scene->m_sceneRadius.z = abs(vp->z);
				}
			}

			if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
			{
				tinygltf::Accessor colorAccessor = m_model.accessors[primitive.attributes["COLOR"]];
				tinygltf::BufferView colorBV = m_model.bufferViews[colorAccessor.bufferView];
				sm.colorsBufferView.bufferId = colorBV.buffer;
				sm.colorsBufferView.byteOffset = colorBV.byteOffset + colorAccessor.byteOffset; // Accessors defines an additional offset
				sm.colorsBufferView.byteLength = colorBV.byteLength;
				sm.colorsBufferView.byteStride = colorBV.byteStride;
				sm.colorsBufferView.count = m_model.accessors[primitive.attributes["COLOR"]].count;

				if (colorAccessor.type == TINYGLTF_TYPE_VEC3) sm.texCoord0BufferView.elemType = BUFFER_ELEM_VEC3;
				if (colorAccessor.type == TINYGLTF_TYPE_VEC4) sm.texCoord0BufferView.elemType = BUFFER_ELEM_VEC4;
				if (colorAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) sm.colorsBufferView.componentType = BUFFER_ELEM_TYPE_FLOAT;
				if (colorAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) sm.colorsBufferView.componentType = BUFFER_ELEM_TYPE_UNSIGNED_CHAR;
				if (colorAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) sm.colorsBufferView.componentType = BUFFER_ELEM_TYPE_UNSIGNED_SHORT;
			}

			if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
			{
				tinygltf::Accessor normalAccessor = m_model.accessors[primitive.attributes["NORMAL"]];
				tinygltf::BufferView normalsBV = m_model.bufferViews[normalAccessor.bufferView];
				sm.normalsBufferView.bufferId = normalsBV.buffer;
				sm.normalsBufferView.byteOffset = normalsBV.byteOffset + normalAccessor.byteOffset;
				sm.normalsBufferView.byteLength = normalsBV.byteLength;
				sm.normalsBufferView.byteStride = normalsBV.byteStride;
				sm.normalsBufferView.count = normalAccessor.count;
				sm.normalsBufferView.elemType = BUFFER_ELEM_VEC3;
				sm.normalsBufferView.componentType = BUFFER_ELEM_TYPE_FLOAT;

				DirectX::XMFLOAT3* normals = (DirectX::XMFLOAT3*)(m_model.buffers[0].data.data() + normalsBV.byteOffset);
				// Flip the z-coordinate becouse we are converting from right handed to left handed
				DirectX::XMFLOAT3* np = (DirectX::XMFLOAT3*)(m_model.buffers[0].data.data() + normalsBV.byteOffset);
				for (int i = 0; i < sm.normalsBufferView.count; i++, np++) { np->z *= -1; }
			}
			else
			{
				// Compute normals
			}

			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
			{
				tinygltf::Accessor texCoord0Accessor = m_model.accessors[primitive.attributes["TEXCOORD_0"]];
				tinygltf::BufferView texCoord0BV = m_model.bufferViews[texCoord0Accessor.bufferView];
				sm.texCoord0BufferView.bufferId = texCoord0BV.buffer;
				sm.texCoord0BufferView.byteOffset = texCoord0BV.byteOffset + texCoord0Accessor.byteOffset;
				sm.texCoord0BufferView.byteLength = texCoord0BV.byteLength;
				sm.texCoord0BufferView.byteStride = texCoord0BV.byteStride;
				sm.texCoord0BufferView.count = texCoord0Accessor.count;
				sm.texCoord0BufferView.elemType = BUFFER_ELEM_VEC2;
				if (texCoord0Accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) sm.texCoord0BufferView.componentType = BUFFER_ELEM_TYPE_FLOAT;
				if (texCoord0Accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) sm.texCoord0BufferView.componentType = BUFFER_ELEM_TYPE_UNSIGNED_CHAR;
				if (texCoord0Accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) sm.texCoord0BufferView.componentType = BUFFER_ELEM_TYPE_UNSIGNED_SHORT;
			}

			if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
			{
				tinygltf::Accessor texCoord1Accessor = m_model.accessors[primitive.attributes["TEXCOORD_1"]];
				tinygltf::BufferView texCoord1BV = m_model.bufferViews[texCoord1Accessor.bufferView];

				sm.texCoord1BufferView.bufferId = texCoord1BV.buffer;
				sm.texCoord1BufferView.byteOffset = texCoord1BV.byteOffset + texCoord1Accessor.byteOffset;
				sm.texCoord1BufferView.byteLength = texCoord1BV.byteLength;
				sm.texCoord1BufferView.byteStride = texCoord1BV.byteStride;
				sm.texCoord1BufferView.count = texCoord1Accessor.count;
				sm.texCoord1BufferView.elemType = BUFFER_ELEM_VEC2;
				if (texCoord1Accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) sm.texCoord1BufferView.componentType = BUFFER_ELEM_TYPE_FLOAT;
				if (texCoord1Accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) sm.texCoord1BufferView.componentType = BUFFER_ELEM_TYPE_UNSIGNED_CHAR;
				if (texCoord1Accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) sm.texCoord1BufferView.componentType = BUFFER_ELEM_TYPE_UNSIGNED_SHORT;
			}

			if (primitive.indices != -1)
			{
				tinygltf::Accessor indicesAccessor = m_model.accessors[primitive.indices];
				tinygltf::BufferView indicesBV = m_model.bufferViews[m_model.accessors[primitive.indices].bufferView];
				sm.indicesBufferView.bufferId = indicesBV.buffer;
				sm.indicesBufferView.byteOffset = indicesBV.byteOffset + indicesAccessor.byteOffset;
				sm.indicesBufferView.byteLength = indicesBV.byteLength;
				sm.indicesBufferView.byteStride = indicesBV.byteStride;
				sm.indicesBufferView.count = m_model.accessors[primitive.indices].count;

				if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					sm.indicesBufferView.componentType = BUFFER_ELEM_TYPE_UNSIGNED_CHAR;
					uint8_t* indexes = (uint8_t*)(m_model.buffers[0].data.data() + indicesBV.byteOffset);
					
					// Switch first and third vertex in a triangle to handle the right handed to left handed coordinate change
					size_t indexesCount = m_model.accessors[primitive.indices].count;
					for (int i = 0; i < indexesCount; i += 3)
					{
						int tmp = indexes[i];
						indexes[i] = indexes[i + 2];
						indexes[i + 2] = tmp;
					}
				}

				if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					sm.indicesBufferView.componentType = BUFFER_ELEM_TYPE_UNSIGNED_SHORT;
					uint16_t* indexes = (uint16_t*)(m_model.buffers[0].data.data() + indicesBV.byteOffset);

					size_t indexesCount = m_model.accessors[primitive.indices].count;
					for (int i = 0; i < indexesCount; i += 3)
					{
						int tmp = indexes[i];
						indexes[i] = indexes[i + 2];
						indexes[i + 2] = tmp;
					}
				}

				if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					sm.indicesBufferView.componentType = BUFFER_ELEM_TYPE_UNSIGNED_INT;
					uint32_t* indexes = (uint32_t*)(m_model.buffers[0].data.data() + indicesBV.byteOffset);
					
					size_t indexesCount = m_model.accessors[primitive.indices].count;
					for (int i = 0; i < indexesCount; i += 3)
					{
						int tmp = indexes[i];
						indexes[i] = indexes[i + 2];
						indexes[i + 2] = tmp;
					}
				}
			}

			if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
			{
				tinygltf::Accessor tangentsAccessor = m_model.accessors[primitive.attributes["TANGENT"]];
				tinygltf::BufferView tangentsBV = m_model.bufferViews[tangentsAccessor.bufferView];
				sm.tangentsBufferView.bufferId = tangentsBV.buffer;
				sm.tangentsBufferView.byteOffset = tangentsBV.byteOffset + tangentsAccessor.byteOffset;
				sm.tangentsBufferView.byteLength = tangentsBV.byteLength;
				sm.tangentsBufferView.byteStride = tangentsBV.byteStride;
				sm.tangentsBufferView.count = m_model.accessors[primitive.attributes["TANGENT"]].count;

				DirectX::XMFLOAT4* tp = (DirectX::XMFLOAT4*)(m_model.buffers[0].data.data() + tangentsBV.byteOffset);
				for (int i = 0; i < sm.tangentsBufferView.count; i++, tp++)
				{
					tp->z *= -1;
				}
			}

			// Load all buffers to the GPU
			for (tinygltf::Buffer buffer : m_model.buffers)
			{
				scene->AddGPUBuffer(gpuHeapUploader.Upload(buffer.data.data(), buffer.data.size()));
			}

			// Compute normals if they are not specified into the file
			if (primitive.attributes.find("NORMAL") == primitive.attributes.end())
			{
				/*
				sm.normalsBufferView.bufferId = m_model.buffers.size(); // A new GPU buffer will be created for normals
				sm.normalsBufferView.byteOffset = 0;
				sm.normalsBufferView.count = m_model.accessors[primitive.attributes["POSITION"]].count;	// As many normals as vertices
				sm.normalsBufferView.byteLength = sm.tangentsBufferView.count * sizeof(DirectX::XMFLOAT4);
				sm.normalsBufferView.byteStride = 0;	// Tighly packed
				ComputeNormals(primitive, scene);
				*/
			}

			// Compute tangents if they are not specified into the file
			if (primitive.attributes.find("TANGENT") == primitive.attributes.end())
			{
				// Tangents computing is only supported for primitives that define texture coords, normals and indices
				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end() && primitive.attributes.find("NORMAL") != primitive.attributes.end() && primitive.indices != -1)
				{
					sm.tangentsBufferView.bufferId = m_model.buffers.size(); // A new GPU buffer will be created for tangents
					sm.tangentsBufferView.byteOffset = 0;
					sm.tangentsBufferView.count = m_model.accessors[primitive.attributes["POSITION"]].count;	// As many tangents as vertices
					sm.tangentsBufferView.byteLength = sm.tangentsBufferView.count * sizeof(DirectX::XMFLOAT4);
					sm.tangentsBufferView.byteStride = 0;	// Tighly packed
					ComputeTangents(primitive, scene);
				}
			}

			if (sm.materialId == -1) primitive.material = 0; // No material in the file, will use the default material

			if (primitive.mode == TINYGLTF_MODE_POINTS) sm.topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			if (primitive.mode == TINYGLTF_MODE_LINE) sm.topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			//if (primitive.mode == tinygltf::TINYGLTF_MODE_LINE_LOOP) sm.topology = ; UNSUPPORTED
			if (primitive.mode == TINYGLTF_MODE_LINE_STRIP) sm.topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			if (primitive.mode == TINYGLTF_MODE_TRIANGLES) sm.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			if (primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) sm.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			//if (primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) sm.topology = ; UNSUPPORTED

			m.AddSubMesh(sm);
		}
		scene->AddMesh(m);
	}
}

void GLTFSceneLoader::LoadLights(Scene* scene)
{
	int lightId = 0;
	for (tinygltf::Light light : m_model.lights) {}
	if (m_model.lights.size() == 0)
	{
		Light light = { { 0.0f, 3.0f, 0.0f, 0.1f }, { 0.5f, 0.5f, 0.5f, 0.1f } };
		scene->AddLight(0, light);
	}
}

void GLTFSceneLoader::LoadMaterials(Scene* scene)
{
	int materialId = 0;
	if (m_model.materials.empty())
	{
		// Default material is black
		RoughMetallicMaterial rmMaterial;
		rmMaterial.baseColorFactor = { 0.0f, 0.0f, 0.0f, 0.0f };
	}
	else
	{
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
	}
}

void GLTFSceneLoader::LoadTextures(Scene* scene)
{
	int textureId = 0;
	for (tinygltf::Texture texture : m_model.textures)
	{
		tinygltf::Image image = m_model.images[texture.source];
		if (image.bufferView != -1)
		{
			tinygltf::BufferView imageBufferView = m_model.bufferViews[image.bufferView];
			tinygltf::Buffer imageBuffer = m_model.buffers[imageBufferView.buffer];
			uint8_t* imageBufferBegin = imageBuffer.data.data() + imageBufferView.byteOffset;
			Microsoft::WRL::ComPtr<ID3D12Resource> pTexture;
			CreateTextureFromMemory(m_device.Get(), m_commandQueue.Get(), imageBufferBegin, imageBufferView.byteLength, &pTexture);
			scene->AddTexture(textureId++, pTexture);
		}
		else
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> pTexture;
			CreateTextureFromFile(m_device.Get(), m_commandQueue.Get(), image.uri, &pTexture);
			scene->AddTexture(textureId++, pTexture);
		}
	}
}

void GLTFSceneLoader::LoadSamplers(Scene* scene)
{
	int samplerId = 0;
	if (m_model.samplers.empty())
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

			samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;	// Other filters type are unsupported 

			if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_REPEAT) samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			else if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			else if (sampler.wrapS == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

			if (sampler.wrapT == TINYGLTF_TEXTURE_WRAP_REPEAT) samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			else if (sampler.wrapT == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			else if (sampler.wrapT == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

			if (sampler.wrapR == TINYGLTF_TEXTURE_WRAP_REPEAT) samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
			else if (sampler.wrapR == TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE) samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
			else if (sampler.wrapR == TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT) samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

			samplerDesc.MinLOD = 0;
			samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
			samplerDesc.MipLODBias = 0.0f;
			samplerDesc.MaxAnisotropy = 1;
			samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			scene->AddSampler(samplerId++, samplerDesc);
		}
	}
}

void GLTFSceneLoader::ComputeNormals(tinygltf::Primitive primitive, Scene* scene) 
{
	// Not implemented yet
}

void GLTFSceneLoader::ComputeTangents(tinygltf::Primitive primitive, Scene* scene)
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
	ZeroMemory(tangent.get(), verticesCount * sizeof(DirectX::XMFLOAT4));

	for (size_t i = 0; i < indexesCount; i += 3)
	{
		long i1 = indexes[i];
		long i2 = indexes[i + 1];
		long i3 = indexes[i + 2];

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