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
	// Direction lastDirection = Direction::up
	Elite::Vector2 lastSearchPosition = {0,0};
};

