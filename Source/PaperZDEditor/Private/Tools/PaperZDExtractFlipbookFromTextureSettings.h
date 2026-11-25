// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "SpriteEditorOnlyTypes.h"
#include "PaperZDExtractFlipbookFromTextureSettings.generated.h"

class UPaperZDAnimationSource_Flipbook;
class UPaperZDAnimSequence_Flipbook;
class UPaperZDAnimationSkin_Flipbook;

USTRUCT()
struct FPaperZDExtractFlipbookColoringSettings
{
    GENERATED_BODY()

public:
    // The color of the sprite boundary outlines
    UPROPERTY(Category = Coloring, EditAnywhere, meta = (HideAlphaChannel))
    FLinearColor OutlineColor;

public:
	//ctor
    FPaperZDExtractFlipbookColoringSettings()
	: OutlineColor(FVector(0.0f))
	{}
};

USTRUCT()
struct FPaperZDExtractFlipbookNamingSettings
{
    GENERATED_BODY()

public:
    // The name of the flipbook that will be created. {0} will get replaced by the generic flipbook name {1} will get replaced by the flipbook number.
    UPROPERTY(Category = Flipbook, EditAnywhere)
    FString FlipbookNamingTemplate;

    // The number to start naming sprites with
    UPROPERTY(Category = Flipbook, EditAnywhere)
    int32 FlipbookNamingStartIndex;

    // The name of the sprite that will be created. {0} will get replaced by the generic sprite name {1} will get replaced by the sprite number.
    UPROPERTY(Category = Sprite, EditAnywhere)
    FString SpriteNamingTemplate;

    // The number to start naming sprites with
    UPROPERTY(Category = Sprite, EditAnywhere)
    int32 SpriteNamingStartIndex;

    // The name of the anim sequence that will be created. {0} will get replaced by the generic anim sequence name {1} will get replaced by the sprite number.
    UPROPERTY(Category = Sprite, EditAnywhere)
    FString AnimSequenceNamingTemplate;

public:
	//ctor
    FPaperZDExtractFlipbookNamingSettings()
	: FlipbookNamingTemplate("{0}_{1}")
    , FlipbookNamingStartIndex(0)
    , SpriteNamingTemplate("{0}_{1}")
    , SpriteNamingStartIndex(0)
    , AnimSequenceNamingTemplate("{0}_{1}")
	{}
};

USTRUCT()
struct FPaperZDExtractFlipbookFlipbookSettings
{
    GENERATED_BODY()

public:
    // If a flipbook should be generated (if not, only the sprites will get extracted)
    UPROPERTY(Category = Flipbook, EditAnywhere)
    bool bGenerateFlipbook;

    // The flipbook speed
    UPROPERTY(Category = Flipbook, EditAnywhere, meta = (EditCondition = "bGenerateFlipbook", EditConditionHides))
    float FramesPerSecond;

    // The material for the flipbook
    UPROPERTY(Category = Flipbook, EditAnywhere, meta = (EditCondition = "bGenerateFlipbook", EditConditionHides))
    UMaterialInterface* Material;

public:
	//ctor
    FPaperZDExtractFlipbookFlipbookSettings()
	: bGenerateFlipbook(true)
    , FramesPerSecond(15.0f)
    , Material(nullptr)
	{}
};

USTRUCT()
struct FPaperZDExtractFlipbookSpriteSettings
{
    GENERATED_BODY()

public:
    // The pivot mode of the extracted sprites
    UPROPERTY(Category = Sprite, EditAnywhere)
    TEnumAsByte<ESpritePivotMode::Type> PivotMode;

    // The pivot point to use as a base for the custom pivot point
    UPROPERTY(Category = Sprite, EditAnywhere, meta = (EditCondition = "PivotMode == ESpritePivotMode::Custom", EditConditionHides))
    TEnumAsByte<ESpritePivotMode::Type> CustomPivotModeReference;

