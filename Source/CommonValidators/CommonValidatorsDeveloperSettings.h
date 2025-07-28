#pragma once

#include "Engine/DeveloperSettings.h"

#include "CommonValidatorsDeveloperSettings.generated.h"

UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Common Validators"))
class COMMONVALIDATORS_API UCommonValidatorsDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// If true, we will validate for empty tick nodes.
	UPROPERTY(Config, EditAnywhere)
	bool bEnableEmptyTickNodeValidator = true;
	
	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnableEmptyTickNodeValidator == true"))
	bool bErrorOnEmptyTickNodes = true;
	
	// if true, we will validate for pure nodes being executed multiple times
	UPROPERTY(Config, EditAnywhere)
	bool bEnablePureNodeMultiExecValidator = true;
	
	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnablePureNodeMultiExecValidator == true"))
	bool bErrorOnPureNodeMultiExec = true;
	
	// If true, we will validate for blocking loads in blueprints
	UPROPERTY(Config, EditAnywhere)
	bool bEnableBlockingLoadValidator = true;
	
	//If true, we throw an error, otherwise a warning!
	UPROPERTY(Config, EditAnywhere, meta = (EditCondition = "bEnableBlockingLoadValidator == true"))
	bool bErrorBlockingLoad = true;
};