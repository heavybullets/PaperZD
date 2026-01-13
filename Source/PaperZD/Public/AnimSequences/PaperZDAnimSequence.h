// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "CoreMinimal.h"
#include "Notifies/PaperZDAnimNotify_Base.h"
#include "PaperZDAnimSequence.generated.h"

DECLARE_DELEGATE(FOnPostEditUndo)
DECLARE_MULTICAST_DELEGATE(FOnNotifyChangeSignature);

class UPaperFlipbook;
class UPaperZDAnimBP;
class UPaperZDAnimNotify_Base;
class UPaperZDPlaybackHandle;
class UPaperZDAnimationSource;

/**
 * Stores the meta-information of a track used to display AnimNotify information in an orderly manner.
 */
 USTRUCT()
 struct FPaperZDAnimTrackMetadata
 {
	GENERATED_BODY()

	/* Color used to display the track visually. */
	UPROPERTY()
	FLinearColor Color;

	/* Editor name for the track. */
	UPROPERTY()
	FName DisplayName;

public:
	FPaperZDAnimTrackMetadata()
		: Color(FLinearColor::Gray)
		, DisplayName(NAME_None)
	{}
 };

 /**
  * Simple structure to be passed when requesting an animation track, which is an Editor-Only data object used to encapsulate the AnimNotifies for later display.
  * The track is valid up until either the notifies or the track itself get modified and should only be used during the scope in which the track was obtained.
  */
 struct FPaperZDAnimTrack
 {
	 /* Array of notifies assigned to the track. */
	 TArray<UPaperZDAnimNotify_Base*> AnimNotifies;

	 /* Extra data used to display the track. */
	 FPaperZDAnimTrackMetadata Metadata;

public:
	 //ctor
	 FPaperZDAnimTrack()
	 {}

	 FPaperZDAnimTrack(const TArray<UPaperZDAnimNotify_Base*>& InAnimNotifies, const FPaperZDAnimTrackMetadata& InMetadata)
		: AnimNotifies(InAnimNotifies)
		, Metadata(InMetadata)
	 {}
  };

/**
 * The AnimSequence is the class responsible of handling how a given Animation source plays on the registered RenderComponent and handling meta info like AnimNotifies. 
 */
UCLASS(abstract, BlueprintType)
class PAPERZD_API UPaperZDAnimSequence : public UObject
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	//Added for version update to AnimSequence only
	friend class FPaperZDRuntimeEditorProxy;
#endif

private:
	/* Default category name when creating an AnimSequence */
	static const FName DefaultCategory;

	/* Name of the AnimSource owner member name. */
	static const FName AnimSourceMemberName;

	/* Animation source that owns this sequence. */
	UPROPERTY(VisibleAnywhere, AssetRegistrySearchable, Category = Animation)
	TObjectPtr<UPaperZDAnimationSource> AnimSource;
	
	/* The list of the AnimNotifies linked to this animation. */
	UPROPERTY()
	TArray<TObjectPtr<UPaperZDAnimNotify_Base>> AnimNotifies;

#if WITH_EDITORONLY_DATA
	/* DEPRECATED: Points to the AnimBP that owned this AnimSequence before the creation of AnimSources. */
	UPROPERTY(AssetRegistrySearchable)
	UPaperZDAnimBP* AnimBP_DEPRECATED;

	/* Track meta information for editor display. */
	UPROPERTY()
	TArray<FPaperZDAnimTrackMetadata> AnimTrackData;

	/* Called when the list of notifies changes somehow. */
	FOnNotifyChangeSignature OnNotifyChange;
#endif

	/* Cached DataSource property for faster lookup. */
	FArrayProperty* CachedAnimDataSourceProperty;

