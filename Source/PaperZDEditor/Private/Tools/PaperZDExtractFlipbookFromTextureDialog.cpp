// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "PaperZDExtractFlipbookFromTextureDialog.h"
#include "PaperZDExtractFlipbookFromTextureSettings.h"

#include "AssetToolsModule.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "IPaperZDEditorProxy.h"
#include "PackageTools.h"
#include "PaperFlipbookFactory.h"
#include "PaperImporterSettings.h"
#include "PaperSprite.h"
#include "PaperSpriteFactory.h"
#include "PaperZDAnimBP.h"
#include "PaperZDAnimBPGeneratedClass.h"
#include "PaperZDAnimInstance.h"
#include "PropertyEditorModule.h"
#include "SPrimaryButton.h"
#include "Editors/Util/PaperZDVersionCompatibility.h"
#if PAPERZD_UE_5_8_OR_LATER
#include "Widgets/Input/SSearchableComboBox.h"
#else
#include "SSearchableComboBox.h"
#endif
#include "TextureCompiler.h"
#include "TextureResource.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include "AnimSequences/PaperZDAnimSequence_Flipbook.h"
#include "AnimSequences/Sources/PaperZDAnimationSource_Flipbook.h"
#include "AnimSequences/Skins/PaperZDAnimationSkin_Flipbook.h"

#include "Factories/PaperZDAnimationSkinFactory.h"

#include "Kismet2/KismetEditorUtilities.h"

#include "Misc/FeedbackContext.h"

#include "Util/PaperZDEditorSettings.h"

#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "PaperZDTools"

namespace SPaperZDExtractFlipbookFromTextureHelper
{
    FVector2D CalculatePivotPointFromPivotMode(const UPaperSprite* Sprite, ESpritePivotMode::Type PivotMode, FVector2D CustomPivotPoint)
    {
        FVector2D PivotPoint(0.0f);

        if (Sprite)
        {
            FVector2D TopLeftUV = Sprite->GetSourceUV();
            FVector2D Dimension = Sprite->GetSourceSize();

            if (Sprite->IsTrimmedInSourceImage())
            {
                TopLeftUV = TopLeftUV - Sprite->GetOriginInSourceImageBeforeTrimming();
                Dimension = Sprite->GetSourceImageDimensionBeforeTrimming();
            }

            if (Sprite->IsRotatedInSourceImage())
            {
                switch (PivotMode)
                {
                    case ESpritePivotMode::Top_Left:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X, TopLeftUV.Y);
                        break;
                    }

                    case ESpritePivotMode::Top_Center:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X, TopLeftUV.Y + Dimension.Y * 0.5f);
                        break;
                    }

                    case ESpritePivotMode::Top_Right:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X, TopLeftUV.Y + Dimension.Y);
                        break;
                    }

                    case ESpritePivotMode::Center_Left:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X * 0.5f, TopLeftUV.Y);
                        break;
                    }

                    case ESpritePivotMode::Center_Center:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X * 0.5f, TopLeftUV.Y + Dimension.Y * 0.5f);
                        break;
                    }

                    case ESpritePivotMode::Center_Right:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X * 0.5f, TopLeftUV.Y + Dimension.Y);
                        break;
                    }

                    case ESpritePivotMode::Bottom_Left:
                    {
                        PivotPoint = TopLeftUV;
                        break;
                    }

                    case ESpritePivotMode::Bottom_Center:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X, TopLeftUV.Y + Dimension.Y * 0.5f);
                        break;
                    }

                    case ESpritePivotMode::Bottom_Right:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X, TopLeftUV.Y + Dimension.Y);
                        break;
                    }

                    default:
                    case ESpritePivotMode::Custom:
                    {
                        if (CustomPivotPoint != FVector2D::ZeroVector)
                        {
                            PivotPoint = CustomPivotPoint;
                        }
                        else
                        {
                            Sprite->GetPivotMode(PivotPoint);
                        }

                        break;
                    }
                }
            }
            else
            {
                switch (PivotMode)
                {
                    case ESpritePivotMode::Top_Left:
                    {
                        PivotPoint = TopLeftUV;
                        break;
                    }

                    case ESpritePivotMode::Top_Center:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X * 0.5f, TopLeftUV.Y);
                        break;
                    }

                    case ESpritePivotMode::Top_Right:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X, TopLeftUV.Y);
                        break;
                    }

                    case ESpritePivotMode::Center_Left:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X, TopLeftUV.Y + Dimension.Y * 0.5f);
                        break;
                    }

                    case ESpritePivotMode::Center_Center:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X * 0.5f, TopLeftUV.Y + Dimension.Y * 0.5f);
                        break;
                    }

                    case ESpritePivotMode::Center_Right:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X, TopLeftUV.Y + Dimension.Y * 0.5f);
                        break;
                    }

                    case ESpritePivotMode::Bottom_Left:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X, TopLeftUV.Y + Dimension.Y);
                        break;
                    }

                    case ESpritePivotMode::Bottom_Center:
                    {
                        PivotPoint = FVector2D(TopLeftUV.X + Dimension.X * 0.5f, TopLeftUV.Y + Dimension.Y);
                        break;
                    }

                    case ESpritePivotMode::Bottom_Right:
                    {
                        PivotPoint = TopLeftUV + Dimension;
                        break;
                    }

                    default:
                    case ESpritePivotMode::Custom:
                    {
                        if (CustomPivotPoint != FVector2D::ZeroVector)
                        {
                            PivotPoint = CustomPivotPoint;
                        }
                        else
                        {
                            Sprite->GetPivotMode(PivotPoint);
                        }

                        break;
                    }
                }
            }
        }

        return PivotPoint;
    }

    FVector2D GetCurrentPivotPoint(const UPaperSprite* Sprite)
    {
        FVector2D PivotPoint(0.0f);
        if (Sprite)
        {
            ESpritePivotMode::Type PivotMode = Sprite->GetPivotMode(PivotPoint);
            PivotPoint = CalculatePivotPointFromPivotMode(Sprite, PivotMode, PivotPoint);
        }

        return PivotPoint;
    }

    FVector2D GetResultPivotPoint(const UPaperSprite* Sprite, const FPaperZDExtractFlipbookSettings& Settings)
    {
        FVector2D PivotPoint(0.0f);
        if (Sprite)
        {
            if (Settings.Sprite.PivotMode == ESpritePivotMode::Custom)
            {
                const FVector2D BasePivotPoint = CalculatePivotPointFromPivotMode(Sprite, Settings.Sprite.CustomPivotModeReference, Settings.Sprite.CustomPivotPoint);
                if (Settings.Sprite.CustomPivotModeReference == ESpritePivotMode::Custom)
                {
                    PivotPoint = Settings.Sprite.CustomPivotPoint;
                }
                else
                {
                    PivotPoint = BasePivotPoint + Settings.Sprite.CustomPivotPoint;
                }
            }
            else
            {
                PivotPoint = CalculatePivotPointFromPivotMode(Sprite, Settings.Sprite.PivotMode, Settings.Sprite.CustomPivotPoint);
            }
        }

        return PivotPoint;
    }

    FSpriteGeometryCollection* GetRenderGeometry(UPaperSprite* Sprite)
    {
        const FStructProperty* StructProp = FindFProperty<FStructProperty>(UPaperSprite::StaticClass(), TEXT("RenderGeometry"));
        return StructProp ? StructProp->ContainerPtrToValuePtr<FSpriteGeometryCollection>(Sprite) : nullptr;
    }

    // Reads the alpha of the texture source, only the 8 bit BGRA source format is supported (the array is left empty otherwise)
    void LoadTextureAlpha(UTexture2D* Texture, TArray<uint8>& OutAlpha)
    {
        OutAlpha.Reset();

        TArray64<uint8> MipData;
        if (Texture && Texture->Source.GetFormat() == TSF_BGRA8 && Texture->Source.GetMipData(MipData, 0))
        {
            OutAlpha.SetNumUninitialized(Texture->Source.GetSizeX() * Texture->Source.GetSizeY());
            for (int32 PixelIndex = 0; PixelIndex < OutAlpha.Num(); ++PixelIndex)
            {
                OutAlpha[PixelIndex] = MipData[PixelIndex * 4 + 3];
            }
        }
    }

    bool IsRectFullyTransparent(const UTexture2D* Texture, const TArray<uint8>& Alpha, const FIntRect& Rect)
    {
        const int32 TextureWidth = Texture->Source.GetSizeX();
        const int32 TextureHeight = Texture->Source.GetSizeY();
        if (Alpha.IsEmpty() || Alpha.Num() != TextureWidth * TextureHeight)
        {
            return false;
        }

        for (int32 Y = FMath::Max(Rect.Min.Y, 0); Y < FMath::Min(Rect.Max.Y, TextureHeight); ++Y)
        {
            for (int32 X = FMath::Max(Rect.Min.X, 0); X < FMath::Min(Rect.Max.X, TextureWidth); ++X)
            {
                if (Alpha[Y * TextureWidth + X] > 0)
                {
                    return false;
                }
            }
        }

        return true;
    }

    // An empty sprite has nothing to fit the geometry to (the tight bounding box becomes invalid), so it gets no render geometry
    void ApplyEmptyRenderGeometry(UPaperSprite* Sprite)
    {
        if (FSpriteGeometryCollection* RenderGeometry = GetRenderGeometry(Sprite))
        {
            RenderGeometry->Shapes.Empty();
            RenderGeometry->GeometryType = ESpritePolygonMode::FullyCustom;
        }
    }

    void ApplyRenderGeometry(UPaperSprite* Sprite, const FPaperZDExtractFlipbookSpriteSettings& Settings)
    {
        FSpriteGeometryCollection* RenderGeometry = GetRenderGeometry(Sprite);
        if (!RenderGeometry)
        {
            return;
        }

        if (Settings.RenderGeometryType != ESpritePolygonMode::FullyCustom)
        {
            RenderGeometry->GeometryType = Settings.RenderGeometryType;
            return;
        }

        // Build the reference geometry first, then move each of its 4 vertices by its offset and freeze it as fully custom
        const bool bValidReference = Settings.CustomRenderGeometryReference != ESpritePolygonMode::FullyCustom;
        RenderGeometry->GeometryType = bValidReference ? Settings.CustomRenderGeometryReference.GetValue() : ESpritePolygonMode::TightBoundingBox;
        Sprite->PostEditChange();

        for (FSpriteGeometryShape& Shape : RenderGeometry->Shapes)
        {
            // Only quad shapes (Source/Tight bounding box) have the 4 vertices the offsets map to
            if (Shape.Vertices.Num() != 4)
            {
                UE_LOG(LogTemp, Warning, TEXT("Custom render geometry offsets require a 4 vertex reference geometry (Source/Tight bounding box), the offsets were ignored."));
                continue;
            }

            for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
            {
                Shape.Vertices[VertexIndex] += Settings.CustomRenderGeometryOffset[VertexIndex];
            }

            // The shape is no longer a box once the vertices move
            Shape.ShapeType = ESpriteShapeType::Polygon;
        }

        RenderGeometry->GeometryType = ESpritePolygonMode::FullyCustom;
    }
}

