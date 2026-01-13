// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

//fwd
class UPaperZDAnimationSource;

/**
 * Asset type actions for the AnimationSource object.
 */
class FAssetTypeActions_ZDAnimationSource: public FAssetTypeActions_Base
{
	/* Custom asset category. */
	EAssetTypeCategories::Type MyAssetCategory;

public:
	//Category Key for registering an advanced category
	static const FName CategoryKey;

	//Display name of the category, will be called ZetaD to avoid confusing the user with Paper2D
	static const FText CategoryDisplayName;

public:
	//ctor
	FAssetTypeActions_ZDAnimationSource(EAssetTypeCategories::Type InAssetCategory);

	// IAssetTypeActions Implementation
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override { return FColor(0, 175, 255); }
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;
	virtual FText GetAssetDescription(const FAssetData& AssetData) const override;
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	// End IAssetTypeActions Implementation

private:
	//Creates an AnimationBlueprint for the selected AnimationSource
	void CreateAnimationBlueprintAsset(UPaperZDAnimationSource* BaseAnimationSource);

	//Creates an AnimationSkin asset for this animation source.
	void CreateAnimationSkinAsset(UPaperZDAnimationSource* BaseAnimationSource);
};
