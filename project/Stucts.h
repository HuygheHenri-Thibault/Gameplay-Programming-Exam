#pragma once

// ENUMS
//enum class Direction
//{
//	up,
//	right,
//	down,
//	left
//};

// STRUCTS
struct ExpandingSearchData
{
	float distance = 0;
	int step = 0;
	Elite::Vector2 lastSearchPosition = {0,0};
	// Direction lastDirection = Direction::up
};

struct Inventory
{
	unsigned int maxGuns = 0;
	unsigned int maxMedkits = 0;
	unsigned int maxFood = 0;

	unsigned int currentGuns = 0;
	unsigned int currentMedkits = 0;
	unsigned int currentFood = 0;

	std::vector<ItemInfo> inventorySlots = {};
};