void SPaperZDExtractFlipbookFromTextureDialog::Construct(const FArguments& InArgs, UTexture2D* Texture)
{
    SourceTexture = Texture;

    DialogSettings = NewObject<UPaperZDExtractFlipbookFromTextureDialogSettings>();
    DialogSettings->AddToRoot();

    FString TextureName = FPaths::GetBaseFilename(SourceTexture->GetPathName());
    if (TextureName.StartsWith(TEXT("T_"), ESearchCase::CaseSensitive))
    {
        TextureName.RightChopInline(2);
    }

    DialogSettings->Naming.FlipbookNamingTemplate = "FB_" + TextureName;
    DialogSettings->Naming.SpriteNamingTemplate = "S_" + TextureName;
    DialogSettings->Naming.AnimationSkinNamingTemplate = "ASK_" + TextureName;
    DialogSettings->Naming.AnimationSequenceNamingTemplate = "ASQ_" + TextureName;
    DialogSettings->Naming.AnimationSourceNamingTemplate = "AS_" + TextureName;
    DialogSettings->Naming.AnimationBlueprintNamingTemplate = "ABP_" + TextureName;

    InitializeFlipbookExtractSettings(DialogSettings->Flipbooks.Add_GetRef(FPaperZDExtractFlipbookSettings()));

    PreviewSelectedFlipbooks();

    FPropertyEditorModule& EditModule = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
    
    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.bAllowSearch = false;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsViewArgs.bHideSelectionTip = true;
    
    MainPropertyView = EditModule.CreateDetailView(DetailsViewArgs);
    MainPropertyView->SetObject(DialogSettings);
    MainPropertyView->OnFinishedChangingProperties().AddSP(this, &SPaperZDExtractFlipbookFromTextureDialog::OnFinishedChangingProperties);

    SavedPresetOptions.Add(MakeShared<FString>(TEXT("None")));
    for (const FPaperZDExtractFlipbookPreset& Preset : GetDefault<UPaperZDEditorSettings>()->ExtractFlipbookToolPresets)
    {
        SavedPresetOptions.Add(MakeShared<FString>(Preset.PresetName));
    }
    SelectedPresetOption = SavedPresetOptions[0];

    TSharedRef<SPaperZDExtractFlipbookFromTextureViewport> Viewport = SNew(SPaperZDExtractFlipbookFromTextureViewport, SourceTexture, Flipbooks, DialogSettings, this);
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(0.0f, 3.0f, 1.0f, 0.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				Viewport
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(2.0f)
				.FillHeight(2)
				[
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
					    MainPropertyView.ToSharedRef()
                    ]
                    + SScrollBox::Slot()
                    .Padding(FMargin(0.0f, 0.0f))
                    [
                        SNew(SExpandableArea)
                        .AreaTitle(FText::FromString(" Extraction Presets"))
                        .AreaTitlePadding(FMargin(0.0f, 4.0f))
                        .InitiallyCollapsed(false)
                        .BorderBackgroundColor(FLinearColor(1.7f, 1.7f, 1.7f))
                        .Padding(0.0f)
                        .BodyContent()
                        [
                            SNew(SVerticalBox)
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(SSeparator)
                                .Thickness(1.0f)
                                .ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(12.0f, 8.0f)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0.f, 0.f, 8.f, 0.f)
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString("Saved Presets"))
                                    .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
                                    .ColorAndOpacity(FSlateColor::UseForeground())
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                .VAlign(VAlign_Center)
                                [
                                    SAssignNew(SavedPresetComboBox, SSearchableComboBox)
                                    .OptionsSource(&SavedPresetOptions)
                                    .InitiallySelectedItem(SelectedPresetOption)
                                    .OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewValue, ESelectInfo::Type)
                                    {
                                        if (NewValue.IsValid())
                                        {
                                            SelectedPresetOption = NewValue;
                                        }
                                    })
                                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> InOption)
                                    {
                                        return SNew(STextBlock).Text(FText::FromString(*InOption));
                                    })
                                    [
                                        SNew(STextBlock)
                                        .Text_Lambda([this]() { return FText::FromString(SelectedPresetOption.IsValid() ? *SelectedPresetOption : TEXT("")); })
                                    ]
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(8.f, 0.f, 0.f, 0.f)
                                [
                                    SNew(SButton)
                                    .Text(FText::FromString("Apply"))
                                    .OnClicked(this, &SPaperZDExtractFlipbookFromTextureDialog::ApplyPresetClicked)
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(8.f, 0.f, 0.f, 0.f)
                                [
                                    SNew(SButton)
                                    .Text(FText::FromString("Delete"))
                                    .OnClicked(this, &SPaperZDExtractFlipbookFromTextureDialog::DeletePresetClicked)
                                ]
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(SSeparator)
                                .Thickness(1.0f)
                                .ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            .Padding(12.0f, 8.0f)
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                .Padding(0.f, 0.f, 8.f, 0.f)
                                [
                                    SNew(STextBlock)
                                    .Text(FText::FromString("New Preset"))
                                    .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
                                    .ColorAndOpacity(FSlateColor::UseForeground())
                                ]
                                + SHorizontalBox::Slot()
                                .FillWidth(1.0f)
                                .VAlign(VAlign_Center)
                                .Padding(0.f, 0.f, 8.f, 0.f)
                                [
                                    SNew(SEditableTextBox)
                                    .HintText(FText::FromString("Preset Name"))
                                    .Text_Lambda([this]() { return FText::FromString(NewPresetName); })
                                    .OnTextChanged_Lambda([this](const FText& NewText)
                                    {
                                        NewPresetName = NewText.ToString();
                                    })
                                    .OnTextCommitted_Lambda([this](const FText& NewText, ETextCommit::Type CommitType)
                                    {
                                        NewPresetName = NewText.ToString();
                                    })
                                ]
                                + SHorizontalBox::Slot()
                                .AutoWidth()
                                .VAlign(VAlign_Center)
                                [
                                    SNew(SButton)
                                    .Text(FText::FromString("Save"))
                                    .OnClicked(this, &SPaperZDExtractFlipbookFromTextureDialog::SavePresetClicked)
                                ]
                            ]
                            + SVerticalBox::Slot()
                            .AutoHeight()
                            [
                                SNew(SSeparator)
                                .Thickness(1.0f)
                                .ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f))
                            ]
                        ]
                    ]
				]
				+ SVerticalBox::Slot()
				.Padding(FMargin(16))
				.HAlign(HAlign_Right)
				.AutoHeight()
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SPrimaryButton)
						.Text(LOCTEXT("PaperExtractSpritesExtractButton", "Extract"))
						.OnClicked(this, &SPaperZDExtractFlipbookFromTextureDialog::ExtractClicked)
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.Text(LOCTEXT("PaperExtractSpritesCancelButton", "Cancel"))
                        .TextStyle(FAppStyle::Get(), "DialogButtonText")
						.HAlign(HAlign_Center)
                        .OnClicked(this, &SPaperZDExtractFlipbookFromTextureDialog::CancelClicked)
					]
				]
			]
		]
	];
}

