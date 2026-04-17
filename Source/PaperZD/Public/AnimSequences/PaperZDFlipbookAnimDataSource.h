// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperFlipbook.h"
#include "PaperZDFlipbookAnimDataSource.generated.h"

/**
 * Controls how a flipbook animation is mirrored at render time.
 * - None: no mirroring.
 * - Horizontal / Vertical / BothAxes: apply the same flip to every key frame.
 * - PerFrame: per-axis key-frame index lists decide which frames mirror on which axis.
 */
UENUM()
enum class EPaperZDFlipbookMirrorMode : uint8
{
	/** No mirroring is applied to any frame. */
	None UMETA(ToolTip = "No mirroring is applied to any frame."),

	/** Every frame of the animation is mirrored horizontally (flips the sprite's X scale). */
	Horizontal UMETA(DisplayName = "Mirror Horizontal", ToolTip = "Every frame of the animation is mirrored horizontally (flips the sprite's X scale)."),

	/** Every frame of the animation is mirrored vertically (flips the sprite's Z scale). */
	Vertical UMETA(DisplayName = "Mirror Vertical", ToolTip = "Every frame of the animation is mirrored vertically (flips the sprite's Z scale)."),

	/** Every frame of the animation is mirrored both horizontally and vertically. */
	BothAxes UMETA(DisplayName = "Mirror Both (Horizontal & Vertical)", ToolTip = "Every frame of the animation is mirrored both horizontally and vertically."),

	/** Mirroring is controlled per key frame via the horizontal and vertical checkbox rows below. */
	PerFrame UMETA(ToolTip = "Mirroring is controlled per key frame via the horizontal and vertical checkbox rows below."),
};

/* The animation data source to be used by the Flipbook AnimSequence. */
USTRUCT()
struct PAPERZD_API FPaperZDFlipbookAnimDataSource
{
	GENERATED_BODY()

	/** Main animation to render as the base layer. */
	UPROPERTY(EditAnywhere, Category = "AnimSequence")
	TObjectPtr<UPaperFlipbook> Animation;

	/** The additional layers to render alongside the main animation. */
	UPROPERTY(EditAnywhere, Category = "AnimSequence")
	TArray<UPaperFlipbook*> CompositeLayerAnimations;

	/** Controls how this animation's frames are mirrored. Switch to PerFrame to enable the per-key-frame checkbox rows. */
	UPROPERTY(EditAnywhere, Category = "AnimSequence", meta = (ToolTip = "Controls how this animation's frames are mirrored. Switch to PerFrame to enable the per-key-frame checkbox rows."))
	EPaperZDFlipbookMirrorMode MirrorMode = EPaperZDFlipbookMirrorMode::None;

	/** Key frame indices that should render horizontally mirrored when MirrorMode == PerFrame. Edited via the Horizontal checkbox row. */
	UPROPERTY(EditAnywhere, Category = "AnimSequence", meta = (ToolTip = "Key frame indices that render horizontally mirrored when MirrorMode is PerFrame. Edited via the Horizontal checkbox row."))
	TArray<int32> MirroredKeyFrames;

	/** Key frame indices that should render vertically mirrored when MirrorMode == PerFrame. Edited via the Vertical checkbox row. */
	UPROPERTY(EditAnywhere, Category = "AnimSequence", meta = (ToolTip = "Key frame indices that render vertically mirrored when MirrorMode is PerFrame. Edited via the Vertical checkbox row."))
	TArray<int32> VerticalMirroredKeyFrames;

public:
	/** Default / flipbook-initialising ctor. Trivial enough to stay inline. */
	FPaperZDFlipbookAnimDataSource(UPaperFlipbook* InFlipbook = nullptr)
		: Animation(InFlipbook)
	{}

	/** Returns true if the provided key frame index should be rendered horizontally mirrored. */
	bool IsKeyFrameMirroredHorizontal(int32 KeyFrameIndex) const;

	/** Returns true if the provided key frame index should be rendered vertically mirrored. */
	bool IsKeyFrameMirroredVertical(int32 KeyFrameIndex) const;

	/**
	 * Migrates data authored before MirrorMode existed: if legacy MirroredKeyFrames has entries
	 * but MirrorMode is still the default (None), promote it to PerFrame so existing behavior is preserved.
	 */
	void PostSerialize(const FArchive& Ar);
};

/* Enables PostSerialize so legacy MirroredKeyFrames data migrates to MirrorMode::PerFrame on load. */
template<>
struct TStructOpsTypeTraits<FPaperZDFlipbookAnimDataSource> : public TStructOpsTypeTraitsBase2<FPaperZDFlipbookAnimDataSource>
{
	enum
	{
		WithPostSerialize = true,
	};
};
