#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	info.BotName = "BestBot";
	info.Student_FirstName = "Henri-Thibault";
	info.Student_LastName = "Huyghe";
	info.Student_Class = "2DAE01";

	m_pBlackboard = new Blackboard();
	m_pBlackboard->AddData("SteeringOutput", SteeringPlugin_Output{});
	m_pBlackboard->AddData("IsRunning", false);
	m_pBlackboard->AddData("StrafeInfo", StrafeInfo{});
	m_pBlackboard->AddData("Target", Elite::Vector2{0,0});
	m_pBlackboard->AddData("PluginInterface", m_pInterface);
	m_pBlackboard->AddData("WorldInfo", m_pInterface->World_GetInfo());
	m_pBlackboard->AddData("AgentInfo", m_pInterface->Agent_GetInfo());

	// Exploring & Houses
	m_pBlackboard->AddData("ExpandingSquareSearchData", ExpandingSearchData{ 25.f, 0, {0,0} });
	m_pBlackboard->AddData("DiscoveredHouses", &m_DiscoveredHouses);
	m_pBlackboard->AddData("LastHouseTargetIndex", 0);
	m_pBlackboard->AddData("IsNewHouseDiscovered", false);
	m_pBlackboard->AddData("IsGoingToHouse", false);
	m_pBlackboard->AddData("HouseTarget", HouseInfo{});

	// Entities
	m_pBlackboard->AddData("ItemsInFOV", &m_ItemsInFOV);
	m_pBlackboard->AddData("EnemiesInFOV", &m_EnemiesInFOV);
	m_pBlackboard->AddData("PurgeZonesInFOV", &m_PurgeZoneInFOV);
	m_pBlackboard->AddData("DangerousPurgeZone", &m_DangerousPurgeZone);

	// Inventory
	m_DesiredInventoryCounts.maxGuns = 2;
	m_DesiredInventoryCounts.maxMedkits = 2;
	m_DesiredInventoryCounts.maxFood = 1;
	m_DesiredInventoryCounts.inventorySlots.resize(m_pInterface->Inventory_GetCapacity());

	m_pBlackboard->AddData("Inventory", &m_DesiredInventoryCounts);
	m_pBlackboard->AddData("MedkitToUse", -1);
	m_pBlackboard->AddData("GunToUse", -1);
	m_pBlackboard->AddData("GarbageSeen", ItemInfo{});
	m_pBlackboard->AddData("ItemMemory", &m_ItemMemory);
	m_pBlackboard->AddData("ItemFetchMaxRange", 75.f);
	m_pBlackboard->AddData("ItemBeingFetched", ItemInfo{});

	m_pBehaviorTree = new BehaviorTree(m_pBlackboard,
		new BehaviorSelector(
			{
#pragma region Item Use
				new BehaviorSequence(
					{
						new BehaviorConditional(IsHurt),
						new BehaviorConditional(ShouldUseMedkit),
						new BehaviorAction(UseMedkit)
					}
				),
				new BehaviorSequence(
					{
						new BehaviorConditional(IsHungry),
						new BehaviorConditional(ShouldEat),
						new BehaviorAction(UseFood)
					}
				),
#pragma endregion
				new BehaviorSequence(
					{
						new BehaviorConditional(IsInPurgeZone),
						new BehaviorAction(LeavePurgeZone),
					}
				),
#pragma region Zombie Killing
				new BehaviorSequence(
					{
						new BehaviorConditional(IsZombieInFOV),
						new BehaviorSelector(
							{
								new BehaviorSequence(
									{
										new BehaviorConditional(IsArmed),
										new BehaviorSelector(
											{
												new BehaviorSequence(
													{
														new BehaviorConditional(IsFacingEnemy),
														new BehaviorAction(Shoot)
													}
												),
												new BehaviorSequence(
													{
														new BehaviorInvertedConditional(IsFacingEnemy),
														new BehaviorAction(SetEnemyAsTarget),
														new BehaviorAction(Face)
													}
												)
											}
										)
									}
								)
							}
						),
						
					}
				),
				new BehaviorSequence(
					{
						new BehaviorConditional(IsStrafing),
						new BehaviorAction(StrafeAndTurn)
					}
				),
				new BehaviorSequence(
					{
						new BehaviorConditional(WasBitten),
						new BehaviorInvertedConditional(IsZombieInFOV),
						new BehaviorConditional(IsArmed),
						new BehaviorAction(StrafeAndTurn)
					}
				),

#pragma endregion				
#pragma region Looting
				new BehaviorSequence(
					{
						new BehaviorConditional(SeesItem),
						new BehaviorAction(PickupItem),
						new BehaviorAction(Seek)
					}
				),
				new BehaviorSequence(
					{
						new BehaviorConditional(SeesGarbage),
						new BehaviorSelector(
							{
								new BehaviorSequence(
									{
										new BehaviorConditional(GarbageIsInGrabRange),
										new BehaviorAction(DestroyGarbageInRange)
									}
								),
								new BehaviorSequence(
									{
										new BehaviorInvertedConditional(GarbageIsInGrabRange),
										new BehaviorAction(SetGarbageAsTarget),
										new BehaviorAction(Seek)
									}
								),
							}
						),
						
					}
				),
				new BehaviorSequence(
					{
						new BehaviorConditional(IsInNeedOfItem),
						new BehaviorConditional(IsANeededItemClose),
						new BehaviorAction(SetNeededItemAsTarget),
						new BehaviorAction(Seek)
					}
				),
#pragma endregion
#pragma region House
				new BehaviorSequence(
					{
						new BehaviorConditional(IsGoingTohouse),
						new BehaviorAction(Seek)
					}
				),
				new BehaviorSequence(
					{
						new BehaviorConditional(IsNewHouseDiscovered),
						new BehaviorAction(Seek)
					}
				),
#pragma endregion
				new BehaviorSequence(
					{
						new BehaviorInvertedConditional(IsDoneExploring),
						new BehaviorAction(ExpandingSquareSearch),
						new BehaviorAction(Seek)
					}
				),
				new BehaviorSequence(
					{
						new BehaviorAction(SetHouseAsTarget),
						new BehaviorAction(Seek)
					}
				)
			// going from house to house once there is nothing to explore anymore
			}
		)
	);
}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded
}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded
}

