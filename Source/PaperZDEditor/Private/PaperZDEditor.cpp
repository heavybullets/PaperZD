// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "PaperZDEditor.h"
#include "Editors/PaperZDAnimBPEditor.h"
#include "Editors/PaperZDAnimationSourceEditor.h"
#include "Editors/Util/PaperZDEditorCommands.h"
#include "AssetTypeActions/AssetTypeActions_PaperZDAnimBP.h"
#include "AssetTypeActions/AssetTypeActions_ZDAnimSeq.h"
#include "AssetTypeActions/AssetTypeActions_ZDAnimationSource.h"
#include "AssetRegistry/AssetData.h"
#include "PaperZDAnimBP.h"
#include "PaperZDCharacter.h"
#include "Editors/Util/PaperZDRuntimeEditorProxy.h"
#include "Editors/Util/PaperZDEditorStyle.h"
#include "KismetCompiler.h"
#include "PropertyEditorModule.h"
#include "Graphs/PaperZDGraphFactory.h"

//Compilation
#include "Compilers/PaperZDAnimBPCompilerContext.h"
#include "Compilers/Handles/PaperZDAnimBPCompilerHandle_Base.h"
#include "Compilers/Handles/PaperZDAnimBPCompilerHandle_StateMachine.h"
#include "Compilers/Handles/PaperZDAnimBPCompilerHandle_CacheAnimation.h"

//Custom Settings
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "ISettingsContainer.h"
#include "Editors/Util/PaperZDEditorSettings.h"

//Detail Customization
#include "PaperZDAnimGraphNode_Base.h"
#include "PaperZDStateGraphNode_Transition.h"
#include "AnimSequences/PaperZDAnimSequence.h"
#include "Editors/DetailCustomizations/AnimGraphNodeDetailCustomization.h"
#include "Editors/DetailCustomizations/TransitionGraphNodeDetailCustomization.h"
#include "Editors/DetailCustomizations/AnimSequenceDetailCustomization.h"
#include "Editors/DetailCustomizations/PaperZDComponentReferenceCustomization.h"
#include "Editors/DetailCustomizations/FlipbookAnimDataSourceCustomization.h"

//For Sequencer support
#include "ISequencerModule.h"
#include "PaperZDAnimationTrackEditor.h"

//Tools
#include "Tools/PaperZDContentBrowserExtension.h"

#define LOCTEXT_NAMESPACE "FPaperZDEditorModule"
void FPaperZDEditorModule::StartupModule()
{
	RegisterAssetActions();
	RegisterCompiler();
	
	// Register the TrackEditor for 2d sequencer animation
	ISequencerModule& SequencerModule = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FPaperZDAnimationTrackEditor::CreateTrackEditor));

	//Register the commands
	FPaperZDEditorCommands::Register();

	//Register the custom settings
	RegisterSettings();
	
	//Register Default Nodes for the editor
	FPaperZDAnimBPEditor::RegisterDefaultEventNodes();

	//Register Detail Customizations
	RegisterCustomizations();

	//Register the editor style
	FPaperZDEditorStyle::Initialize();

	//Register the custom graph factories
	RegisterGraphFactories();

	//Register tools
    if (!IsRunningCommandlet())
    {
        FPaperZDContentBrowserExtension::InstallHooks();
    }

	//Finally register the EditorProxy, so the runtime part can configure editor-only functionalities when this module is up and running
	FPaperZDRuntimeEditorProxy::Register();
}

void FPaperZDEditorModule::ShutdownModule()
{
	UnregisterAssetActions();
	UnregisterGraphFactories();

	FPaperZDAnimBPEditor::UnregisterDefaultEventNodes();

    if (!IsRunningCommandlet())
    {
        FPaperZDContentBrowserExtension::RemoveHooks();
    }
}

void FPaperZDEditorModule::RegisterCompiler()
{
	//Register for blueprint compilation manager
	FKismetCompilerContext::RegisterCompilerForBP(UPaperZDAnimBP::StaticClass(), &FPaperZDEditorModule::GetCompilerForAnimBP);

	//Register every handle that needs to be added
	FPaperZDAnimBPCompilerContext::RegisterCompileHandleFactory<FPaperZDCompilerFactory_Base>();
	FPaperZDAnimBPCompilerContext::RegisterCompileHandleFactory<FPaperZDCompilerFactory_StateMachine>();
	FPaperZDAnimBPCompilerContext::RegisterCompileHandleFactory<FPaperZDCompilerFactory_CacheAnimation>();
}

void FPaperZDEditorModule::RegisterSettings()
{
	// Register custom editor settings
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule)
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "PaperZDEditor",
			LOCTEXT("EditorSettingsName", "PaperZD (Editor)"),
			LOCTEXT("EditorSettingsDescription", "Configure common PaperZD settings"),
			GetMutableDefault<UPaperZDEditorSettings>());
	}
}

void FPaperZDEditorModule::UnregisterSettings()
{
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule)
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "PaperZDEditor");
	}
}

