#pragma once
#include <gl2d/gl2d.h>

namespace tiles
{

	constexpr int width = 8;
	constexpr int height = 14;
	constexpr int tilesCount = 107;
	constexpr int pixelSize = 8;

	bool isSolid(int id);
	bool canBulletPass(int id);
	bool willSlow(int id);
	glm::vec4 getTileUV(int id);
	

};