public:
	UPROPERTY()
	FName DisplayName_DEPRECATED; //@Deprecated
		
	UPROPERTY()
	UPaperFlipbook* PaperFlipbook_DEPRECATED; //@Deprecated

	/**
	 * If true, multiple animations sources can be added to this sequence at different angles.
	 * Used for when the sequence needs to represent multiple rotations of the same animation (i.e. top-down and isometric games).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimSequence", meta = (DisplayName = "Multi-Directional Sequence"))
	bool bDirectionalSequence;

	/**
	 * Offset applied to all the animations on the directional map.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimSequence", meta = (UIMin = "-45.0", ClampMin = "-45.0", UIMax = "45.0", ClampMax = "45.0"))
	float DirectionalAngleOffset;

	/**
	 * Index to use while previewing a directional animation sequence
	 */
	 int32 DirectionalPreviewIndex;

	/* The category of this sequence, displayed on the tree view on the AnimSequence editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = "AnimSequence")
	FName Category;

	//Delegate for PostEditChanges - Needed for the editor undo support
	FOnPostEditUndo OnPostEditUndo; //@TODO: SHOULD WE REMOVE THIS?

public:
	//Required for version support
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;

	/* Called after initializing the properties, but before serialization. */
	virtual void PostInitProperties() override;

	/* Helper to get the AnimSource member name for the editor and others property windows */
	static FName GetAnimSourceMemberName() { return UPaperZDAnimSequence::AnimSourceMemberName; }

	/* Obtains the Animation Source linked to this Sequence */
	FORCEINLINE UPaperZDAnimationSource* GetAnimSource() const { return AnimSource; }

	/* Set the Animation source that owns this sequence. */
	void SetAnimSource(class UPaperZDAnimationSource* InAnimSource);

	/* Get the AnimNotifies linked to this sequence. */
	const TArray<UPaperZDAnimNotify_Base*>& GetAnimNotifies() const;

	/* Originally meant to hold the Display name, can be overridden if you don't want the default FName to be used when referring to this sequence. */
	virtual FName GetSequenceName() const;

	/* Total duration of this Sequence, should be overridden */
	UFUNCTION(BlueprintPure, Category = "AnimSequence")
	virtual float GetTotalDuration() const;

	/* Frames per second, used to show the grid on the AnimSequence editor */
	UFUNCTION(BlueprintPure, Category = "AnimSequence")
	virtual float GetFramesPerSecond() const;

	/* Obtains the number of frames on this animation. */
	virtual int32 GetNumberOfFrames() const;

	/* Obtain the frame number, given the playback time. */
	int32 GetFrameAtTime(const float Time) const;

	/* Obtain the playback time, given the frame number. */
	float GetTimeAtFrame(const int32 Frame) const;

	/**
	 * Returns true if the given data source entry is considered as "set".
	 * Override this entry if the AnimSequence supports Directional AnimDataSources to help it understand when a value is not set.
	 */
	virtual bool IsDataSourceEntrySet(int32 EntryIndex) const { return false; }

	/* Obtains the property that points to the array that serves as data source for this sequence. */
	FArrayProperty* GetAnimDataSourceProperty() const;

	/**
	 * Agnostic getter implementation for the internal AnimationData Source array.
	 * For better performance, one should code a specialization.
	 */
	template<typename T>
	T GetAnimationDataByIndex(int32 DirectionalIndex = 0) const
	{
		FArrayProperty* ArrayProperty = GetAnimDataSourceProperty();
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<uint8>(this));

		const int32 Index = bDirectionalSequence ? DirectionalIndex : 0;
		uint8* Ptr = ArrayHelper.GetRawPtr(Index);
		return *reinterpret_cast<T*>(Ptr);
	}

	/**
	 * Agnostic animation data getter.
	 * @param	DirectionalAngle		The angle in degrees against the top of the animation.
	 * @param	bPreviewPlayer			(Editor only) If true, the system will use the cached DirectionalPreviewIndex.
	 * @return							The animation data source object that defines the rendering for this sequence.
	 */
	 template <typename T>
	 T GetAnimationData(float DirectionalAngle = 0.0f, bool bPreviewPlayer = false) const
	 {
 #if WITH_EDITOR
		if (bPreviewPlayer)
		{
			return GetAnimationDataByIndex<T>(DirectionalPreviewIndex);
		}
#endif	
		if (bDirectionalSequence)
		{
			//Obtain the directional preview index from the given angle
			const int32 DirectionIndex = GetDirectionIndexByAngle(DirectionalAngle);
			return GetAnimationDataByIndex<T>(DirectionIndex);
		}
		else
		{ 
			//Non-directional sequences will always use the first index
			return GetAnimationDataByIndex<T>();
		}
	 }

	/* True if this sequence is currently being used as a "Directional Sequence" */
	FORCEINLINE bool IsDirectionalSequence() const { return bDirectionalSequence; }

	/* Angle to offset the "Directional Sequence" */
	FORCEINLINE float GetDirectionalAngleOffset() const { return DirectionalAngleOffset; }

	/* Translate the directional angle to a usable directional animation index. */
	int32 GetDirectionIndexByAngle(float DirectionalAngle) const
	{
		//Obtain the directional preview index from the given angle
		FArrayProperty* ArrayProperty = GetAnimDataSourceProperty();
		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<uint8>(this));
		const int32 Num = ArrayHelper.Num();
		const float AngleSeparation = 360.0f / Num;

		//We need to account for angles that are negative, for this we add a full revolution and then obtain the modulo, which will ensure we're always at a normalized range
		const int32 Area = (DirectionalAngle + DirectionalAngleOffset + AngleSeparation / 2.0f + 360.0f) / AngleSeparation;
		return Area % Num;
	}

