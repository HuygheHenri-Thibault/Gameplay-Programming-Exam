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
using namespace Elite;
//-----------------------------------------------------------------
// Behaviors
//-----------------------------------------------------------------

BehaviorState ExpandingSquareSearch(Elite::Blackboard* pBlackboard)
{	
	const float squaredSearchDistanceMargin = 0.1f;
	ExpandingSearchData searchData;
	AgentInfo agentInfo;

	bool dataAvailable = pBlackboard->GetData("ExpandingSquareSearchData", searchData)
		&& pBlackboard->GetData("AgentInfo", agentInfo);

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

	pBlackboard->ChangeData("Target", searchData.lastSearchPosition);
	return Success;
}


// MOVEMENT
BehaviorState Seek(Elite::Blackboard* pBlackboard)
{
	Vector2 targetPos{};
	AgentInfo agentInfo{};
	SteeringPlugin_Output output{};
	bool canRun = false;

	bool dataAvailable = pBlackboard->GetData("Target", targetPos)
		&& pBlackboard->GetData("AgentInfo", agentInfo)
		&& pBlackboard->GetData("IsRunning", canRun);

	if (!dataAvailable)
	{
		return Failure;
	}

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