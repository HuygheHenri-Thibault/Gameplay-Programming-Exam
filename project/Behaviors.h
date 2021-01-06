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
bool IsNotDoneExploring(Elite::Blackboard* pBlackboard)
{
	return !IsDoneExploring(pBlackboard);
}
bool IsDoneExploring(Elite::Blackboard* pBlackboard)
{
	WorldInfo worldInfo{};
	ExpandingSearchData searchData{};

	pBlackboard->GetData("WorldInfo", worldInfo);
	pBlackboard->GetData("ExpandingSquareSearchData", searchData);

	const float halfWidth = worldInfo.Dimensions.x / 2;
	const float halfHeight = worldInfo.Dimensions.y / 2;

	if (searchData.lastSearchPosition.x < worldInfo.Center.x - halfWidth
		|| worldInfo.Center.x + halfWidth < searchData.lastSearchPosition.x)
	{
		return true;
	}

	if (searchData.lastSearchPosition.y < worldInfo.Center.y - halfHeight
		|| worldInfo.Center.y + halfHeight < searchData.lastSearchPosition.y)
	{
		return true;
	}

	return false;
}
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

	for (EntityInfo& item : *itemsInFov) // TODO: move this to own conditional?
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
			inventory->inventorySlots[indexOffset + index] = item;

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

bool IsHurt(Elite::Blackboard* pBlackboard)
{
	const float maxHealth = 10.f; // hardcoded cause no var for it
	AgentInfo agentInfo{};

	pBlackboard->GetData("AgentInfo", agentInfo);

	return (maxHealth - agentInfo.Health) > 0.0001f;
}
bool ShouldUseMedkit(Elite::Blackboard* pBlackboard)
{
	const float maxHealth = 10.f;
	AgentInfo agentInfo{};
	Inventory* inventory = nullptr;
	IExamInterface* pluginInterface = nullptr;

	pBlackboard->GetData("AgentInfo", agentInfo);
	pBlackboard->GetData("Inventory", inventory);
	pBlackboard->GetData("PluginInterface", pluginInterface);

	if (inventory->currentMedkits != 0)
	{
		const float damageTaken = maxHealth - agentInfo.Health;

		for (unsigned int i = inventory->maxGuns; i < (inventory->maxGuns + inventory->maxMedkits); i++)
		{
			if (inventory->inventorySlots[i].ItemHash != 0)
			{
				const int healthRestored = pluginInterface->Medkit_GetHealth(inventory->inventorySlots[i]);
				if (healthRestored < damageTaken)
				{
					pBlackboard->ChangeData("MedkitToUse", int(i));
					return true;
				}
			}
		}
		pBlackboard->ChangeData("MedkitToUse", -1);
		return false;
	}
	else
	{
		pBlackboard->ChangeData("MedkitToUse", -1);
		return false;
	}
}
BehaviorState UseMedkit(Elite::Blackboard* pBlackboard)
{
	int indexOfMedkitToUse = 0;
	Inventory* inventory = nullptr;
	IExamInterface* pluginInterface = nullptr;

	pBlackboard->GetData("MedkitToUse", indexOfMedkitToUse);
	pBlackboard->GetData("Inventory", inventory);
	pBlackboard->GetData("PluginInterface", pluginInterface);

	if (unsigned int(indexOfMedkitToUse) < inventory->maxGuns 
		|| unsigned int(indexOfMedkitToUse) >= inventory->maxGuns + inventory->maxMedkits 
		|| inventory->inventorySlots[indexOfMedkitToUse].ItemHash == 0)
	{
		// if index isn't a medkit index or if the item at the index doesn't exist
		return Failure;
	}

	pluginInterface->Inventory_UseItem(indexOfMedkitToUse);
	pluginInterface->Inventory_RemoveItem(indexOfMedkitToUse);
	inventory->inventorySlots[indexOfMedkitToUse] = ItemInfo{};
	inventory->currentMedkits--;
	pBlackboard->ChangeData("MedkitToUse", -1);

	return Success;
}