    // The relative value to add on top of the base pivot point
    UPROPERTY(Category = Sprite, EditAnywhere, meta = (EditCondition = "PivotMode == ESpritePivotMode::Custom", EditConditionHides))
    FVector2D CustomPivotPoint;

    // Draw the bounding box that will be generated for the sprite in the viewport
    UPROPERTY(Category = Sprite, EditAnywhere)
    bool bPreviewBoundingBox;

    // The material for the sprite (leave empty to use the default material from the importer settings)
    UPROPERTY(Category = Sprite, EditAnywhere)
    UMaterialInterface* Material;

    // The collision configuration
    UPROPERTY(Category = Sprite, EditAnywhere)
    TEnumAsByte<ESpriteCollisionMode::Type> Collision;

    // The render geometry type
    UPROPERTY(Category = Sprite, EditAnywhere)
    TEnumAsByte<ESpritePolygonMode::Type> RenderGeometryType;

    // The geometry type to use as a base for the fully custom render geometry
    UPROPERTY(Category = Sprite, EditAnywhere, meta = (EditCondition = "RenderGeometryType == ESpritePolygonMode::FullyCustom", EditConditionHides))
    TEnumAsByte<ESpritePolygonMode::Type> CustomRenderGeometryReference;

    // The relative value (in pixels) to add on top of each vertex of the base render geometry box (Top-Left, Top-Right, Bottom-Right, Bottom-Left)
    UPROPERTY(Category = Sprite, EditAnywhere, meta = (EditCondition = "RenderGeometryType == ESpritePolygonMode::FullyCustom", EditConditionHides))
    FVector2D CustomRenderGeometryOffset[4];

public:
	//ctor
    FPaperZDExtractFlipbookSpriteSettings()
	: PivotMode(ESpritePivotMode::Center_Center)
    , CustomPivotModeReference(ESpritePivotMode::Center_Center)
    , CustomPivotPoint(0.0f)
    , bPreviewBoundingBox(false)
    , Material(nullptr)
    , Collision(ESpriteCollisionMode::Use3DPhysics)
    , RenderGeometryType(ESpritePolygonMode::TightBoundingBox)
    , CustomRenderGeometryReference(ESpritePolygonMode::TightBoundingBox)
	{
        for (FVector2D& Offset : CustomRenderGeometryOffset)
        {
            Offset = FVector2D::ZeroVector;
        }
    }
};

USTRUCT()
struct FPaperZDExtractFlipbookAnimationSettings
{
    GENERATED_BODY()

public:
    // The id of the animation sequence to use for the extracted flipbook (Must match with the Animation > Animation Sequence Settings > Index) (Leave to -1 to have separate animation sequences)
    UPROPERTY(Category = Animation, EditAnywhere)
    int32 AnimationSequenceID;

    // The direction index of the animation clockwise (leave it to -1 to auto pick)
    // E.g. In an animation sequence of 4 directions: 0 = up, 1 = right, 2 = down, 3 = left
    UPROPERTY(Category = Animation, EditAnywhere)
    int32 AnimationDirectionIndex;

    // The id of the animation skin to use for the extracted flipbook (Must match with the Animation > Animation Skin Settings > Index)
    UPROPERTY(Category = Animation, EditAnywhere)
    int32 AnimationSkinID;

    // The id of the animation skin sequence to use for the extracted flipbook (Must match with the Animation > Animation Skin Settings > Animation Sequences > Index)
    UPROPERTY(Category = Animation, EditAnywhere)
    int32 AnimationSkinSequenceID;

public:
	//ctor
    FPaperZDExtractFlipbookAnimationSettings()
	: AnimationSequenceID(INDEX_NONE)
    , AnimationDirectionIndex(INDEX_NONE)
    , AnimationSkinID(INDEX_NONE)
    , AnimationSkinSequenceID(INDEX_NONE)
	{}
};

