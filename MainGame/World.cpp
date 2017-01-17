#include <algorithm>
#include <fstream>
#include <iomanip>
#include <windows.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "World.h"
#include "Deerchant.h"

using namespace sf;

World::World(int width, int height) : screenSizeX(0), screenSizeY(0), focusedObject(nullptr)
{
	this->width = width;
	this->height = height;
	auto blockSize = 1000;// int(sqrt(min(width, height)));
	staticGrid = GridList<StaticObject>(width, height, blockSize);
	dynamicGrid = GridList<DynamicObject>(width, height, blockSize);
}

bool cmpImgDraw(const WorldObject* first, const WorldObject* second)
{
	return first->getPosition().y + first->getTextureSize().y < second->getPosition().y + second->getTextureSize().y;
}

void World::initSpriteMap()
{
	std::ifstream fin("World/objects.txt");

	int objectsNumber;
	fin >> objectsNumber;

	for (auto i = 0; i < objectsNumber; i++)
	{
		std::string name;
		fin >> name;
		spriteMap.insert({ name, boardSprite{} });
		auto sprite = &spriteMap[name];
		fin >> sprite->type;
		sprite->texture.loadFromFile("World/" + name);
		sprite->sprite.setTexture(sprite->texture);
	}
	fin.close();
}

void World::generate(int objCount)
{
	auto s = int(sqrt(objCount));

	auto stoneTextureSize = Vector2f(spriteMap["stone.png"].texture.getSize());
	auto stoneSize = Vector2f(stoneTextureSize.x/1.25, stoneTextureSize.y / 4);
	auto stoneTextureOffset = Vector2f(stoneTextureSize.x / 4-40, stoneTextureSize.y - stoneSize.y-30);

	for (auto i = 0; i < s; i++)
	{
		for (auto j = 0; j < s; j++)
		{
			auto name = "stone" + std::to_string(i * s + j);
			auto position = Vector2f(i * (width / s), j * (height / s));
			staticGrid.addItem(new Stone(name, FloatRect(position, stoneSize), stoneTextureOffset, stoneTextureSize), name, position.x, position.y);
		}
	}

	std::string heroName = "hero";
	auto heroTextureSize = Vector2f(spriteMap["heroF_0.png"].texture.getSize());
	auto heroSize = Vector2f(heroTextureSize.x/2, heroTextureSize.y / 5);
	auto heroTextureOffset = Vector2f(heroTextureSize.x / 4, heroTextureSize.y - heroSize.y);
	auto heroPosition = Vector2f(200, 200);
	dynamicGrid.addItem(new Deerchant(heroName, FloatRect(heroPosition, heroSize), heroTextureOffset, heroTextureSize), heroName, int(heroPosition.x), int(heroPosition.y));

	focusedObject = dynamicGrid.getItemByName(heroName);
}

