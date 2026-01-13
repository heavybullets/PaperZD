// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimSequences/PaperZDAnimSequence_Flipbook.h"
#include "PaperZDCustomVersion.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimSequence_Flipbook)
#endif

void UPaperZDAnimSequence_Flipbook::PostLoad()
{
	Super::PostLoad();

	//Backwards compatibility: after the AnimBlueprint rework, the AnimSequences now can support "multi-directional" data sources
	//which are stored on an array that accommodates its size to the number of directions it needs to use.
	//A non-directional sequence uses a size-1 array, thus we need to move the data from the old non-directional variable into the array.
	//The resulting AnimSequence will be non-directional due to the old version having no directional support whatsoever.
	const int32 ZDVersion = GetLinkerCustomVersion(FPaperZDCustomVersion::GUID);
	if (ZDVersion < FPaperZDCustomVersion::AnimBlueprintRework)
	{
		AnimDataSource_DEPRECATED.SetNum(1);
		AnimDataSource_DEPRECATED[0] = Flipbook_DEPRECATED;
		bDirectionalSequence = false;
	}

	//Composite layers added
	if (ZDVersion < FPaperZDCustomVersion::AddedCompositeLayers)
	{
		AnimData.Empty(AnimDataSource_DEPRECATED.Num());
		for (UPaperFlipbook* Flipbook : AnimDataSource_DEPRECATED)
		{
			AnimData.Emplace(Flipbook);
		}
	}
}

void UPaperZDAnimSequence_Flipbook::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FPaperZDCustomVersion::GUID);
}

float UPaperZDAnimSequence_Flipbook::GetTotalDuration() const
{
	const UPaperFlipbook* PrimaryFlipbook = GetPrimaryFlipbook();
	return PrimaryFlipbook ? PrimaryFlipbook->GetTotalDuration() : 0.0f;
}

float UPaperZDAnimSequence_Flipbook::GetFramesPerSecond() const
{
	//Default value is 15 fps
	const UPaperFlipbook* PrimaryFlipbook = GetPrimaryFlipbook();
	return PrimaryFlipbook ? PrimaryFlipbook->GetFramesPerSecond() : 15.0f;
}

bool UPaperZDAnimSequence_Flipbook::IsDataSourceEntrySet(int32 EntryIndex) const
{
	return AnimData.IsValidIndex(EntryIndex) ? AnimData[EntryIndex].Animation != nullptr : false;
}
