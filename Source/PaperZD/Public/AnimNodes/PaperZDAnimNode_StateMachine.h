// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "AnimNodes/PaperZDAnimNode_Base.h"
#include "PaperZDAnimNode_StateMachine.generated.h"

struct FPaperZDAnimStateMachine;
struct FPaperZDAnimStateMachineLink;
class UPaperZDAnimBPGeneratedClass;
class UPaperZDAnimSequence;

/**
 * Manages a FSM that can drive animations depending of different states and transition rules.
 */
USTRUCT()
struct PAPERZD_API FPaperZDAnimNode_StateMachine : public FPaperZDAnimNode_Base
{
	GENERATED_BODY()

	//Context to be passed around when evaluating if a node can be transitioned to
	struct FNodeEvaluationContext
	{
		UObject* AnimInstance;
		TSet<int32> VisitedNodes;

		//ctor
		FNodeEvaluationContext(UObject* InAnimInstance = nullptr)
			: AnimInstance(InAnimInstance)
		{}
	};

	// Updates the current animation node and makes sure that any transitional data is correctly set when the scope ends
	class FScopedAnimationUpdate
	{
		FPaperZDAnimNode_StateMachine* StateMachine;
		const FPaperZDAnimationUpdateContext UpdateContext;
		FDelegateHandle SequenceCompleteHandle;

	public:
		//ctor
		FScopedAnimationUpdate(FPaperZDAnimNode_StateMachine* InStateMachine, const FPaperZDAnimationUpdateContext& UpdateContext);
		~FScopedAnimationUpdate();

		/* Updates the animation. */
		void Update();
		
		/* Called when the transitional animation completes. */
		void OnTransitionalAnimationComplete(const UPaperZDAnimSequence* AnimSequence);
	};

	/* Index of the baked state machine definition on the generated class. */
	UPROPERTY()
	int32 StateMachineIndex;

	/* Cached state machine definition. */
	const FPaperZDAnimStateMachine* CachedStateMachine;

	/* Index of the current state. */
	int32 CurrentStateIndex;

	/* Accumulated time spent on the current state. */
	float CurrentStateTime;

	/* Current state's AnimNode that will be updated/evaluated. */
	FPaperZDAnimNode_Base* CurrentStateAnimNode;

	/* Optional AnimNode to the TransitionalGraph that is playing before the State. */
	FPaperZDAnimNode_Base* CurrentTransitionalAnimNode;

	/* If true, the transitional animation graph has completed and should be popped on the next frame. */
	bool bPopTransitionalAnimNode;

public:
	//ctor
	FPaperZDAnimNode_StateMachine();

	//~Begin FPaperZDAnimNode_Base Interface
	virtual void OnInitialize(const FPaperZDAnimationInitContext& InitContext) override;
	virtual void OnUpdate(const FPaperZDAnimationUpdateContext& UpdateContext) override;
	virtual void OnEvaluate(FPaperZDAnimationPlaybackData& OutData) override;
	//~End FPaperZDAnimNode_Base Interface

	/* Obtain the name of the state machine linked to this node. */
	FName GetMachineName() const;

	/* Takes the given JumpLink and forcefully sets the new target state to the JumpNode's target. Returns whether the jump was found and applied. */
	bool JumpToNode(FName Name, const FPaperZDAnimationBaseContext& Context);

private:
	/* Sets the given state, triggering any delegate and adding the state's AnimNode to the queue. */
	void SetState(int32 NewState, const FPaperZDAnimationBaseContext& Context);

	/* True if the given node can be entered (its a state or a valid conduit). */
	bool CanEnterNode(int32 NodeIndex, const FNodeEvaluationContext& EvaluationContext) const;

	/* Check the list of transitions on the given node and return whether any of the transitions can be taken. */
	const FPaperZDAnimStateMachineLink* CheckValidTransition(int32 NodeIndex, FNodeEvaluationContext& EvaluationContext) const;

	/* Returns the current AnimNode that should be updated/evaluated. */
	FPaperZDAnimNode_Base* GetCurrentAnimNode() const { return CurrentTransitionalAnimNode ? CurrentTransitionalAnimNode : CurrentStateAnimNode; }

	/* Calls the given event on the AnimInstance that owns this state machine. */
	void CallEvent(FName Name, const FPaperZDAnimationBaseContext& Context);
};
