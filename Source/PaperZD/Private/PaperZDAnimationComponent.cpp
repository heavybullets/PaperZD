// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "PaperZDAnimationComponent.h"
#include "PaperZDAnimInstance.h"
#include "Components/PrimitiveComponent.h"
#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "PaperZDAnimBPGeneratedClass.h"
#include "PaperZDCustomVersion.h"
#include "Engine/World.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimationComponent)
#endif

// Sets default values for this component's properties
UPaperZDAnimationComponent::UPaperZDAnimationComponent()
	: AnimInstanceClass(nullptr)
	, bBuildCompositeLayersAutomatically(false)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}

void UPaperZDAnimationComponent::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	//Apply the patches to different versions.
	//
	//This extra check fixes an edge case with 'blueprint spawned' components. For whatever reason they call 'PostLoad' always.
	//They might return a wrong 'LinkerVersion' when loading subsequent maps due to LinkerVersions checking the outermost instead of the object normally, which can be the world.
	//Hence why we make sure we aren't on a 'game world' while applying the fixes.
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		const int32 ZDVersion = GetLinkerCustomVersion(FPaperZDCustomVersion::GUID);
		if (ZDVersion < FPaperZDCustomVersion::AddedCustomComponentPicker)
		{
			//Use the new component ref type which uses the new and improved picker
			RenderComponentRef.ComponentProperty = RenderComponent_DEPRECATED.ComponentProperty;
			RenderComponentRef.PathToComponent = RenderComponent_DEPRECATED.PathToComponent;
			RenderComponentRef.OtherActor = nullptr;
			RenderComponentRef.OverrideComponent = nullptr;
		}
	}
#endif
}

void UPaperZDAnimationComponent::BeginPlay()
{
	Super::BeginPlay();

	//Create a fresh AnimInstance object
	GenerateCompositeLayers();
	CreateAnimInstance();
}

void UPaperZDAnimationComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FPaperZDCustomVersion::GUID);
}

void UPaperZDAnimationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Need to update the AnimInstance
	if (AnimInstance)
	{
		AnimInstance->Tick(DeltaTime);
	}
}

AActor* UPaperZDAnimationComponent::GetOwningActor() const
{
	return GetOwner();
}

UPrimitiveComponent* UPaperZDAnimationComponent::GetRenderComponent() const
{
	return Cast<UPrimitiveComponent>(RenderComponentRef.GetComponent(GetOwner()));
}

TArray<FPaperZDLayerLinkData> UPaperZDAnimationComponent::GetLayerLinkData() const
{
	return CompositeLayerLinkData;
}

TSubclassOf<UPaperZDAnimInstance> UPaperZDAnimationComponent::GetSequencerAnimInstanceClass() const
{
	return AnimInstanceClass;
}

UPaperZDAnimInstance* UPaperZDAnimationComponent::GetSequencerAnimInstance()
{
	return GetOrCreateAnimInstance();
}

UPaperZDAnimInstance* UPaperZDAnimationComponent::GetOrCreateAnimInstance()
{
	if (!AnimInstance || !AnimInstance->GetPlayer())
	{
		CreateAnimInstance();
	}

	return AnimInstance;
}

