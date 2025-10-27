// Fill out your copyright notice in the Description page of Project Settings.


#include "RogueAIDebuggerSettings.h"

#define LOCTEXT_NAMESPACE "URogueAIEditorSettings"

URogueAIDebuggerSettings::URogueAIDebuggerSettings()
{
}

FName URogueAIDebuggerSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}

#if WITH_EDITOR
void URogueAIDebuggerSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
	Super::PostEditChangeProperty(PropertyChangedEvent);
	TryUpdateDefaultConfigFile();
}
#endif
#undef LOCTEXT_NAMESPACE
