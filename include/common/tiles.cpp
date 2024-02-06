#include "tiles.h"

namespace tiles
{
	constexpr const char *collisionMap =
		"-X------"
		"-----X--"
		"-X-XXX--"
		"-X-XXX--"
		"-XXXXX-X"
		"XXXXXXXX"
		"------XX"
		"--------"
		"--------"
		"--XX--XX"

		"--------"
		"---SSSSS"
		"SSS-----"
		"-BB-----"
		;

	gl2d::TextureAtlasPadding textureAtlas{width, height, width * pixelSize, height * pixelSize};

	bool isSolid(int id)
	{
		return(collisionMap[id] == 'X');
	}

	bool canBulletPass(int id)
	{
		return(collisionMap[id] == 'B');

	}
	
	bool willSlow(int id)
	{
		return(collisionMap[id] == 'S');

	}

	glm::vec4 getTileUV(int id)
	{
		int x = id % width;
		int y = id / width;
		return textureAtlas.get(x, y);
	}


};