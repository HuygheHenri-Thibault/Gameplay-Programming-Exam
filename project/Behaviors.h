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

	if (DistanceSquared(targetPos, agentInfo.Position) < exp2f(2.f))
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
	return Success;
}
BehaviorState Face(Elite::Blackboard* pBlackboard)
{
	Vector2 target{};
	AgentInfo agentInfo{};
	pBlackboard->GetData("Target", target);
	pBlackboard->GetData("AgentInfo", agentInfo);

	SteeringPlugin_Output output{};

	const Vector2 toTarget{ target - agentInfo.Position };

	const float angleTo{ atan2f(toTarget.y, toTarget.x) + float(E_PI_2) };
	float angleFrom{ agentInfo.Orientation };
	
	angleFrom = atan2f(sinf(angleFrom), cosf(angleFrom)); // makes angle between 0 & 360deg

	float deltaAngle = angleTo - angleFrom;

	constexpr float Pi2 = float(E_PI) * 2.f;
	if (deltaAngle > E_PI)
		deltaAngle -= Pi2;
	else if (deltaAngle < -E_PI)
		deltaAngle += Pi2;

	// multiply desired by some value to make it go as fast as possible (30.f)
	output.AngularVelocity = deltaAngle * 50.f;

	output.AutoOrient = false;

	pBlackboard->ChangeData("SteeringOutput", output);
	return Success;
}
BehaviorState StrafeAndTurn(Elite::Blackboard* pBlackboard)
{
	StrafeInfo strafeInfo{};
	AgentInfo agentInfo{};
	pBlackboard->GetData("StrafeInfo", strafeInfo);
	pBlackboard->GetData("AgentInfo", agentInfo);

	if (!strafeInfo.isStrafing)
	{
		strafeInfo.startOrientation = agentInfo.Orientation;
		strafeInfo.endOrientation = agentInfo.Orientation + float(E_PI); // +180deg
		strafeInfo.startLinearVelocity = agentInfo.LinearVelocity;
		strafeInfo.isStrafing = true;
		pBlackboard->ChangeData("StrafeInfo", strafeInfo);
	}
	else
	{
		if (AreEqual(agentInfo.Orientation, strafeInfo.endOrientation, 0.01f))
		{
			strafeInfo.isStrafing = false;
			pBlackboard->ChangeData("StrafeInfo", strafeInfo);
			return Success;
		}
	}

	// seek in the direction we were going
	Vector2 seekTarget = agentInfo.Position + (strafeInfo.startLinearVelocity * 5);
	pBlackboard->ChangeData("Target", seekTarget);
	Seek(pBlackboard);

	SteeringPlugin_Output seekOutput{};
	pBlackboard->GetData("SteeringOutput", seekOutput);

	// get point to face to
	Vector2 faceTarget = agentInfo.Position + Elite::OrientationToVector(strafeInfo.endOrientation);
	pBlackboard->ChangeData("Target", faceTarget);
	Face(pBlackboard);

	SteeringPlugin_Output output{};
	pBlackboard->GetData("SteeringOutput", output);
	
	output.LinearVelocity = seekOutput.LinearVelocity;
	pBlackboard->ChangeData("SteeringOutput", output);
	return Success;
}

// Decision making & Actions
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

#pragma region Delete Item from memory and fetch
			// Remove item from memory & reset fetch
			pBlackboard->ChangeData("ItemBeingFetched", ItemInfo{});
			RemoveItemFromMemory(item, pBlackboard);
#pragma endregion

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
bool SeesGarbage(Elite::Blackboard* pBlackboard)
{
	std::list<EntityInfo>* itemsInFov = nullptr;
	IExamInterface* pInterface = nullptr;

	pBlackboard->GetData("ItemsInFOV", itemsInFov);
	pBlackboard->GetData("PluginInterface", pInterface);

	for (EntityInfo& e : (*itemsInFov))
	{
		ItemInfo item{};
		pInterface->Item_GetInfo(e, item);
		if (item.Type == eItemType::GARBAGE)
		{
			pBlackboard->ChangeData("GarbageSeen", item);
			return true;
		}
	}
	return false;
}
bool GarbageIsInGrabRange(Elite::Blackboard* pBlackboard)
{
	ItemInfo garbageItem{};
	AgentInfo agentInfo{};
	pBlackboard->GetData("GarbageSeen", garbageItem);
	pBlackboard->GetData("AgentInfo", agentInfo);
	

	if (garbageItem.ItemHash != 0)
	{
		const float sqrGrabRange = exp2f(agentInfo.GrabRange);
		if (DistanceSquared(agentInfo.Position, garbageItem.Location) < sqrGrabRange)
		{
			return true;
		}
	}
	return false;
}
BehaviorState DestroyGarbageInRange(Elite::Blackboard* pBlackboard)
{
	ItemInfo garbageItem{};
	AgentInfo agentInfo{};
	IExamInterface* pInterface = nullptr;
	std::list<EntityInfo>* itemsInFov = nullptr;

	pBlackboard->GetData("GarbageSeen", garbageItem);
	pBlackboard->GetData("AgentInfo", agentInfo);
	pBlackboard->GetData("PluginInterface", pInterface);
	pBlackboard->GetData("ItemsInFOV", itemsInFov);

	if (garbageItem.ItemHash == 0)
	{
		return Failure;
	}


	for (EntityInfo& e : (*itemsInFov))
	{
		if (e.Location == garbageItem.Location)
		{
			ItemInfo item{};
			pInterface->Item_GetInfo(e, item);
			RemoveItemFromMemory(item, pBlackboard);

			pInterface->Item_Destroy(e);
			pBlackboard->ChangeData("GarbageSeen", ItemInfo{});
			return Success;
		}
	}
	return Failure;
}
BehaviorState SetGarbageAsTarget(Elite::Blackboard* pBlackboard)
{
	ItemInfo garbageItem{};
	pBlackboard->GetData("GarbageSeen", garbageItem);

	if (garbageItem.ItemHash == 0)
	{
		return Failure;
	}

	pBlackboard->ChangeData("Target", garbageItem.Location);
	return Success;
}