bool IsHungry(Elite::Blackboard* pBlackboard)
{
	const float maxEnergy = 10.f;
	AgentInfo agentInfo{};

	pBlackboard->GetData("AgentInfo", agentInfo);

	return (maxEnergy - agentInfo.Energy) > 0.0001f;
}
bool ShouldEat(Elite::Blackboard* pBlackboard)
{
	const float maxEnergy = 10.f;
	const unsigned int foodIndex = 4;
	AgentInfo agentInfo{};
	Inventory* inventory = nullptr;
	IExamInterface* pluginInterface = nullptr;

	pBlackboard->GetData("AgentInfo", agentInfo);
	pBlackboard->GetData("Inventory", inventory);
	pBlackboard->GetData("PluginInterface", pluginInterface);

	if (inventory->currentFood != 0)
	{
		const float energyNeed = maxEnergy - agentInfo.Energy;

		if (inventory->inventorySlots[foodIndex].ItemHash != 0)
		{
			const int energyRestored = pluginInterface->Food_GetEnergy(inventory->inventorySlots[foodIndex]);
			if (energyRestored < energyNeed)
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		return false;
	}
}
BehaviorState UseFood(Elite::Blackboard* pBlackboard)
{
	int indexOfFoodToUse = 4;
	Inventory* inventory = nullptr;
	IExamInterface* pluginInterface = nullptr;

	pBlackboard->GetData("Inventory", inventory);
	pBlackboard->GetData("PluginInterface", pluginInterface);

	if (inventory->inventorySlots[indexOfFoodToUse].ItemHash == 0)
	{
		return Failure;
	}

	pluginInterface->Inventory_UseItem(indexOfFoodToUse);
	pluginInterface->Inventory_RemoveItem(indexOfFoodToUse);
	inventory->inventorySlots[indexOfFoodToUse] = ItemInfo{};
	inventory->currentFood--;

	return Success;
}

bool SeesPurgeZone(Elite::Blackboard* pBlackboard)
{
	std::list<PurgeZoneInfo>* purgeZoneInFOV = nullptr;
	pBlackboard->GetData("PurgeZonesInFOV", purgeZoneInFOV);

	return purgeZoneInFOV->size() > 0;
}
bool IsInPurgeZone(Elite::Blackboard* pBlackboard)
{
	std::list<PurgeZoneInfo>* purgeZoneInFOV = nullptr;
	PurgeZoneInfo* dangerousPurgeZone = nullptr;
	AgentInfo agentInfo{};

	pBlackboard->GetData("PurgeZonesInFOV", purgeZoneInFOV);
	pBlackboard->GetData("DangerousPurgeZone", dangerousPurgeZone);
	pBlackboard->GetData("AgentInfo", agentInfo);

	bool isInsidePurgeZone = false;

	for (PurgeZoneInfo& purgeZone : (*purgeZoneInFOV))
	{
		if (DistanceSquared(purgeZone.Center, agentInfo.Position) < exp2f(purgeZone.Radius))
		{
			dangerousPurgeZone = &purgeZone;
			isInsidePurgeZone = true;
		}
	}

	return isInsidePurgeZone;
}
BehaviorState LeavePurgeZone(Elite::Blackboard* pBlackboard)
{
	AgentInfo agentInfo{};
	PurgeZoneInfo* dangerousPurgeZone = nullptr;
	pBlackboard->GetData("DangerousPurgeZone", dangerousPurgeZone);
	pBlackboard->GetData("AgentInfo", agentInfo);

	if (!dangerousPurgeZone)
	{
		dangerousPurgeZone = nullptr;
		return Failure;
	}

	pBlackboard->ChangeData("Target", dangerousPurgeZone->Center);
	Flee(pBlackboard);

	return Success;
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
BehaviorState Flee(Elite::Blackboard* pBlackboard)
{
	Seek(pBlackboard);

	SteeringPlugin_Output output{};
	pBlackboard->GetData("SteeringOutput", output);

	output.LinearVelocity *= -1;

	pBlackboard->ChangeData("SteeringOutput", output);
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