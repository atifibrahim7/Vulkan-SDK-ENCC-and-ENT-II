// VulkanBuffers.cpp : Vulkan Buffer Management
#include "DrawComponents.h"
#include "../CCL.h"
#include "ModelManager.cpp"
#include "../GAME/GameComponents.h"
namespace DRAW
{
	void Destroy_MeshCollection(entt::registry& registry, entt::entity entity)
	{
		auto& collection = registry.get<DRAW::MeshCollection>(entity);
		for (auto meshEntity : collection.meshes)
		{
			registry.destroy(meshEntity);
		}
	}
	//*** HELPERS ***//
	void AddSceneData(entt::registry& registry, entt::entity entity)
	{
		auto& sceneData = registry.emplace<SceneData>(entity,
			SceneData{ { -1.0f, -1.0f, 2.0f }, { 0.9f, 0.9f, 0.9f }, { 0.2f, 0.2f, 0.2f } });

		GW::MATH::GVector::NormalizeF(sceneData.sunDirection, sceneData.sunDirection);
		auto& renderer = registry.get<VulkanRenderer>(entity);
		sceneData.projectionMatrix = renderer.projMatrix;
	}

	//*** SYSTEMS ***//
	// Forward Declare
	void Destroy_VulkanVertexBuffer(entt::registry& registry, entt::entity entity);

	void Update_VulkanVertexBuffer(entt::registry& registry, entt::entity entity) {
		auto& gpuBuffer = registry.get<VulkanVertexBuffer>(entity);
		// upload the buffer to the GPU
		if (registry.all_of<VulkanRenderer, std::vector<H2B::VERTEX>>(entity)) {
			// if there is already a gpu buffer attached, lets delete it
			if (gpuBuffer.buffer != VK_NULL_HANDLE)
				Destroy_VulkanVertexBuffer(registry, entity);
			// if there is a cpu buffer attached, lets upload it to the GPU then delete it
			auto& vkRenderer = registry.get<VulkanRenderer>(entity);
			auto& cpuBuffer = registry.get<std::vector<H2B::VERTEX>>(entity);
			// Transfer triangle data to the vertex buffer. (staging would be preferred here)
			GvkHelper::create_buffer(vkRenderer.physicalDevice, vkRenderer.device, sizeof(H2B::VERTEX) * cpuBuffer.size(),
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &gpuBuffer.buffer, &gpuBuffer.memory);
			GvkHelper::write_to_buffer(vkRenderer.device, gpuBuffer.memory,
				cpuBuffer.data(), sizeof(H2B::VERTEX) * cpuBuffer.size());
			// remove the CPU buffer
			vkDeviceWaitIdle(vkRenderer.device);
			registry.remove<std::vector<H2B::VERTEX>>(entity);
		}
	}

	void Destroy_VulkanVertexBuffer(entt::registry& registry, entt::entity entity) {
		// check if the buffer is allocated, if so, release it
		if (registry.all_of<VulkanVertexBuffer, VulkanRenderer>(entity)) {

			auto& vkRenderer = registry.get<VulkanRenderer>(entity);
			auto& gpuBuffer = registry.get<VulkanVertexBuffer>(entity);
			vkDeviceWaitIdle(vkRenderer.device);
			// Release allocated buffers, shaders & pipeline
			vkDestroyBuffer(vkRenderer.device, gpuBuffer.buffer, nullptr);
			vkFreeMemory(vkRenderer.device, gpuBuffer.memory, nullptr);
		}

	}

	// Forward declare
	void Destroy_VulkanIndexBuffer(entt::registry& registry, entt::entity entity);

	void Update_VulkanIndexBuffer(entt::registry& registry, entt::entity entity) {
		auto& gpuBuffer = registry.get<VulkanIndexBuffer>(entity);
		// upload the buffer to the GPU
		if (registry.all_of<VulkanRenderer, std::vector<unsigned int>>(entity)) {
			// if there is already a gpu buffer attached, lets delete it
			if (gpuBuffer.buffer != VK_NULL_HANDLE)
				Destroy_VulkanIndexBuffer(registry, entity);
			// if there is a cpu buffer attached, lets upload it to the GPU then delete it
			auto& vkRenderer = registry.get<VulkanRenderer>(entity);
			auto& cpuBuffer = registry.get<std::vector<unsigned int>>(entity);

			// Transfer triangle data to the vertex buffer. (staging would be preferred here)
			GvkHelper::create_buffer(vkRenderer.physicalDevice, vkRenderer.device, sizeof(unsigned int) * cpuBuffer.size(),
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &gpuBuffer.buffer, &gpuBuffer.memory);
			GvkHelper::write_to_buffer(vkRenderer.device, gpuBuffer.memory,
				cpuBuffer.data(), sizeof(unsigned int) * cpuBuffer.size());
			// remove the CPU buffer
			vkDeviceWaitIdle(vkRenderer.device);
			registry.remove<std::vector<unsigned int>>(entity);
		}
	}