SPaperZDExtractFlipbookFromTextureDialog::~SPaperZDExtractFlipbookFromTextureDialog()
{
    if (DialogSettings && DialogSettings->IsValidLowLevel())
    {
        DialogSettings->RemoveFromRoot();
    }
}

void SPaperZDExtractFlipbookFromTextureDialog::PreviewSelectedFlipbooks()
{
    Flipbooks.Empty();
    if (SourceTexture != nullptr)
    {
        for (int32 i = 0; i < DialogSettings->Flipbooks.Num(); ++i)
        {
            const FPaperZDExtractFlipbookSettings& FlipbookSettings = DialogSettings->Flipbooks[i];
            FPaperZDExtractedFlipbook& ExtractedFlipbook = Flipbooks.Add_GetRef(FPaperZDExtractedFlipbook());
            ExtractedFlipbook.Index = i;
            ExtractedFlipbook.Flipbook = nullptr;

            int32 ExtractedRectIndex = FlipbookSettings.Naming.SpriteNamingStartIndex;

            // Calculate rects
            const FIntPoint TextureSize(SourceTexture->GetImportedSize());
            const int32 TextureWidth = TextureSize.X;
            const int32 TextureHeight = TextureSize.Y;

            const int32 EffectiveCellWidth = FMath::Clamp(FlipbookSettings.Extraction.CellWidth, 1, TextureWidth);
            const int32 EffectiveCellHeight = FMath::Clamp(FlipbookSettings.Extraction.CellHeight, 1, TextureHeight);

            int NumExtractedCellsY = 0;
            for (int32 Y = FlipbookSettings.Extraction.MarginY; Y + EffectiveCellHeight <= TextureHeight; Y += EffectiveCellHeight + FlipbookSettings.Extraction.SpacingY)
            {
                int NumExtractedCellsX = 0;
                for (int32 X = FlipbookSettings.Extraction.MarginX; X + EffectiveCellWidth <= TextureWidth; X += EffectiveCellWidth + FlipbookSettings.Extraction.SpacingX)
                {
                    FPaperZDExtractedSprite* Sprite = new(ExtractedFlipbook.Sprites)FPaperZDExtractedSprite();

                    Sprite->Index = ExtractedRectIndex;
                    Sprite->Rect = FIntRect(X, Y, X + EffectiveCellWidth, Y + EffectiveCellHeight);
                    ExtractedRectIndex++;

                    ++NumExtractedCellsX;
                    if (FlipbookSettings.Extraction.NumCellsX > 0 && NumExtractedCellsX >= FlipbookSettings.Extraction.NumCellsX)
                    {
                        break;
                    }
                }

                ++NumExtractedCellsY;
                if (FlipbookSettings.Extraction.NumCellsY > 0 && NumExtractedCellsY >= FlipbookSettings.Extraction.NumCellsY)
                {
                    break;
                }
            }
        }
    }
}

FReply SPaperZDExtractFlipbookFromTextureDialog::ExtractClicked()
{
    ExtractFlipbooks();

    CloseContainingWindow();
    return FReply::Handled();
}

FReply SPaperZDExtractFlipbookFromTextureDialog::CancelClicked()
{
    CloseContainingWindow();
    return FReply::Handled();
}

void SPaperZDExtractFlipbookFromTextureDialog::CloseContainingWindow()
{
    TSharedPtr<SWindow> ContainingWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
    if (ContainingWindow.IsValid())
    {
        ContainingWindow->RequestDestroyWindow();
    }
}

FPaperZDExtractFlipbookPreset* GetPreset(const FString& PresetName, UPaperZDEditorSettings* PluginSettings)
{
    FPaperZDExtractFlipbookPreset* FoundPreset = nullptr;
    if (PluginSettings && !PresetName.IsEmpty())
    {
        for (FPaperZDExtractFlipbookPreset& Preset : PluginSettings->ExtractFlipbookToolPresets)
        {
            if (Preset.PresetName == PresetName)
            {
                FoundPreset = &Preset;
                break;
            }
        }
    }

    return FoundPreset;
}

FReply SPaperZDExtractFlipbookFromTextureDialog::ApplyPresetClicked()
{
    if (SelectedPresetOption.IsValid() && SelectedPresetOption != SavedPresetOptions[0])
    {
        UPaperZDEditorSettings* PluginSettings = GetMutableDefault<UPaperZDEditorSettings>();
        if (const FPaperZDExtractFlipbookPreset* Preset = GetPreset(*SelectedPresetOption, PluginSettings))
        {
            DialogSettings->Animation = Preset->AnimationSettings;
            DialogSettings->Flipbooks = Preset->FlipbookSettings;
            NewPresetName = Preset->PresetName;
            PreviewSelectedFlipbooks();
        }
    }

    return FReply::Handled();
}

FReply SPaperZDExtractFlipbookFromTextureDialog::DeletePresetClicked()
{
    if (SelectedPresetOption.IsValid() && SelectedPresetOption != SavedPresetOptions[0])
    {
        UPaperZDEditorSettings* PluginSettings = GetMutableDefault<UPaperZDEditorSettings>();
        if (FPaperZDExtractFlipbookPreset* Preset = GetPreset(*SelectedPresetOption, PluginSettings))
        {
            PluginSettings->ExtractFlipbookToolPresets.RemoveAll([Preset](const FPaperZDExtractFlipbookPreset& Item)
            {
                return Item.PresetName == Preset->PresetName;
            });

            PluginSettings->TryUpdateDefaultConfigFile();
            SavedPresetOptions.Remove(SelectedPresetOption);
            SelectedPresetOption = SavedPresetOptions[0];
            NewPresetName.Empty();
            SavedPresetComboBox->RefreshOptions();
        }
    }

    return FReply::Handled();
}