void UPaperZDAnimationComponent::CreateAnimInstance()
{
	if (AnimInstanceClass)
	{
		AnimInstance = NewObject<UPaperZDAnimInstance>(this, AnimInstanceClass);
		AnimInstance->Init(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No animation class defined on '%s', cannot create instance."), *GetName());
	}
}

void UPaperZDAnimationComponent::InitRenderComponent(UPrimitiveComponent* InRenderComponent)
{
	const FString PathToComponent = *InRenderComponent->GetPathName(GetOwner());
	RenderComponentRef.PathToComponent = PathToComponent;

	//We need to also set the old deprecated component, because if this call was used on a constructor to set the correct path
	//the PostLoad implementation will then override the new RenderComponent data with the old one, during the version upgrade process.
	//If this hasn't been setup by that point, it will effectively clear the RenderComponentRef during upgrade.
	RenderComponent_DEPRECATED.PathToComponent = PathToComponent;
}

void UPaperZDAnimationComponent::InitCompositeLayerLinkData(TArray<UPrimitiveComponent*> InCompositeLayers)
{
	CompositeLayerComponentsRefs.Empty(InCompositeLayers.Num());
	for (const UPrimitiveComponent* CompositeLayer : InCompositeLayers)
	{
		FPaperZDComponentReference& CompositeLayerComponentRef = CompositeLayerComponentsRefs.AddDefaulted_GetRef();
		const FString PathToComponent = *CompositeLayer->GetPathName(GetOwner());
		CompositeLayerComponentRef.PathToComponent = PathToComponent;
	}
}

void UPaperZDAnimationComponent::SetAnimInstanceClass(TSubclassOf<UPaperZDAnimInstance> InAnimInstanceClass)
{
	AnimInstanceClass = InAnimInstanceClass;

	//Potentially re-create the anim instance, only if we already initialized the AnimInstance
	AnimInstance = nullptr;
	CreateAnimInstance();
}

void UPaperZDAnimationComponent::InitAnimInstanceClass(TSubclassOf<UPaperZDAnimInstance> InAnimInstanceClass)
{
	AnimInstanceClass = InAnimInstanceClass;
	AnimInstance = nullptr;
}

void UPaperZDAnimationComponent::GenerateCompositeLayers()
{
	if (bBuildCompositeLayersAutomatically)
	{
		//Obtain the CDO of the AnimInstance so we can check the data from the Animation Source
		const UPaperZDAnimInstance* AnimInstanceCDO = GetDefault<UPaperZDAnimInstance>(AnimInstanceClass);
		if (UPaperZDAnimBPGeneratedClass* AnimClass = Cast<UPaperZDAnimBPGeneratedClass>(AnimInstanceClass))
		{
			const UPaperZDAnimationSource* AnimSource = AnimClass->GetSupportedAnimationSource();
			UPrimitiveComponent* RenderComponent = GetRenderComponent();
			if (AnimSource && RenderComponent)
			{
				//Auto build the layer data
				const TArray<FPaperZDCompositeLayerData>& CompositeData = AnimSource->GetCompositeLayerData();
				CompositeLayerLinkData.Empty(CompositeData.Num());
				for (int32 i = 0; i < CompositeData.Num(); i++)
				{
					const FPaperZDCompositeLayerData& LayerData = CompositeData[i];
					UPrimitiveComponent* CompositeComponent = Cast<UPrimitiveComponent>(GetOwner()->AddComponentByClass(AnimSource->GetRenderComponentClass(), true, FTransform(LayerData.Offset), true));
					CompositeComponent->SetupAttachment(RenderComponent);
					GetOwner()->FinishAddComponent(CompositeComponent, true, FTransform(LayerData.Offset));

					//Create the link
					FPaperZDLayerLinkData& Link = CompositeLayerLinkData.AddDefaulted_GetRef();
					Link.LinkedComponentPtr = CompositeComponent;
					Link.LayerIndex = i + 1;
				}
			}
		}
	}
	else
	{
		CompositeLayerLinkData.Empty(CompositeLayerComponentsRefs.Num());

		//Cache the reference into a weak pointer that can be used by the AnimPlayer
		for (int32 i = 0; i < CompositeLayerComponentsRefs.Num(); i++)
		{
			const FPaperZDComponentReference& Ref = CompositeLayerComponentsRefs[i];
			UPrimitiveComponent* PrimComponent = Cast<UPrimitiveComponent>(Ref.GetComponent(GetOwner()));
			
			//Create the link
			FPaperZDLayerLinkData& Link = CompositeLayerLinkData.AddDefaulted_GetRef();
			Link.LinkedComponentPtr = PrimComponent;
			Link.LayerIndex = i + 1;
		}
	}
}