USTRUCT()
struct FPaperZDExtractFlipbookExtractionSettings
{
    GENERATED_BODY()

public:
    // The width of each sprite in grid mode
    UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 1, ClampMin = 1))
    int32 CellWidth;

    // The height of each sprite in grid mode
    UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 1, ClampMin = 1))
    int32 CellHeight;

    // Number of cells extracted horizontally. Can be used to limit the number of sprites extracted. Set to 0 to extract all sprites
    UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
    int32 NumCellsX;

    // Number of cells extracted vertically. Can be used to limit the number of sprites extracted. Set to 0 to extract all sprites
    UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
    int32 NumCellsY;

    // Margin from the left of the texture to the first sprite
    UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
    int32 MarginX;

    // Margin from the top of the texture to the first sprite
    UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
    int32 MarginY;

    // Horizontal spacing between sprites
    UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
    int32 SpacingX;

    // Vertical spacing between sprites
    UPROPERTY(Category = Grid, EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
    int32 SpacingY;

public:
	//ctor
    FPaperZDExtractFlipbookExtractionSettings()
	: CellWidth(0)
    , CellHeight(0)
    , NumCellsX(0)
    , NumCellsY(0)
    , MarginX(0)
    , MarginY(0)
    , SpacingX(0)
    , SpacingY(0)
	{}
};

USTRUCT()
struct FPaperZDExtractFlipbookSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Category = Settings, EditAnywhere)
    FPaperZDExtractFlipbookColoringSettings Coloring;

    UPROPERTY(Category = Settings, EditAnywhere)
    FPaperZDExtractFlipbookNamingSettings Naming;

    UPROPERTY(Category = Settings, EditAnywhere)
    FPaperZDExtractFlipbookFlipbookSettings Flipbook;

    UPROPERTY(Category = Settings, EditAnywhere)
    FPaperZDExtractFlipbookSpriteSettings Sprite;

    UPROPERTY(Category = Settings, EditAnywhere)
    FPaperZDExtractFlipbookAnimationSettings Animation;

    UPROPERTY(Category = Settings, EditAnywhere)
    FPaperZDExtractFlipbookExtractionSettings Extraction;
};

USTRUCT()
struct FPaperZDExtractFlipbooksColoringSettings
{
    GENERATED_BODY()

public:
    // Apply a tint to the texture in the viewport to improve outline visibility in this editor
    UPROPERTY(Category = Coloring, EditAnywhere)
    FLinearColor ViewportTextureTint;

    // The viewport background color
    UPROPERTY(Category = Coloring, EditAnywhere, meta = (HideAlphaChannel))
    FLinearColor BackgroundColor;

public:
	//ctor
    FPaperZDExtractFlipbooksColoringSettings()
	: ViewportTextureTint(FLinearColor::Gray)
    , BackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f))
	{}
};

USTRUCT()
struct FPaperZDExtractFlipbooksNamingSettings
{
    GENERATED_BODY()

public:
    // The name of all the flipbook that will be created.
    UPROPERTY(Category = Naming, EditAnywhere)
    FString FlipbookNamingTemplate;

    // The name of all the sprite that will be created.
    UPROPERTY(Category = Naming, EditAnywhere)
    FString SpriteNamingTemplate;

    // The name of all the animation skins that will be created.
    UPROPERTY(Category = Naming, EditAnywhere)
    FString AnimationSkinNamingTemplate;

    // The name of all the animation sequences that will be created.
    UPROPERTY(Category = Naming, EditAnywhere)
    FString AnimationSequenceNamingTemplate;

    // The name of the animation source that will be created.
    UPROPERTY(Category = Naming, EditAnywhere)
    FString AnimationSourceNamingTemplate;

    // The name of the animation blueprint that will be created.
    UPROPERTY(Category = Naming, EditAnywhere)
    FString AnimationBlueprintNamingTemplate;
};

