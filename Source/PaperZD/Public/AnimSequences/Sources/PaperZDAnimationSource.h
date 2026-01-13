// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Templates/SubclassOf.h"
#include "PaperZDAnimationSource.generated.h"

class UPaperZDAnimSequence;
class UPaperZDAnimationSkin;
class UPaperZDPlaybackHandle;

//delegate implementations
DECLARE_MULTICAST_DELEGATE(FOnAnimationSourcePropertyChangeSignature);
DECLARE_MULTICAST_DELEGATE(FOnAnimationSourceNotifyChangeSignature);

/* Additional composite layer data that can be rendered alongside the main render component. */
USTRUCT()
struct FPaperZDCompositeLayerData
{
	GENERATED_BODY()

	/* Offset of the layer against the main render component. */
	UPROPERTY(EditAnywhere, Category = "Layers")
	FVector Offset;
};

/**
 * An animation source is an object that serves as a root for AnimSequences to be grouped and shared through Animation Blueprints.
 * The animation source contains information regarding how to render the Animation Sequence family and any data required for correct initialization of the animation render component.
 * (i.e. Skeleton information in skeletal animation sources).
 */
UCLASS(abstract)
class PAPERZD_API UPaperZDAnimationSource : public UObject
{
	GENERATED_BODY()

	//Friendship for access to version updating
	friend class FPaperZDRuntimeEditorProxy;
	
#if WITH_EDITORONLY_DATA
	/* Names of the registered notifies that the Animation blueprints that use this source should implement. */
	UPROPERTY()
	TArray<FName> RegisteredNotifyNames;

	/* Delegate to be called whenever the animation source changes in some important manner that the editor must know about. */
	FOnAnimationSourcePropertyChangeSignature OnAnimationSourcePropertyChange;

	/* Delegate called when the custom notifies change. */
	FOnAnimationSourceNotifyChangeSignature OnCustomNotifyChange;
#endif

protected:
	/* The supported AnimSequence class that we build by default when creating a new sequence for this source. */
	UPROPERTY(VisibleAnywhere, Category = "Features")
	TSubclassOf<UPaperZDAnimSequence> SupportedAnimSequenceClass;

	/* The animation skin asset that interfaces with this source. */
	UPROPERTY(VisibleAnywhere, Category = "Features")
	TSubclassOf<UPaperZDAnimationSkin> SupportedAnimationSkinClass;

	/* Whether the animation source supports blending based animations (2d skeletal animations). */
	UPROPERTY(VisibleAnywhere, Category = "Features")
	bool bSupportsBlending;

	/* Whether the animation source supports blend layers. */
	UPROPERTY(VisibleAnywhere, Category = "Features")
	bool bSupportsBlendLayers;

	/* Whether the animation source supports animation layers. */
	UPROPERTY(VisibleAnywhere, Category = "Features")
	bool bSupportsCompositeAnimationLayers;

public:
	//ctor
	UPaperZDAnimationSource();

	//getters
	TSubclassOf<UPaperZDAnimSequence> GetSupportedAnimSequenceClass() const { return SupportedAnimSequenceClass; }
	TSubclassOf<UPaperZDAnimationSkin> GetSupportedAnimationSkinClass() const { return SupportedAnimationSkinClass; }
	FORCEINLINE bool SupportsBlending() const { return bSupportsBlending; }
	FORCEINLINE bool SupportsBlendLayers() const { return bSupportsBlendLayers; }
	FORCEINLINE bool SupportsCompositeAnimationLayers() const { return bSupportsCompositeAnimationLayers; }

	/* Returns the playback handle that should be used for managing the rendering of the animations of this source. */
	virtual TSubclassOf<UPaperZDPlaybackHandle> GetPlaybackHandleClass() const { return nullptr; }

	/* Initializes the playback handle, passing any data that would be needed for initializing the render component. */
	virtual void InitPlaybackHandle(UPaperZDPlaybackHandle* Handle) const {}

	/* Returns the render component class to use for this animation source. Used for creating preview render components and validate which components can be used when trying to render the animations contained on this source. */
	virtual TSubclassOf<UPrimitiveComponent> GetRenderComponentClass() const { return nullptr; }

	/* Obtain the amount of additional composite layers for the source. */
	virtual TArray<FPaperZDCompositeLayerData> GetCompositeLayerData() const { return TArray<FPaperZDCompositeLayerData>(); }

#if WITH_EDITOR
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	/* Get the delegate that triggers whenever a property has been changed on this object. */
	FOnAnimationSourcePropertyChangeSignature& GetOnAnimationSourcePropertyChange() { return OnAnimationSourcePropertyChange; }

	/* Get the delegate that triggers whenever the custom notifies change. */
	FOnAnimationSourceNotifyChangeSignature& GetOnCustomNotifyChange() { return OnCustomNotifyChange; }

	/**
	 * Unregisters a custom notify from the function map.
	 * @param NotifyName	The name of the notify to register
	 * @return				Whether the notify was registered or not (in case it already existed)
	 */
	bool RegisterCustomNotify(const FName& NotifyName);

	/* Unregisters a custom notify from the function map. */
	void UnregisterCustomNotify(const FName& NotifyName);

	/* Renames the given notify. */
	bool RenameCustomNotify(const FName& OldName, const FName& NewName);

	/* True if the animation source contains the given notify name. */
	const TArray<FName>& GetRegisteredNotifyNames() const { return RegisteredNotifyNames; }
#endif
};
