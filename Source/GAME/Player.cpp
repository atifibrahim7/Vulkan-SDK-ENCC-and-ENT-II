#include "GameComponents.h"
#include "../DRAW/DrawComponents.h"
#include "../UTIL/Utilities.h"

namespace GAME
{
    void Player::Update(entt::registry& registry, entt::entity entity)
    {
        auto& input = registry.get<UTIL::Input>(registry.view<UTIL::Input>().front());
        auto& deltaTime = registry.get<UTIL::DeltaTime>(registry.view<UTIL::DeltaTime>().front());
        auto& transform = registry.get<Transform>(entity);
        auto& config = registry.get<UTIL::Config>(registry.view<UTIL::Config>().front());
        float speed = (*config.gameConfig).at("Player").at("speed").as<float>();
        float fireRate = (*config.gameConfig).at("Player").at("firerate").as<float>();
        GW::MATH::GVECTORF movement = { 0, 0, 0, 0 };

        // Check WASD keys
        float keyW, keyA, keyS, keyD;
        input.immediateInput.GetState(G_KEY_W, keyW);
        input.immediateInput.GetState(G_KEY_A, keyA);
        input.immediateInput.GetState(G_KEY_S, keyS);
        input.immediateInput.GetState(G_KEY_D, keyD);

        movement.z += keyW - keyS;
        movement.x += keyD - keyA;

        if (movement.x != 0 || movement.z != 0)
        {
            GW::MATH::GVector::NormalizeF(movement, movement);
        }

        GW::MATH::GVector::ScaleF(movement, speed * deltaTime.dtSec, movement);

        GW::MATH::GMATRIXF translation;
        GW::MATH::GMatrix::TranslateGlobalF(transform.matrix, movement, translation);
        transform.matrix = translation;

        // Handle firing cooldown
        if (registry.all_of<Firing>(entity))
        {
            auto& firing = registry.get<Firing>(entity);
            firing.cooldown -= deltaTime.dtSec;
            if (firing.cooldown <= 0)
            {
                registry.remove<Firing>(entity);
            }
        }
        else
        {
            // Checking  firing keys
            float keyLeft, keyRight, keyUp, keyDown;
            input.immediateInput.GetState(G_KEY_LEFT, keyLeft);
            input.immediateInput.GetState(G_KEY_RIGHT, keyRight);
            input.immediateInput.GetState(G_KEY_UP, keyUp);
            input.immediateInput.GetState(G_KEY_DOWN, keyDown);

            if (keyLeft || keyRight || keyUp || keyDown)
            {
                auto bulletEntity = registry.create();
                registry.emplace<Bullet>(bulletEntity);
                registry.emplace<Transform>(bulletEntity, Transform{ transform.matrix });

                auto& modelManager = registry.get<DRAW::ModelManager>(registry.view<DRAW::ModelManager>().front());
                std::string bulletModelName = (*config.gameConfig).at("Bullet").at("model").as<std::string>();
                auto& bulletMeshCollection = modelManager.GetCollection(bulletModelName);

                auto& bulletCollection = registry.emplace<DRAW::MeshCollection>(bulletEntity);

                for (const auto& mesh : bulletMeshCollection.meshes)
                {
                    auto meshEntity = registry.create();
                    registry.emplace<DRAW::GPUInstance>(meshEntity, registry.get<DRAW::GPUInstance>(mesh));
                    registry.emplace<DRAW::GeometryData>(meshEntity, registry.get<DRAW::GeometryData>(mesh));
                    bulletCollection.meshes.push_back(meshEntity);
                }

                registry.emplace<Firing>(entity, Firing{ fireRate });
            }
        }
    }
   
}