// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Layout/Visibility.h"
#include "PropertyHandle.h"

class IPropertyUtilities;
class SBox;

/**
 * Property type customization for FPaperZDFlipbookAnimDataSource.
 * Exposes the MirrorMode enum as a dropdown and, when MirrorMode == PerFrame,
 * replaces the raw MirroredKeyFrames / VerticalMirroredKeyFrames arrays with
 * per-frame checkbox rows derived from the assigned flipbook's key frame count.
 */
class FPaperZDFlipbookAnimDataSourceCustomization : public IPropertyTypeCustomization
{
	/** Which axis a checkbox/row is operating on. Local so we don't pull in Engine's EAxis. */
	enum class EMirrorAxis : uint8
	{
		Horizontal,
		Vertical,
	};

	/** Handle for the struct being customized. */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	/** Child handle for the Animation (UPaperFlipbook*) property. */
	TSharedPtr<IPropertyHandle> AnimationHandle;

	/** Child handle for the MirrorMode enum. */
	TSharedPtr<IPropertyHandle> MirrorModeHandle;

	/** Child handle for the horizontal MirroredKeyFrames array. */
	TSharedPtr<IPropertyHandle> HorizontalMirroredKeyFramesHandle;

	/** Child handle for the vertical MirroredKeyFrames array. */
	TSharedPtr<IPropertyHandle> VerticalMirroredKeyFramesHandle;

	/**
	 * Host boxes for the per-axis checkbox widgets. Lets us rebuild just the checkbox grid on flipbook
	 * assignment changes without forcing a full detail-panel refresh (which would reset sibling
	 * customizations like the directional-animation direction picker).
	 */
	TSharedPtr<SBox> HorizontalCheckboxHost;
	TSharedPtr<SBox> VerticalCheckboxHost;

	/** Property utilities, retained as a fallback refresh path if targeted updates become impossible. */
	TSharedPtr<IPropertyUtilities> PropertyUtilities;

public:
	/** Makes a new instance of this customization. */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~ Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;
	//~ End IPropertyTypeCustomization Interface

private:
	/** Builds the checkbox widget for all key frames in the current flipbook for a given axis. */
	TSharedRef<SWidget> BuildCheckboxWidget(EMirrorAxis Axis);

	/** Returns the checked state for a specific frame index on a given axis. */
	ECheckBoxState IsFrameMirrored(int32 FrameIndex, EMirrorAxis Axis) const;

	/** Called when a frame's mirror checkbox is toggled on a given axis. */
	void OnFrameMirrorToggled(ECheckBoxState NewState, int32 FrameIndex, EMirrorAxis Axis);

	/** Called when the Animation property changes; swaps the checkbox host content for the new frame count. */
	void OnAnimationChanged();

	/** Returns the array property handle for the requested axis. */
	TSharedPtr<IPropertyHandle> GetArrayHandleForAxis(EMirrorAxis Axis) const;

	/** Reads the current MirrorMode via raw struct access. */
	bool IsPerFrameModeActive() const;

	/** Visibility attribute for the per-frame rows; Visible only when MirrorMode == PerFrame. */
	EVisibility GetPerFrameRowVisibility() const;
};
