#include "gameLayer.h"
#include "gl2d/gl2d.h"
#include "platformInput.h"
#include "imgui.h"
#include <iostream>
#include <sstream>
#include "Phisics.h"
#include <enet/enet.h>
#include "glui/glui.h"
#include "serverClient.h"
#include <thread>
#include <ctime>

gl2d::Renderer2D renderer;

Textures textures;

bool initGame()
{
	renderer.create();
	textures.font.createFromFile(RESOURCES_PATH "font/ANDYB.TTF");
	textures.sprites.loadFromFileWithPixelPadding(RESOURCES_PATH "jawbreaker_tiles.png", tiles::pixelSize, true, true);
	textures.character.loadFromFile(RESOURCES_PATH "character.png", true, true);
	textures.medKit.loadFromFile(RESOURCES_PATH "medkit.png", true, true);
	textures.battery.loadFromFile(RESOURCES_PATH "battery.png", true, true);
	textures.cross.loadFromFile(RESOURCES_PATH "cross.png", true, true);
	textures.pills.loadFromFile(RESOURCES_PATH "pills.png", true, true);
	textures.ak.loadFromFile(RESOURCES_PATH "ak.png", true, true);
	textures.glock.loadFromFile(RESOURCES_PATH "glock.png", true, true);

	glui::gluiInit();

	if (enet_initialize() != 0)
	{
		return false;
	}

	std::srand(std::time(0));

	return true;
}


bool gameLogic(float deltaTime)
{
#pragma region init stuff
	int w = 0; int h = 0;
	w= platform::getWindowSizeX();
	h = platform::getWindowSizeY();
	
	renderer.updateWindowMetrics(w, h);
	renderer.clearScreen();
#pragma endregion

	//std::cout <
	// < "3\n";

	//0 main menu
	//1 client
	//2 server
	static int state = 0;
	static char ip[17] = {};
	static char port[6] = {};
	static char name[playerNameSize] = {};
	static bool headless = false;

	if (state == 0)
	{
		glui::Begin(1);
		glui::Text("BloodBox", Colors_Blue);
		glui::Text(" ", Colors_Blue);
		glui::Text("Enter your name:", Colors_White);

		glui::InputText("Enter name##1", name, sizeof(name));
		
		glui::BeginMenu("Host server", Colors_White, {});
			
			glui::Toggle(headless == true ? "Headless: Enabled" : "Headless: Disabled", { 0, 0, 0, 0 }, &headless);
			glui::BeginMenu("Choose Map", Colors_White, {});
				if (glui::Button("Ruins", Colors_White)) {
					dat::map = "ruins";
				}
				if (glui::Button("Field", Colors_White)) {
					dat::map = "field";
				}
				if (glui::Button("Facility", Colors_White)) {
					dat::map = "facility";
				}
				if (glui::Button("Outcast", Colors_White)) {
					dat::map = "outcast";
				}

			glui::EndMenu();

			if (glui::Button("start", Colors_White))
			{
				std::thread t(serverFunction);
				t.detach();
				resetClient();
				state = 2;
			}
		glui::EndMenu();
		glui::BeginMenu("Join server", Colors_White, {});
			glui::Text("enter ip: ", Colors_White);
			glui::InputText("input ip", ip, sizeof(ip));
			if (glui::Button("join", Colors_White))
			{
				resetClient();
				state = 1;
			}
		glui::EndMenu();
		
		glui::BeginMenu("Options", Colors_White, {});
			glui::Text("Options:", Colors_White);
			glui::Text("WASD to move", Colors_White);
			glui::Text("F to pickup", Colors_White);
			glui::Text("E to use", Colors_White);
			glui::Text("Esc to exit server", Colors_White);
			glui::Toggle(platform::autopickup == true ? "Autopickup: Enabled" : "Autopickup: Disabled", {0, 0, 0, 0}, &platform::autopickup);
			glui::Text("enter port: ", Colors_White);

			glui::InputText("input Port", port, sizeof(port));
		glui::EndMenu();

		if (glui::Button("Exit", Colors_Red))
		{
			return 0;
		}
		glui::End();
		glui::renderFrame(renderer, textures.font, platform::getRelMousePosition(),
			platform::isLMousePressed(), platform::isLMouseHeld(), platform::isLMouseReleased(),
			platform::isKeyReleased(platform::Button::Escape), platform::getTypedInput(), deltaTime);

	}
	else if (state == 1)
	{
		if (!clientFunction(deltaTime, renderer, textures, ip, port, name)) {
			state = 0;
		}
	}
	else if (state == 2)
	{
		if (!headless)
			clientFunction(deltaTime, renderer, textures, ip, port, name);
	}

	
#pragma region set finishing stuff
	renderer.flush();

	return true;
#pragma endregion

}

void closeGame()
{

	closeServer();
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	closeFunction();


}
