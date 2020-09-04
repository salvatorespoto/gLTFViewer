#include "GLTFSceneLoader.h"
#include "Texture.h"
#include "Material.h"
#include "Mesh.h"
#include "Scene.h"
#include <map>
#include <cmath>

using Microsoft::WRL::ComPtr;

using DirectX::XMFLOAT4, DirectX::XMFLOAT4X4, DirectX::XMVECTOR, DirectX::XMMATRIX, DirectX::XMLoadFloat4, DirectX::XMStoreFloat4,
DirectX::XMVector3Normalize, DirectX::XMVectorSubtract, DirectX::XMVectorScale, DirectX::XMVectorMultiply, 
DirectX::XMVector3Dot, DirectX::XMVectorGetX, DirectX::XMVector3Cross, DirectX::XMMatrixTranslation, 
DirectX::XMMatrixRotationQuaternion, DirectX::XMMatrixScaling, DirectX::XMMatrixMultiply;

GLTFSceneLoader::GLTFSceneLoader(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue)
{
	m_device = device;
	m_commandQueue = commandQueue;
}

void GLTFSceneLoader::Load(std::string fileName)
{
	std::string err;
	std::string warn;

	bool ret = false;
	if(fileName.find(".glb") != std::string::npos)
	{
		ret = m_glTFLoader.LoadBinaryFromFile(&m_model, &err, &warn, fileName.c_str());
	}
	else if(fileName.find(".gltf") != std::string::npos)
	{
		ret = m_glTFLoader.LoadASCIIFromFile(&m_model, &err, &warn, fileName.c_str());
	}
	if (!warn.empty()) { printf("Warn: %s\n", warn.c_str()); }
	if (!err.empty()) { printf("Err: %s\n", err.c_str()); }
	if (!ret) { printf("Failed to parse glTF\n"); return; }
	
}