FReply SPaperZDExtractFlipbookFromTextureDialog::SavePresetClicked()
{
    if (!NewPresetName.IsEmpty())
    {
        UPaperZDEditorSettings* PluginSettings = GetMutableDefault<UPaperZDEditorSettings>();
        FPaperZDExtractFlipbookPreset* Preset = GetPreset(NewPresetName, PluginSettings);
        if (!Preset)
        {
            FPaperZDExtractFlipbookPreset NewPreset;
            NewPreset.PresetName = NewPresetName;
            Preset = &PluginSettings->ExtractFlipbookToolPresets.Add_GetRef(NewPreset);
            SavedPresetOptions.Add(MakeShared<FString>(Preset->PresetName));
        }

        if (Preset)
        {
            Preset->AnimationSettings = DialogSettings->Animation;
            Preset->FlipbookSettings = DialogSettings->Flipbooks;
            PluginSettings->TryUpdateDefaultConfigFile();
            SavedPresetComboBox->RefreshOptions();
        }
    }

    return FReply::Handled();
}

void SPaperZDExtractFlipbookFromTextureDialog::ExtractFlipbooks()
{
    FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    const UPaperImporterSettings* ImporterSettings = GetDefault<UPaperImporterSettings>();

    TArray<UObject*> ObjectsToSync;
    bool bCancelled = false;

    // Read once, used to detect fully transparent sprites
    TArray<uint8> TextureAlpha;
    SPaperZDExtractFlipbookFromTextureHelper::LoadTextureAlpha(SourceTexture, TextureAlpha);

    // A single task for all the flipbooks, so the progress bar shows the overall progress (1 frame per sprite + 1 per generated flipbook)
    const int32 NumFlipbooks = DialogSettings->Flipbooks.Num();
    float TotalWork = 0.0f;
    for (int32 i = 0; i < NumFlipbooks; ++i)
    {
        TotalWork += Flipbooks[i].Sprites.Num() + (DialogSettings->Flipbooks[i].Flipbook.bGenerateFlipbook ? 1 : 0);
    }

    FScopedSlowTask Feedback(TotalWork, NSLOCTEXT("TiledIntegration", "TiledIntegration_ExtractFlipbooksFromTexture", "Extracting Flipbooks From Texture"));
    Feedback.MakeDialog(true);

    for (int32 i = 0; i < NumFlipbooks; ++i)
    {
        const FPaperZDExtractFlipbookSettings& FlipbookSettings = DialogSettings->Flipbooks[i];
        FPaperZDExtractedFlipbook& ExtractedFlipbook = Flipbooks[i];
        bool bUserCancelled = false;

        FString SpriteNamingTemplate = FlipbookSettings.Naming.SpriteNamingTemplate;
        if (SpriteNamingTemplate.Find(TEXT("{1}")) == INDEX_NONE && (FlipbookSettings.Extraction.NumCellsX > 1 || FlipbookSettings.Extraction.NumCellsY > 1))
        {
            SpriteNamingTemplate.Append(TEXT("_{1}"));
        }

        TArray<UPaperSprite*> SpritesExtracted;
        int32 SpriteProgressIndex = 0;
        for (const FPaperZDExtractedSprite& ExtractedSprite : ExtractedFlipbook.Sprites)
        {
            Feedback.EnterProgressFrame(1, FText::Format(NSLOCTEXT("TiledIntegration", "TiledIntegration_ExtractSprite", "Flipbook {0}/{1}: Extracting sprite {2}/{3}"), FText::AsNumber(i + 1), FText::AsNumber(NumFlipbooks), FText::AsNumber(++SpriteProgressIndex), FText::AsNumber(ExtractedFlipbook.Sprites.Num())));

            FString SpriteName = SpriteNamingTemplate;
            SpriteName.ReplaceInline(TEXT("{0}"), *DialogSettings->Naming.SpriteNamingTemplate);
            SpriteName.ReplaceInline(TEXT("{1}"), *FString::Printf(TEXT("%d"), ExtractedSprite.Index));

            FString Path = FPaths::GetPath(SourceTexture->GetOutermost()->GetName()) / SpriteName;

            FString Name;
            FString PackageName;
            FString Suffix;

            AssetToolsModule.Get().CreateUniqueAssetName(Path, Suffix, PackageName, Name);
            const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
            UPackage* Package = CreatePackage(*PackageName);

            const FIntRect& SpriteRect = ExtractedSprite.Rect;
            FSpriteAssetInitParameters SpriteInitParams;
            SpriteInitParams.Texture = SourceTexture;
            SpriteInitParams.Offset = SpriteRect.Min;
            SpriteInitParams.Dimension = FIntPoint(SpriteRect.Width(), SpriteRect.Height());

            // Look for normal map
            bool bFoundNormalMap = false;
            {
                TArray<FString> NormalMapFileNamesToTest;
                const FString NormalMapNameNoSuffix = ImporterSettings->RemoveSuffixFromBaseMapName(SourceTexture->GetName());
                ImporterSettings->GenerateNormalMapNamesToTest(NormalMapNameNoSuffix, NormalMapFileNamesToTest);
                ImporterSettings->GenerateNormalMapNamesToTest(SourceTexture->GetName(), NormalMapFileNamesToTest);
                const FString SanitizedTexturePackageName = UPackageTools::SanitizePackageName(SourceTexture->GetOutermost()->GetName());
                const FString TexturePackagePath = FPackageName::GetLongPackagePath(SanitizedTexturePackageName);

                // Test each name for an existing asset
                for (const FString& NameToTest : NormalMapFileNamesToTest)
                {
                    const FString NormalMapFilePath = TexturePackagePath / (NameToTest + FString(TEXT(".")) + NameToTest);
                    FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(NormalMapFilePath));
                    if (AssetData.IsValid())
                    {
                        if (UTexture2D* NormalMapTexture = Cast<UTexture2D>(AssetData.GetAsset()))
                        {
                            bFoundNormalMap = true;
                            SpriteInitParams.AdditionalTextures.Add(NormalMapTexture);
                            break;
                        }
                    }
                }
            }

            if (UPaperSprite* NewAsset = NewObject<UPaperSprite>(Package, UPaperSprite::StaticClass(), *Name, RF_Public | RF_Standalone | RF_Transactional))
            {
                ImporterSettings->ApplySettingsForSpriteInit(SpriteInitParams, bFoundNormalMap ? ESpriteInitMaterialLightingMode::ForceLit : ESpriteInitMaterialLightingMode::Automatic);
                NewAsset->InitializeSprite(SpriteInitParams);

                const FVector2D TargetPivotPoint = SPaperZDExtractFlipbookFromTextureHelper::GetResultPivotPoint(NewAsset, FlipbookSettings);
                NewAsset->SetPivotMode(FlipbookSettings.Sprite.PivotMode, TargetPivotPoint);
                
                if (SPaperZDExtractFlipbookFromTextureHelper::IsRectFullyTransparent(SourceTexture, TextureAlpha, SpriteRect))
                {
                    SPaperZDExtractFlipbookFromTextureHelper::ApplyEmptyRenderGeometry(NewAsset);
                }
                else
                {
                    SPaperZDExtractFlipbookFromTextureHelper::ApplyRenderGeometry(NewAsset, FlipbookSettings.Sprite);
                }

                if (FlipbookSettings.Sprite.Material)
                {
                    if (FObjectProperty* Property = FindFProperty<FObjectProperty>(UPaperSprite::StaticClass(), TEXT("DefaultMaterial")))
                    {
                        Property->SetObjectPropertyValue_InContainer(NewAsset, FlipbookSettings.Sprite.Material);
                    }
                }

                if (FProperty* Property = FindFProperty<FProperty>(UPaperSprite::StaticClass(), TEXT("SpriteCollisionDomain")))
                {
                    void* ValuePtr = Property->ContainerPtrToValuePtr<void>(NewAsset);
                    if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
                    {
                        ByteProp->SetIntPropertyValue(ValuePtr, (int64)FlipbookSettings.Sprite.Collision);
                    }
                }

                FAssetRegistryModule::AssetCreated(NewAsset);
                NewAsset->MarkPackageDirty();
                NewAsset->PostEditChange();

                SpritesExtracted.Add(NewAsset);
                ObjectsToSync.Add(NewAsset);
            }

            if (GWarn->ReceivedUserCancel())
            {
                bCancelled = true;
                bUserCancelled = true;
                break;
            }
        }

        if (bUserCancelled)
        {
            break;
        }

        bCancelled |= !FlipbookSettings.Flipbook.bGenerateFlipbook;
        if (!bCancelled)
        {
            Feedback.EnterProgressFrame(1, FText::Format(NSLOCTEXT("TiledIntegration", "TiledIntegration_GenerateFlipbook", "Flipbook {0}/{1}: Generating flipbook"), FText::AsNumber(i + 1), FText::AsNumber(NumFlipbooks)));
            UPaperFlipbookFactory* FlipbookFactory = NewObject<UPaperFlipbookFactory>();
            FlipbookFactory->KeyFrames.Empty();

            for (UPaperSprite* Sprite : SpritesExtracted)
            {
                FPaperFlipbookKeyFrame KeyFrame;
                KeyFrame.Sprite = Sprite;
                KeyFrame.FrameRun = 1;
                FlipbookFactory->KeyFrames.Add(KeyFrame);
            }

            if (!FlipbookFactory->KeyFrames.IsEmpty())
            {
                FString FlipbookName = FlipbookSettings.Naming.FlipbookNamingTemplate;
                FlipbookName.ReplaceInline(TEXT("{0}"), *DialogSettings->Naming.FlipbookNamingTemplate);
                FlipbookName.ReplaceInline(TEXT("{1}"), *FString::Printf(TEXT("%d"), i + FlipbookSettings.Naming.FlipbookNamingStartIndex));
                FString Path = FPaths::GetPath(SourceTexture->GetOutermost()->GetName()) / FlipbookName;

                FString Name;
                FString PackageName;
                FString Suffix;

                AssetToolsModule.Get().CreateUniqueAssetName(Path, Suffix, PackageName, Name);
                const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);

                if (UPaperFlipbook* NewAsset = Cast<UPaperFlipbook>(AssetToolsModule.Get().CreateAsset(Name, PackagePath, UPaperFlipbook::StaticClass(), FlipbookFactory)))
                {
                    if (FProperty* Property = FindFProperty<FProperty>(UPaperFlipbook::StaticClass(), TEXT("FramesPerSecond")))
                    {
                        if (float* FramesPerSecondPtr = reinterpret_cast<float*>(Property->ContainerPtrToValuePtr<void>(NewAsset)))
                        {
                            *FramesPerSecondPtr = FlipbookSettings.Flipbook.FramesPerSecond;
                        }
                    }

                    if (FObjectProperty* Property = FindFProperty<FObjectProperty>(UPaperFlipbook::StaticClass(), TEXT("DefaultMaterial")))
                    {
                        Property->SetObjectPropertyValue_InContainer(NewAsset, FlipbookSettings.Flipbook.Material);
                    }

                    ExtractedFlipbook.Flipbook = NewAsset;
                    ObjectsToSync.Add(NewAsset);
                }
            }
        }
    }

    UPaperZDAnimationSource_Flipbook* AnimationSource = DialogSettings->Animation.AnimationSource;
    if (DialogSettings->Animation.bCreateAnimationSource && !AnimationSource && !bCancelled)
    {
        FString AnimationSourceName = DialogSettings->Naming.AnimationSourceNamingTemplate;
        FString Path = FPaths::GetPath(SourceTexture->GetOutermost()->GetName()) / AnimationSourceName;
        FString Name, PackageName, Suffix;
        AssetToolsModule.Get().CreateUniqueAssetName(Path, Suffix, PackageName, Name);
        const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
        UPackage* Package = CreatePackage(*PackageName);

        if (UPaperZDAnimationSource_Flipbook* NewAsset = NewObject<UPaperZDAnimationSource_Flipbook>(Package, UPaperZDAnimationSource_Flipbook::StaticClass(), *Name, RF_Public | RF_Standalone | RF_Transactional))
        {
            AnimationSource = NewAsset;

            FAssetRegistryModule::AssetCreated(NewAsset);
            NewAsset->MarkPackageDirty();
            NewAsset->PostEditChange();

            ObjectsToSync.Add(NewAsset);
        }
    }

    if (DialogSettings->Animation.bCreateAnimationBlueprint && AnimationSource && !bCancelled)
    {
        FString AnimationBlueprintName = DialogSettings->Naming.AnimationBlueprintNamingTemplate;
        FString Path = FPaths::GetPath(SourceTexture->GetOutermost()->GetName()) / AnimationBlueprintName;
        FString Name, PackageName, Suffix;
        AssetToolsModule.Get().CreateUniqueAssetName(Path, Suffix, PackageName, Name);
        const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
        UPackage* Package = CreatePackage(*PackageName);

        if (UPaperZDAnimBP* NewAsset = CastChecked<UPaperZDAnimBP>(FKismetEditorUtilities::CreateBlueprint(UPaperZDAnimInstance::StaticClass(), Package, *Name, EBlueprintType::BPTYPE_Normal, UPaperZDAnimBP::StaticClass(), UPaperZDAnimBPGeneratedClass::StaticClass())))
        {
            NewAsset->SupportedAnimationSource = AnimationSource;

            FAssetRegistryModule::AssetCreated(NewAsset);
            NewAsset->MarkPackageDirty();
            NewAsset->PostEditChange();

            ObjectsToSync.Add(NewAsset);
        }
    }

    if (AnimationSource && !bCancelled && DialogSettings->Animation.bCreateAnimationSequence)
    {
        int32 LastNoAnimSequenceID = INT32_MAX - 1;
        TMap<uint32, UPaperZDAnimSequence_Flipbook*> AnimSequences;
        for (int32 i = 0; i < DialogSettings->Flipbooks.Num(); ++i)
        {
            const FPaperZDExtractFlipbookSettings& FlipbookSettings = DialogSettings->Flipbooks[i];
            FPaperZDExtractedFlipbook& ExtractedFlipbook = Flipbooks[i];
            if (UPaperFlipbook* Flipbook = ExtractedFlipbook.Flipbook)
            {
                UPaperZDAnimSequence_Flipbook* AnimSequence = nullptr;
                int32 AnimationSequenceID = FlipbookSettings.Animation.AnimationSequenceID;
                if (FlipbookSettings.Animation.AnimationSequenceID < 0)
                {
                    AnimationSequenceID = LastNoAnimSequenceID;
                    LastNoAnimSequenceID--;
                }

                if (auto FoundAnimSequence = AnimSequences.Find(AnimationSequenceID))
                {
                    AnimSequence = *FoundAnimSequence;
                }
                else
                {
                    FPaperZDExtractFlipbooksAnimationSequenceSettings AnimSequenceSettings;
                    if (DialogSettings->Animation.AnimationSequenceSettings.IsValidIndex(AnimationSequenceID))
                    {
                        AnimSequenceSettings = DialogSettings->Animation.AnimationSequenceSettings[AnimationSequenceID];
                    }

                    FString AnimationSequenceName = FlipbookSettings.Naming.AnimSequenceNamingTemplate;
                    AnimationSequenceName.ReplaceInline(TEXT("{0}"), *AnimSequenceSettings.AnimationSequenceNamingTemplate);
                    AnimationSequenceName.ReplaceInline(TEXT("{1}"), *FString::Printf(TEXT("%d"), i + FlipbookSettings.Naming.FlipbookNamingStartIndex));
                    AnimationSequenceName.ReplaceInline(TEXT("{0}"), *DialogSettings->Naming.AnimationSequenceNamingTemplate);

                    FString Path = FPaths::GetPath(SourceTexture->GetOutermost()->GetName()) / AnimationSequenceName;
                    FString Name, PackageName, Suffix;
                    AssetToolsModule.Get().CreateUniqueAssetName(Path, Suffix, PackageName, Name);
                    const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
                    UPackage* Package = CreatePackage(*PackageName);

                    if (UPaperZDAnimSequence_Flipbook* NewAsset = NewObject<UPaperZDAnimSequence_Flipbook>(Package, UPaperZDAnimSequence_Flipbook::StaticClass(), *Name, RF_Public | RF_Standalone | RF_Transactional))
                    {
                        int32 FlipbooksNum = 0;
                        for (const FPaperZDExtractFlipbookSettings& OtherFlipbookSettings : DialogSettings->Flipbooks)
                        {
                            if (OtherFlipbookSettings.Animation.AnimationSequenceID == AnimationSequenceID)
                            {
                                FlipbooksNum++;
                            }
                        }

                        FlipbooksNum = FlipbooksNum > 0 ? FlipbooksNum : 1;
                        NewAsset->SetAnimSource(AnimationSource);
                        NewAsset->bDirectionalSequence = FlipbooksNum > 1;
                        NewAsset->Category = *AnimSequenceSettings.Category;

                        TArray<FPaperZDFlipbookAnimDataSource>& AnimDataSource = NewAsset->AnimData;
                        AnimDataSource.Empty();
                        AnimDataSource.Init(nullptr, FlipbooksNum);

                        FAssetRegistryModule::AssetCreated(NewAsset);
                        NewAsset->MarkPackageDirty();
                        NewAsset->PostEditChange();

                        AnimSequence = NewAsset;

                        ObjectsToSync.Add(NewAsset);
                        AnimSequences.Add(AnimationSequenceID, AnimSequence);
                    }
                }

                if (AnimSequence)
                {
                    int32 DirectionIndex = FlipbookSettings.Animation.AnimationDirectionIndex;
                    TArray<FPaperZDFlipbookAnimDataSource>& AnimDataSource = AnimSequence->AnimData;
                    if (DirectionIndex < 0)
                    {
                        for (int32 j = 0; j < AnimDataSource.Num(); ++j)
                        {
                            TObjectPtr<UPaperFlipbook>& AnimDataFlipbook = AnimDataSource[j].Animation;
                            if (AnimDataFlipbook == nullptr)
                            {
                                DirectionIndex = j;
                                break;
                            }
                        }
                    }

                    if (AnimDataSource.IsValidIndex(DirectionIndex))
                    {
                        AnimDataSource[DirectionIndex].Animation = Flipbook;
                    }
                }
            }
        }
    }

    if (AnimationSource && !bCancelled && !DialogSettings->Animation.AnimationSkinSettings.IsEmpty())
    {
        for (int32 i = 0; i < DialogSettings->Animation.AnimationSkinSettings.Num(); ++i)
        {
            TMap<UPaperZDAnimSequence_Flipbook*, TArray<TObjectPtr<UPaperFlipbook>>> AnimationSkinSequences;
            const FPaperZDExtractFlipbooksAnimationSkinSettings& SkinSettings = DialogSettings->Animation.AnimationSkinSettings[i];
            for (int32 j = 0; j < SkinSettings.AnimationSequences.Num(); ++j)
            {
                UPaperZDAnimSequence_Flipbook* AnimationSkinSequence = SkinSettings.AnimationSequences[j];
                
                TArray<TObjectPtr<UPaperFlipbook>> AnimationSkinSequenceFlipbooks;
                AnimationSkinSequenceFlipbooks.Init(nullptr, AnimationSkinSequence->AnimData.Num());

                for (int32 k = 0; k < DialogSettings->Flipbooks.Num(); ++k)
                {
                    const FPaperZDExtractFlipbookSettings& FlipbookSettings = DialogSettings->Flipbooks[k];
                    if (FlipbookSettings.Animation.AnimationSkinID == i && FlipbookSettings.Animation.AnimationSkinSequenceID == j)
                    {
                        FPaperZDExtractedFlipbook& ExtractedFlipbook = Flipbooks[k];
                        if (UPaperFlipbook* Flipbook = ExtractedFlipbook.Flipbook)
                        {
                            int32 DirectionIndex = FlipbookSettings.Animation.AnimationDirectionIndex;
                            if (DirectionIndex < 0)
                            {
                                for (int32 l = 0; l < AnimationSkinSequenceFlipbooks.Num(); ++l)
                                {
                                    if (AnimationSkinSequenceFlipbooks[l] == nullptr)
                                    {
                                        DirectionIndex = l;
                                        break;
                                    }
                                }
                            }

                            if (AnimationSkinSequenceFlipbooks.IsValidIndex(DirectionIndex))
                            {
                                AnimationSkinSequenceFlipbooks[DirectionIndex] = Flipbook;
                            }
                        }
                    }
                }

                if (!AnimationSkinSequenceFlipbooks.IsEmpty())
                {
                    AnimationSkinSequences.Add(AnimationSkinSequence, AnimationSkinSequenceFlipbooks);
                }
            }

            UPaperZDAnimationSkin_Flipbook* AnimationSkin = SkinSettings.AnimationSkin;
            if (!AnimationSkin)
            {
                FString AnimationSkinName = DialogSettings->Naming.AnimationSkinNamingTemplate;
                FString Path = FPaths::GetPath(SourceTexture->GetOutermost()->GetName()) / AnimationSkinName;

                FString Name, PackageName, Suffix;
                AssetToolsModule.Get().CreateUniqueAssetName(Path, Suffix, PackageName, Name);
                const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
                UPackage* Package = CreatePackage(*PackageName);

                if (UPaperZDAnimationSkin_Flipbook* NewAsset = NewObject<UPaperZDAnimationSkin_Flipbook>(Package, UPaperZDAnimationSkin_Flipbook::StaticClass(), *Name, RF_Public | RF_Standalone | RF_Transactional))
                {
                    NewAsset->SetAnimationSource(AnimationSource);

                    FAssetRegistryModule::AssetCreated(NewAsset);
                    ObjectsToSync.Add(NewAsset);

                    AnimationSkin = NewAsset;
                }
            }

            if (AnimationSkin)
            {
                // Assign the flipbooks to the skin directions
                for (auto& Pair : AnimationSkin->SkinsPerAnimation)
                {
                    const UPaperZDAnimSequence* AnimationSkinSequence = Pair.Key.IsValid() ? Pair.Key.Get() : nullptr;
                    TArray<TObjectPtr<UPaperFlipbook>>& AnimationSkinSequenceFlipbooks = Pair.Value.AnimationDirections;

                    for (auto& Pair2 : AnimationSkinSequences)
                    {
                        UPaperZDAnimSequence_Flipbook* OtherAnimationSkinSequence = Pair2.Key;
                        TArray<TObjectPtr<UPaperFlipbook>>& OtherAnimationSkinSequenceFlipbooks = Pair2.Value;
                        if (AnimationSkinSequence == OtherAnimationSkinSequence)
                        {
                            for (int32 m = 0; m < AnimationSkinSequenceFlipbooks.Num(); ++m)
                            {
                                // Prevent replacing the skin flipbooks if null (only replace the newly created ones)
                                if (OtherAnimationSkinSequenceFlipbooks[m])
                                {
                                    AnimationSkinSequenceFlipbooks[m] = OtherAnimationSkinSequenceFlipbooks[m];
                                }
                            }

                            break;
                        }
                    }
                }

                AnimationSkin->MarkPackageDirty();
                AnimationSkin->PostEditChange();
            }
        }
    }

    if (!ObjectsToSync.IsEmpty())
    {
        ContentBrowserModule.Get().SyncBrowserToAssets(ObjectsToSync);
    }
}