void World::interact(RenderWindow& window, long long elapsedTime)
{
	const auto extra = 600;
	auto screenSize = window.getSize();
	auto characterSize = focusedObject->getTextureSize();
	auto characterPosition = focusedObject->getTexturePosition();

	Vector2i worldUpperLeft(int(characterPosition.x - screenSize.x / 2 + characterSize.x / 2), int(characterPosition.y - screenSize.y / 2 + characterSize.y / 2));
	Vector2i worldBottomRight(int(characterPosition.x + screenSize.x / 2 + characterSize.x / 2), int(characterPosition.y + screenSize.y / 2 + characterSize.y / 2));

	auto staticItems = staticGrid.getItems(worldUpperLeft.x - extra, worldUpperLeft.y - extra, worldBottomRight.x + extra, worldBottomRight.y + extra);
	auto dynamicItems = dynamicGrid.getItems(worldUpperLeft.x - extra, worldUpperLeft.y - extra, worldBottomRight.x + extra, worldBottomRight.y + extra);
	for (auto dynamicItem : dynamicItems)
	{
		bool intersects = false;
		if (dynamicItem->direction != STAND)
		{
			auto newPosition = move(*dynamicItem, elapsedTime);
			for (auto staticItem : staticItems)
			{
				if (isIntersect(newPosition, *dynamicItem, *staticItem))
				{
					if (dynamicItem->direction == UP || dynamicItem->direction == DOWN)
					{
						if (dynamicItem->getPosition().x + dynamicItem->getSize().x - staticItem->getPosition().x <= staticItem->getSmoothBorderX().x)
						{
							newPosition = Vector2f(dynamicItem->getPosition().x - dynamicItem->speed*500, dynamicItem->getPosition().y);
							dynamicItem->setPosition(newPosition);
							dynamicGrid.updateItemPosition(dynamicItem->getName(), newPosition.x, newPosition.y);
							continue;
						}
						else
						if (dynamicItem->getPosition().x - staticItem->getPosition().x >= staticItem->getSmoothBorderX().y)
						{
							newPosition = Vector2f(dynamicItem->getPosition().x + dynamicItem->speed * 500, dynamicItem->getPosition().y);
							dynamicItem->setPosition(newPosition);
							dynamicGrid.updateItemPosition(dynamicItem->getName(), newPosition.x, newPosition.y);
							continue;
						}
					}
					if (dynamicItem->direction == LEFT || dynamicItem->direction == RIGHT)
					{
						if (dynamicItem->getPosition().y + dynamicItem->getSize().y - staticItem->getPosition().y <= staticItem->getSmoothBorderY().x)
						{
							newPosition = Vector2f(dynamicItem->getPosition().x, dynamicItem->getPosition().y - dynamicItem->speed*500);
							dynamicItem->setPosition(newPosition);
							dynamicGrid.updateItemPosition(dynamicItem->getName(), newPosition.x, newPosition.y);
							continue;
						}
						else
						if (dynamicItem->getPosition().y - staticItem->getPosition().y >= staticItem->getSmoothBorderY().y)
						{
							newPosition = Vector2f(dynamicItem->getPosition().x, dynamicItem->getPosition().y + dynamicItem->speed*500);
							dynamicItem->setPosition(newPosition);
							dynamicGrid.updateItemPosition(dynamicItem->getName(), newPosition.x, newPosition.y);
							continue;
						}
					}

					intersects = true;
					break;
				}
			}
			if (intersects)
				continue;
			for (auto otherDynamicItem : dynamicItems)
			{
				if (otherDynamicItem == dynamicItem)
					continue;
				if (isIntersect(newPosition, *dynamicItem, *otherDynamicItem))
				{
					intersects = true;
					break;
				}
			}
			if (!intersects)
			{
				dynamicItem->setPosition(newPosition);
				dynamicGrid.updateItemPosition(dynamicItem->getName(), newPosition.x, newPosition.y);
			}
		}
	}
}

