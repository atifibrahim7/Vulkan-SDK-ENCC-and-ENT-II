#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"

namespace GAME
{
    void GameManager::Update(entt::registry& registry, entt::entity entity)
    {
        // Update model locations
        auto view = registry.view<Transform, DRAW::MeshCollection>();
        for (auto entity : view)
        {
            auto& transform = view.get<Transform>(entity);
            auto& meshCollection = view.get<DRAW::MeshCollection>(entity);
            for (auto meshEntity : meshCollection.meshes)
            {
                auto& gpuInstance = registry.get<DRAW::GPUInstance>(meshEntity);
                gpuInstance.transform = transform.matrix;
            }
        }
        // Update player
        registry.patch<Player>(registry.view<Player>().front());
    }
   
}