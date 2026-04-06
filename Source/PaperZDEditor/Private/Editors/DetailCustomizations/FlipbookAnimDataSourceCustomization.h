// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

class IPropertyUtilities;

/**
 * Property type customization for FPaperZDFlipbookAnimDataSource.
 * Replaces the raw MirroredKeyFrames array with per-frame checkboxes
 * derived from the assigned flipbook's key frame count.
 */
class FPaperZDFlipbookAnimDataSourceCustomization : public IPropertyTypeCustomization
{
	/** Handle for the struct being customized. */
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	/** Child handle for the Animation (UPaperFlipbook*) property. */
	TSharedPtr<IPropertyHandle> AnimationHandle;

	/** Child handle for the MirroredKeyFrames array. */
	TSharedPtr<IPropertyHandle> MirroredKeyFramesHandle;

	/** Property utilities, used to force detail panel refresh. */
	TSharedPtr<IPropertyUtilities> PropertyUtilities;

public:
	/** Makes a new instance of this customization. */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~ Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils) override;
	//~ End IPropertyTypeCustomization Interface

private:
	/** Builds the checkbox widget for all key frames in the current flipbook. */
	TSharedRef<SWidget> BuildCheckboxWidget();

	/** Returns the checked state for a specific frame index. */
	ECheckBoxState IsFrameMirrored(int32 FrameIndex) const;

	/** Called when a frame's mirror checkbox is toggled. */
	void OnFrameMirrorToggled(ECheckBoxState NewState, int32 FrameIndex);

	/** Called when the Animation property changes, forces a UI rebuild. */
	void OnAnimationChanged();
};