	void Destroy_VulkanIndexBuffer(entt::registry& registry, entt::entity entity) {
		// check if the buffer is allocated, if so, release it
		if (registry.all_of<VulkanIndexBuffer, VulkanRenderer>(entity)) {

			auto& vkRenderer = registry.get<VulkanRenderer>(entity);
			auto& gpuBuffer = registry.get<VulkanIndexBuffer>(entity);
			
			vkDeviceWaitIdle(vkRenderer.device);
			// Release allocated buffers, shaders & pipeline
			vkDestroyBuffer(vkRenderer.device, gpuBuffer.buffer, nullptr);
			vkFreeMemory(vkRenderer.device, gpuBuffer.memory, nullptr);
		}
	}

	void Construct_VulkanGPUInstanceBuffer(entt::registry& registry, entt::entity entity) {
		auto& bufferComponent = registry.get<VulkanGPUInstanceBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		unsigned int frameCount;
		renderer.vlkSurface.GetSwapchainImageCount(frameCount);
		bufferComponent.memory.resize(frameCount);
		bufferComponent.buffer.resize(frameCount);

		for (unsigned int i = 0; i < frameCount; i++)
		{
			GvkHelper::create_buffer(renderer.physicalDevice, renderer.device, sizeof(GPUInstance)*bufferComponent.element_count,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &bufferComponent.buffer[i], &bufferComponent.memory[i]);
		}
		
	}
		
	// Forward declare
	void Destroy_VulkanGPUInstanceBuffer(entt::registry& registry, entt::entity entity);

	void Update_VulkanGPUInstanceBuffer(entt::registry& registry, entt::entity entity) {
		if (!registry.all_of<std::vector<GPUInstance>>(entity))
			return; // No Instances, so nothing to write. Bail

		auto& gpuBuffer = registry.get<VulkanGPUInstanceBuffer>(entity);		
		auto& instances = registry.get<std::vector<GPUInstance>>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		// Resize buffer if needed
		if (instances.size() > gpuBuffer.element_count)
		{			
			Destroy_VulkanGPUInstanceBuffer(registry, entity);

			gpuBuffer.element_count *= 2; // Double the storage size if we ran out
			
			for (unsigned int i = 0; i < gpuBuffer.buffer.size(); i++)
			{
				GvkHelper::create_buffer(renderer.physicalDevice, renderer.device, sizeof(GPUInstance) * gpuBuffer.element_count,
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &gpuBuffer.buffer[i], &gpuBuffer.memory[i]);

				VkDescriptorBufferInfo storageBufferInfo = { gpuBuffer.buffer[i], 0, VK_WHOLE_SIZE };
				VkWriteDescriptorSet storageWrite = {};
				storageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				storageWrite.dstSet = renderer.descriptorSets[i];
				storageWrite.dstBinding = 1; // 1 For the storage buffer
				storageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				storageWrite.descriptorCount = 1;
				storageWrite.pBufferInfo = &storageBufferInfo;

				vkUpdateDescriptorSets(renderer.device, 1, &storageWrite, 0, nullptr);
			}
		}		

		if (instances.size() > 0)
		{
			unsigned int sizeInBytes = sizeof(GPUInstance) * instances.size();
			unsigned int frame;
			renderer.vlkSurface.GetSwapchainCurrentImage(frame);
			vkDeviceWaitIdle(renderer.device);
			GvkHelper::write_to_buffer(renderer.device, gpuBuffer.memory[frame], instances.data(), sizeInBytes);
		}

		vkDeviceWaitIdle(renderer.device);
		registry.remove<std::vector<GPUInstance>>(entity);
	}

	void Destroy_VulkanGPUInstanceBuffer(entt::registry& registry, entt::entity entity) {
		auto& gpuBuffer = registry.get<VulkanGPUInstanceBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		vkDeviceWaitIdle(renderer.device);
		for (auto handle : gpuBuffer.buffer)
		{
			vkDestroyBuffer(renderer.device, handle, nullptr);
		}
		for (auto data : gpuBuffer.memory)
		{
			vkFreeMemory(renderer.device, data, nullptr);
		}
	}

