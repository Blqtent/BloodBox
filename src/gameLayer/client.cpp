#include "serverClient.h"
#include <platform/platformInput.h>
#include <Phisics.h>
#include <packet.h>
#include <unordered_map>
#include "Ui.h"
#include <glui/glui.h>
#include <iostream>

phisics::MapData ruins;
phisics::MapData field;
phisics::MapData facility;

phisics::MapData map;
ENetPeer *server = {};
int32_t cid = {};
bool joined = false;
ENetHost *client;
static bool isConnected = false;

std::unordered_map<int32_t, phisics::Entity> players;

static std::vector<phisics::Bullet> bullets;
static std::vector<phisics::Bullet> ownBullets;
static std::vector<phisics::Item> items;
static std::vector<platform::Message> messages;
static bool hasBatery = 0;
static bool hasPills = 0;


static bool hasAk = 0;
static bool hasGlock = true;

static int deaths = 0;
static int kills = 0;
static int playerSpeed = 10;
glm::ivec2 spawnPositions[] =
{
	{5,5},
	{2,46},
	{44,44},
	{45,4}
};

glm::ivec2 getSpawnPosition()
{
	return spawnPositions[rand() % (sizeof(spawnPositions) / sizeof(spawnPositions[0]))];
}

void resetClient()
{

	if (!ruins.load(RESOURCES_PATH "ruins.bin"))
	{
		return;
	}
	if (!field.load(RESOURCES_PATH "field.bin"))
	{
		return;
	}
	if (!facility.load(RESOURCES_PATH "facility.bin"))
	{
		return;
	}
	if (!map.load(RESOURCES_PATH "empty.bin"))
	{
		return;
	}

	players.clear();
	bullets.clear();
	ownBullets.clear();
	items.clear();
	hasBatery = false;
	hasPills = false;

	//todo add a struct here

	joined = false;
	client = nullptr;

	//todo
	//enet_host_destroy(server);
	server = {};
	cid = {};
}

void sendPlayerData(phisics::Entity &e, bool reliable)
{
	Packet p;
	p.cid = cid;
	p.header = headerUpdateConnection;
	sendPacket(server, p, (const char *)&e, sizeof(phisics::Entity), reliable, 0);
}

bool connectToServer(ENetHost *&client, ENetPeer *&server, int32_t &cid, std::string ip, std::string port, char *playerName)
{
	ENetAddress adress;
	ENetEvent event;

	if (ip.empty())
	{
		if (enet_address_set_host(&adress, "127.0.0.1") >= 0);
		else return false;
	}
	else
	{
		if (enet_address_set_host(&adress, ip.c_str()) >= 0);
		else return false;
	}
	//enet_address_set_host(&adress, "95.76.249.14");
	//enet_address_set_host(&adress, "192.168.1.11");
	if (port.empty())
		adress.port = 7778;
	else
		adress.port = std::stoi(port);

	//client, adress, channels, data to send rightAway
	server = enet_host_connect(client, &adress, SERVER_CHANNELS, 0);
	std::cout << "Connect\n";
	if (server == nullptr)
	{
		std::cout << "Attempt failed\n";
		return false;
	}

	//see if we got events by server
	//client, event, ms to wait(0 means that we don't wait)
	if (enet_host_service(client, &event, 5000) > 0
		&& event.type == ENET_EVENT_TYPE_CONNECT)
	{
		std::cout << "connected\n";
	}
	else
	{
		enet_peer_reset(server);
		return false;
	}


	if (enet_host_service(client, &event, 5000) > 0
		&& event.type == ENET_EVENT_TYPE_RECEIVE)
	{
		Packet p = {};
		size_t size;
		auto data = parsePacket(event, p, size);

		if (p.header == headerReceiveCIDAndData)
		{
			cid = p.cid;

			glm::vec3 color = *(glm::vec3*)data;
			auto e = phisics::Entity();
			e.pos = getSpawnPosition();
			e.lastPos = e.pos;
			e.color = color;
			memcpy(e.name, playerName, playerNameSize);
			players[cid] = e;

			sendPlayerData(e, true);
		}
		else {
			enet_peer_reset(server);
			return false;
		}

		std::cout << "received cid: " << cid << "\n";
		enet_packet_destroy(event.packet);
		return true;
	}
	else
	{
		std::cout << "succeed\n";
		enet_peer_reset(server);
		return 0;
	}

	std::cout << "succeed\n";
	return true;
}

