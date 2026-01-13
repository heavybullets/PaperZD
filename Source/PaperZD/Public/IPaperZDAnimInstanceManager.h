// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AnimSequences/Players/PaperZDLayerLink.h"
#include "IPaperZDAnimInstanceManager.generated.h"

class AActor;
class UPrimitiveComponent;
class UPaperZDAnimPlayer;
class UWorld;

//UInterface 
UINTERFACE()
class PAPERZD_API UPaperZDAnimInstanceManager : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface used for querying context data for the AnimInstance without compromising into using the component directly.
 */
class PAPERZD_API IPaperZDAnimInstanceManager : public IInterface
{
	GENERATED_BODY()

public:
	/**
	 * Obtains the actor that owns the AnimInstance, this will be the return object passed when the AnimBP asks for it.
	 * If the passed actor is a PaperZDCharacter it will be stored for backwards support.
	 */
	virtual AActor* GetOwningActor() const;

	/**
	 * Obtains the "Render Component" to use when initializing the AnimPlayer on the AnimInstance.
	 */
	virtual UPrimitiveComponent* GetRenderComponent() const;

	/**
	 * Obtains the layer link data for rendering additional "Render Components".
	 */
	 virtual TArray<FPaperZDLayerLinkData> GetLayerLinkData() const;

	/**
	 * Called when setting up the AnimPlayer in order to do any custom setup needed.
	 */
	virtual void OnSetupAnimPlayer(UPaperZDAnimPlayer* AnimPlayer) {}

	/**
	 * Called to obtain the world context, defaults to the world of the owning actor, if available.
	 * If none returns, then methods that require world context won't be available.
	 */
	virtual UWorld* OnGetWorld() const;
};
