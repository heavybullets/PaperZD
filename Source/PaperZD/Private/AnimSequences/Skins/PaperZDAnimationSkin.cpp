// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "AnimSequences/Skins/PaperZDAnimationSkin.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(PaperZDAnimationSkin)
#endif

void UPaperZDAnimationSkin::ApplySkinToAnimation(const UPaperZDAnimSequence* AnimSequence, UPrimitiveComponent* RenderComponent, float DirectionalAngle /* = 0.0f */)
{
	//Filled by child classes
}

void UPaperZDAnimationSkin::SetAnimationSource(UPaperZDAnimationSource* InAnimSource)
{
	AnimSource = InAnimSource;
	OnAnimationSourceChanged();
}