#pragma region Debug
//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be usefull to inspect certain behaviours (Default = false)
	params.AutoGrabClosestItem = true; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	Vector2 targetPos{};
	m_pBlackboard->GetData("Target", targetPos);
	m_pInterface->Draw_SolidCircle(targetPos, .7f, { 0,0 }, { 1, 0, 0 });
	
	AgentInfo agentInfo{};
	float maxFetchRange{};
	m_pBlackboard->GetData("AgentInfo", agentInfo);
	m_pBlackboard->GetData("ItemFetchMaxRange", maxFetchRange);

	m_pInterface->Draw_Circle(agentInfo.Position, maxFetchRange, { 1, 0, 0 });
}
#pragma endregion

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	// Reset Data
	m_pBlackboard->ChangeData("IsNewHouseDiscovered", false);
	m_pBlackboard->ChangeData("IsRunning", false);
	m_ItemsInFOV.clear();
	m_EnemiesInFOV.clear();
	m_PurgeZoneInFOV.clear();

	AssignEntitiesInFOV();
	AddNewItemsToMemory();

	auto steering = SteeringPlugin_Output();

	auto agentInfo = m_pInterface->Agent_GetInfo();
	m_pBlackboard->ChangeData("AgentInfo", agentInfo);

	auto vHousesInFOV = GetHousesInFOV();
	auto vEntitiesInFOV = GetEntitiesInFOV();

	for (auto& houseInFOV : vHousesInFOV)
	{
		AddHouseIfNew(houseInFOV);
	}

	m_pBehaviorTree->Update(dt);

	m_pBlackboard->GetData("SteeringOutput", steering);

	//Reset State
	m_GrabItem = false; 
	m_UseItem = false;
	m_RemoveItem = false;

	return steering;
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}
vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}

void Plugin::AddHouseIfNew(const HouseInfo& houseInfo)
{
	const float distanceMargin = 0.001f;
	bool IsHouseInList = false;
	
	for (const HouseInfo& houseInList : m_DiscoveredHouses)
	{
		if (houseInList.Center == houseInfo.Center && houseInList.Size == houseInfo.Size)
		{
			IsHouseInList = true;
			break;
		}
	}

	if (!IsHouseInList)
	{
		m_DiscoveredHouses.push_back(houseInfo);
		m_pBlackboard->ChangeData("IsNewHouseDiscovered", true);
	}
}
void Plugin::AssignEntitiesInFOV()
{
	auto vEntitiesInFOV = GetEntitiesInFOV();

	for (auto& e : vEntitiesInFOV)
	{
		if (e.Type == eEntityType::ITEM)
		{
			//ItemInfo itemInfo;
			//m_pInterface->Item_GetInfo(e, itemInfo);
			m_ItemsInFOV.push_back(e);
		}

		if (e.Type == eEntityType::ENEMY)
		{
			EnemyInfo enemyInfo;
			m_pInterface->Enemy_GetInfo(e, enemyInfo);
			m_EnemiesInFOV.push_back(enemyInfo);
		}

		if (e.Type == eEntityType::PURGEZONE)
		{
			PurgeZoneInfo zoneInfo;
			m_pInterface->PurgeZone_GetInfo(e, zoneInfo);
			m_PurgeZoneInFOV.push_back(zoneInfo);
		}
	}
}
void Plugin::AddNewItemsToMemory()
{
	for (EntityInfo& e : m_ItemsInFOV)
	{
		ItemInfo item{};
		m_pInterface->Item_GetInfo(e, item);

		bool isItemInMemory = false;
		for(ItemInfo& itemInMemory : m_ItemMemory)
		{
			if (itemInMemory.Location == item.Location)
			{
				isItemInMemory = true;
			}
		};

		if (!isItemInMemory)
		{
			m_ItemMemory.push_back(item);
		}
	}
}

