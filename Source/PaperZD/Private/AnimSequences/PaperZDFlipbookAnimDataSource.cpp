// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#include "AnimSequences/PaperZDFlipbookAnimDataSource.h"

bool FPaperZDFlipbookAnimDataSource::IsKeyFrameMirroredHorizontal(int32 KeyFrameIndex) const
{
	switch (MirrorMode)
	{
	case EPaperZDFlipbookMirrorMode::Horizontal:
	case EPaperZDFlipbookMirrorMode::BothAxes:
		return true;
	case EPaperZDFlipbookMirrorMode::PerFrame:
		return MirroredKeyFrames.Contains(KeyFrameIndex);
	default:
		return false;
	}
}

bool FPaperZDFlipbookAnimDataSource::IsKeyFrameMirroredVertical(int32 KeyFrameIndex) const
{
	switch (MirrorMode)
	{
	case EPaperZDFlipbookMirrorMode::Vertical:
	case EPaperZDFlipbookMirrorMode::BothAxes:
		return true;
	case EPaperZDFlipbookMirrorMode::PerFrame:
		return VerticalMirroredKeyFrames.Contains(KeyFrameIndex);
	default:
		return false;
	}
}

void FPaperZDFlipbookAnimDataSource::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading() && MirrorMode == EPaperZDFlipbookMirrorMode::None && MirroredKeyFrames.Num() > 0)
	{
		MirrorMode = EPaperZDFlipbookMirrorMode::PerFrame;
	}
}
