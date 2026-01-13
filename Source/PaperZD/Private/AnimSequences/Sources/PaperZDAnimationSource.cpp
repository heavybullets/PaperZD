// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "IPaperZDEditorProxy.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimationSource)
#endif

UPaperZDAnimationSource::UPaperZDAnimationSource()
 : bSupportsBlending(false)
 , bSupportsBlendLayers(false)
 , bSupportsCompositeAnimationLayers(false)
{}

#if WITH_EDITOR
void UPaperZDAnimationSource::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	//Trigger the change delegate, so that the editor objects can be notified about the changes (and act accordingly)
	OnAnimationSourcePropertyChange.Broadcast();
}

void UPaperZDAnimationSource::UnregisterCustomNotify(const FName& NotifyName)
{
	//Delete the notify from the array
	RegisteredNotifyNames.Remove(NotifyName);
	OnCustomNotifyChange.Broadcast();
	IPaperZDEditorProxy::Get()->UpdateNotifyFunctions(this);
}

bool UPaperZDAnimationSource::RegisterCustomNotify(const FName& NotifyName)
{
	if (!RegisteredNotifyNames.Contains(NotifyName))
	{
		RegisteredNotifyNames.Add(NotifyName);
		OnCustomNotifyChange.Broadcast();
		IPaperZDEditorProxy::Get()->UpdateNotifyFunctions(this);
		MarkPackageDirty();
		return true;
	}

	return false;
}

bool UPaperZDAnimationSource::RenameCustomNotify(const FName& OldName, const FName& NewName)
{
	const int32 Index = RegisteredNotifyNames.Find(OldName);
	if (Index != INDEX_NONE && !RegisteredNotifyNames.Contains(NewName))
	{
		RegisteredNotifyNames[Index] = NewName;
		return true;
	}

	return false;
}

#endif // WITH_EDITOR
