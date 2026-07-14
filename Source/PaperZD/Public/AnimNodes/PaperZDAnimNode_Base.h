// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "AnimSequences/Players/PaperZDAnimationPlaybackData.h"
#include "PaperZDAnimNode_Base.generated.h"

struct FPaperZDAnimNode_Base;
class UPaperZDAnimInstance;
class UPaperZDAnimBPGeneratedClass;

/**
 * Evaluation context passed around, as a structure for potential extensions.
 */
struct FPaperZDAnimationBaseContext
{
	/* The AnimInstance object. */
	UPaperZDAnimInstance* AnimInstance;

	//ctor
	FPaperZDAnimationBaseContext(UPaperZDAnimInstance* InAnimInstance)
		: AnimInstance(InAnimInstance)
	{}

	/* Helper function to obtain the GeneratedClass of the context. */
	UPaperZDAnimBPGeneratedClass* GetAnimBPClass() const;
};

/**
 * Context passed around when initializing the animation nodes
 */
struct FPaperZDAnimationInitContext : public FPaperZDAnimationBaseContext
{
	//ctor
	FPaperZDAnimationInitContext(UPaperZDAnimInstance* InAnimInstance)
		: FPaperZDAnimationBaseContext(InAnimInstance)
	{}
};

/**
 * Context passed around when updating the animation nodes
 */
struct FPaperZDAnimationUpdateContext : public FPaperZDAnimationBaseContext
{
	/* The delta time of this update. */
	float DeltaTime;

	/* The effective weight that the updates node will have, in respect to the sink node. */
	float Weight;

	/* Identifier shared by every traversal of the same graph update. */
	uint64 GraphUpdateId;

	/* True when the graph is being updated only to commit an explicit animation jump. */
	bool bIsJumpUpdate;

	//ctor
	FPaperZDAnimationUpdateContext(UPaperZDAnimInstance* InAnimInstance, float InDeltaTime)
		: FPaperZDAnimationBaseContext(InAnimInstance)
		, DeltaTime(InDeltaTime)
		, Weight(1.0f)
		, GraphUpdateId(GFrameNumber)
		, bIsJumpUpdate(false)
	{}

	//copy constructor
	FPaperZDAnimationUpdateContext(const FPaperZDAnimationUpdateContext& OtherContext)
		: FPaperZDAnimationBaseContext(OtherContext.AnimInstance)
		, DeltaTime(OtherContext.DeltaTime)
		, Weight(OtherContext.Weight)
		, GraphUpdateId(OtherContext.GraphUpdateId)
		, bIsJumpUpdate(OtherContext.bIsJumpUpdate)
	{}

	/* Returns a copy of this context, after applying a fractional weight to it. */
	FPaperZDAnimationUpdateContext FractionalWeight(float Multiplier) const
	{
		FPaperZDAnimationUpdateContext NewContext(*this);
		NewContext.Weight *= Multiplier;
		return NewContext;
	}
};

/**
 * Represents a link to another Animation Node.
 */
 USTRUCT()
struct FPaperZDAnimDataLink
 {
	GENERATED_BODY()

private:
	/* The cached non-serialized output link. */
	FPaperZDAnimNode_Base* AnimNode;

public:
	/* Target ID that this link points to*/
	UPROPERTY()
	int32 LinkID;

#if WITH_EDITORONLY_DATA
	/* Source ID that this link comes from */
	UPROPERTY()
	int32 SourceLinkID;
#endif

public:
	//ctor
	FPaperZDAnimDataLink()
		: AnimNode(nullptr)
		, LinkID(INDEX_NONE)
#if WITH_EDITORONLY_DATA
		, SourceLinkID(INDEX_NONE)
#endif
	{}

	/* Initializes this link and the node linked to it, if available. */
	void Initialize(const FPaperZDAnimationInitContext& Context);

	/* Called to update the target link when the AnimInstance ticks. */
	void Update(const FPaperZDAnimationUpdateContext& Context);

	/* Obtains the animation data from the node connected to this link. */
	void Evaluate(FPaperZDAnimationPlaybackData& OutputData);
 };

 // An exposed value updater
USTRUCT()
struct PAPERZD_API FPaperZDExposedValueHandler
{
	GENERATED_BODY()

	/* The function to call to update associated properties (can be NULL). */
	UPROPERTY()
	FName BoundFunction;

	/* Cached UFunction. */
	UPROPERTY()
	TObjectPtr<UFunction> Function;

	// Node property that this value handler is associated with, when the node
	// is instantiated from this property the node's ExposedValueHandler will 
	// point back to this FExposedValueHandler:
	UPROPERTY()
	TFieldPath<FStructProperty> ValueHandlerNodeProperty;

	/* Prevent multiple initializations. */
	bool bInitialized;

public:
	FPaperZDExposedValueHandler()
	 : BoundFunction(NAME_None)
	 , Function(nullptr)
	 , ValueHandlerNodeProperty(nullptr)
	 , bInitialized(false)
	 {}

	 /* Initialize this handler, by caching the required data. */
	 void Initialize(UClass* Class);

	 /* Execute the update. */
	 void Update(FPaperZDAnimationBaseContext& Context);

	 /* Called to initialize the handlers of a given class. */
	 static void InitClass(TArray<FPaperZDExposedValueHandler>& ValueHandlers, UObject* InClassDefaultObject);
};

/**
 * Runtime implementation of an Animation Node.
 */
USTRUCT()
struct PAPERZD_API FPaperZDAnimNode_Base
{
	GENERATED_BODY()

	//Friendship for initial setup
	friend struct FPaperZDExposedValueHandler;

	/* Pointer to the value handler that is responsible of updating the internal values by calling the generated functions. */
	FPaperZDExposedValueHandler* ExposedValueHandler;

public:
	//ctor
	FPaperZDAnimNode_Base();
	virtual ~FPaperZDAnimNode_Base(){}

	/* Initialize method for the AnimNode, called once when the AnimInstance initializes itself. */
	void Initialize(const FPaperZDAnimationInitContext& Context);

	/* Updates any value needed for evaluating the data. */
	void Update(const FPaperZDAnimationUpdateContext& Context);

	/* Evaluates the node data to obtain the final Animation Data structure to be output. */
	void Evaluate(FPaperZDAnimationPlaybackData& OutAnimationData);

protected:
	/* Initialize method for the AnimNode, called once when the AnimInstance initializes itself. */
	virtual void OnInitialize(const FPaperZDAnimationInitContext& Context) {}

	/* Updates any value needed for evaluating the data. */
	virtual void OnUpdate(const FPaperZDAnimationUpdateContext& Context) {}

	/* Evaluates the node data to obtain the final Animation Data structure to be output. */
	virtual void OnEvaluate(FPaperZDAnimationPlaybackData& OutAnimationData) {}
};
