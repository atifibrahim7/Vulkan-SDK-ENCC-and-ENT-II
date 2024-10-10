#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
	/// TODO: Create the tags and components for the game
	/// 
	///*** Tags ***///
	struct Player {

	};
	struct Enemy {

	};
	struct Bullet {

	};

	///*** Components ***///
	// add a Transform component that holds a matrix to represent the transform of these game objects.
	struct Transform {	
		GW::MATH::GMATRIXF matrix; 
	};

}// namespace GAME
#endif // !GAME_COMPONENTS_H_