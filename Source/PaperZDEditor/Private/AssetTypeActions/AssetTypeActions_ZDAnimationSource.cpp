// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AssetTypeActions/AssetTypeActions_ZDAnimationSource.h"
#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "AnimSequences/Skins/PaperZDAnimationSkin.h"
#include "Factories/PaperZDAnimBPFactory.h"
#include "Factories/PaperZDAnimationSkinFactory.h"
#include "Editors/Util/PaperZDEditorStyle.h"
#include "Toolkits/IToolkitHost.h"
#include "Modules/ModuleManager.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "PaperZDEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActionsPaperZDAnimationSource"

//static defines
const FName FAssetTypeActions_ZDAnimationSource::CategoryKey = FName("PaperZD_AssetActionsKey");
const FText FAssetTypeActions_ZDAnimationSource::CategoryDisplayName = FText::FromString("PaperZD");

FAssetTypeActions_ZDAnimationSource::FAssetTypeActions_ZDAnimationSource(EAssetTypeCategories::Type InAssetCategory) :
MyAssetCategory(InAssetCategory)
{}

FText FAssetTypeActions_ZDAnimationSource::GetName() const 
{ 
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_ZDAnimationSource", "AnimationSource");
}

UClass* FAssetTypeActions_ZDAnimationSource::GetSupportedClass() const
{
	return UPaperZDAnimationSource::StaticClass();
}

void FAssetTypeActions_ZDAnimationSource::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	FPaperZDEditorModule* EditorModule = &FModuleManager::LoadModuleChecked<FPaperZDEditorModule>("PaperZDEditor");

	//Open the asset editor for the selected objects
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		UPaperZDAnimationSource* AnimSource = Cast<UPaperZDAnimationSource>(*ObjIt);
		if (AnimSource)
		{
			EditorModule->CreateAnimationSourceEditor(Mode, EditWithinLevelEditor, AnimSource);
		}
	}
}

uint32 FAssetTypeActions_ZDAnimationSource::GetCategories()
{
	return MyAssetCategory;
}

FText FAssetTypeActions_ZDAnimationSource::GetAssetDescription(const FAssetData& AssetData) const
{
	return FText::FromString("Represents a single source of animations that will be used by the AnimationBlueprints to drive animations on characters.");
}

bool FAssetTypeActions_ZDAnimationSource::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

void FAssetTypeActions_ZDAnimationSource::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	TArray<TWeakObjectPtr<UPaperZDAnimationSource>> AnimationSources = GetTypedWeakObjectPtrs<UPaperZDAnimationSource>(InObjects);
	if (AnimationSources.Num() == 1)
	{
		//Create AnimBP
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AnimSource_CreateAnimBP", "Create AnimationBlueprint Asset"),
			LOCTEXT("AnimSource_CreateAnimBPTooltip", "Creates an AnimationBlueprint asset based on this Animation Source."),
			//FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.PaperFlipbook"),
			FSlateIcon(FPaperZDEditorStyle::GetStyleSetName(), "ClassThumbnail.PaperZDAnimBP"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetTypeActions_ZDAnimationSource::CreateAnimationBlueprintAsset, AnimationSources[0].Get()),
				FCanExecuteAction()
			)
		);

		//Create AnimSkin
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AnimSource_CreateSkinAsset", "Create AnimationSkin Asset"),
			LOCTEXT("AnimSource_CreateSkinAssetTooltip", "Creates an AnimationSkin asset for use on this Animation Source."),
			//FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.PaperFlipbook"),
			FSlateIcon(FPaperZDEditorStyle::GetStyleSetName(), "ClassThumbnail.PaperZDAnimBP"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FAssetTypeActions_ZDAnimationSource::CreateAnimationSkinAsset, AnimationSources[0].Get()),
				FCanExecuteAction()
			)
		);
	}	
}

void FAssetTypeActions_ZDAnimationSource::CreateAnimationBlueprintAsset(UPaperZDAnimationSource* BaseAnimationSource)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	//Create a default name
	const FString PathName = BaseAnimationSource->GetOutermost()->GetPathName();
	const FString LongPackagePath = FPackageName::GetLongPackagePath(PathName);

	const FString NewFlipBookDefaultPath = LongPackagePath + TEXT("/NewPaperZDAnimBP");
	FString DefaultSuffix;
	FString AssetName;
	FString PackageName;
	AssetToolsModule.Get().CreateUniqueAssetName(NewFlipBookDefaultPath, DefaultSuffix, /*out*/ PackageName, /*out*/ AssetName);

	//Make a factory and generate the asset
	UPaperZDAnimBPFactory* Factory = NewObject<UPaperZDAnimBPFactory>();
	Factory->SupportedAnimationSource = BaseAnimationSource;

	const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
	ContentBrowserModule.Get().CreateNewAsset(AssetName, PackagePath, UPaperZDAnimBP::StaticClass(), Factory);
}

void FAssetTypeActions_ZDAnimationSource::CreateAnimationSkinAsset(UPaperZDAnimationSource* BaseAnimationSource)
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");

	//Create a default name
	const FString PathName = BaseAnimationSource->GetOutermost()->GetPathName();
	const FString LongPackagePath = FPackageName::GetLongPackagePath(PathName);

	const FString NewFlipBookDefaultPath = LongPackagePath + TEXT("/NewPaperZDAnimationSkin");
	FString DefaultSuffix;
	FString AssetName;
	FString PackageName;
	AssetToolsModule.Get().CreateUniqueAssetName(NewFlipBookDefaultPath, DefaultSuffix, /*out*/ PackageName, /*out*/ AssetName);

	//Make a factory and generate the asset
	UPaperZDAnimationSkinFactory* Factory = NewObject<UPaperZDAnimationSkinFactory>();
	Factory->TargetAnimSource = BaseAnimationSource;

	const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
	ContentBrowserModule.Get().CreateNewAsset(AssetName, PackagePath, UPaperZDAnimationSkin::StaticClass(), Factory);
}

#undef LOCTEXT_NAMESPACE
