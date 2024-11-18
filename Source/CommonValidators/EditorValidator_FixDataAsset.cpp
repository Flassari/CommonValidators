// Fill out your copyright notice in the Description page of Project Settings.


#include "EditorValidator_FixDataAsset.h"

#include "DataValidationFixers.h"
#include "Misc/DataValidation.h"

bool UEditorValidator_FixDataAsset::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return InObject->IsA(UFixDataAsset::StaticClass());
}

EDataValidationResult UEditorValidator_FixDataAsset::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	UFixDataAsset* FixDataAsset = Cast<UFixDataAsset>(InAsset);
		
	//if our flag is not True we need to fail validation and offer a fix.
	if(!FixDataAsset->FixToTrue)
	{
		TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Error);
		Message->AddToken(FAssetNameToken::Create(GetPathNameSafe(FixDataAsset), INVTEXT("FixToTrue is set to false. The following action can address it")));
			
		//Make a Fixer
		TSharedRef<UE::DataValidation::FMutuallyExclusiveFixSet> FixController = MakeShareable(new UE::DataValidation::FMutuallyExclusiveFixSet());

		TFunction<FFixResult()> ApplyFix = [FixDataAsset]()
		{
			FixDataAsset->FixToTrue = true;
			return FFixResult::Success();
		};

		const TSharedRef<UE::DataValidation::IFixer> Fixer = UE::DataValidation::MakeFix(MoveTemp(ApplyFix));
			
		FixController->Add(FText::FormatOrdered(INVTEXT("Convert FixToTrue from {0} to {1}")
										, FText::FromString("False")
										, FText::FromString("True"))
									, Fixer);
			
		// for every IFixer instance we create call bellow will create a FFixToken
		// related to that specific "fix". We attach all the tokens to the initial "here are your options" message
		FixController->CreateTokens([Message](TSharedRef<FFixToken> FixToken)
			{
				Message->AddToken(FixToken);
			}
		);

		Context.AddMessage(Message);
			
		return EDataValidationResult::Invalid;
	}
		
	return EDataValidationResult::Valid; 
}
