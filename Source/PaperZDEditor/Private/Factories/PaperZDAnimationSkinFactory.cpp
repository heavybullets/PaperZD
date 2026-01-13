// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#include "Factories/PaperZDAnimationSkinFactory.h"
#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "AnimSequences/Skins/PaperZDAnimationSkin.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/AppStyle.h"
#include "AnimSequences/PaperZDAnimSequence.h"
#include "Modules/ModuleManager.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "Misc/FeedbackContext.h"
#include "Editor.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimationSkinFactory)
#endif

#define LOCTEXT_NAMESPACE "ZDAnimationSkinFactory"

//////////////////////////////////////////////////////////////////////////
//// Factory
//////////////////////////////////////////////////////////////////////////
UPaperZDAnimationSkinFactory::UPaperZDAnimationSkinFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	SupportedClass = UPaperZDAnimationSkin::StaticClass();
}

bool UPaperZDAnimationSkinFactory::ConfigureProperties()
{
	// Optionally select the AnimBP if it hasn't been provided yet
	if (TargetAnimSource == nullptr)
	{
		// Load the content browser module to display an asset picker
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

		FAssetPickerConfig AssetPickerConfig;

		/** The asset picker will only show AnimBP */
		AssetPickerConfig.Filter.ClassPaths.Add(UPaperZDAnimationSource::StaticClass()->GetClassPathName());
		AssetPickerConfig.Filter.bRecursiveClasses = true;

		/** The delegate that fires when an asset was selected */
		AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateUObject(this, &UPaperZDAnimationSkinFactory::OnTargetAnimSourceSelected);

		/** The default view mode should be a list view */
		AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;

		PickerWindow = SNew(SWindow)
			.Title(LOCTEXT("CreateAnimSequenceOptions", "Pick Parent Animation Source"))
			.ClientSize(FVector2D(500, 600))
			.SupportsMinimize(false).SupportsMaximize(false)
			[
				SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("Menu.Background"))
					[
						ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
					]
			];

		GEditor->EditorAddModalWindow(PickerWindow.ToSharedRef());
		PickerWindow.Reset();
	}

	//Will work if we have an AnimSource
	return TargetAnimSource != nullptr;
}

UObject* UPaperZDAnimationSkinFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if (TargetAnimSource && TargetAnimSource->GetSupportedAnimationSkinClass())
	{
		UPaperZDAnimationSkin* AnimSkin = NewObject<UPaperZDAnimationSkin>(InParent, TargetAnimSource->GetSupportedAnimationSkinClass(), Name, Flags);
		AnimSkin->SetAnimationSource(TargetAnimSource);
		return AnimSkin;
	}
	else if (TargetAnimSource->GetSupportedAnimationSkinClass() == nullptr)
	{
		Warn->Logf(ELogVerbosity::Error, TEXT("Could not create skin asset, Animation Source has no valid default supported skin class."));
	}

	return nullptr;
}

void UPaperZDAnimationSkinFactory::OnTargetAnimSourceSelected(const FAssetData& SelectedAsset)
{
	TargetAnimSource = Cast<UPaperZDAnimationSource>(SelectedAsset.GetAsset());
	PickerWindow->RequestDestroyWindow();
}

#undef LOCTEXT_NAMESPACE