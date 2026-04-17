// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#include "FlipbookAnimDataSourceCustomization.h"
#include "AnimSequences/PaperZDFlipbookAnimDataSource.h"
#include "PaperFlipbook.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PaperZDFlipbookAnimDataSourceCustomization"

TSharedRef<IPropertyTypeCustomization> FPaperZDFlipbookAnimDataSourceCustomization::MakeInstance()
{
	return MakeShareable(new FPaperZDFlipbookAnimDataSourceCustomization);
}

void FPaperZDFlipbookAnimDataSourceCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils)
{
	StructPropertyHandle = InPropertyHandle;

	HeaderRow.NameContent()
	[
		InPropertyHandle->CreatePropertyNameWidget()
	];
}

void FPaperZDFlipbookAnimDataSourceCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& PropertyTypeCustomizationUtils)
{
	PropertyUtilities = PropertyTypeCustomizationUtils.GetPropertyUtilities();

	// Cache child handles
	AnimationHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPaperZDFlipbookAnimDataSource, Animation));
	TSharedPtr<IPropertyHandle> CompositeLayersHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPaperZDFlipbookAnimDataSource, CompositeLayerAnimations));
	MirrorModeHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPaperZDFlipbookAnimDataSource, MirrorMode));
	HorizontalMirroredKeyFramesHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPaperZDFlipbookAnimDataSource, MirroredKeyFrames));
	VerticalMirroredKeyFramesHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPaperZDFlipbookAnimDataSource, VerticalMirroredKeyFrames));

	// Show Animation and CompositeLayerAnimations normally
	if (AnimationHandle.IsValid())
	{
		StructBuilder.AddProperty(AnimationHandle.ToSharedRef());

		// Swap the checkbox host content when the flipbook assignment changes (no full detail refresh).
		AnimationHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPaperZDFlipbookAnimDataSourceCustomization::OnAnimationChanged));
	}

	if (CompositeLayersHandle.IsValid())
	{
		StructBuilder.AddProperty(CompositeLayersHandle.ToSharedRef());
	}

	// MirrorMode dropdown. Visibility of the per-frame rows below is bound to an attribute,
	// so we intentionally don't hook SetOnPropertyValueChanged here (no refresh needed).
	if (MirrorModeHandle.IsValid())
	{
		StructBuilder.AddProperty(MirrorModeHandle.ToSharedRef());
	}

	// Per-frame checkbox rows. Always added, but only visible when MirrorMode == PerFrame.
	// Row-level visibility binding lets us avoid ForceRefresh when the mode changes — which would
	// otherwise reset sibling customizations like the directional-animation direction picker.
	const TAttribute<EVisibility> PerFrameVisibility = TAttribute<EVisibility>::CreateSP(this, &FPaperZDFlipbookAnimDataSourceCustomization::GetPerFrameRowVisibility);

	const FText HorizontalRowTooltip = LOCTEXT("HorizontalMirroredFramesTooltip", "Tick each key frame of the assigned flipbook that should render horizontally mirrored (flips the sprite's X scale).");
	const FText VerticalRowTooltip = LOCTEXT("VerticalMirroredFramesTooltip", "Tick each key frame of the assigned flipbook that should render vertically mirrored (flips the sprite's Z scale).");

	StructBuilder.AddCustomRow(LOCTEXT("HorizontalMirroredFramesFilter", "Mirrored Frames (Horizontal)"))
		.Visibility(PerFrameVisibility)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("HorizontalMirroredFramesLabel", "Mirrored Key Frames (Horizontal)"))
			.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
			.ToolTipText(HorizontalRowTooltip)
		]
		.ValueContent()
		.MinDesiredWidth(400.0f)
		.MaxDesiredWidth(0.0f)
		[
			SAssignNew(HorizontalCheckboxHost, SBox)
			[
				BuildCheckboxWidget(EMirrorAxis::Horizontal)
			]
		];

	StructBuilder.AddCustomRow(LOCTEXT("VerticalMirroredFramesFilter", "Mirrored Frames (Vertical)"))
		.Visibility(PerFrameVisibility)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("VerticalMirroredFramesLabel", "Mirrored Key Frames (Vertical)"))
			.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
			.ToolTipText(VerticalRowTooltip)
		]
		.ValueContent()
		.MinDesiredWidth(400.0f)
		.MaxDesiredWidth(0.0f)
		[
			SAssignNew(VerticalCheckboxHost, SBox)
			[
				BuildCheckboxWidget(EMirrorAxis::Vertical)
			]
		];
}