void SPaperZDExtractFlipbookFromTextureDialog::ShowWindow(UTexture2D* InTexture)
{
    const FText TitleText = NSLOCTEXT("TiledIntegration", "TI_ExtractFlipbookFromTexture", "Extract Flipbook");

    // Create the window to pick the class
    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(TitleText)
        .SizingRule(ESizingRule::UserSized)
        .ClientSize(FVector2D(1000.f, 1000.f))
        .AutoCenter(EAutoCenter::PrimaryWorkArea)
        .SupportsMinimize(true)
        .IsInitiallyMaximized(true);

    TSharedRef<SPaperZDExtractFlipbookFromTextureDialog> Dialog = SNew(SPaperZDExtractFlipbookFromTextureDialog, InTexture);

    Window->SetContent(Dialog);

    TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
    if (RootWindow.IsValid())
    {
        FSlateApplication::Get().AddWindowAsNativeChild(Window, RootWindow.ToSharedRef());
    }
    else
    {
        FSlateApplication::Get().AddWindow(Window);
    }
}

void SPaperZDExtractFlipbookFromTextureDialog::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
    if (PropertyChangedEvent.Property != nullptr)
    {
        const FName PropertyName(PropertyChangedEvent.Property->GetFName());
        if (PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbookExtractionSettings, CellWidth) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbookExtractionSettings, CellHeight) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbookExtractionSettings, NumCellsX) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbookExtractionSettings, NumCellsY) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbookExtractionSettings, MarginX) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbookExtractionSettings, MarginY) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbookExtractionSettings, SpacingX) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbookExtractionSettings, SpacingY))
        {
            PreviewSelectedFlipbooks();
        }

        if (PropertyName == GET_MEMBER_NAME_CHECKED(UPaperZDExtractFlipbookFromTextureDialogSettings, Flipbooks))
        {
            if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
            {
                InitializeFlipbookExtractSettings(DialogSettings->Flipbooks.Last());
            }

            if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet ||
                PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd || 
                PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayRemove ||
                PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayClear ||
                PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayMove)
            {
                PreviewSelectedFlipbooks();
            }
        }

        if ((PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbooksAnimationSettings, AnimationSkinSettings) && PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd) ||
            PropertyName == GET_MEMBER_NAME_CHECKED(FPaperZDExtractFlipbooksAnimationSkinSettings, AnimationSkin))
        {
            for (auto& AnimationSkinSettings : DialogSettings->Animation.AnimationSkinSettings)
            {
                InitializeAnimationSkinSettings(AnimationSkinSettings);
            }
        }
    }
}

