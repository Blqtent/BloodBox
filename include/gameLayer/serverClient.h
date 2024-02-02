#pragma once
#include <enet/enet.h>
#include <gl2d/gl2d.h>
#include "phisics.h"

struct Textures;

void serverFunction();

void clientFunction(float deltaTime, gl2d::Renderer2D &renderer, Textures textures, std::string ip, std::string port, char *playerName);
void resetClient();
void closeFunction();
void closeServer();

struct Textures
{
	gl2d::Texture sprites;
	gl2d::Texture character;
	gl2d::Texture medKit;
	gl2d::Texture battery;
	gl2d::Texture cross;
	gl2d::Texture pills;

	gl2d::Texture ak;
	gl2d::Texture glock;

	gl2d::Font font;
};