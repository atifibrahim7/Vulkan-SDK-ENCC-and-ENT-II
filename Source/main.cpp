// main entry point for the application
// enables components to define their behaviors locally in an .hpp file
#include "CCL.h"
#include "UTIL/Utilities.h"
// include all components, tags, and systems used by this program
#include "DRAW/DrawComponents.h"
#include "GAME/GameComponents.h"
#include "APP/Window.hpp"

// Local routines for specific application behavior
void GraphicsBehavior(entt::registry &registry);
void GameplayBehavior(entt::registry &registry);
void MainLoopBehavior(entt::registry &registry);

// Architecture is based on components/entities pushing updates to other components/entities (via "patch" function)
int main()
{

	// All components, tags, and systems are stored in a single registry
	entt::registry registry;

	// initialize the ECS Component Logic
	CCL::InitializeComponentLogic(registry);

	auto gameConfig = registry.create();
	registry.emplace<UTIL::Config>(gameConfig);

	GraphicsBehavior(registry); // create windows, surfaces, and renderers

	GameplayBehavior(registry); // create entities and components for gameplay

	MainLoopBehavior(registry); // update windows and input

	// clear all entities and components from the registry
	// invokes on_destroy() for all components that have it
	// registry will still be intact while this is happening
	registry.clear();

	return 0; // now destructors will be called for all components
}

// This function will be called by the main loop to update the graphics
// It will be responsible for loading the Level, creating the VulkanRenderer, and all VulkanInstances
void GraphicsBehavior(entt::registry &registry)
{
	std::shared_ptr<const GameConfig> config = registry.get<UTIL::Config>(
														   registry.view<UTIL::Config>().front())
												   .gameConfig;

	int windowWidth = (*config).at("Window").at("width").as<int>();
	int windowHeight = (*config).at("Window").at("height").as<int>();
	int startX = (*config).at("Window").at("xstart").as<int>();
	int startY = (*config).at("Window").at("ystart").as<int>();
	std::string title = (*config).at("Window").at("title").as<std::string>();

	// Add an entity to handle all the graphics data
	auto display = registry.create();
	registry.emplace<APP::Window>(display,
								  APP::Window{startX, startY, windowWidth, windowHeight, GW::SYSTEM::GWindowStyle::WINDOWEDBORDERED, title});

	std::string vertShader = (*config).at("Shaders").at("vertex").as<std::string>();
	std::string pixelShader = (*config).at("Shaders").at("pixel").as<std::string>();
	registry.emplace<DRAW::VulkanRendererInitialization>(display,
														 DRAW::VulkanRendererInitialization{
															 vertShader, pixelShader, {{0.2f, 0.2f, 0.25f, 1}}, {1.0f, 0u}, 75.f, 0.1f, 100.0f});
	registry.emplace<DRAW::VulkanRenderer>(display);

	// TODO : Load the Level then update the Vertex and Index Buffers  part 2a
	std::string cpuLevelJsonPath = (*config).at("Level1").at("levelFile").as<std::string>();
	std::string cpuLevelModelFolderPath = (*config).at("Level1").at("modelPath").as<std::string>();

	registry.emplace<DRAW::CPULevel>(display, cpuLevelJsonPath, cpuLevelModelFolderPath);
	registry.emplace<DRAW::GPULevel>(display);

	// Register for Vulkan clean up
	GW::CORE::GEventResponder shutdown;
	shutdown.Create([&](const GW::GEvent &e)
					{
		GW::GRAPHICS::GVulkanSurface::Events event;
		GW::GRAPHICS::GVulkanSurface::EVENT_DATA data;
		if (+e.Read(event, data) && event == GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES) {
			registry.clear<DRAW::VulkanRenderer>();
		} });
	registry.get<DRAW::VulkanRenderer>(display).vlkSurface.Register(shutdown);
	registry.emplace<GW::CORE::GEventResponder>(display, shutdown.Relinquish());

	// Create a camera entity and emplace it
	auto camera = registry.create();
	GW::MATH::GMATRIXF initialCamera;
	GW::MATH::GVECTORF translate = {0.0f, 45.0f, -5.0f};
	GW::MATH::GVECTORF lookat = {0.0f, 0.0f, 0.0f};
	GW::MATH::GVECTORF up = {0.0f, 1.0f, 0.0f};
	GW::MATH::GMatrix::TranslateGlobalF(initialCamera, translate, initialCamera);
	GW::MATH::GMatrix::LookAtLHF(translate, lookat, up, initialCamera);
	// Inverse to turn it into a camera matrix, not a view matrix. This will let us do
	// camera manipulation in the component easier
	GW::MATH::GMatrix::InverseF(initialCamera, initialCamera);
	registry.emplace<DRAW::Camera>(camera,
								   DRAW::Camera{initialCamera});
}

