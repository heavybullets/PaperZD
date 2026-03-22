// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimSequences/Skins/PaperZDAnimationSkin.h"
#include "PaperZDAnimationSkin_Flipbook.generated.h"

//fwd
class UPaperZDAnimSequence;
class UPrimitiveComponent;
class UPaperFlipbook;

/* Structure that holds a single flipbook skin override. */
USTRUCT()
struct FPaperZDFlipbookSkinData
{
	GENERATED_BODY()

	/* The animation data source. */
	UPROPERTY(EditAnywhere, Category = "Skins")
	TArray<TObjectPtr<UPaperFlipbook>> AnimationDirections;
};

/**
 * A skin file that can be used to override the flipbook used on a specific animation layer.
 */
UCLASS()
class PAPERZD_API UPaperZDAnimationSkin_Flipbook : public UPaperZDAnimationSkin
{
	friend class SPaperZDExtractFlipbookFromTextureDialog;

	GENERATED_BODY()

	/* Animation source data to use. */
	UPROPERTY(EditAnywhere, Category = "PaperZD")
	TMap<TSoftObjectPtr<UPaperZDAnimSequence>, FPaperZDFlipbookSkinData> SkinsPerAnimation;

public:
	//ctor
	UPaperZDAnimationSkin_Flipbook()
	{}

	//~ Begin UPaperZDAnimationSkin_Flipbook Interface
	virtual bool ApplySkinToAnimation(const UPaperZDAnimSequence* AnimSequence, UPrimitiveComponent* RenderComponent, float DirectionalAngle = 0.0f) override;
	virtual void OnAnimationSourceChanged() override;
	//~ End UPaperZDAnimationSkin_Flipbook Interface

	FORCEINLINE const TMap<TSoftObjectPtr<UPaperZDAnimSequence>, FPaperZDFlipbookSkinData>& GetSkinsPerAnimation() const { return SkinsPerAnimation; }

private:
#if WITH_EDITOR
	/* Sync the animation source and fills this skin file with the registered animations. */
	UFUNCTION(CallInEditor, Category = "PaperZD")
	void SyncWithAnimationSource();
#endif
};