TSharedPtr<FKismetCompilerContext> FPaperZDEditorModule::GetCompilerForAnimBP(class UBlueprint* BP, class FCompilerResultsLog& InMessageLog, const struct FKismetCompilerOptions& InCompileOptions)
{
	return TSharedPtr<FKismetCompilerContext>(new FPaperZDAnimBPCompilerContext(BP, InMessageLog, InCompileOptions));
}

void FPaperZDEditorModule::RegisterAssetActions()
{
	// Register the ZD Editor asset type actions
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type CustomType = AssetTools.RegisterAdvancedAssetCategory(FAssetTypeActions_PaperZDAnimBP::CategoryKey, FAssetTypeActions_PaperZDAnimBP::CategoryDisplayName);

	AnimBPAssetTypeActions = MakeShareable(new FAssetTypeActions_PaperZDAnimBP(CustomType));
	AssetTools.RegisterAssetTypeActions(AnimBPAssetTypeActions.ToSharedRef());

	AnimSequenceAssetTypeActions = MakeShareable(new FAssetTypeActions_ZDAnimSeq(CustomType));
	AssetTools.RegisterAssetTypeActions(AnimSequenceAssetTypeActions.ToSharedRef());

	AnimationSourceAssetTypeActions = MakeShareable(new FAssetTypeActions_ZDAnimationSource(CustomType));
	AssetTools.RegisterAssetTypeActions(AnimationSourceAssetTypeActions.ToSharedRef());
}

void FPaperZDEditorModule::UnregisterAssetActions()
{
	// Register the ZD Editor asset type actions
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(AnimBPAssetTypeActions.ToSharedRef());
		AssetTools.UnregisterAssetTypeActions(AnimSequenceAssetTypeActions.ToSharedRef());
		AssetTools.UnregisterAssetTypeActions(AnimationSourceAssetTypeActions.ToSharedRef());

		AnimBPAssetTypeActions = nullptr;
		AnimSequenceAssetTypeActions = nullptr;
		AnimationSourceAssetTypeActions = nullptr;
	}
}

void FPaperZDEditorModule::RegisterCustomizations()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	//Custom detail views
	PropertyModule.RegisterCustomClassLayout(UPaperZDAnimGraphNode_Base::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FPaperZDAnimGraphNodeDetailCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(UPaperZDStateGraphNode_Transition::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FPaperZDTransitionGraphNodeDetailCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(UPaperZDAnimSequence::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FPaperZDAnimSequenceDetailCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("PaperZDComponentReference", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPaperZDComponentReferenceCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout("PaperZDFlipbookAnimDataSource", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPaperZDFlipbookAnimDataSourceCustomization::MakeInstance));
	//PropertyModule.RegisterCustomClassLayout(UPaperZDAnimGraphNode_Transition::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FAnimGraphNodeDetailCustomization::MakeInstance));
}

void FPaperZDEditorModule::RegisterGraphFactories()
{
	GraphNodeFactory = MakeShared<FPaperZDGraphNodeFactory>();
	GraphPinFactory = MakeShared<FPaperZDGraphPinFactory>();
	GraphPinConnectionFactory = MakeShared<FPaperZDGraphPinConnectionFactory>();

	FEdGraphUtilities::RegisterVisualNodeFactory(GraphNodeFactory);
	FEdGraphUtilities::RegisterVisualPinFactory(GraphPinFactory);
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(GraphPinConnectionFactory);
}

void FPaperZDEditorModule::UnregisterGraphFactories()
{
	FEdGraphUtilities::UnregisterVisualNodeFactory(GraphNodeFactory);
	FEdGraphUtilities::UnregisterVisualPinFactory(GraphPinFactory);
	FEdGraphUtilities::UnregisterVisualPinConnectionFactory(GraphPinConnectionFactory);
}

//////////////////////////////////////////////////////////////////////////
//// Editors
//////////////////////////////////////////////////////////////////////////
TSharedRef<FPaperZDAnimBPEditor> FPaperZDEditorModule::CreateAnimBPEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UPaperZDAnimBP* InitAnimBP)
{
	TSharedRef<FPaperZDAnimBPEditor> NewAnimBPEditor(new FPaperZDAnimBPEditor());
	NewAnimBPEditor->InitAnimBPEditor(Mode, InitToolkitHost, InitAnimBP);
	return NewAnimBPEditor;
}

TSharedRef<FPaperZDAnimationSourceEditor> FPaperZDEditorModule::CreateAnimationSourceEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, class UPaperZDAnimationSource* InAnimationSource, class UPaperZDAnimSequence* SelectedAnimSequence /* = nullptr */)
{
	TSharedRef<FPaperZDAnimationSourceEditor> NewEditor(new FPaperZDAnimationSourceEditor());
	NewEditor->InitAnimationSourceEditor(Mode, InitToolkitHost, InAnimationSource);

	//Optionally ask the editor to focus on the given sequence
	if (SelectedAnimSequence)
	{
		NewEditor->OpenAsset(FAssetData(SelectedAnimSequence));
	}

	return NewEditor;
}

void FPaperZDEditorModule::CustomizeAnimBlueprintEditorDetails(const TSharedRef<class IDetailsView>& InDetailsView)
{
	InDetailsView->SetExtensionHandler(MakeShared<FPaperZDAnimGraphNodeBindingExtension>());
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPaperZDEditorModule, PaperZDEditor)
