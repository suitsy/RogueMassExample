// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RogueHud.generated.h"

class URogueSimStatsWidget;
/**
 * 
 */
UCLASS()
class ROGUEMASSEXAMPLE_API ARogueHud : public AHUD
{
	GENERATED_BODY()

public:
	ARogueHud();

	UPROPERTY(EditAnywhere, Category="Rogue|UI")
	TSubclassOf<URogueSimStatsWidget> StatsWidgetClass;

	/** How often to refresh (seconds) */
	UPROPERTY(EditAnywhere, Category="Rogue|UI", meta=(ClampMin="0.05"))
	float RefreshInterval = 0.25f;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	URogueSimStatsWidget* StatsWidget = nullptr;
	
	FTimerHandle RefreshTimer;

	void RefreshStats() const;
};
