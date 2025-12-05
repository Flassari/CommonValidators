// Copyright Notice
// Author

// This Header
#include "EditorValidator_HeavyReference.h"

// Unreal
#include "AssetManagerEditor/Public/AssetManagerEditorModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/AssetManager.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/DataValidation.h"

// Project

// Local
#include "CommonValidatorsStatics.h"

// Gen CPP
#include UE_INLINE_GENERATED_CPP_BY_NAME(EditorValidator_HeavyReference)

#define LOCTEXT_NAMESPACE "CommonValidators"

bool UEditorValidator_HeavyReference::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InObject,
	FDataValidationContext& InContext) const
{
	if (!IsValid(InObject))
	{
		return false;
	}

	// Early out to prevent chewing CPU time when not enabled
	if (!GetDefault<UCommonValidatorsDeveloperSettings>()->bEnableHeavyReferenceValidator)
	{
		return false;
	}

	UBlueprint* Blueprint = Cast<UBlueprint>(InObject);
	if (!IsValid(Blueprint))
	{
		return false;
	}

	// Check if we want to run validation here
	// Remove any BPs that inherit from the classes in class and child list
	{
		const TArray<TSubclassOf<UObject>>& IgnoreChildrenList = GetDefault<UCommonValidatorsDeveloperSettings>()->
			HeavyValidatorClassAndChildIgnoreList;
		for (const TSubclassOf<UObject>& IgnoredChild : IgnoreChildrenList)
		{
			if (UCommonValidatorsStatics::IsObjectAChildOf(InObject, IgnoredChild))
			{
				return false;
			}
		}
	}

	// Limit to BPs for now?
	return true && InObject->IsA<UBlueprint>();
}


EDataValidationResult UEditorValidator_HeavyReference::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset,
                                                                                          FDataValidationContext& Context)
{
	const UCommonValidatorsDeveloperSettings* const DevSettings = GetDefault<UCommonValidatorsDeveloperSettings>();
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	const IAssetRegistry* const AssetRegistry = &AssetRegistryModule.Get();
	IAssetManagerEditorModule* const EditorModule = &IAssetManagerEditorModule::Get();

	// Ignore non-BP types
	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
	if (!IsValid(Blueprint))
	{
		return EDataValidationResult::NotValidated;
	}

	// Remove any BPs that inherit from the classes in class and child list
	{
		const TArray<TSubclassOf<UObject>>& IgnoreChildrenList = DevSettings->HeavyValidatorClassAndChildIgnoreList;
		for (const TSubclassOf<UObject>& IgnoredChild : IgnoreChildrenList)
		{
			if (UCommonValidatorsStatics::IsObjectAChildOf(InAsset, IgnoredChild))
			{
				return EDataValidationResult::NotValidated;
			}
		}
	}
	
	// Convert to AssetIdentifier as that's what we are using in the loop
	FAssetIdentifier InAssetIdentifier = UCommonValidatorsStatics::GetAssetIdentifierFromAssetData(InAssetData);


	// Got assets. We want to sizemap these
	TSet<FAssetIdentifier> VisitList;
	TArray<FAssetIdentifier> FoundAssetList;
	FoundAssetList.Add(InAssetIdentifier);
	uint64 TotalSize = 0;

	for (uint64 Index = 0; Index < FoundAssetList.Num(); ++Index)
	{
		const FAssetIdentifier& FoundAssetId = FoundAssetList[Index];
		if (VisitList.Contains(FoundAssetId))
		{
			continue;
		}

		VisitList.Add(FoundAssetId);

		// Size this asset first
		const FName AssetPackageName = FoundAssetId.IsPackage() ? FoundAssetId.PackageName : NAME_None;

		// Set some defaults for this node. These will be used if we can't actually locate the asset.
		FAssetData ThisAssetData{};
		if (!GetAssetData(AssetRegistry, FoundAssetId, ThisAssetData))
		{
			continue;
		}

		// Go for asset sizing
		// Skip including checks on the first iteration
		if (Index == 0 || IsAssetIncluded(DevSettings, InAsset, ThisAssetData))
		{
			FAssetManagerDependencyQuery DependencyQuery = SetupDependencyQuery(AssetPackageName);

			// Ignore ourselves in this calc.
			// We are not a reference: we are us.
			if (AssetPackageName != NAME_None && Index > 0)
			{
				int64 FoundSize = 0;
				if (EditorModule->GetIntegerValueForCustomColumn(ThisAssetData, IAssetManagerEditorModule::ResourceSizeName, FoundSize))
				{
					TotalSize += FoundSize;
				}
				else if (DevSettings->bWarnOnUnsizableChildren)
				{
					const FString AssetPackageNameString = (AssetPackageName != NAME_None) ? AssetPackageName.ToString() : FString();

					TSharedRef<FTokenizedMessage> ResultMessage = UCommonValidatorsStatics::CreateLinkedMessage(InAssetData,
							FText::Format(
								LOCTEXT("CommonValidators.HeavyRef.AssetWarning", "Failed to get memory size for {0}! ({1})"),
								FText::FromString(FoundAssetId.ToString()),
								FText::FromString(AssetPackageNameString)
								),
							EMessageSeverity::Warning
						);
					
					Context.AddMessage(ResultMessage);
				}
			}

			// Find lowers
			TArray<FAssetIdentifier> OutAssetData;
			AssetRegistry->GetDependencies(FoundAssetId, OutAssetData, DependencyQuery.Categories, DependencyQuery.Flags);

			// The TArray may have realloc'd and caused our pointer to invalidate at this point.
			// FoundAssetId is now unsafe to use.
			IAssetManagerEditorModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(OutAssetData, DependencyQuery, true);

			FoundAssetList.Append(OutAssetData);
		}
	}

	if (TotalSize > DevSettings->MaximumAllowedReferenceSizeKiloBytes * 1024)
	{
		TSharedRef<FTokenizedMessage> ResultMessage = UCommonValidatorsStatics::CreateLinkedMessage(InAssetData,
				FText::Format(
					LOCTEXT("CommonValidators.HeavyRef.AssetWarning", "Heavy references in asset {0}! ({1})"),
					FText::FromString(InAssetIdentifier.ToString()),
					TotalSize
					),
				(DevSettings->bErrorHeavyReference ? EMessageSeverity::Error : EMessageSeverity::PerformanceWarning)
			);
		
		Context.AddMessage(ResultMessage);

		return DevSettings->bErrorHeavyReference ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
	}

	return EDataValidationResult::Valid;
}

