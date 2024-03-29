#pragma once
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"
#include <algorithm>
#include <gl2d/gl2d.h>
#include "tiles.h"

#undef min
#undef max

constexpr float worldMagnification = 48;
constexpr int playerNameSize = 10;

namespace phisics
{
	inline float deltaTime;
	struct input {
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	};

	struct BlockInfo
	{
		
		char type;
		
		bool isCollidable();
		bool willSlow();
		bool canPass();
		bool canSpawnItem();
	};
	
	struct MapData
	{

		BlockInfo* data;
		BlockInfo nullBlock = {};
	
		int w = 0;
		int h = 0;
	
		void create(int w, int h, const char* d);
		BlockInfo& get(int x, int y);
	
		void render(gl2d::Renderer2D &renderer, gl2d::Texture texture);

		void cleanup();
	
		bool load(const char *file);
		void save(const char *file);
	};
	
	struct Entity
	{
		char name[playerNameSize] = {};
		glm::vec2 pos = {};
		glm::vec2 lastPos = {};
		glm::vec2 input = {};

		glm::vec2 dimensions = {0.8,0.8};
		glm::vec3 color = {1,1,1};
		bool isBoosted = false;
		float hitTime = 0.f;
		int life = maxLife;

		static constexpr int maxLife = 10;
		static constexpr float invincibilityTime = 0.10;

		void resolveConstrains(MapData& mapData);
	
	
		bool moving = 0;
	
	
		// 0 1  -> used for animations
		bool movingRight = 0;
		
	
		void move(glm::vec2 dir);
		
		bool hit();
	
		//should be called only once per frame
		void updateMove(float deltaTime);
	
		void draw(gl2d::Renderer2D& renderer, float deltaTime, gl2d::Texture characterSprite, gl2d::Font font);
	
	
	private:
		void checkCollisionBrute(glm::vec2& pos, glm::vec2 lastPos, MapData& mapData,
			bool& upTouch, bool& downTouch, bool& leftTouch, bool& rightTouch);
		glm::vec2 performCollision(MapData& mapData, glm::vec2 pos, glm::vec2 size, glm::vec2 delta,
			bool& upTouch, bool& downTouch, bool& leftTouch, bool& rightTouch);
	};

	struct Bullet
	{
		glm::vec2 pos = {};
		glm::vec2 direction = {};
		glm::vec3 color = {1,1,1};
		int32_t cid = 0;
		float size = 0.4;

		bool checkCollisionMap(MapData &mapData);
		bool checkCollisionPlayer(Entity &e);
		void updateMove(float deltaTime);
		void draw(gl2d::Renderer2D &renderer, gl2d::Texture bulletSprite);
		glm::vec4 getTransform();
	};


	enum
	{
		itemTypeHealth = 1,
		itemTypeBatery = 2,
		itemTypePills = 3,
		itemTypeAk = 4,
		itemTypeGlock = 5,
		itemsCount = 5,
	};

	struct Item
	{
		Item() {};
		Item(glm::vec2 pos, uint32_t itemId, int itemType) 
			:pos(pos), itemId(itemId), itemType(itemType)
		{};
		
		glm::vec2 pos = {};
		uint32_t itemId = 0;
		int itemType = 0;

		bool checkCollisionPlayer(Entity &e);
		void draw(gl2d::Renderer2D &renderer, gl2d::Texture medkitTexture, gl2d::Texture bateryTexture, gl2d::Texture pillsTexture, gl2d::Texture akTexture, gl2d::Texture glockTexture);

	};
	
	
	//pos and size on on every component
	bool aabb(glm::vec4 b1, glm::vec4 b2);


};
