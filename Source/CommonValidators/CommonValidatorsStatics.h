#pragma once
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CommonValidatorsStatics.generated.h"

class UBlueprint;
class UEdGraph;
class UEdGraphNode;

UCLASS()
class COMMONVALIDATORS_API UCommonValidatorsStatics : public UBlueprintFunctionLibrary
{

	GENERATED_BODY()

public:
	UFUNCTION()
	static void OpenBlueprint(UBlueprint* Blueprint);
	
    UFUNCTION()
    static void OpenBlueprintAndFocusNode(UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node);

    UFUNCTION()
    static void DeleteNodeFromBlueprint(UBlueprint* Blueprint, UEdGraph* Graph, UEdGraphNode* Node);

	UFUNCTION()
	static bool IsObjectAChildOf(const UObject* const AnyAssetReference, const TSubclassOf<UObject> ObjectClass);

	UFUNCTION()
	static bool IsAssetAChildOf(const FAssetData& AnyAssetReference, const TSubclassOf<UObject> ObjectClass);
	
	static TSharedRef<FTokenizedMessage> CreateLinkedMessage(const FAssetData& InAssetData, const FText& Text, EMessageSeverity::Type Severity);
};