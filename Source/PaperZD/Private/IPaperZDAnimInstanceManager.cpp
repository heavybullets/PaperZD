// Copyright 2017 ~ 2022 Critical Failure Studio Ltd. All rights reserved.

#include "IPaperZDAnimInstanceManager.h"
#include "GameFramework/Actor.h"

#if ZD_VERSION_INLINED_CPP_SUPPORT
#include UE_INLINE_GENERATED_CPP_BY_NAME(IPaperZDAnimInstanceManager)
#endif

AActor* IPaperZDAnimInstanceManager::GetOwningActor() const
{
	return nullptr;
}

UPrimitiveComponent* IPaperZDAnimInstanceManager::GetRenderComponent() const
{
	return nullptr;
}

TArray<FPaperZDLayerLinkData> IPaperZDAnimInstanceManager::GetLayerLinkData() const
{
	return TArray<FPaperZDLayerLinkData>();
}

UWorld* IPaperZDAnimInstanceManager::OnGetWorld() const
{
	return GetOwningActor() ? GetOwningActor()->GetWorld() : nullptr;
}
