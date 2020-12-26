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
//#include "../Shared/Agario/AgarioAgent.h"
//#include "../Shared/Agario/AgarioFood.h"
//#include "../App_Steering/SteeringBehaviors.h"
using namespace Elite;
//-----------------------------------------------------------------
// Behaviors
//-----------------------------------------------------------------

//bool IsCloseToFood(Elite::Blackboard* pBlackboard)
//{
//	AgarioAgent* pAgent = nullptr;
//	std::vector<AgarioFood*>* foodVec = nullptr;
//
//	auto dataAvailable = pBlackboard->GetData("Agent", pAgent) &&
//		pBlackboard->GetData("FoodVec", foodVec);
//
//	if (!pAgent || !foodVec || !dataAvailable)
//		return false;
//
//	const float closeToRange = 20.f;
//	auto foodItr = std::find_if(foodVec->begin(), foodVec->end(), 
//		[pAgent, closeToRange](AgarioFood* food)
//		{
//			//return true if close to agent
//			return DistanceSquared(pAgent->GetPosition(), food->GetPosition()) < pow(closeToRange, 2);
//		}
//	);
//
//	if (foodItr != foodVec->end())
//	{
//		pBlackboard->ChangeData("Target", (*foodItr)->GetPosition());
//		return true;
//	}
//	
//	return false;
//}
//
//bool IsThereAnyFood(Elite::Blackboard* pBlackboard)
//{
//	AgarioAgent* pAgent = nullptr;
//	std::vector<AgarioFood*>* foodVec = nullptr;
//
//	auto dataAvailable = pBlackboard->GetData("Agent", pAgent) &&
//		pBlackboard->GetData("FoodVec", foodVec);
//
//	if (!pAgent || !foodVec || !dataAvailable)
//		return false;
//
//	float closestFoodSquaredDistance = FLT_MAX;
//	AgarioFood* closestFood = nullptr;
//	for (AgarioFood* food : (*foodVec))
//	{
//		if (DistanceSquared(pAgent->GetPosition(), food->GetPosition()) < closestFoodSquaredDistance)
//		{
//			closestFood = food;
//		}
//	}
//	if (closestFood != nullptr)
//	{
//		pBlackboard->ChangeData("Target", closestFood->GetPosition());
//		return true;
//	}
//	
//	return false;
//}
//
//bool IsCloseToPrey(Elite::Blackboard* pBlackboard)
//{
//	AgarioAgent* pAgent = nullptr;
//	std::vector<AgarioAgent*>* enemyVec = nullptr;
//
//	auto dataAvailable = pBlackboard->GetData("Agent", pAgent) &&
//		pBlackboard->GetData("AgentsVec", enemyVec);
//
//	if (!pAgent || !enemyVec || !dataAvailable)
//		return false;
//
//	const float closeToRange = 30.f;
//	const float radiusAdjust = 5.f;
//	auto enemyItr = std::find_if(enemyVec->begin(), enemyVec->end(),
//		[pAgent, closeToRange, radiusAdjust](AgarioAgent* enemy)
//		{
//			//return true if close to agent
//			return DistanceSquared(pAgent->GetPosition(), enemy->GetPosition()) < pow(closeToRange, 2)
//				&& pAgent->GetRadius() > enemy->GetRadius() + radiusAdjust;
//		}
//	);
//
//	if (enemyItr != enemyVec->end())
//	{
//		pBlackboard->ChangeData("Target", (*enemyItr)->GetPosition());
//		return true;
//	}
//
//	return false;
//}
//
//bool IsDangerousEnemyClose(Elite::Blackboard* pBlackboard)
//{
//	AgarioAgent* pAgent = nullptr;
//	std::vector<AgarioAgent*>* enemyVec = nullptr;
//
//	auto dataAvailable = pBlackboard->GetData("Agent", pAgent) &&
//		pBlackboard->GetData("AgentsVec", enemyVec);
//
//	if (!pAgent || !enemyVec || !dataAvailable)
//		return false;
//
//	const float closeToRange = pAgent->GetRadius() + 20.f;
//	auto enemyItr = std::find_if(enemyVec->begin(), enemyVec->end(),
//		[pAgent, closeToRange](AgarioAgent* enemy)
//		{
//			//return true if close to agent
//			return DistanceSquared(pAgent->GetPosition(), enemy->GetPosition()) < pow(closeToRange, 2)
//				&& pAgent->GetRadius() < enemy->GetRadius();
//		}
//	);
//
//	if (enemyItr != enemyVec->end())
//	{
//		pBlackboard->ChangeData("FleeTarget", (*enemyItr)->GetPosition());
//		return true;
//	}
//
//	return false;
//}
//
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