bool UEditorValidator_HeavyReference::IsAssetIncluded(const UCommonValidatorsDeveloperSettings* const DevSettings, const UObject* const InAsset, const FAssetData& ThisAssetData)
{
	// Gather Specific Ref Classes to ignore for the root asset
	TArray<TSubclassOf<UObject>, TInlineAllocator<8>> IgnoredClassList;
	for (auto& ClassToIgnoreEntry : DevSettings->HeavyValidatorClassSpecificClassIgnoreList)
	{
		// Does this apply to this asset?
		// Allowed on the root (idx0) and if propagation is set.
		if (ClassToIgnoreEntry.Value.AllowPropagationToChildren)
		{
			const TSubclassOf<UObject> IgnoredClass = ClassToIgnoreEntry.Key;

			if (UCommonValidatorsStatics::IsObjectAChildOf(InAsset, IgnoredClass))
			{
				IgnoredClassList.Append(ClassToIgnoreEntry.Value.ClassList);
			}
		}
	}

	// Ignore if this asset is in the ignore list
	for (const TSubclassOf<UObject>& IgnoreClass : IgnoredClassList)
	{
		// Needs to resolve BP class..
		if (UCommonValidatorsStatics::IsAssetAChildOf(ThisAssetData, IgnoreClass))
		{
			return false;
		}
	}
	return true;
}

bool UEditorValidator_HeavyReference::GetAssetData(const IAssetRegistry* const AssetRegistry, const FAssetIdentifier& FoundAssetId, FAssetData& OutAssetData)
{
	const FName AssetPackageName = FoundAssetId.IsPackage() ? FoundAssetId.PackageName : NAME_None;
	const FString AssetPackageNameString = (AssetPackageName != NAME_None) ? AssetPackageName.ToString() : FString();
	const FPrimaryAssetId AssetPrimaryId = FoundAssetId.GetPrimaryAssetId();

	// Only support packages and primary assets
	if (AssetPackageName == NAME_None && !AssetPrimaryId.IsValid())
	{
		return false;
	}

	// Don't bother showing code references
	if (AssetPackageNameString.StartsWith(TEXT("/Script/")))
	{
		return false;
	}

	if (AssetPackageName != NAME_None)
	{
		const FString AssetPathString = AssetPackageNameString + TEXT(".") + FPackageName::GetLongPackageAssetName(AssetPackageNameString);
		FAssetData FoundData = AssetRegistry->GetAssetByObjectPath(FSoftObjectPath(AssetPathString));

		if (!FoundData.IsValid())
		{
			return false;
		}

		OutAssetData = MoveTemp(FoundData);
	}
	else
	{
		OutAssetData = IAssetManagerEditorModule::CreateFakeAssetDataFromPrimaryAssetId(AssetPrimaryId);
	}

	return OutAssetData.IsValid();
}

FAssetManagerDependencyQuery UEditorValidator_HeavyReference::SetupDependencyQuery(const FName& AssetName)
{
	FAssetManagerDependencyQuery DependencyQuery = FAssetManagerDependencyQuery::None();
	DependencyQuery.Flags = UE::AssetRegistry::EDependencyQuery::Game;

	if (AssetName != NAME_None)
	{
		DependencyQuery.Categories = UE::AssetRegistry::EDependencyCategory::Package;
		DependencyQuery.Flags |= UE::AssetRegistry::EDependencyQuery::Hard;
	}
	else
	{
		DependencyQuery.Categories = UE::AssetRegistry::EDependencyCategory::Manage;
		DependencyQuery.Flags |= UE::AssetRegistry::EDependencyQuery::Direct;
	}

	return DependencyQuery;
}


#undef LOCTEXT_NAMESPACE