// This function will be called by the main loop to update the gameplay
// It will be responsible for updating the VulkanInstances and any other gameplay components
void GameplayBehavior(entt::registry &registry)
{
	std::shared_ptr<const GameConfig> config = registry.get<UTIL::Config>(
														   registry.view<UTIL::Config>().front())
												   .gameConfig;

	auto &modelManager = registry.get<DRAW::ModelManager>(registry.view<DRAW::ModelManager>().front());

	// Create the input objects
	auto input = registry.create();
	registry.emplace<UTIL::Input>(input);

	// Seed the rand
	unsigned int time = std::chrono::steady_clock::now().time_since_epoch().count();
	srand(time);

	// Create the player entity
	auto playerEntity = registry.create();
	// Create the enemy entity
	auto enemyEntity = registry.create();

	registry.emplace<GAME::Player>(playerEntity);
	registry.emplace<GAME::Transform>(playerEntity, GAME::Transform{});
	std::string playerModelName = (*config).at("Player").at("model").as<std::string>();
	auto &playerMeshCollection = modelManager.GetCollection(playerModelName);

	auto &playerCollection = registry.emplace<DRAW::MeshCollection>(playerEntity);

	// Fill out the Player's MeshCollection
	for (const auto &mesh : playerMeshCollection.meshes)
	{
		auto meshEntity = registry.create();
		registry.emplace<DRAW::GPUInstance>(meshEntity, registry.get<DRAW::GPUInstance>(mesh));
		registry.emplace<DRAW::GeometryData>(meshEntity, registry.get<DRAW::GeometryData>(mesh));
		playerCollection.meshes.push_back(meshEntity);
	}

	// Set the Player's initial transform
	if (!playerCollection.meshes.empty())
	{
		auto &playerTransform = registry.get<GAME::Transform>(playerEntity);
		playerTransform.matrix = registry.get<DRAW::GPUInstance>(playerCollection.meshes[0]).transform;
	}

	registry.emplace<GAME::Enemy>(enemyEntity);
	registry.emplace<GAME::Transform>(enemyEntity, GAME::Transform{});
	std::string enemyModelName = (*config).at("Enemy1").at("model").as<std::string>();
	auto &enemyMeshCollection = modelManager.GetCollection(enemyModelName);

	auto &enemyCollection = registry.emplace<DRAW::MeshCollection>(enemyEntity);

	//  Enemy's MeshCollection
	for (const auto &mesh : enemyMeshCollection.meshes)
	{
		auto meshEntity = registry.create();
		registry.emplace<DRAW::GPUInstance>(meshEntity, registry.get<DRAW::GPUInstance>(mesh));
		registry.emplace<DRAW::GeometryData>(meshEntity, registry.get<DRAW::GeometryData>(mesh));
		enemyCollection.meshes.push_back(meshEntity);
	}

	// Enemy's initial transform
	if (!enemyCollection.meshes.empty())
	{
		auto &enemyTransform = registry.get<GAME::Transform>(enemyEntity);
		enemyTransform.matrix = registry.get<DRAW::GPUInstance>(enemyCollection.meshes[0]).transform;
	}

	// Create GameManager entity
	auto gameManagerEntity = registry.create();
	registry.emplace<GAME::GameManager>(gameManagerEntity);

}
// This function will be called by the main loop to update the main loop
// It will be responsible for updating any created windows and handling any input
void MainLoopBehavior(entt::registry &registry)
{
	// main loop
	int closedCount;							 // count of closed windows
	auto winView = registry.view<APP::Window>(); // for updating all windows
	auto &deltaTime = registry.emplace<UTIL::DeltaTime>(registry.create()).dtSec;
	// for updating all windows
	do
	{
		static auto start = std::chrono::steady_clock::now();
		double elapsed = std::chrono::duration<double>(
							 std::chrono::steady_clock::now() - start)
							 .count();
		start = std::chrono::steady_clock::now();
		deltaTime = elapsed;

		// TODO : Update Game
		registry.patch<GAME::GameManager>(registry.view<GAME::GameManager>().front());
		closedCount = 0;
		// find all Windows that are not closed and call "patch" to update them
		for (auto entity : winView)
		{	
		/*	registry.patch<GAME::GameManager>(entity);*/
			if (registry.any_of<APP::WindowClosed>(entity))
				++closedCount;
			else
				registry.patch<APP::Window>(entity); // calls on_update()
		}
	} while (winView.size() != closedCount); // exit when all windows are closed
}
