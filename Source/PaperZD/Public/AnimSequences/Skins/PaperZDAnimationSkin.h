// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Templates/SubclassOf.h"
#include "PaperZDAnimationSkin.generated.h"

//fwd
class UPaperZDAnimSequence;
class UPaperZDAnimationSource;
class UPrimitiveComponent;

/**
 * A skin file that can be applied to an animation instance to modify how the render components displays the AnimSequences.
 */
UCLASS(abstract, BlueprintType)
class PAPERZD_API UPaperZDAnimationSkin : public UObject
{
	GENERATED_BODY()

	/* Animation source that owns this sequence. */
	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = Animation)
	TObjectPtr<UPaperZDAnimationSource> AnimSource;

public:
	//ctor
	UPaperZDAnimationSkin()
	{}

	//Getters
	FORCEINLINE UPaperZDAnimationSource* GetAnimationSource() const { return AnimSource; }

	/** 
	 * Modify how to render the primitive.
	 * Returns whether the skin actually overrides the rendering logic, or if the playback handle should just continue with its logic instead.
	 */
	virtual bool ApplySkinToAnimation(const UPaperZDAnimSequence* AnimSequence, UPrimitiveComponent* RenderComponent, float DirectionalAngle = 0.0f);

	/* Set the Animation source that owns this sequence. */
	void SetAnimationSource(class UPaperZDAnimationSource* InAnimSource);

protected:
	/* Called when the animation source gets set. */
	virtual void OnAnimationSourceChanged() {}
};