TSharedRef<SWidget> FPaperZDFlipbookAnimDataSourceCustomization::BuildCheckboxWidget(EMirrorAxis Axis)
{
	// Read the flipbook from the Animation property
	UObject* FlipbookObject = nullptr;
	if (AnimationHandle.IsValid())
	{
		AnimationHandle->GetValue(FlipbookObject);
	}

	UPaperFlipbook* Flipbook = Cast<UPaperFlipbook>(FlipbookObject);
	if (!Flipbook)
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("NoFlipbook", "Assign a flipbook to configure mirrored frames"))
			.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
			.ColorAndOpacity(FSlateColor::UseSubduedForeground());
	}

	const int32 NumKeyFrames = Flipbook->GetNumKeyFrames();
	if (NumKeyFrames == 0)
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("NoFrames", "Flipbook has no key frames"))
			.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
			.ColorAndOpacity(FSlateColor::UseSubduedForeground());
	}

	// Build a wrap box with one checkbox per key frame
	TSharedRef<SWrapBox> WrapBox = SNew(SWrapBox)
		.UseAllottedSize(true);

	const FText AxisTooltipFormat = Axis == EMirrorAxis::Horizontal
		? LOCTEXT("FrameMirrorHorizontalTooltip", "Mirror frame {0} horizontally")
		: LOCTEXT("FrameMirrorVerticalTooltip", "Mirror frame {0} vertically");

	for (int32 i = 0; i < NumKeyFrames; i++)
	{
		WrapBox->AddSlot()
			.Padding(FMargin(2.0f, 4.0f, 2.0f, 4.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::AsNumber(i))
					.Font(FCoreStyle::Get().GetFontStyle("SmallFont"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				[
					SNew(SCheckBox)
					.IsChecked(this, &FPaperZDFlipbookAnimDataSourceCustomization::IsFrameMirrored, i, Axis)
					.OnCheckStateChanged(this, &FPaperZDFlipbookAnimDataSourceCustomization::OnFrameMirrorToggled, i, Axis)
					.ToolTipText(FText::Format(AxisTooltipFormat, FText::AsNumber(i)))
				]
			];
	}

	return WrapBox;
}

ECheckBoxState FPaperZDFlipbookAnimDataSourceCustomization::IsFrameMirrored(int32 FrameIndex, EMirrorAxis Axis) const
{
	// Read the struct through raw data access so we can inspect the correct per-axis array
	TArray<const void*> RawData;
	StructPropertyHandle->AccessRawData(RawData);

	if (RawData.Num() > 0 && RawData[0] != nullptr)
	{
		const FPaperZDFlipbookAnimDataSource* DataSource = static_cast<const FPaperZDFlipbookAnimDataSource*>(RawData[0]);
		const TArray<int32>& Array = Axis == EMirrorAxis::Horizontal
			? DataSource->MirroredKeyFrames
			: DataSource->VerticalMirroredKeyFrames;
		return Array.Contains(FrameIndex) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	return ECheckBoxState::Unchecked;
}

void FPaperZDFlipbookAnimDataSourceCustomization::OnFrameMirrorToggled(ECheckBoxState NewState, int32 FrameIndex, EMirrorAxis Axis)
{
	TSharedPtr<IPropertyHandle> ArrayHandle = GetArrayHandleForAxis(Axis);
	if (!ArrayHandle.IsValid() || !StructPropertyHandle.IsValid())
	{
		return;
	}

	// Read current array state through raw data
	TArray<const void*> RawData;
	StructPropertyHandle->AccessRawData(RawData);
	if (RawData.Num() == 0 || RawData[0] == nullptr)
	{
		return;
	}

	const FPaperZDFlipbookAnimDataSource* DataSource = static_cast<const FPaperZDFlipbookAnimDataSource*>(RawData[0]);
	TArray<int32> NewMirroredFrames = Axis == EMirrorAxis::Horizontal
		? DataSource->MirroredKeyFrames
		: DataSource->VerticalMirroredKeyFrames;

	// Add or remove the frame index
	if (NewState == ECheckBoxState::Checked)
	{
		NewMirroredFrames.AddUnique(FrameIndex);
	}
	else
	{
		NewMirroredFrames.Remove(FrameIndex);
	}

	// Sort for consistent ordering in the serialized data
	NewMirroredFrames.Sort();

	// Build the formatted string representation for the property system: (elem0,elem1,...)
	FString FormattedValue;
	if (NewMirroredFrames.Num() > 0)
	{
		TArray<FString> Elements;
		for (int32 Idx : NewMirroredFrames)
		{
			Elements.Add(FString::FromInt(Idx));
		}
		FormattedValue = FString::Printf(TEXT("(%s)"), *FString::Join(Elements, TEXT(",")));
	}
	else
	{
		FormattedValue = TEXT("()");
	}

	// Apply through the property handle for undo/redo support
	FScopedTransaction Transaction(LOCTEXT("ToggleMirroredFrame", "Toggle Mirrored Frame"));
	ArrayHandle->SetValueFromFormattedString(FormattedValue);
}

void FPaperZDFlipbookAnimDataSourceCustomization::OnAnimationChanged()
{
	// Swap only the checkbox-host content so the new flipbook's frame count is reflected,
	// without triggering a full detail-panel refresh (which would reset sibling customizations).
	if (HorizontalCheckboxHost.IsValid())
	{
		HorizontalCheckboxHost->SetContent(BuildCheckboxWidget(EMirrorAxis::Horizontal));
	}
	if (VerticalCheckboxHost.IsValid())
	{
		VerticalCheckboxHost->SetContent(BuildCheckboxWidget(EMirrorAxis::Vertical));
	}
}

TSharedPtr<IPropertyHandle> FPaperZDFlipbookAnimDataSourceCustomization::GetArrayHandleForAxis(EMirrorAxis Axis) const
{
	return Axis == EMirrorAxis::Horizontal ? HorizontalMirroredKeyFramesHandle : VerticalMirroredKeyFramesHandle;
}

bool FPaperZDFlipbookAnimDataSourceCustomization::IsPerFrameModeActive() const
{
	if (!StructPropertyHandle.IsValid())
	{
		return false;
	}

	TArray<const void*> RawData;
	StructPropertyHandle->AccessRawData(RawData);
	if (RawData.Num() == 0 || RawData[0] == nullptr)
	{
		return false;
	}

	const FPaperZDFlipbookAnimDataSource* DataSource = static_cast<const FPaperZDFlipbookAnimDataSource*>(RawData[0]);
	return DataSource->MirrorMode == EPaperZDFlipbookMirrorMode::PerFrame;
}

EVisibility FPaperZDFlipbookAnimDataSourceCustomization::GetPerFrameRowVisibility() const
{
	return IsPerFrameModeActive() ? EVisibility::Visible : EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
