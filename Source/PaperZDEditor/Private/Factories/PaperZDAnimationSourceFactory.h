// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#pragma once
#include "Factories/Factory.h"
#include "AnimSequences/Sources/PaperZDAnimationSource.h"
#include "PaperZDAnimationSourceFactory.generated.h"

/**
 * Factory for PaperZD Animation Source objects.
 */
UCLASS()
class UPaperZDAnimationSourceFactory : public UFactory
{
	GENERATED_BODY()

public:
	/* Class to create when the factory needs to do so. */
	UPROPERTY(Transient)
	TSubclassOf<class UPaperZDAnimationSource> SelectedClass;

public:
	UPaperZDAnimationSourceFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ Begin UFactory Interface

private:
	/* Picks the animation source to use as base. */
	bool PickAnimationSourceClass(UClass*& PickedClass);

	/* Returns true if more than one implementable Animation Source class exists and thus, we need to display the dialog window. */
	bool ShouldDisplayPickerWindow() const;
};
