// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "Factories/Factory.h"
#include "PaperZDAnimationSkinFactory.generated.h"

class UPaperZDAnimationSource;
class SWindow;
struct FAssetData;

/**
 * Factory for PaperZD Animation Skins.
 */
UCLASS()
class UPaperZDAnimationSkinFactory : public UFactory
{
	GENERATED_BODY()

	/* Picked window Ptr */
	TSharedPtr<SWindow> PickerWindow;
		
public:
	/* Animation source to create this skin for. */
	UPROPERTY()
	TObjectPtr<class UPaperZDAnimationSource> TargetAnimSource;

public:
	UPaperZDAnimationSkinFactory(const FObjectInitializer& ObjectInitializer);
	
	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	//~ Begin UFactory Interface

private:
	/* Called when the user selects an AnimationSource from the asset picker. */
	void OnTargetAnimSourceSelected(const FAssetData& SelectedAsset);
};
