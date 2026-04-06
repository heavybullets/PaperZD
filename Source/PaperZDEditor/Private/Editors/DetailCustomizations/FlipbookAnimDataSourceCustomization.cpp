// Copyright 2017 ~ 2026 Critical Failure Studio Ltd. All rights reserved.

#include "FlipbookAnimDataSourceCustomization.h"
#include "AnimSequences/PaperZDFlipbookAnimDataSource.h"
#include "PaperFlipbook.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "ScopedTransaction.h"
#include "Widgets/Input/SCheckBox.h"
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
	MirroredKeyFramesHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPaperZDFlipbookAnimDataSource, MirroredKeyFrames));
	TSharedPtr<IPropertyHandle> CompositeLayersHandle = InPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPaperZDFlipbookAnimDataSource, CompositeLayerAnimations));

	// Show Animation and CompositeLayerAnimations normally
	if (AnimationHandle.IsValid())
	{
		StructBuilder.AddProperty(AnimationHandle.ToSharedRef());

		// Rebuild checkboxes when the flipbook assignment changes
		AnimationHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this, &FPaperZDFlipbookAnimDataSourceCustomization::OnAnimationChanged));
	}

	if (CompositeLayersHandle.IsValid())
	{
		StructBuilder.AddProperty(CompositeLayersHandle.ToSharedRef());
	}

	// Replace the raw MirroredKeyFrames array with a checkbox row
	StructBuilder.AddCustomRow(LOCTEXT("MirroredFramesFilter", "Mirrored Frames"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("MirroredFramesLabel", "Mirrored Key Frames"))
			.Font(IPropertyTypeCustomizationUtils::GetRegularFont())
		]
		.ValueContent()
		.MinDesiredWidth(400.0f)
		.MaxDesiredWidth(0.0f)
		[
			BuildCheckboxWidget()
		];
}

TSharedRef<SWidget> FPaperZDFlipbookAnimDataSourceCustomization::BuildCheckboxWidget()
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
					.IsChecked(this, &FPaperZDFlipbookAnimDataSourceCustomization::IsFrameMirrored, i)
					.OnCheckStateChanged(this, &FPaperZDFlipbookAnimDataSourceCustomization::OnFrameMirrorToggled, i)
					.ToolTipText(FText::Format(LOCTEXT("FrameMirrorTooltip", "Mirror frame {0}"), FText::AsNumber(i)))
				]
			];
	}

	return WrapBox;
}

ECheckBoxState FPaperZDFlipbookAnimDataSourceCustomization::IsFrameMirrored(int32 FrameIndex) const
{
	if (!MirroredKeyFramesHandle.IsValid())
	{
		return ECheckBoxState::Unchecked;
	}

	// Read the array through raw data access
	TArray<const void*> RawData;
	StructPropertyHandle->AccessRawData(RawData);

	if (RawData.Num() > 0 && RawData[0] != nullptr)
	{
		const FPaperZDFlipbookAnimDataSource* DataSource = static_cast<const FPaperZDFlipbookAnimDataSource*>(RawData[0]);
		return DataSource->IsKeyFrameMirrored(FrameIndex) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	return ECheckBoxState::Unchecked;
}

void FPaperZDFlipbookAnimDataSourceCustomization::OnFrameMirrorToggled(ECheckBoxState NewState, int32 FrameIndex)
{
	if (!MirroredKeyFramesHandle.IsValid() || !StructPropertyHandle.IsValid())
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
	TArray<int32> NewMirroredFrames = DataSource->MirroredKeyFrames;

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
	MirroredKeyFramesHandle->SetValueFromFormattedString(FormattedValue);
}

void FPaperZDFlipbookAnimDataSourceCustomization::OnAnimationChanged()
{
	// Force a detail panel refresh so checkboxes rebuild with the new flipbook's frame count
	if (PropertyUtilities.IsValid())
	{
		PropertyUtilities->ForceRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