	void Construct_VulkanUniformBuffer(entt::registry& registry, entt::entity entity) {

		auto& bufferComponent = registry.get<VulkanUniformBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		// If there isn't a SceneData, add one
		if (!registry.all_of<SceneData>(entity))
		{
			AddSceneData(registry, entity);
		}

		unsigned int frameCount;
		renderer.vlkSurface.GetSwapchainImageCount(frameCount);
		bufferComponent.memory.resize(frameCount);
		bufferComponent.buffer.resize(frameCount);

		for (unsigned int i = 0; i < frameCount; i++)
		{
			GvkHelper::create_buffer(renderer.physicalDevice, renderer.device, sizeof(SceneData),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &bufferComponent.buffer[i], &bufferComponent.memory[i]);
		}
	}

	void Update_VulkanUniformBuffer(entt::registry& registry, entt::entity entity) {
		auto& gpuBuffer = registry.get<VulkanUniformBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);
		auto& data = registry.get<SceneData>(entity);

		// TODO : Update Dynamic parts of the Scene Data here
		// We really only support one camera, so use the first one
		auto& camera = registry.get<Camera>(registry.view<Camera>().front());

		data.camPos = camera.camMatrix.row4;
		GW::MATH::GMatrix::InverseF(camera.camMatrix, data.viewMatrix);