void GLTFSceneLoader::GetScene(const int sceneId, std::shared_ptr<Scene>& scene)
{	
	if (sceneId >= m_model.scenes.size()) DXUtil::ThrowException("Scene index out of range");

	GPUHeapUploader gpuHeapUploader(m_device.Get(), m_commandQueue.Get());
	scene = std::make_shared<Scene>(m_device.Get());

	// Set up scene graph
	tinygltf::Scene root = m_model.scenes[sceneId];
	
	std::function<std::shared_ptr<SceneNode>(const int, const tinygltf::Model)> ParseSceneNode;
	ParseSceneNode = [&ParseSceneNode](const int nodeId, const tinygltf::Model& model) -> std::shared_ptr<SceneNode>
	{
		tinygltf::Node currentNode = model.nodes[nodeId];
		std::shared_ptr<SceneNode> sceneNode = std::make_shared<SceneNode>();
		sceneNode->meshId = -1;
		sceneNode->meshId = currentNode.mesh;
		
		XMMATRIX M, T, R, S;
		M = T = R = S = DirectX::XMMatrixIdentity();
		std::vector<double> t = currentNode.translation;
		std::vector<double> r = currentNode.rotation;
		std::vector<double> s = currentNode.scale;

		if (!t.empty()) { T = XMMatrixTranslation(static_cast<float>(t[0]), static_cast<float>(t[1]), static_cast<float>(t[2])); }
		if (!r.empty()) { R = XMMatrixRotationQuaternion(XMLoadFloat4(&XMFLOAT4(static_cast<float>(r[0]), static_cast<float>(r[1]), static_cast<float>(r[2]), static_cast<float>(r[3])))); }
		if (!s.empty()) { S = XMMatrixScaling(static_cast<float>(s[0]), static_cast<float>(s[1]), static_cast<float>(s[2])); }
		M = XMMatrixTranspose(XMMatrixMultiply(S, XMMatrixMultiply(R,T)));

		if(!currentNode.matrix.empty()) 
		{
			DirectX::XMFLOAT4X4 m;
			for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { m(j, i) = static_cast<float>(currentNode.matrix[4 * i + j]); }
			M = XMMatrixTranspose(DirectX::XMLoadFloat4x4(&m));
		}
		
		DirectX::XMStoreFloat4x4(&sceneNode->transformMtx, M);
		
		for(int childId : currentNode.children)
		{
			sceneNode->children.push_back(ParseSceneNode(childId, model));
		}

		return sceneNode;
	};

	std::function<std::shared_ptr<SceneNode>(const int, const tinygltf::Model)> ParseSceneGraph;
	ParseSceneGraph = [ParseSceneNode](const int sceneId, const tinygltf::Model& model)->std::shared_ptr<SceneNode>
	{
		std::shared_ptr<SceneNode> root = std::make_shared<SceneNode>();
		root->meshId = -1;
		root->transformMtx = DXUtil::IdentityMtx();
		for (int childId : model.scenes[sceneId].nodes)
		{
			root->children.push_back(ParseSceneNode(childId, model));
		}
		return root;
	};

	std::shared_ptr<SceneNode> sceneGraphRoot = ParseSceneGraph(sceneId, m_model);
	scene->m_sceneRoot = *sceneGraphRoot;

	// Set up meshes
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
			sm.normalsBufferView.bufferId = -1;
			sm.tangentsBufferView.bufferId = -1;
			sm.texCoord0BufferView.bufferId = -1;

			// Attribute (es. "POSITION") -> Accessors -> BufferView -> Buffer
			if(primitive.attributes.find("POSITION") != primitive.attributes.end()) 
			{
				tinygltf::BufferView positionsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["POSITION"]].bufferView];
				sm.verticesBufferView.bufferId = positionsBV.buffer;
				sm.verticesBufferView.byteOffset = positionsBV.byteOffset + m_model.accessors[primitive.attributes["POSITION"]].byteOffset; // Accessors defines an additional offset
				sm.verticesBufferView.byteLength = positionsBV.byteLength;
				sm.verticesBufferView.byteStride = positionsBV.byteStride;
				sm.verticesBufferView.count = m_model.accessors[primitive.attributes["POSITION"]].count;

				// Flip the z-coordinate becouse we are converting from right handed to left handed
				DirectX::XMFLOAT3* vp = (DirectX::XMFLOAT3*)(m_model.buffers[0].data.data() + positionsBV.byteOffset);
				for (int i = 0; i < sm.verticesBufferView.count; i++, vp++) { vp->z *= -1; }	
			}

			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) 
			{
				tinygltf::BufferView normalsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["NORMAL"]].bufferView];
				sm.normalsBufferView.bufferId = normalsBV.buffer;
				sm.normalsBufferView.byteOffset = normalsBV.byteOffset + m_model.accessors[primitive.attributes["NORMAL"]].byteOffset;
				sm.normalsBufferView.byteLength = normalsBV.byteLength;
				sm.normalsBufferView.byteStride = normalsBV.byteStride;
				sm.normalsBufferView.count = m_model.accessors[primitive.attributes["NORMAL"]].count;

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
				tinygltf::BufferView texCoord0BV = m_model.bufferViews[m_model.accessors[primitive.attributes["TEXCOORD_0"]].bufferView];
				sm.texCoord0BufferView.bufferId = texCoord0BV.buffer;
				sm.texCoord0BufferView.byteOffset = texCoord0BV.byteOffset;
				sm.texCoord0BufferView.byteLength = texCoord0BV.byteLength;
				sm.texCoord0BufferView.byteStride = texCoord0BV.byteStride;
				sm.texCoord0BufferView.count = m_model.accessors[primitive.attributes["TEXCOORD_0"]].count;
			}

			if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
			{
				tinygltf::BufferView texCoord1BV = m_model.bufferViews[m_model.accessors[primitive.attributes["TEXCOORD_1"]].bufferView];
				sm.texCoord1BufferView.bufferId = texCoord1BV.buffer;
				sm.texCoord1BufferView.byteOffset = texCoord1BV.byteOffset;
				sm.texCoord1BufferView.byteLength = texCoord1BV.byteLength;
				sm.texCoord1BufferView.byteStride = texCoord1BV.byteStride;
				sm.texCoord1BufferView.count = m_model.accessors[primitive.attributes["TEXCOORD_1"]].count;
			}

			if (primitive.indices != -1)
			{
				tinygltf::BufferView indicesBV = m_model.bufferViews[m_model.accessors[primitive.indices].bufferView];
				sm.indicesBufferView.bufferId = indicesBV.buffer;
				sm.indicesBufferView.byteOffset = indicesBV.byteOffset;
				sm.indicesBufferView.byteLength = indicesBV.byteLength;
				sm.indicesBufferView.byteStride = indicesBV.byteStride;
				sm.indicesBufferView.count = m_model.accessors[primitive.indices].count;

				// Switch first and third vertex in a triangle to handle the right handed to left handed coordinate change
				uint16_t* indexes = (uint16_t*)(m_model.buffers[0].data.data() + indicesBV.byteOffset);
				size_t indexesCount = m_model.accessors[primitive.indices].count;
				for (int i = 0; i < indexesCount; i += 3) 
				{ 
					int tmp = indexes[i]; 
					indexes[i] = indexes[i + 2]; 
					indexes[i + 2] = tmp;
				}
			}

			if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
			{
				tinygltf::BufferView tangentsBV = m_model.bufferViews[m_model.accessors[primitive.attributes["TANGENT"]].bufferView];
				sm.tangentsBufferView.bufferId = tangentsBV.buffer;
				sm.tangentsBufferView.byteOffset = tangentsBV.byteOffset;
				sm.tangentsBufferView.byteLength = tangentsBV.byteLength;
				sm.tangentsBufferView.byteStride = tangentsBV.byteStride;
				sm.tangentsBufferView.count = m_model.accessors[primitive.attributes["TANGENT"]].count;

				DirectX::XMFLOAT3* tp = (DirectX::XMFLOAT3*)(m_model.buffers[0].data.data() + tangentsBV.byteOffset);
				for (int i = 0; i < sm.tangentsBufferView.count; i++, tp++) { tp->z *= -1; }
			}

			// Load all buffers to the GPU
			for (tinygltf::Buffer buffer : m_model.buffers)
			{
				scene->AddGPUBuffer(gpuHeapUploader.Upload(buffer.data.data(), buffer.data.size()));
			}

			if (primitive.attributes.find("TANGENT") == primitive.attributes.end())
			{
				sm.tangentsBufferView.bufferId = m_model.buffers.size(); // A new GPU buffer will be created for tangents
				sm.tangentsBufferView.byteOffset = 0;
				sm.tangentsBufferView.count = m_model.accessors[primitive.attributes["POSITION"]].count;
				sm.tangentsBufferView.byteLength = sm.tangentsBufferView.count * sizeof(DirectX::XMFLOAT4);
				sm.tangentsBufferView.byteStride = 0;	// Tighly packed
				ComputeTangentSpace(primitive, scene.get());
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

	// Load lights
	// Set up materials
	int lightId = 0;
	for (tinygltf::Light light : m_model.lights) {}
	if(m_model.lights.size() == 0) 
	{
		Light light = { { 0.0f, 3.0f, 0.0f, 0.1f }, { 0.5f, 0.5f, 0.5f, 0.1f } };
		scene->AddLight(0, light);
	}

	// Set up MATERIALS
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

	// Set up textures
	int textureId = 0;
	for (tinygltf::Texture texture : m_model.textures)
	{
		tinygltf::Image image = m_model.images[texture.source];
		if(image.bufferView != -1) 
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
			CreateTextureFromFile(m_device.Get(), m_commandQueue.Get(), "models/" + image.uri, &pTexture);
			scene->AddTexture(textureId++, pTexture);
		}
		
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

	scene->m_isInitialized = true;
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
	std::unique_ptr<DirectX::XMFLOAT4[]> tangent(new DirectX::XMFLOAT4[verticesCount]);
	ZeroMemory(tangent.get(), verticesCount * sizeof(DirectX::XMFLOAT3));

	std::unique_ptr<int[]> idxAvgCount(new int[indexesCount]);
	ZeroMemory(idxAvgCount.get(), indexesCount * sizeof(int));

	for(size_t i=0; i<indexesCount; i+=3)
	{
		long i1 = indexes[i];
		long i2 = indexes[i+1];
		long i3 = indexes[i+2];
		
		DirectX::XMFLOAT3 v0 = vertices[i1];
		DirectX::XMFLOAT3 v1 = vertices[i2];
		DirectX::XMFLOAT3 v2 = vertices[i3];
		
		DirectX::XMFLOAT2 uv0 = texCoords[i1];
		DirectX::XMFLOAT2 uv1 = texCoords[i2];
		DirectX::XMFLOAT2 uv2 = texCoords[i3];

		DirectX::XMFLOAT3 e0 = { v1.x - v0.x, v1.y - v0.y, v1.z - v0.z };
		DirectX::XMFLOAT3 e1 = { v2.x - v0.x, v2.y - v0.y, v2.z - v0.z };

		float deltaU0 = uv1.x - uv0.x;	// U coordinate differences between v1 and v0
		float deltaV0 = uv1.y - uv0.y;	// V coordinate differences between v1 and v0
		float deltaU1 = uv2.x - uv0.x;	// U coordinate differences between v2 and v0
		float deltaV1 = uv2.y - uv0.y;	// V coordinate differences between v1 and v0
		
		float denominator = deltaU0 * deltaV1 - deltaV0 * deltaU1;

		DirectX::XMFLOAT4 T;
		DirectX::XMFLOAT4 B;

		T.x = (deltaV1 * e0.x - deltaV0 * e1.x) / denominator;
		T.y = (deltaV1 * e0.y - deltaV0 * e1.y) / denominator;
		T.z = (deltaV1 * e0.z - deltaV0 * e1.z) / denominator;
		T.w = 1.0f;

		B.x = (-deltaV0 * e0.x + deltaU0 * e1.x) / denominator;
		B.y = (-deltaV0 * e0.y + deltaU0 * e1.y) / denominator;
		B.z = (-deltaV0 * e0.z + deltaU0 * e1.z) / denominator;
		B.w = 1.0f;

		XMStoreFloat4(&T, XMVector3Normalize(XMLoadFloat4(&T)));
		tangent[i1] = { tangent[i1].x + T.x, tangent[i1].y + T.y, tangent[i1].z + T.z, 1.0f };
		tangent[i2] = { tangent[i2].x + T.x, tangent[i2].y + T.y, tangent[i2].z + T.z, 1.0f };
		tangent[i3] = { tangent[i3].x + T.x, tangent[i3].y + T.y, tangent[i3].z + T.z, 1.0f };
		idxAvgCount[i1]++;
		idxAvgCount[i2]++;
		idxAvgCount[i3]++;
	}

	for (size_t i = 0; i < verticesCount; i++)
	{ 
		tangent[i].x /= (float)idxAvgCount[i]; 
		tangent[i].y /= (float)idxAvgCount[i];
		tangent[i].z /= (float)idxAvgCount[i];
		tangent[i].w /= (float)idxAvgCount[i];
	}
	GPUHeapUploader gpuHeapUploader(m_device, m_commandQueue);
	scene->AddGPUBuffer(gpuHeapUploader.Upload(tangent.get(), verticesCount * sizeof(DirectX::XMFLOAT4)));
}