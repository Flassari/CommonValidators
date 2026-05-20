// all rights reserved


#include "EditorValidator_MaterialTextureSampler.h"

#include "CommonValidatorsDeveloperSettings.h"
#include "CommonValidatorsStatics.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Misc/DataValidation.h"

bool UEditorValidator_MaterialTextureSampler::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	const bool bIsValidatorEnabled = GetDefault<UCommonValidatorsDeveloperSettings>()->bEnableMaterialTextureSamplerValidator;
	return bIsValidatorEnabled && (InObject != nullptr) && InObject->IsA<UMaterial>();
}

EDataValidationResult UEditorValidator_MaterialTextureSampler::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	if(UMaterial *Material = Cast<UMaterial>(InAsset))
	{
		bool bFoundInvalidSampler = false;
		for(const UMaterialExpressionTextureSample* TextureSampler : GetMaterialTextureSamplers(Material))
		{
			if(TextureSampler->SamplerSource.GetValue() == ESamplerSourceMode::SSM_FromTextureAsset)
			{
				bFoundInvalidSampler = true;

				const FText Title = FText::FromName(TextureSampler->GetParameterName());
				const FText Message = FText::Format(NSLOCTEXT("MaterialTextureSamplerValidator", "SamplerTypeWarning", "TextureSampler {0} Sampler Source is set to 'From texture asset'."), Title);
				
				const TSharedRef<FTokenizedMessage> TokenMessage = FTokenizedMessage::Create(EMessageSeverity::Warning, Message);

				TokenMessage->AddToken(
					FActionToken::Create(
						NSLOCTEXT("MaterialTextureSamplerValidator", "OpenNode", "Focus Node"),
						NSLOCTEXT("MaterialTextureSamplerValidator", "OpenNodeTooltip", "Open this node in the Material Editor"),
						FOnActionTokenExecuted::CreateLambda([Material, TextureSampler]()
						{
							UCommonValidatorsStatics::OpenMaterialAndFocusExpression(Material, TextureSampler);
						}),
						/*bEnabled=*/false
					)
				);

				Context.AddMessage(TokenMessage);
			}		
		}

		return bFoundInvalidSampler ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
	}

	return EDataValidationResult::NotValidated;
}

TArray<const UMaterialExpressionTextureSample*> UEditorValidator_MaterialTextureSampler::GetMaterialTextureSamplers(const UMaterial* Material)
{	
	if (!Material)
	{
		return {};
	}

	TArray<const UMaterialExpressionTextureSample*> Result;

	// iterate over all expressions in the material (graph)	
	for(UMaterialExpression* Expression : Material->GetExpressions())
	{
		if(const UMaterialExpressionTextureSample* TextureSample = Cast<UMaterialExpressionTextureSample>(Expression))
		{
			// TextureObject Parameters inherit UMaterialExpressionTextureSample but don't give the user a choice of the sampler source 
			if(TextureSample->IsA<UMaterialExpressionTextureObjectParameter>())
				continue;
			
			Result.Add(TextureSample);
		}
	}

	return Result;
}