void SPaperZDExtractFlipbookFromTextureDialog::InitializeFlipbookExtractSettings(FPaperZDExtractFlipbookSettings& FlipbookSettings)
{
    FlipbookSettings.Coloring.OutlineColor = FLinearColor(FMath::FRand(), FMath::FRand(), FMath::FRand());

    FlipbookSettings.Extraction.CellWidth = SourceTexture->GetImportedSize().X;
    FlipbookSettings.Extraction.CellHeight = SourceTexture->GetImportedSize().Y;

    FlipbookSettings.Flipbook.Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Paper2D/MaskedUnlitSpriteMaterial"));
    
    FlipbookSettings.Sprite.Material = FlipbookSettings.Flipbook.Material;
}

void SPaperZDExtractFlipbookFromTextureDialog::InitializeAnimationSkinSettings(FPaperZDExtractFlipbooksAnimationSkinSettings& SkinSettings)
{
    SkinSettings.AnimationSequences.Empty();
    if (DialogSettings->Animation.AnimationSource || SkinSettings.AnimationSkin)
    {
        if (SkinSettings.AnimationSkin)
        {
            const auto& SkinsPerAnimation = SkinSettings.AnimationSkin->GetSkinsPerAnimation();
            for (const auto& Pair : SkinsPerAnimation)
            {
                if (Pair.Key.IsValid())
                {
                    if (UPaperZDAnimSequence_Flipbook* Sequence = Cast<UPaperZDAnimSequence_Flipbook>(Pair.Key.Get()))
                    {
                        SkinSettings.AnimationSequences.Add(Sequence);
                    }
                }
            }
        }
        else
        {
            TArray<FAssetData> AssetData = IPaperZDEditorProxy::Get()->GetAnimSequencesForSource(DialogSettings->Animation.AnimationSource);
            for (const FAssetData& Data : AssetData)
            {
                UPaperZDAnimSequence_Flipbook* Sequence = Cast<UPaperZDAnimSequence_Flipbook>(Data.GetAsset());
                if (Sequence)
                {
                    SkinSettings.AnimationSequences.Add(Sequence);
                }
            }
        }
    }
}

