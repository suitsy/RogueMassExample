// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RogueAIDebuggerSettings.generated.h"

USTRUCT(BlueprintType)
struct ROGUEAIDEBUGGER_API FRogueAIEditorGameplayDebuggerConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDrawPassengerOverheads = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDrawTrainOverheads = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDrawCarriageOverheads = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDrawStationOverheads = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bDrawTrackOverheads = false;
};

/**
 * 
 */
UCLASS(Config=RogueEditorSettings, defaultconfig)
class ROGUEAIDEBUGGER_API URogueAIDebuggerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	URogueAIDebuggerSettings();

	FORCEINLINE static URogueAIDebuggerSettings* Get() { return GetMutableDefault<URogueAIDebuggerSettings>(); }
	virtual FName GetCategoryName() const override;

	UPROPERTY(config, EditAnywhere, Category = "GameplayDebugger Persistence")
	FRogueAIEditorGameplayDebuggerConfig GameplayDebuggerConfig;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