#if WITH_EDITOR
	void PostEditUndo() override;

	/* Initializes the AnimTracks, making sure that we have enough metadata for any AnimNotify that we have stored. */
	void InitTracks();

	/* Get the delegate called when a notify changes internally. */
	FOnNotifyChangeSignature& GetOnNotifyChange() { return OnNotifyChange; }

	/**
	 * Creates a track data object. 
	 * @param InsertInto	The index in which to insert the track, or INDEX_NONE to append it to the end
	 * @return				The track index.
	 */
	int32 CreateTrack(int32 InsertInto = INDEX_NONE);

	/* Removes the track with the given index, deleting any and all notifies that are stored on it. */
	void RemoveTrack(int32 TrackIndex);

	/* Creates a notify of the given class and adds it to the given track. */
	void AddNotifyToTrack(TSubclassOf<class UPaperZDAnimNotify_Base> NotifyClass, int32 TrackIndex, FName NotifyName, float InitTime = 0.0f);

	/* Removes the notify. */
	void RemoveNotify(class UPaperZDAnimNotify_Base* Notify);

	/* Moves the notify to the given track. */
	void MoveNotify(class UPaperZDAnimNotify_Base* Notify, int32 ToTrack);

	/* Imports the given notifies into the sequence, usually due to copy/paste operations. */
	void ImportNotifies(TArray<UPaperZDAnimNotify_Base*> Notifies);

	/* Return the number of tracks currently stored on the sequence. */
	int32 GetNumTracks() const;

	/* Obtain the metadata of the track on the given index, returns whether the track was found or not. */
	bool GetTrackMetadata(int32 Index, FPaperZDAnimTrackMetadata& OutMetadata) const;

	/* Obtain the metadata of the track on the given index, assumes the index is correct or asserts if not. */
	const FPaperZDAnimTrackMetadata& GetTrackMetadataChecked(int32 Index) const;
	FPaperZDAnimTrackMetadata& GetMutableTrackMetadataChecked(int32 Index);

	/* Obtain the track on the given index, returns whether the track was found or not. */
	bool GetTrack(int32 Index, FPaperZDAnimTrack& OutTrack) const;

	/* Obtain the track on the given index, assumes the index is correct or asserts if not. */
	FPaperZDAnimTrack GetTrackChecked(int32 Index) const;

	/* Purge any custom notify that's currently referencing a function that isn't registered. */
	void PurgeInvalidCustomNotifies(const TArray<FName>& ValidRegisteredNames);
#endif

protected:
	/* Requests the name of the array property to be used as the "AnimDataSource". */
	virtual FName GetDataSourcePropertyName() const;

private:
	/* Initializes the Animation Data Source and makes sure its correctly configured for later use. */
	void InitDataSource();
};
