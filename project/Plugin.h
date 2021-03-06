#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"

using namespace Elite;
// Own Includes
#include "EBehaviorTree.h"
#include "EBlackboard.h"
#include "Stucts.h"
#include "Behaviors.h"

class IBaseInterface;
class IExamInterface;

class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;

	Elite::Vector2 target = {};
	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose


	// Own Additions
	Blackboard* m_pBlackboard = nullptr;
	BehaviorTree* m_pBehaviorTree = nullptr;
	std::vector<HouseInfo> m_DiscoveredHouses = {}; // might make a struct which has a time since last visited & have list of that instead

	std::list<EntityInfo> m_ItemsInFOV = {};
	std::list<EnemyInfo> m_EnemiesInFOV = {};
	std::list<PurgeZoneInfo> m_PurgeZoneInFOV = {};
	PurgeZoneInfo m_DangerousPurgeZone = {};
	
	Inventory m_DesiredInventoryCounts{};
	std::vector<ItemInfo> m_ItemMemory{};

	void AddHouseIfNew(const HouseInfo& houseInfo);
	void AssignEntitiesInFOV();
	void AddNewItemsToMemory();
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}