SPaperZDExtractFlipbookFromTextureViewport::~SPaperZDExtractFlipbookFromTextureViewport()
{
    TypedViewportClient = nullptr;
}

void SPaperZDExtractFlipbookFromTextureViewport::Construct(const FArguments& InArgs, UTexture2D* Texture, const TArray<FPaperZDExtractedFlipbook>& InExtractedFlipbooks, const class UPaperZDExtractFlipbookFromTextureDialogSettings* Settings, class SPaperZDExtractFlipbookFromTextureDialog* InDialog)
{
    TexturePtr = Texture;
    Dialog = InDialog;

    TypedViewportClient = MakeShareable(new FPaperZDExtractFlipbookFromTextureViewportClient(Texture, InExtractedFlipbooks, Settings));
    SPaperZDToolViewport::Construct(SPaperZDToolViewport::FArguments(), TypedViewportClient.ToSharedRef());

    // Make sure we get input instead of the viewport stealing it
    ViewportWidget->SetVisibility(EVisibility::HitTestInvisible);

    Invalidate();
}

FText SPaperZDExtractFlipbookFromTextureViewport::GetTitleText() const
{
    return FText::FromString(TexturePtr->GetName());
}

#undef LOCTEXT_NAMESPACE

FPaperZDExtractFlipbookFromTextureViewportClient::FPaperZDExtractFlipbookFromTextureViewportClient(UTexture2D* Texture, const TArray<FPaperZDExtractedFlipbook>& InExtractedFlipbooks, const UPaperZDExtractFlipbookFromTextureDialogSettings* InSettings)
: TextureBeingExtracted(Texture)
, ExtractedFlipbooks(InExtractedFlipbooks)
, Settings(InSettings)
{
    
}

void FPaperZDExtractFlipbookFromTextureViewportClient::Draw(FViewport* InViewport, FCanvas* Canvas)
{
    // Clear the viewport 
    Canvas->Clear(GetBackgroundColor());

    if (UTexture2D* Texture = TextureBeingExtracted.Get())
    {
        FTextureCompilingManager::Get().FinishCompilation({ Texture });

        const bool bUseTranslucentBlend = Texture->HasAlphaChannel();

        // Fully stream in the texture before drawing it.
        Texture->SetForceMipLevelsToBeResident(30.0f);
        Texture->WaitForStreaming();

        const FLinearColor TextureDrawColor = Settings->Coloring.ViewportTextureTint;
        const float XPos = -ZoomPos.X * ZoomAmount;
        const float YPos = -ZoomPos.Y * ZoomAmount;
        const float Width = Texture->GetSurfaceWidth() * ZoomAmount;
        const float Height = Texture->GetSurfaceHeight() * ZoomAmount;

        Canvas->DrawTile(XPos, YPos, Width, Height, 0.0f, 0.0f, 1.0f, 1.0f, TextureDrawColor, Texture->GetResource(), bUseTranslucentBlend);

        for (int32 i = 0; i < ExtractedFlipbooks.Num(); ++i)
        {
            const FPaperZDExtractFlipbookSettings& FlipbookSettings = Settings->Flipbooks[i];
            const FPaperZDExtractedFlipbook& ExtractedFlipbook = ExtractedFlipbooks[i];
            const FLinearColor RectOutlineColor = FlipbookSettings.Coloring.OutlineColor;

            for (int32 j = 0; j < ExtractedFlipbook.Sprites.Num(); ++j)
            {
                const FPaperZDExtractedSprite& Sprite = ExtractedFlipbook.Sprites[j];
                DrawSprite(Canvas, RectOutlineColor, Sprite, i, j);

                if (FlipbookSettings.Sprite.bPreviewBoundingBox)
                {
                    DrawBoundingBoxPreview(Canvas, FlipbookSettings.Sprite, Sprite.Rect);
                }
            }
        }
    }
}