		unsigned int frame;
		renderer.vlkSurface.GetSwapchainCurrentImage(frame);
		GvkHelper::write_to_buffer(renderer.device, gpuBuffer.memory[frame], &data, sizeof(SceneData));
	}

	void Destroy_VulkanUniformBuffer(entt::registry& registry, entt::entity entity) {
		auto& gpuBuffer = registry.get<VulkanUniformBuffer>(entity);
		auto& renderer = registry.get<VulkanRenderer>(entity);

		for (auto handle : gpuBuffer.buffer)
		{
			vkDestroyBuffer(renderer.device, handle, nullptr);
		}
		for (auto data : gpuBuffer.memory)
		{
			vkFreeMemory(renderer.device, data, nullptr);
		}
	}

	//part 1c


	void Construct_CPULevel(entt::registry& registry, entt::entity entity)
	{
		// Get the CPULevel component that's being emplaced
		auto& cpuLevel = registry.get<CPULevel>(entity);

		// Create a GLog instance
		GW::SYSTEM::GLog log;
		log.Create("LevelLoader");
		log.EnableConsoleLogging(true);  // Enable console logging for debugging

		// Load the level
		try {
		bool loadSuccess = cpuLevel.levelData.LoadLevel(
			cpuLevel.jsonPath.c_str(),
			cpuLevel.modelFolderPath.c_str(),
			log
		);
		if (!loadSuccess)
			log.LogCategorized("ERROR", "Failed to load level data");

		else
			log.LogCategorized("INFO", "Level data loaded successfully");
		}
		catch (std::exception& e)
		{			log.LogCategorized("ERROR", e.what());    }
		
		//part 1c
		/*auto& cpuLevel = registry.get<CPULevel>(entity);
		auto modelManagerEntity = registry.create();
		auto& modelManager = registry.emplace<ModelManager>(modelManagerEntity);*/

		
		
	}
	void Construct_GPULevel(entt::registry& registry, entt::entity entity)
	{
		// Try to get the CPULevel component
		auto* cpuLevel = registry.try_get<CPULevel>(entity);
		GW::SYSTEM::GLog log;
		log.Create("GPULevelConstructor");
		log.EnableConsoleLogging(true);
		if (!cpuLevel)
		{
			// Handle error: CPULevel not found
		
			log.LogCategorized("ERROR", "CPULevel component not found when constructing GPULevel");
			return;
		}
		registry.emplace<VulkanVertexBuffer>(entity); 

		registry.emplace<std::vector<H2B::VERTEX>>(entity, cpuLevel->levelData.levelVertices);

		registry.patch<VulkanVertexBuffer>(entity);

		registry.emplace<VulkanIndexBuffer>(entity);

		registry.emplace<std::vector<unsigned int>>(entity, cpuLevel->levelData.levelIndices);

		registry.patch<VulkanIndexBuffer>(entity);
		
		// Part 1c
		auto modelManagerEntity = registry.create();
		auto& modelManager = registry.emplace<ModelManager>(modelManagerEntity);
		for (const auto& blenderObject : cpuLevel->levelData.blenderObjects)
		{
			const auto& model = cpuLevel->levelData.levelModels[blenderObject.modelIndex];

			
		}



		// Create entities for each mesh in the level
		for (const auto& blenderObject : cpuLevel->levelData.blenderObjects)
		{
			const auto& model = cpuLevel->levelData.levelModels[blenderObject.modelIndex];

			for (unsigned int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex)
			{
				const auto& mesh = cpuLevel->levelData.levelMeshes[model.meshStart + meshIndex];
				if (model.isDynamic)
				{
					MeshCollection collection;

					for (unsigned int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex)
					{
						const auto& mesh = cpuLevel->levelData.levelMeshes[model.meshStart + meshIndex];

						auto meshEntity = registry.create();

						registry.emplace<GeometryData>(meshEntity, GeometryData{
							static_cast<unsigned int>(model.indexStart + mesh.drawInfo.indexOffset),
							mesh.drawInfo.indexCount,
							static_cast<unsigned int>(model.vertexStart)
							});

						registry.emplace<GPUInstance>(meshEntity, GPUInstance{
							cpuLevel->levelData.levelTransforms[blenderObject.transformIndex],
							cpuLevel->levelData.levelMaterials[model.materialStart + mesh.materialIndex].attrib
							});

						// Add DoNotRender component to hide the mesh initially
						registry.emplace<DRAW::DoNotRender>(meshEntity);

						// Add mesh entity to the collection
						collection.meshes.push_back(meshEntity);
					}

					// Add collection to ModelManager
					modelManager.collections[blenderObject.blendername] = std::move(collection);
				}
				else	
				{

					// Create a new entity for this mesh
					auto meshEntity = registry.create();

					// Add GeometryData component
					registry.emplace<GeometryData>(meshEntity, GeometryData{
						static_cast<unsigned int>(model.indexStart + mesh.drawInfo.indexOffset),
						mesh.drawInfo.indexCount,
						static_cast<unsigned int>(model.vertexStart)  // Note: We don't have vertexOffset in BATCH, so we use the model's vertexStart
						});

					// Add GPUInstance component
					registry.emplace<GPUInstance>(meshEntity, GPUInstance{
						cpuLevel->levelData.levelTransforms[blenderObject.transformIndex],
						cpuLevel->levelData.levelMaterials[model.materialStart + mesh.materialIndex].attrib
						});

					if (model.isDynamic) {
						registry.emplace<DRAW::DoNotRender>(meshEntity);
					}
				}
			}
		}
		if (!registry.all_of<VulkanVertexBuffer, VulkanIndexBuffer>(entity)) {
			if (registry.all_of<VulkanVertexBuffer>(entity)) {
				std::cout << "VulkanVertexBuffer present, but VulkanIndexBuffer missing(ON CONSTRUCT)" << std::endl;
			}
			else if (registry.all_of<VulkanIndexBuffer>(entity)) {
				std::cout << "VulkanIndexBuffer present, but VulkanVertexBuffer missing(ON CONSTRUCT)" << std::endl;
			}
			else {
				std::cout << "Both VulkanVertexBuffer and VulkanIndexBuffer are missing(ON CONSTRUCT)" << std::endl;
			}
		}
		else {
			std::cout << "Both VulkanVertexBuffer and VulkanIndexBuffer are present (ON CONSTRUCT)" << std::endl;
		}

		

		std::cout << "GPULevel construction completed for entity: " << static_cast<uint32_t>(entity) << std::endl;

	}

	
	// Use this MACRO to connect the EnTT Component Logic
	CONNECT_COMPONENT_LOGIC() {
	
		// register the Window component's logic
		registry.on_update<VulkanVertexBuffer>().connect<Update_VulkanVertexBuffer>();
		registry.on_destroy<VulkanVertexBuffer>().connect<Destroy_VulkanVertexBuffer>();

		registry.on_update<VulkanIndexBuffer>().connect<Update_VulkanIndexBuffer>();
		registry.on_destroy<VulkanIndexBuffer>().connect<Destroy_VulkanIndexBuffer>();

		registry.on_construct<VulkanGPUInstanceBuffer>().connect<Construct_VulkanGPUInstanceBuffer>();
		registry.on_update<VulkanGPUInstanceBuffer>().connect<Update_VulkanGPUInstanceBuffer>();
		registry.on_destroy<VulkanGPUInstanceBuffer>().connect<Destroy_VulkanGPUInstanceBuffer>();

		registry.on_construct<VulkanUniformBuffer>().connect<Construct_VulkanUniformBuffer>();
		registry.on_update<VulkanUniformBuffer>().connect<Update_VulkanUniformBuffer>();
		registry.on_destroy<VulkanUniformBuffer>().connect<Destroy_VulkanUniformBuffer>();
		//part 1b 
		registry.on_construct<CPULevel>().connect<Construct_CPULevel>();
		registry.on_construct<GPULevel>().connect<Construct_GPULevel>();

		registry.on_destroy<ModelManager>().connect<Destroy_MeshCollection>();
		 registry.on_update<GAME::GameManager>().connect<&GAME::GameManager::Update>();           //add & if not working 
		 registry.on_update<GAME::Player>().connect<&GAME::Player::Update>();
	}

} // namespace DRAW