bool IsInNeedOfItem(Elite::Blackboard* pBlackboard)
{
	Inventory* inventory = nullptr;
	pBlackboard->GetData("Inventory", inventory);

	bool inNeedOfPistol = inventory->currentGuns < inventory->maxGuns;
	bool inNeedOfMedkit = inventory->currentMedkits < inventory->maxMedkits;
	bool inNeedOfFood = inventory->currentFood < inventory->maxFood;

	return inNeedOfPistol || inNeedOfMedkit || inNeedOfFood;
}
bool IsANeededItemClose(Elite::Blackboard* pBlackboard)
{
	Inventory* inventory = nullptr;
	std::vector<ItemInfo>* pItemMemory = nullptr;
	float itemFetchMaxRange = 0;
	AgentInfo agentInfo{};

	pBlackboard->GetData("Inventory", inventory);
	pBlackboard->GetData("ItemMemory", pItemMemory);
	pBlackboard->GetData("ItemFetchMaxRange", itemFetchMaxRange);
	pBlackboard->GetData("AgentInfo", agentInfo);

	const float sqrMaxRange = exp2f(itemFetchMaxRange);

	bool inNeedOfPistol = inventory->currentGuns < inventory->maxGuns;
	bool inNeedOfMedkit = inventory->currentMedkits < inventory->maxMedkits;
	bool inNeedOfFood = inventory->currentFood < inventory->maxFood;

	ItemInfo itemInRange{};

	for (ItemInfo& item : (*pItemMemory))
	{
		if (DistanceSquared(item.Location, agentInfo.Position) > sqrMaxRange)
		{
			continue;
		}

		if (inNeedOfPistol && item.Type == eItemType::PISTOL)
		{
			itemInRange = item;
			break;
		}
		else if (inNeedOfMedkit && item.Type == eItemType::MEDKIT)
		{
			itemInRange = item;
			break;
		}
		else if (inNeedOfFood && item.Type == eItemType::FOOD)
		{
			itemInRange = item;
			break;
		}
	}

	if (itemInRange.ItemHash != 0)
	{
		pBlackboard->ChangeData("ItemBeingFetched", itemInRange);
		return true;
	}
	return false;
}
BehaviorState SetNeededItemAsTarget(Elite::Blackboard* pBlackboard)
{
	ItemInfo neededItem{};
	pBlackboard->GetData("ItemBeingFetched", neededItem);

	if (neededItem.ItemHash == 0)
	{
		return Failure;
	}

	pBlackboard->ChangeData("Target", neededItem.Location);
	return Success;
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
	pBlackboard->ChangeData("IsRunning", true);
	Flee(pBlackboard);

	return Success;
}