void World::draw(RenderWindow& window, long long elapsedTime)
{
	const auto extra = 600;
	auto screenSize = window.getSize();
	auto screenCenter = Vector2i(screenSize.x / 2, screenSize.y / 2);
	auto characterSize = focusedObject->getTextureSize();
	auto characterHalfSize = Vector2i(characterSize.x / 2, characterSize.y / 2);
	auto characterPosition = focusedObject->getTexturePosition();

	Vector2i worldUpperLeft(int(characterPosition.x - screenCenter.x + characterHalfSize.x), int(characterPosition.y - screenCenter.y + characterHalfSize.y));
	Vector2i worldBottomRight(int(characterPosition.x + screenCenter.x + characterHalfSize.x), int(characterPosition.y + screenCenter.y + characterHalfSize.y));

	auto staticItems = staticGrid.getItems(worldUpperLeft.x - extra, worldUpperLeft.y - extra, worldBottomRight.x + extra, worldBottomRight.y + extra);
	auto dynamicItems = dynamicGrid.getItems(worldUpperLeft.x - extra, worldUpperLeft.y - extra, worldBottomRight.x + extra, worldBottomRight.y + extra);
	auto visibleItems = std::vector<WorldObject*>(staticItems.begin(), staticItems.end());
	auto visibleDynamicItems = std::vector<WorldObject*>(dynamicItems.begin(), dynamicItems.end());

	visibleItems.insert(visibleItems.end(), visibleDynamicItems.begin(), visibleDynamicItems.end());
	sort(visibleItems.begin(), visibleItems.end(), cmpImgDraw);
	//WorldObject* worldItem = visibleItems[0];
	//Vector2f test = worldItem->getSmoothBorderX();
	for (auto worldItem : visibleItems)
	{
		auto worldItemPosition = worldItem->getTexturePosition();
		auto spriteItem = &spriteMap[worldItem->getSpriteName(elapsedTime)];

		auto spriteX = float(worldItemPosition.x - characterPosition.x + screenCenter.x - characterHalfSize.x);
		auto spriteY = float(worldItemPosition.y - characterPosition.y + screenCenter.y - characterHalfSize.y);
		spriteItem->sprite.setPosition(Vector2f(spriteX, spriteY));

		window.draw(spriteItem->sprite);

		/*auto box = worldItem->getBoundingBox();
		auto rectangle = sf::RectangleShape();
		rectangle.setSize(sf::Vector2f(box.width, box.height));
		rectangle.setOutlineColor(sf::Color::Red);
		rectangle.setFillColor(sf::Color::Transparent);
		rectangle.setOutlineThickness(1);
		rectangle.setPosition(box.left - characterPosition.x + screenCenter.x - characterHalfSize.x, box.top - characterPosition.y + screenCenter.y - characterHalfSize.y);
		window.draw(rectangle);*/
	}
	auto rectangle = sf::RectangleShape();
	rectangle.setSize(sf::Vector2f(width, height));
	rectangle.setOutlineColor(sf::Color::Green);
	rectangle.setFillColor(sf::Color::Transparent);
	rectangle.setOutlineThickness(2);
	rectangle.setPosition(0 - characterPosition.x + screenCenter.x - characterHalfSize.x, 0 - characterPosition.y + screenCenter.y - characterHalfSize.y);
	window.draw(rectangle);

	auto blockSize = staticGrid.getBlockSize();
	for (auto x = 0; x < width; x += blockSize)
		for (auto y = 0; y < height; y += blockSize)
		{
			auto rectangle = sf::RectangleShape();
			rectangle.setSize(sf::Vector2f(blockSize, blockSize));
			rectangle.setOutlineColor(sf::Color::Blue);
			rectangle.setFillColor(sf::Color::Transparent);
			rectangle.setOutlineThickness(1);
			rectangle.setPosition(x - characterPosition.x + screenCenter.x - characterHalfSize.x, y - characterPosition.y + screenCenter.y - characterHalfSize.y);
			window.draw(rectangle);
		}
}

Vector2f World::move(const DynamicObject& dynamicObject, long long elapsedTime)
{
	auto angle = dynamicObject.direction * M_PI / 180;
	auto position = dynamicObject.getPosition();

	position.x = float(position.x + dynamicObject.speed * cos(angle) * elapsedTime);
	position.y = float(position.y - dynamicObject.speed * sin(angle) * elapsedTime);

	return position;
}

bool World::isIntersect(Vector2f position, const DynamicObject& dynamic, const WorldObject& other)
{
	Rect<float> first(position.x, position.y, dynamic.getSize().x, dynamic.getSize().y);
	Rect<float> second = other.getBoundingBox();
	return (first.left <= second.left && first.left + first.width >= second.left || second.left <= first.left && second.left + second.width >= first.left)
		&& (first.top <= second.top && first.top + first.height >= second.top || second.top <= first.top && second.top + second.height >= first.top);
}

