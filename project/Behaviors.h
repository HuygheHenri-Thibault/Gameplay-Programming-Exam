/*=============================================================================*/
// Copyright 2020-2021 Elite Engine
/*=============================================================================*/
// Behaviors.h: Implementation of certain reusable behaviors for the BT version of the Agario Game
/*=============================================================================*/
#ifndef ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
#define ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
//-----------------------------------------------------------------
// Includes & Forward Declarations
//-----------------------------------------------------------------
#include "EliteMath/EMath.h"
#include "EBehaviorTree.h"
#include "Stucts.h"
#include "IExaminterface.h"
using namespace Elite;
//-----------------------------------------------------------------
// Behaviors
//-----------------------------------------------------------------

BehaviorState ExpandingSquareSearch(Elite::Blackboard* pBlackboard)
{	
	const float squaredSearchDistanceMargin = 5.0f;
	ExpandingSearchData searchData;
	AgentInfo agentInfo;
	IExamInterface* pluginInterface = nullptr;

	bool dataAvailable = pBlackboard->GetData("ExpandingSquareSearchData", searchData)
		&& pBlackboard->GetData("AgentInfo", agentInfo)
		&& pBlackboard->GetData("PluginInterface", pluginInterface);

	if (!dataAvailable)
	{
		return Failure;
	}

	if (DistanceSquared(searchData.lastSearchPosition, agentInfo.Position) < squaredSearchDistanceMargin)
	{
		float distanceToTravel = searchData.distance * (1 + searchData.step / 2);
		Elite::Vector2 newTarget = searchData.lastSearchPosition;

		switch (searchData.step % 4)
		{
		case 0:
			// go left
			newTarget.x -= distanceToTravel;
			break;
		case 1:
			// go up
			newTarget.y += distanceToTravel;
			break;
		case 2:
			// go right
			newTarget.x += distanceToTravel;
			break;
		case 3:
			// go down
			newTarget.y -= distanceToTravel;
			break;
		}

		searchData.lastSearchPosition = newTarget;
		searchData.step++;
		pBlackboard->ChangeData("ExpandingSquareSearchData", searchData);
	}

	pBlackboard->ChangeData("Target", pluginInterface->NavMesh_GetClosestPathPoint(searchData.lastSearchPosition));
	return Success;
}

bool IsNewHouseDiscovered(Elite::Blackboard* pBlackboard)
{
	bool isNewHouseFound = false;
	pBlackboard->GetData("IsNewHouseDiscovered", isNewHouseFound);

	if (isNewHouseFound)
	{
		std::vector<HouseInfo>* houses;
		pBlackboard->GetData("DiscoveredHouses", houses);

		HouseInfo newHouse = (*houses)[houses->size() - 1];
		pBlackboard->ChangeData("HouseTarget", newHouse);
		pBlackboard->ChangeData("IsGoingToHouse", true);
	}

	return isNewHouseFound;
}
bool IsGoingTohouse(Elite::Blackboard* pBlackboard)
{
	bool isGoingToHouse = false;
	pBlackboard->GetData("IsGoingToHouse", isGoingToHouse);

	if (isGoingToHouse)
	{
		HouseInfo houseTarget{};
		pBlackboard->GetData("HouseTarget", houseTarget);

		AgentInfo agentInfo{};
		pBlackboard->GetData("AgentInfo", agentInfo);

		if (DistanceSquared(agentInfo.Position, houseTarget.Center) < 2.f)
		{
			pBlackboard->ChangeData("IsGoingToHouse", false);
		}
		
		pBlackboard->ChangeData("Target", houseTarget.Center);
	}

	return isGoingToHouse;
}
bool SeesItem(Elite::Blackboard* pBlackboard)
{
	std::list<EntityInfo>* itemsInFov = nullptr;
	pBlackboard->GetData("ItemsInFOV", itemsInFov);

	return itemsInFov->size() > 0;
}
BehaviorState PickupItem(Elite::Blackboard* pBlackboard)
{
	std::list<EntityInfo>* itemsInFov = nullptr;
	IExamInterface* pluginInterface = nullptr;
	AgentInfo agentInfo{};
	Inventory* inventory = nullptr;
	pBlackboard->GetData("ItemsInFOV", itemsInFov);
	pBlackboard->GetData("PluginInterface", pluginInterface);
	pBlackboard->GetData("AgentInfo", agentInfo);
	pBlackboard->GetData("Inventory", inventory);
	const float squaredGrabRange = exp2f(agentInfo.GrabRange);

	EntityInfo itemToGrab{};

	for (EntityInfo item : *itemsInFov)
	{
		ItemInfo itemInfo;
		pluginInterface->Item_GetInfo(item, itemInfo);

		if (inventory->currentGuns < inventory->maxGuns && itemInfo.Type == eItemType::PISTOL)
		{
			itemToGrab = item;
			break;
		}

		if (inventory->currentMedkits < inventory->maxMedkits && itemInfo.Type == eItemType::MEDKIT)
		{
			itemToGrab = item;
			break;
		}

		if (inventory->currentFood < inventory->maxFood && itemInfo.Type == eItemType::FOOD)
		{
			itemToGrab = item;
			break;
		}
	}

	if (itemToGrab.EntityHash != 0)
	{
		ItemInfo item;
		if (DistanceSquared(itemToGrab.Location, agentInfo.Position) < squaredGrabRange && pluginInterface->Item_Grab(itemToGrab, item))
		{
			unsigned int index = 0;
			switch (item.Type)
			{
			case eItemType::PISTOL:
				index = inventory->currentGuns;
				break;
			case eItemType::MEDKIT:
				index = inventory->currentMedkits;
				break;
			case eItemType::FOOD:
				index = inventory->currentFood;
				break;
			}

			unsigned int indexOffset = 0;
			switch (item.Type)
			{
			case eItemType::MEDKIT:
				indexOffset = inventory->maxGuns;
				break;
			case eItemType::FOOD:
				indexOffset = inventory->maxGuns + inventory->maxMedkits;
				break;
			}

			pluginInterface->Inventory_AddItem(indexOffset + index, item);
			switch (item.Type)
			{
			case eItemType::PISTOL:
				inventory->currentGuns++;
				break;
			case eItemType::MEDKIT:
				inventory->currentMedkits++;
				break;
			case eItemType::FOOD:
				inventory->currentFood++;
				break;
			}
		}

		pBlackboard->ChangeData("Target", itemToGrab.Location);
		return Success;
	}

	return Failure;
}