void msgLoop(ENetHost *client)
{
	
	ENetEvent event;
	if(enet_host_service(client, &event, 0) > 0)
	{
		switch (event.type)
		{
			case ENET_EVENT_TYPE_RECEIVE:
			{
				//std::cout << event.packet->dataLength << "\n";
				//std::cout << "recieved: " << event.packet->data << "\n";
				//std::cout << event.peer->data << "\n"; //recieved from
				//std::cout << event.peer->address.host << "\n"; //recieved from
				//std::cout << event.peer->address.port << "\n"; //recieved from
				//std::cout << event.channelID << "\n";
				Packet p = {};
				size_t size = {};
				auto data = parsePacket(event, p, size);


				if (p.header == headerReceiveMapData) {
					if (p.cid == 100000) {
						map = ruins;
					}
					else if (p.cid == 100001) {
						map = field;
					}
					else if (p.cid == 100002) {
						map = facility;
					}
				}
				if (p.header == headerAnounceConnection)
				{
					players[p.cid] = *(phisics::Entity*)data;

				}else if (p.header == headerUpdateConnection)
				{

					players[p.cid] = *(phisics::Entity *)data;

				}else if (p.header == headerAnounceDisconnect)
				{
					auto find = players.find(p.cid);
					players.erase(find);
				}else if (p.header == headerSendBullet)
				{
					bullets.push_back(*(phisics::Bullet *)data);
				}
				else if (p.header == headerRegisterHit)
				{
					auto find = players.find(p.cid);
					bool h = find->second.hit();

					if (h && find->first == cid)
					{
						find->second.life -= 1;

						if (find->second.life <= 0)
						{
							auto &player = find->second;
							player.pos = getSpawnPosition();
							player.lastPos = player.pos;
							player.life = player.maxLife;
							sendPlayerData(player, true);
							deaths++;
							hasAk = false;
							hasGlock = true;
						}

					}
				}
				else if (p.header == headerSpawnItem)
				{
					items.push_back(*(phisics::Item *)data);
				}
				else if (p.header == headerPickupItem)
				{
					auto item = *(phisics::Item*)data;
					auto f = std::find_if(items.begin(), items.end(), [item](phisics::Item &i) { return i.itemId == item.itemId; });

					if (f != items.end())
					{
						items.erase(f);
					}

					if (p.cid == cid)
					{
						auto find = players.find(p.cid);

						if (item.itemType == phisics::itemTypeHealth)
						{
							find->second.life = phisics::Entity::maxLife;
						}
						else if (item.itemType == phisics::itemTypeBatery)
						{
							hasBatery = true;
						}
						else if (item.itemType == phisics::itemTypePills)
						{
							hasPills = true;
						}
						else if (item.itemType == phisics::itemTypeAk)
						{
							hasAk = true;
							hasGlock = false;
						}
						else if (item.itemType == phisics::itemTypeGlock)
						{
							hasGlock = true;
							hasAk = false;
						}

					}

				}
				else if (p.header == headerServerMessage)
				{
					std::cout << "Recv Msg\n";
					platform::Message msg;
					msg.alpha = 255;
					if (p.cid == 65536) {
						msg.s = "A new Player Has Joined. Go beat him up.";
						msg.display_time = 300;
						messages.push_back(msg);
					}
					
					if (p.cid == 65537) {
						msg.s = "A player has died! Nice one, whoever did that.";
						msg.display_time = 300;
						messages.push_back(msg);
					}
				}
				enet_packet_destroy(event.packet);

				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT:
			{
				//std::cout << "disconect\n";
				//exit(0);
				isConnected = false;
				joined = false;
				break;
			}
		}
	}

}

void closeFunction()
{
	if (!server) { return; }

	ENetEvent event;

	enet_peer_disconnect(server, 0);
	//wait for disconect
	while (enet_host_service(client, &event, 10) > 0)
	{
		switch (event.type)
		{
			case ENET_EVENT_TYPE_RECEIVE:
			{
				enet_packet_destroy(event.packet);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT:
			{
				break;
			}
		}
	}


}


bool clientFunction(float deltaTime, gl2d::Renderer2D &renderer, Textures textures, std::string ip, std::string port, char *playerName)
{

	if (!joined)
	{
		
		if (!client)
		{
			client = enet_host_create(nullptr, 1, 1, 0, 0);
		}
		try
		{
			if (connectToServer(client, server, cid, ip, port, playerName))
			{
				joined = true;
				isConnected = true;
			}
			else {
				throw 1;
			}
		}
		catch (const int code) {
			return false;
		}
	}
	else
	{

		msgLoop(client);
		if (!isConnected) return false;
		auto &player = players[cid];

	#pragma region input
		static int pillBoost = 0;

		if (hasPills) {
			pillBoost = 500;
		}

		if (pillBoost > 0) 
		{
			hasPills = false;
			playerSpeed = 13;
			pillBoost--;
		}
		else {
			playerSpeed = 10;
		}

		float speed = playerSpeed * deltaTime;
		float bulletSpeed = 50;
		float posy = 0;
		float posx = 0;
		constexpr float CONTROLLER_MARGIN = 0.5;

		if (platform::isKeyHeld(platform::Button::Up)
			|| platform::isKeyHeld(platform::Button::W)
			|| platform::getControllerButtons().buttons[platform::ControllerButtons::Up].held
			|| platform::getControllerButtons().LStick.y < -CONTROLLER_MARGIN
			)
		{
			posy = -1;
		}
		if (platform::isKeyHeld(platform::Button::Down)
			|| platform::isKeyHeld(platform::Button::S)
			|| platform::getControllerButtons().buttons[platform::ControllerButtons::Down].held
			|| platform::getControllerButtons().LStick.y > CONTROLLER_MARGIN
			)
		{
			posy = 1;
		}
		if (platform::isKeyHeld(platform::Button::Left)
			|| platform::isKeyHeld(platform::Button::A)
			|| platform::getControllerButtons().buttons[platform::ControllerButtons::Left].held
			|| platform::getControllerButtons().LStick.x < -CONTROLLER_MARGIN
			)
		{
			posx = -1;
		}
		if (platform::isKeyHeld(platform::Button::Right)
			|| platform::isKeyHeld(platform::Button::D)
			|| platform::getControllerButtons().buttons[platform::ControllerButtons::Right].held
			|| platform::getControllerButtons().LStick.x > CONTROLLER_MARGIN
			)
		{
			posx = 1;
		}

		if (platform::isKeyPressedOn(platform::Button::Enter))
		{
			platform::setFullScreen(!platform::isFullScreen());
		}
		
		if (platform::isKeyPressedOn(platform::Button::Escape) && platform::isKeyPressedOn(platform::Button::L))
		{
			Packet p;
			p.cid = cid;
			p.header = headerRequestDisconnect;

			sendPacket(server, p, {}, 0, true, 1);
		}

		auto mousePos = platform::getRelMousePosition();
		//renderer.renderText(mousePos, ".", textures.font, Colors_Black);
		static float culldown = 0;
		static float culldownTime = 0.3;
		static int bateryShooting = 0;

		if (hasAk) {
			culldownTime = 0.05;
		}
		else if (hasGlock){
			culldownTime = 0.3;
		}

		if (culldown > 0)
		{
			culldown -= deltaTime;
		}



		if ((platform::isKeyReleased(platform::Button::E)))
		{
			if (hasBatery)
			{
				bateryShooting = 50;
				hasBatery = false;
			}
		}


		if (bateryShooting > 0)
		{

			static float batteryShootingDellay = 0;
			constexpr float batteryShootingDellayCulldownTime = 0.06;
			batteryShootingDellay -= deltaTime;

			if (batteryShootingDellay < 0.f)
			{
				batteryShootingDellay += batteryShootingDellayCulldownTime;

				phisics::Bullet b;
				b.pos = player.pos + (player.dimensions / 2.f);
				b.color = player.color;
				b.cid = cid;
				b.direction = { 1,0 };

				float angle = (bateryShooting / 10.f) * 2.f * 3.14159265;

				b.direction = glm::mat2(std::cos(angle), -std::sin(angle), std::sin(angle), std::cos(angle)) * b.direction;
				b.direction = glm::normalize(b.direction);

				Packet p;
				p.cid = cid;
				p.header = headerSendBullet;
				sendPacket(server, p, (const char*)&b, sizeof(phisics::Bullet), true, 1);

				ownBullets.push_back(b);

				bateryShooting--;
			}

		}


		if ((platform::isLMouseHeld() 
			||
			platform::getControllerButtons().LT > CONTROLLER_MARGIN
			)
			&& culldown <= 0.f)
		{

			culldown = culldownTime;

			phisics::Bullet b;
			b.pos = player.pos + (player.dimensions/2.f);
			b.color = player.color;
			b.cid = cid;

			glm::vec2 thumbDir = {platform::getControllerButtons().RStick.x,platform::getControllerButtons().RStick.y};

			if (glm::length(thumbDir) > 0.f)
			{
				thumbDir = glm::normalize(thumbDir);
				b.direction = thumbDir;
			}
			else
			{
				auto mousePos = platform::getRelMousePosition();
				auto screenCenter = glm::vec2(renderer.windowW, renderer.windowH) / 2.f;

				auto delta = glm::vec2(mousePos) - screenCenter;

				float magnitude = glm::length(delta);
				if (magnitude == 0)
				{
					b.direction = {1,0};
				}
				else
				{
					b.direction = delta / magnitude;
				}
			}

			

			Packet p;
			p.cid = cid;
			p.header = headerSendBullet;
			sendPacket(server, p, (const char *)&b, sizeof(phisics::Bullet), true, 1);

			ownBullets.push_back(b);

			
		}



	#pragma endregion

	#pragma region items

		for (int i = 0; i < items.size(); i++)
		{
			//pickup item
			if (items[i].checkCollisionPlayer(player) && (platform::autopickup || platform::isKeyTyped(platform::Button::F)))
			{
				Packet p;
				p.cid = cid;
				p.header = headerPickupItem;
				uint32_t itemId = items[i].itemId;
				sendPacket(server, p, (const char *)&itemId, sizeof(itemId), true, 1);
				//sendPacket(server, p, (const char*)&itemId, sizeof(itemId), true, 1);

				items.erase(items.begin() + i);
				i--;
				continue;
			}

		}

	#pragma endregion

	
	#pragma region player
		{

			bool playerChaged = 0;

			if (player.input.x != posx || player.input.y != posy)
			{
				playerChaged = true;
			}

			player.input = {posx, posy};

			for (auto &i : players)
			{
				glm::vec2 dir = i.second.input;
				if (dir.x != 0 || dir.y != 0)
				{
					if (map.get(i.second.pos.x, i.second.pos.y).willSlow())
						i.second.move(glm::normalize(dir) * (float)(speed * 0.5));
					else 
						i.second.move(glm::normalize(dir) * speed);
				}
				i.second.resolveConstrains(map);
				i.second.updateMove(deltaTime);
			}

			renderer.currentCamera.follow(player.pos * worldMagnification, deltaTime * 5, 3, renderer.windowW, renderer.windowH);
			//renderer.currentCamera.clip(glm::vec2(map.w, map.h) *worldMagnification, {renderer.windowW, renderer.windowH});

			map.render(renderer, textures.sprites);

			for (auto &i : players)
			{
				i.second.draw(renderer, deltaTime, textures.character, textures.font);
			}

			static float timer = 0;
			constexpr float updateTime = 1.f / 10;

			timer -= deltaTime;
			if (playerChaged || timer <= 0)
			{
				timer = updateTime;
				playerChaged = true;
			}

			if (playerChaged)
			{
				sendPlayerData(player, false);
			}
		}
	#pragma endregion

	#pragma region items

		for (int i = 0; i < items.size(); i++)
		{
			items[i].draw(renderer, textures.medKit, textures.battery, textures.pills, textures.ak, textures.glock);

		}

	#pragma endregion



	#pragma region bullets

		for (int i = 0; i < bullets.size(); i++)
		{
			bullets[i].updateMove(deltaTime * bulletSpeed);
			bullets[i].draw(renderer, textures.character);

			for (auto &e : players)
			{
				if (bullets[i].cid != e.first)
				{
					if (bullets[i].checkCollisionPlayer(e.second))
					{
						//hit player
						bullets.erase(bullets.begin() + i);
						i--;
						break;
					}
				}
			}
		}

		for (int i = 0; i < bullets.size(); i++)
		{
			if (bullets[i].checkCollisionMap(map))
			{
				bullets.erase(bullets.begin() + i);
				i--;
				continue;
			}
		}

		bool killInced = false;
		for (int i = 0; i < ownBullets.size(); i++)
		{
			ownBullets[i].updateMove(deltaTime * bulletSpeed);
			ownBullets[i].draw(renderer, textures.character);

			for (auto &e : players)
			{
				if (e.first != cid)
				{
					if (ownBullets[i].checkCollisionPlayer(e.second))
					{
						//hit player, register hit
						e.second.hit();

						Packet p;
						p.header = headerRegisterHit;
						p.cid = cid;
						sendPacket(server, p, (const char*)&e.first, sizeof(int32_t), true, 1);
						if (e.second.life == 1 && !killInced) {
							killInced = true;
							kills++;
							platform::Message msg;
							msg.alpha = 255;
							msg.s = "You have killed a player! Nice one.";
							msg.display_time = 300;
							messages.push_back(msg);
						}
						ownBullets.erase(ownBullets.begin() + i);
						i--;
						break;
					}
				}
			}


			
		}

		for (int i = 0; i < ownBullets.size(); i++)
		{
			if (ownBullets[i].checkCollisionMap(map))
			{
				ownBullets.erase(ownBullets.begin() + i);
				i--;
				continue;
			}
		}

	#pragma endregion


	#pragma region ui
		{
			Ui::Frame f({0,0, renderer.windowW, renderer.windowH});

			auto c = renderer.currentCamera; //todo push pop camera
			renderer.currentCamera.setDefault();

			float xLeft = 0.95;
			float xSize = 0.04;
			float xAdvance = 0.05 - 0.025;

			for (int i = 0; i < player.life; i++)
			{
				auto crossPos = Ui::Box().xLeftPerc(xLeft).yTopPerc(0.03).xDimensionPercentage(0.04).yAspectRatio(1.f);
				auto crossPosDown = Ui::Box().xLeftPerc(xLeft+0.003).yTopPerc(0.025).xDimensionPercentage(0.04).yAspectRatio(1.f);
				renderer.renderRectangle(crossPosDown, {0.f,0.f,0.f,1.f}, {}, 0.f, textures.cross);
				renderer.renderRectangle(crossPos, {1.f,1.f,1.f,1.f}, {}, 0.f, textures.cross);
				xLeft -= xAdvance;
			}
			xLeft = 0.95;

			if (hasBatery)
			{
				auto pos = Ui::Box().xLeftPerc(xLeft - 0.05).yTopPerc(0.035 + xSize).xDimensionPercentage(xSize).yAspectRatio(1.f);
				auto posDown = Ui::Box().xLeftPerc(xLeft + 0.003 - 0.05).yTopPerc(0.040 + xSize).xDimensionPercentage(xSize).yAspectRatio(1.f);
				renderer.renderRectangle(posDown, {0.f,0.f,0.f,1.f}, {}, 0.f, textures.battery);
				renderer.renderRectangle(pos, {1.f,1.f,1.f,1.f}, {}, 0.f, textures.battery);
			}
			xLeft = 0.5;
			if (hasAk)
			{
				auto pos = Ui::Box().xLeftPerc(xLeft - 0.05).yTopPerc(0.035 + xSize).xDimensionPercentage(xSize).yAspectRatio(1.f);
				auto posDown = Ui::Box().xLeftPerc(xLeft + 0.003 - 0.05).yTopPerc(0.040 + xSize).xDimensionPercentage(xSize).yAspectRatio(1.f);
				renderer.renderRectangle(posDown, {0.f,0.f,0.f,1.f}, {}, 0.f, textures.ak);
				renderer.renderRectangle(pos, {1.f,1.f,1.f,1.f}, {}, 0.f, textures.ak);
			}
			else if (hasGlock)
			{
				auto pos = Ui::Box().xLeftPerc(xLeft - 0.05).yTopPerc(0.035 + xSize).xDimensionPercentage(xSize).yAspectRatio(1.f);
				auto posDown = Ui::Box().xLeftPerc(xLeft + 0.003 - 0.05).yTopPerc(0.040 + xSize).xDimensionPercentage(xSize).yAspectRatio(1.f);
				renderer.renderRectangle(posDown, {0.f,0.f,0.f,1.f}, {}, 0.f, textures.glock);
				renderer.renderRectangle(pos, {1.f,1.f,1.f,1.f}, {}, 0.f, textures.glock);
			}

			renderer.renderText({ 75, 50}, ("Kills: " + std::to_string(kills)).c_str(), textures.font, Colors_White, 0.5);
			renderer.renderText({ 80, 100}, ("Deaths: " + std::to_string(deaths)).c_str(), textures.font, Colors_White, 0.5);
			
			int y = 150;
			int x = 0;
			
			for (platform::Message& msg : messages) {
				float offset = msg.s.length() * 8.65;
				renderer.renderText({ offset, y }, msg.s.c_str(), textures.font, {1, 1, 1, msg.alpha}, 0.5);
				msg.display_time--;
				//msg.alpha--;
				std::cout << std::to_string(msg.display_time) << "\n";
				if (msg.display_time <= 0) {
					//std::cout << "ERASEERASEERASEERASE" << "\n";
					messages.erase(messages.begin() + x);
				}
				x++;
				y += 50;
			}

			renderer.currentCamera = c;

		

		}
	#pragma endregion
		return true;



	}

	

}