USTRUCT()
struct FPaperZDExtractFlipbooksAnimationSequenceSettings
{
    GENERATED_BODY()

public:
    // The category set for the animation sequence
    UPROPERTY(Category = Animation, EditAnywhere)
    FString Category;

    // The naming template used for the animation sequence where {0} will get replaced by the generic anim sequence name.
    UPROPERTY(Category = Animation, EditAnywhere)
    FString AnimationSequenceNamingTemplate;

public:
	//ctor
    FPaperZDExtractFlipbooksAnimationSequenceSettings()
	: Category("Default")
    , AnimationSequenceNamingTemplate("{0}")
	{}
};

USTRUCT()
struct FPaperZDExtractFlipbooksAnimationSkinSettings
{
    GENERATED_BODY()

public:
    // The animation skin to use (leave empty to create a new one)
    UPROPERTY(Category = Animation, EditAnywhere)
    UPaperZDAnimationSkin_Flipbook* AnimationSkin;

    // The animation sequences available for the skin
    UPROPERTY(Category = Animation, VisibleAnywhere)
    TArray<UPaperZDAnimSequence_Flipbook*> AnimationSequences;

public:
	//ctor
    FPaperZDExtractFlipbooksAnimationSkinSettings()
	: AnimationSkin(nullptr)
	{}
};

USTRUCT()
struct FPaperZDExtractFlipbooksAnimationSettings
{
    GENERATED_BODY()

public:
    // The animation source to use for the created animation blueprint, sequences and/or skins created
    UPROPERTY(Category = Animation, EditAnywhere)
    UPaperZDAnimationSource_Flipbook* AnimationSource;

    // If a animation blueprint should be created using the given (or newly created) animation source
    UPROPERTY(Category = Animation, EditAnywhere)
    bool bCreateAnimationBlueprint;

    // If a animation source should be created
    UPROPERTY(Category = Animation, EditAnywhere)
    bool bCreateAnimationSource;

    // If one or more animation sequences should be created with the extracted flipbooks
    UPROPERTY(Category = Animation, EditAnywhere)
    bool bCreateAnimationSequence;

    // The configuration for the animation sequences created
    UPROPERTY(Category = Animation, EditAnywhere)
    TArray<FPaperZDExtractFlipbooksAnimationSequenceSettings> AnimationSequenceSettings;

    // The configuration for the animation skins created
    UPROPERTY(Category = Animation, EditAnywhere)
    TArray<FPaperZDExtractFlipbooksAnimationSkinSettings> AnimationSkinSettings;

public:
	//ctor
    FPaperZDExtractFlipbooksAnimationSettings()
	: AnimationSource(nullptr)
    , bCreateAnimationBlueprint(false)
    , bCreateAnimationSource(false)
    , bCreateAnimationSequence(false)
	{}
};

UCLASS()
class UPaperZDExtractFlipbookFromTextureDialogSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Category = Settings, EditAnywhere)
    FPaperZDExtractFlipbooksColoringSettings Coloring;

    UPROPERTY(Category = Settings, EditAnywhere)
    FPaperZDExtractFlipbooksNamingSettings Naming;

    UPROPERTY(Category = Settings, EditAnywhere)
    FPaperZDExtractFlipbooksAnimationSettings Animation;

    UPROPERTY(Category = Settings, EditAnywhere)
    TArray<FPaperZDExtractFlipbookSettings> Flipbooks;
};

USTRUCT()
struct FPaperZDExtractFlipbookPreset
{
    GENERATED_BODY()

public:
    UPROPERTY(Category = Preset, EditAnywhere)
    FString PresetName;

    UPROPERTY(Category = Preset, EditAnywhere)
    FPaperZDExtractFlipbooksAnimationSettings AnimationSettings;

    UPROPERTY(Category = Preset, EditAnywhere)
    TArray<FPaperZDExtractFlipbookSettings> FlipbookSettings;
};