bool IsZombieInFOV(Elite::Blackboard* pBlackboard)
{
	std::list<EnemyInfo>* enemiesInFOV = nullptr;
	pBlackboard->GetData("EnemiesInFOV", enemiesInFOV);

	return enemiesInFOV->size() > 0;
}
bool IsArmed(Elite::Blackboard* pBlackboard)
{
	Inventory* inventory = nullptr;
	pBlackboard->GetData("Inventory", inventory);

	return inventory->currentGuns > 0;
}
bool IsFacingEnemy(Elite::Blackboard* pBlackboard)
{
	std::list<EnemyInfo>* enemiesInFOV = nullptr;
	AgentInfo agentInfo{};
	SteeringPlugin_Output lastSteering{};

	pBlackboard->GetData("EnemiesInFOV", enemiesInFOV);
	pBlackboard->GetData("AgentInfo", agentInfo);
	pBlackboard->GetData("SteeringOutput", lastSteering);

	if (enemiesInFOV->size() == 0)
	{
		return Failure;
	}

	EnemyInfo& targetEnemy = enemiesInFOV->front();

	Vector2 toTargetNormal = (targetEnemy.Location - agentInfo.Position).GetNormalized();
	Vector2 heading = OrientationToVector(agentInfo.Orientation);

	const float dotResult = heading.Dot(toTargetNormal);

	const float longDistanceSquared = exp2f((agentInfo.FOV_Range / 3) * 2);
	if (DistanceSquared(agentInfo.Position, targetEnemy.Location) > longDistanceSquared)
	{
		return abs(dotResult - 1.f) < 0.005f; // long range needs a narrower margin, at close range this causes jittering
	}

	return abs(dotResult - 1.f) < 0.008f;
}
bool IsBitten(Elite::Blackboard* pBlackboard)
{
	AgentInfo agentInfo{};

	pBlackboard->GetData("AgentInfo", agentInfo);

	return agentInfo.Bitten;
}
bool WasBitten(Elite::Blackboard* pBlackboard)
{
	AgentInfo agentInfo{};

	pBlackboard->GetData("AgentInfo", agentInfo);

	return agentInfo.WasBitten;
}
bool IsStrafing(Elite::Blackboard* pBlackboard)
{
	StrafeInfo strafeInfo{};
	pBlackboard->GetData("StrafeInfo", strafeInfo);
	
	return strafeInfo.isStrafing;
}
BehaviorState Shoot(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pluginInterface = nullptr;
	AgentInfo agentInfo{};
	Inventory* inventory = nullptr;
	pBlackboard->GetData("PluginInterface", pluginInterface);
	pBlackboard->GetData("AgentInfo", agentInfo);
	pBlackboard->GetData("Inventory", inventory);

	if (inventory->currentGuns == 0)
	{
		return Failure;
	}

	if (IsStrafing(pBlackboard))
	{
		StrafeInfo strafeInfo{};
		pBlackboard->ChangeData("StrafeInfo", strafeInfo);
	}

	unsigned int lowestAmmo = 100;
	int indexOfWeaponWithLeastAmmo = -1;
	for (unsigned int i = 0; i < inventory->maxGuns; i++)
	{
		ItemInfo& itemAtIndex = inventory->inventorySlots[i];

		if (itemAtIndex.ItemHash != 0)
		{
			unsigned int weaponAmmo = pluginInterface->Weapon_GetAmmo(itemAtIndex);
			if (weaponAmmo < lowestAmmo)
			{
				lowestAmmo = weaponAmmo;
				indexOfWeaponWithLeastAmmo = i;
			}
		}
	}

	if (indexOfWeaponWithLeastAmmo < 0)
	{
		return Failure;
	}

	ItemInfo& weaponToUse = inventory->inventorySlots[indexOfWeaponWithLeastAmmo];
	pluginInterface->Inventory_UseItem(indexOfWeaponWithLeastAmmo);
	if (pluginInterface->Weapon_GetAmmo(weaponToUse) == 0)
	{
		pluginInterface->Inventory_RemoveItem(indexOfWeaponWithLeastAmmo);
		inventory->inventorySlots[indexOfWeaponWithLeastAmmo] = ItemInfo{};
		inventory->currentGuns--;
	}
	return Success;
}
BehaviorState SetEnemyAsTarget(Elite::Blackboard* pBlackboard)
{
	std::list<EnemyInfo>* enemiesInFOV = nullptr;

	pBlackboard->GetData("EnemiesInFOV", enemiesInFOV);

	if (enemiesInFOV->size() <= 0)
	{
		return Failure;
	}

	EnemyInfo& targetEnemy = enemiesInFOV->front();

	pBlackboard->ChangeData("Target", targetEnemy.Location);
	return Success;
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

	if (WasBitten(pBlackboard))
	{
		pBlackboard->ChangeData("IsRunning", true);
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

BehaviorState SetHouseAsTarget(Elite::Blackboard* pBlackboard)
{
	std::vector<HouseInfo>* pDiscoveredHouses = nullptr;
	int lastHouseIndex = 0;
	AgentInfo agentInfo{};
	pBlackboard->GetData("DiscoveredHouses", pDiscoveredHouses);
	pBlackboard->GetData("LastHouseTargetIndex", lastHouseIndex);
	pBlackboard->GetData("AgentInfo", agentInfo);

	const bool isCloseEnough = DistanceSquared((*pDiscoveredHouses)[lastHouseIndex].Center, agentInfo.Position) < 10.f;

	if (isCloseEnough)
	{
		lastHouseIndex++;
		if (lastHouseIndex == (*pDiscoveredHouses).size())
		{
			lastHouseIndex = 0;
		}
		pBlackboard->ChangeData("LastHouseTargetIndex", lastHouseIndex);
	}

	pBlackboard->ChangeData("Target", (*pDiscoveredHouses)[lastHouseIndex].Center);

	return Success;
}

// Helpers //
void RemoveItemFromMemory(const ItemInfo& item, Elite::Blackboard* pBlackboard)
{
	std::vector<ItemInfo>* pItemMemory = nullptr;
	pBlackboard->GetData("ItemMemory", pItemMemory);

	for (ItemInfo& itemInMemory : (*pItemMemory))
	{
		if (itemInMemory.Location == item.Location)
		{
			itemInMemory = (*pItemMemory)[(*pItemMemory).size() - 1];
			pItemMemory->pop_back();
			break;
		}
	}
}
#endif