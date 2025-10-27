#include "RogueAIDebugger.h"
#include "GameplayDebugger.h"

#define LOCTEXT_NAMESPACE "FRogueAIDebuggerModule"

#if WITH_GAMEPLAY_DEBUGGER
#include "RogueAIDebugCategory.h"
#endif

void FRogueAIDebuggerModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& Debugger = IGameplayDebugger::Get();
	Debugger.RegisterCategory(
		TEXT("RogueMass"),
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FRogueAIDebugCategory::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate,
		/*SlotIdx*/ 9 /* pick a free slot 0..15 */
	);
	/*Debugger.RegisterCategory(
		TEXT("RogueTrain"),
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FRogueAIDebugCategory::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate,
		/*SlotIdx#1# 1 /* pick a free slot 0..15 #1#
	);
	Debugger.RegisterCategory(
		TEXT("RogueCarriage"),
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FRogueAIDebugCategory::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate,
		/*SlotIdx#1# 2 /* pick a free slot 0..15 #1#
	);
	Debugger.RegisterCategory(
		TEXT("RogueStation"),
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FRogueAIDebugCategory::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate,
		/*SlotIdx#1# 3 /* pick a free slot 0..15 #1#
	);
	Debugger.RegisterCategory(
		TEXT("RogueTrack"),
		IGameplayDebugger::FOnGetCategory::CreateStatic(&FRogueAIDebugCategory::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate,
		/*SlotIdx#1# 4 /* pick a free slot 0..15 #1#
	);*/
	Debugger.NotifyCategoriesChanged();
#endif
}

void FRogueAIDebuggerModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger::Get().UnregisterCategory(TEXT("RoguePassenger"));
		IGameplayDebugger::Get().UnregisterCategory(TEXT("RogueTrain"));
		IGameplayDebugger::Get().UnregisterCategory(TEXT("RogueCarriage"));
		IGameplayDebugger::Get().UnregisterCategory(TEXT("RogueStation"));
		IGameplayDebugger::Get().UnregisterCategory(TEXT("RogueTrack"));
		IGameplayDebugger::Get().NotifyCategoriesChanged();
	}
#endif
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FRogueAIDebuggerModule, RogueAIDebugger)