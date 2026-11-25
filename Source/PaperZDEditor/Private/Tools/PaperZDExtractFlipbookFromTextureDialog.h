// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once

#include "PaperZDToolViewport.h"
#include "PaperZDToolViewportClient.h"

class UPaperFlipbook;
class UTexture2D;
class UPaperZDExtractFlipbookFromTextureDialogSettings;
class SSearchableComboBox;

struct FPropertyChangedEvent;
struct FPaperZDExtractFlipbookSettings;
struct FPaperZDExtractFlipbookSpriteSettings;
struct FPaperZDExtractFlipbooksAnimationSkinSettings;

struct FPaperZDExtractedSprite
{
    uint32 Index;
    FIntRect Rect;
};

struct FPaperZDExtractedFlipbook
{
    uint32 Index;
    TArray<FPaperZDExtractedSprite> Sprites;
    UPaperFlipbook* Flipbook;
};

class SPaperZDExtractFlipbookFromTextureDialog : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SPaperZDExtractFlipbookFromTextureDialog) {}
    SLATE_END_ARGS()

    // Constructs this widget with InArgs
    void Construct(const FArguments& InArgs, UTexture2D* Texture);

    virtual ~SPaperZDExtractFlipbookFromTextureDialog();

    static void ShowWindow(UTexture2D* Texture);

private:
    // Calculate a list of extracted sprite rects
    void PreviewSelectedFlipbooks();

    // Actually create extracted sprites
    void ExtractFlipbooks();

    // Handler for Extract button
    FReply ExtractClicked();

    // Handler for Cancel button
    FReply CancelClicked();
    void CloseContainingWindow();

    // Handler for Apply Preset button
    FReply ApplyPresetClicked();

    // Handler for Delete Preset button
    FReply DeletePresetClicked();

    // Handler for Save Preset button
    FReply SavePresetClicked();

    void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

    void InitializeFlipbookExtractSettings(FPaperZDExtractFlipbookSettings& FlipbookSettings);

    void InitializeAnimationSkinSettings(FPaperZDExtractFlipbooksAnimationSkinSettings& SkinSettings);

private:
    UTexture2D* SourceTexture;
    UPaperZDExtractFlipbookFromTextureDialogSettings* DialogSettings;

    TSharedPtr<class IDetailsView> MainPropertyView;
    TSharedPtr<class IDetailsView> DetailsPropertyView;
    TArray<FPaperZDExtractedFlipbook> Flipbooks;

    TSharedPtr<FString> SelectedPresetOption;
    TArray<TSharedPtr<FString>> SavedPresetOptions;
    TSharedPtr<SSearchableComboBox> SavedPresetComboBox;

    FString NewPresetName;
};

class FPaperZDExtractFlipbookFromTextureViewportClient : public FPaperZDToolViewportClient
{
public:
    FPaperZDExtractFlipbookFromTextureViewportClient(UTexture2D* Texture, const TArray<FPaperZDExtractedFlipbook>& ExtractedFlipbooks, const UPaperZDExtractFlipbookFromTextureDialogSettings* Settings);

    virtual void Draw(FViewport* Viewport, FCanvas* Canvas) override;
    virtual FLinearColor GetBackgroundColor() const override;

private:
    // Tile set
    TWeakObjectPtr<UTexture2D> TextureBeingExtracted;
    const TArray<FPaperZDExtractedFlipbook>& ExtractedFlipbooks;
    const UPaperZDExtractFlipbookFromTextureDialogSettings* Settings;

    void DrawSprite(FCanvas* Canvas, const FLinearColor& Color, const FPaperZDExtractedSprite& Sprite, int32 FlipbookIndex, int32 SpriteIndex);

    // Draws the bounding box that the extracted sprite will get as render geometry
    void DrawBoundingBoxPreview(FCanvas* Canvas, const FPaperZDExtractFlipbookSpriteSettings& SpriteSettings, const FIntRect& SpriteRect);

    // Calculates the rect that excludes the fully transparent borders of the given rect (returns the same rect if the texture data can't be read)
    FIntRect GetTightRect(const FIntRect& SpriteRect);

    // Cached tight rects, per source rect
    TMap<TTuple<int32, int32, int32, int32>, FIntRect> TightRectCache;

    // Cached alpha of the texture (one byte per pixel), empty until first requested
    TArray<uint8> TextureAlpha;
    bool bTextureAlphaLoaded = false;
};

class SPaperZDExtractFlipbookFromTextureViewport : public SPaperZDToolViewport
{
public:
    SLATE_BEGIN_ARGS(SPaperZDExtractFlipbookFromTextureViewport) {}
    SLATE_END_ARGS()

    ~SPaperZDExtractFlipbookFromTextureViewport();
    void Construct(const FArguments& InArgs, UTexture2D* Texture, const TArray<FPaperZDExtractedFlipbook>& ExtractedFlipbooks, const class UPaperZDExtractFlipbookFromTextureDialogSettings* Settings, class SPaperZDExtractFlipbookFromTextureDialog* InDialog);

protected:
    virtual FText GetTitleText() const override;

private:
    TWeakObjectPtr<class UTexture2D> TexturePtr;
    TSharedPtr<class FPaperZDExtractFlipbookFromTextureViewportClient> TypedViewportClient;
    class SPaperZDExtractFlipbookFromTextureDialog* Dialog;
};