FLinearColor FPaperZDExtractFlipbookFromTextureViewportClient::GetBackgroundColor() const
{
    if (Settings != nullptr)
    {
        return Settings->Coloring.BackgroundColor;
    }
    else
    {
        return FEditorViewportClient::GetBackgroundColor();
    }
}

FIntRect FPaperZDExtractFlipbookFromTextureViewportClient::GetTightRect(const FIntRect& SpriteRect)
{
    const TTuple<int32, int32, int32, int32> Key(SpriteRect.Min.X, SpriteRect.Min.Y, SpriteRect.Max.X, SpriteRect.Max.Y);
    if (const FIntRect* Cached = TightRectCache.Find(Key))
    {
        return *Cached;
    }

    UTexture2D* Texture = TextureBeingExtracted.Get();
    const int32 TextureWidth = Texture ? Texture->Source.GetSizeX() : 0;
    const int32 TextureHeight = Texture ? Texture->Source.GetSizeY() : 0;

    // Read the alpha of the texture once, only the 8 bit BGRA source format is supported
    if (!bTextureAlphaLoaded && Texture)
    {
        bTextureAlphaLoaded = true;
        TArray64<uint8> MipData;
        if (Texture->Source.GetFormat() == TSF_BGRA8 && Texture->Source.GetMipData(MipData, 0))
        {
            TextureAlpha.SetNumUninitialized(TextureWidth * TextureHeight);
            for (int32 PixelIndex = 0; PixelIndex < TextureAlpha.Num(); ++PixelIndex)
            {
                TextureAlpha[PixelIndex] = MipData[PixelIndex * 4 + 3];
            }
        }
    }

    FIntRect Result = SpriteRect;
    if (TextureAlpha.Num() == TextureWidth * TextureHeight && TextureAlpha.Num() > 0)
    {
        FIntRect Bounds(MAX_int32, MAX_int32, MIN_int32, MIN_int32);
        const int32 MinX = FMath::Max(SpriteRect.Min.X, 0);
        const int32 MinY = FMath::Max(SpriteRect.Min.Y, 0);
        const int32 MaxX = FMath::Min(SpriteRect.Max.X, TextureWidth);
        const int32 MaxY = FMath::Min(SpriteRect.Max.Y, TextureHeight);
        for (int32 Y = MinY; Y < MaxY; ++Y)
        {
            for (int32 X = MinX; X < MaxX; ++X)
            {
                if (TextureAlpha[Y * TextureWidth + X] > 0)
                {
                    Bounds.Min.X = FMath::Min(Bounds.Min.X, X);
                    Bounds.Min.Y = FMath::Min(Bounds.Min.Y, Y);
                    Bounds.Max.X = FMath::Max(Bounds.Max.X, X + 1);
                    Bounds.Max.Y = FMath::Max(Bounds.Max.Y, Y + 1);
                }
            }
        }

        // A fully transparent sprite keeps the source rect
        if (Bounds.Min.X != MAX_int32)
        {
            Result = Bounds;
        }
    }

    TightRectCache.Add(Key, Result);
    return Result;
}

void FPaperZDExtractFlipbookFromTextureViewportClient::DrawBoundingBoxPreview(FCanvas* Canvas, const FPaperZDExtractFlipbookSpriteSettings& SpriteSettings, const FIntRect& SpriteRect)
{
    const bool bFullyCustom = SpriteSettings.RenderGeometryType == ESpritePolygonMode::FullyCustom;
    ESpritePolygonMode::Type BaseType = bFullyCustom ? SpriteSettings.CustomRenderGeometryReference.GetValue() : SpriteSettings.RenderGeometryType.GetValue();

    // Same fallback as the generation when the reference is not valid
    if (BaseType == ESpritePolygonMode::FullyCustom)
    {
        BaseType = ESpritePolygonMode::TightBoundingBox;
    }

    // Only the box based geometry types can be previewed
    FIntRect BaseRect;
    switch (BaseType)
    {
        case ESpritePolygonMode::SourceBoundingBox:
        case ESpritePolygonMode::Diced:
            BaseRect = SpriteRect;
            break;

        case ESpritePolygonMode::TightBoundingBox:
            BaseRect = GetTightRect(SpriteRect);
            break;

        default:
            return;
    }

    // Top-Left, Top-Right, Bottom-Right, Bottom-Left, same order as the custom offsets
    FVector2D Vertices[4] =
    {
        FVector2D(BaseRect.Min.X, BaseRect.Min.Y),
        FVector2D(BaseRect.Max.X, BaseRect.Min.Y),
        FVector2D(BaseRect.Max.X, BaseRect.Max.Y),
        FVector2D(BaseRect.Min.X, BaseRect.Max.Y)
    };

    for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
    {
        if (bFullyCustom)
        {
            Vertices[VertexIndex] += SpriteSettings.CustomRenderGeometryOffset[VertexIndex];
        }

        Vertices[VertexIndex] = (Vertices[VertexIndex] - FVector2D(ZoomPos)) * ZoomAmount;
    }

    const FLinearColor PreviewColor(1.0f, 0.25f, 0.0f);
    for (int32 VertexIndex = 0; VertexIndex < 4; ++VertexIndex)
    {
        FCanvasLineItem Line(Vertices[VertexIndex], Vertices[(VertexIndex + 1) % 4]);
        Line.SetColor(PreviewColor);
        Canvas->DrawItem(Line);
    }
}

void FPaperZDExtractFlipbookFromTextureViewportClient::DrawSprite(FCanvas* Canvas, const FLinearColor& Color, const FPaperZDExtractedSprite& Sprite, int32 FlipbookIndex, int32 SpriteIndex)
{
    FVector2D TopLeft(-ZoomPos.X * ZoomAmount + Sprite.Rect.Min.X * ZoomAmount, -ZoomPos.Y * ZoomAmount + Sprite.Rect.Min.Y * ZoomAmount);
    FVector2D BottomRight(-ZoomPos.X * ZoomAmount + Sprite.Rect.Max.X * ZoomAmount, -ZoomPos.Y * ZoomAmount + Sprite.Rect.Max.Y * ZoomAmount);
    FVector2D RectVertices[4];
    RectVertices[0] = FVector2D(TopLeft.X, TopLeft.Y);
    RectVertices[1] = FVector2D(BottomRight.X, TopLeft.Y);
    RectVertices[2] = FVector2D(BottomRight.X, BottomRight.Y);
    RectVertices[3] = FVector2D(TopLeft.X, BottomRight.Y);
    for (int32 RectVertexIndex = 0; RectVertexIndex < 4; ++RectVertexIndex)
    {
        const int32 NextVertexIndex = (RectVertexIndex + 1) % 4;
        FCanvasLineItem RectLine(RectVertices[RectVertexIndex], RectVertices[NextVertexIndex]);
        RectLine.SetColor(Color);
        Canvas->DrawItem(RectLine);
    }

    UFont* Font = GEngine->GetSmallFont();
    const FVector2D TextScale(1.0f);

    const FVector2D FlipbookIndexTextPosition = RectVertices[0] + FVector2D(4.0f, 2.0f);
    FCanvasTextItem FlipbookIndexTextItem(FlipbookIndexTextPosition, FText::FromString(FString::Printf(TEXT("%d"), FlipbookIndex)), Font, Color);
    FlipbookIndexTextItem.Scale = TextScale;
    Canvas->DrawItem(FlipbookIndexTextItem);

    FString SpriteIndexStr = FString::FromInt(FMath::Abs(SpriteIndex));
    int32 SpriteIndexDigits = SpriteIndexStr.Len();

    const FVector2D SpriteIndexTextPosition = RectVertices[1] + FVector2D(-10.0f * SpriteIndexDigits, 2.0f);
    FCanvasTextItem SpriteIndexTextItem(SpriteIndexTextPosition, FText::FromString(FString::Printf(TEXT("%d"), SpriteIndex)), Font, Color);
    SpriteIndexTextItem.Scale = TextScale;
    Canvas->DrawItem(SpriteIndexTextItem);
}