// MOVEMENT
BehaviorState Seek(Elite::Blackboard* pBlackboard)
{
	Vector2 targetPos{};
	AgentInfo agentInfo{};
	SteeringPlugin_Output output{};
	bool canRun = false;
	IExamInterface* pluginInterface = nullptr;

	bool dataAvailable = pBlackboard->GetData("Target", targetPos)
		&& pBlackboard->GetData("AgentInfo", agentInfo)
		&& pBlackboard->GetData("IsRunning", canRun)
		&& pBlackboard->GetData("PluginInterface", pluginInterface);

	if (!dataAvailable)
	{
		return Failure;
	}

	targetPos = pluginInterface->NavMesh_GetClosestPathPoint(targetPos);

	output.RunMode = canRun;
	output.LinearVelocity = targetPos - agentInfo.Position;
	output.LinearVelocity.Normalize();
	output.LinearVelocity *= agentInfo.MaxLinearSpeed;

	if (Distance(targetPos, agentInfo.Position) < 2.f)
	{
		output.LinearVelocity = Elite::ZeroVector2;
	}

	pBlackboard->ChangeData("SteeringOutput", output);
	return Success;
}

//BehaviorState ChangeToWander(Elite::Blackboard* pBlackboard)
//{
//	AgarioAgent* pAgent = nullptr;
//	auto dataAvailable = pBlackboard->GetData("Agent", pAgent);
//
//	if (!pAgent)
//		return Failure;
//
//	pAgent->SetToWander();
//
//	return Success;
//}
//
//BehaviorState ChangeToSeek(Elite::Blackboard* pBlackboard)
//{
//	AgarioAgent* pAgent = nullptr;
//	Vector2 seekTarget{};
//	auto dataAvailable = pBlackboard->GetData("Agent", pAgent) && 
//		pBlackboard->GetData("Target", seekTarget);
//
//
//	if (!pAgent || !dataAvailable)
//		return Failure;
//	
//	pAgent->SetToSeek(seekTarget);
//
//	return Success;
//}
//
//BehaviorState ChangeToFlee(Elite::Blackboard* pBlackboard)
//{
//	AgarioAgent* pAgent = nullptr;
//	Vector2 fleeTarget{};
//	auto dataAvailable = pBlackboard->GetData("Agent", pAgent) &&
//		pBlackboard->GetData("FleeTarget", fleeTarget);
//
//
//	if (!pAgent || !dataAvailable)
//		return Failure;
//
//	fleeTarget = pAgent->GetPosition() + ( (fleeTarget - pAgent->GetPosition()) * -1);
//
//	pAgent->SetToSeek(fleeTarget);
//
//	return Success;
//}

#endif