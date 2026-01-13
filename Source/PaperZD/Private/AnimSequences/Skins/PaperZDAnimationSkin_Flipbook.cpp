// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimSequences/Skins/PaperZDAnimationSkin_Flipbook.h"
#include "AnimSequences/PaperZDAnimSequence_Flipbook.h"
#include "PaperFlipbookComponent.h"
#include "IPaperZDEditorProxy.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimationSkin_Flipbook)
#endif

void UPaperZDAnimationSkin_Flipbook::ApplySkinToAnimation(const UPaperZDAnimSequence* AnimSequence, UPrimitiveComponent* RenderComponent, float DirectionalAngle /* = 0.0f */)
{
	UPaperFlipbookComponent* Sprite = Cast<UPaperFlipbookComponent>(RenderComponent);
	if (Sprite)
	{
		//Obtain the directional index to use
		const int32 DirectionalIndex = AnimSequence->GetDirectionIndexByAngle(DirectionalAngle);
		if (SkinsPerAnimation.Contains(AnimSequence))
		{
			UPaperFlipbook* Flipbook = SkinsPerAnimation[AnimSequence].AnimationDirections.IsValidIndex(DirectionalIndex) ? SkinsPerAnimation[AnimSequence].AnimationDirections[DirectionalIndex] : nullptr;

			//Check if the flipbook hasn't changed
			if (Sprite->GetFlipbook() != Flipbook)
			{
				UPaperFlipbook* From = Sprite->GetFlipbook();
				Sprite->SetFlipbook(Flipbook);
			}
		}
	}
}

void UPaperZDAnimationSkin_Flipbook::OnAnimationSourceChanged()
{
#if WITH_EDITOR
	SyncWithAnimationSource();
#endif
}


#if WITH_EDITOR
void UPaperZDAnimationSkin_Flipbook::SyncWithAnimationSource()
{
	TArray<FAssetData> AssetData = IPaperZDEditorProxy::Get()->GetAnimSequencesForSource(GetAnimationSource());
	SkinsPerAnimation.Reserve(AssetData.Num());
	for (const FAssetData& Data : AssetData)
	{
		UPaperZDAnimSequence_Flipbook* Sequence = Cast<UPaperZDAnimSequence_Flipbook>(Data.GetAsset());
		if (Sequence)
		{
			TSoftObjectPtr<UPaperZDAnimSequence> SoftPtr(Data.GetSoftObjectPath());
			FPaperZDFlipbookSkinData& SkinData = SkinsPerAnimation.FindOrAdd(SoftPtr);
			const int32 Delta = Sequence->GetAnimDataSource().Num() - SkinData.AnimationDirections.Num();
			if (Delta > 0)
			{
				SkinData.AnimationDirections.AddDefaulted(Delta);
			}
		}
	}

	MarkPackageDirty();
}
#endif
