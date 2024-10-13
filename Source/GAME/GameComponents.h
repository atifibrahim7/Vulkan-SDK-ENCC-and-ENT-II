#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

#include "../CCL.h"
namespace GAME
{
	/// TODO: Create the tags and components for the game
	/// 
	///*** Tags ***///
	struct Player {
		static void Update(entt::registry& registry, entt::entity entity);
	};
	struct Enemy {

	};
	struct Bullet {

	};
	struct Firing {
		float cooldown;
	};

	///*** Components ***///
	struct Transform {	
		GW::MATH::GMATRIXF matrix; 
	};
	struct GameManager {
		static void Update(entt::registry& registry, entt::entity entity); 

	};

	//CONNECT_COMPONENT_LOGIC() {
	//	registry.on_update<GameManager>().connect<&GameManager::Update>();           //add & if not working 
	//	registry.on_update<Player>().connect<&Player::Update>();
	//}

}// namespace GAME
#endif // !GAME_COMPONENTS_H_