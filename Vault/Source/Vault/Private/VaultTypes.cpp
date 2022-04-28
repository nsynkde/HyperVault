#include "VaultTypes.h"
#include "Vault.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistryModule.h"

FSimpleDelegate& FVaultMetadata::OnRenameRequested()
{
	return RenameRequestedEvent;
}

FSimpleDelegate& FVaultMetadata::OnRenameCanceled()
{
	return RenameCanceledEvent;
}

int32 FVaultMetadata::CheckVersion()
{
	InProjectVersion = 0;
	FVaultMetadata LocalAsset;
	for (FVaultMetadata iAsset : FVaultModule::Get().ImportedMetaFileCache)
	{
		if (iAsset.FileId == this->FileId)
		{
			LocalAsset = iAsset;
			break;
		}
	}

	



	if (LocalAsset.IsMetaValid())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FString ObjectPath = LocalAsset.ObjectsInPack.Array()[0];
		ObjectPath.RemoveFromStart(TEXT("/Game/"));
		ObjectPath = FPaths::ProjectContentDir() + ObjectPath + ".uasset";

		if (LocalAsset.LastModified < this->LastModified) {
			InProjectVersion = -1;
			if (!AssetRegistryModule.Get().IsLoadingAssets())
			{
				if (!FPaths::FileExists(ObjectPath))
				{
					InProjectVersion = -2;
				}
			}
		}
		else if (LocalAsset.LastModified >= this->LastModified)
		{
			InProjectVersion = 1;
			if (!FPaths::FileExists(ObjectPath)) {
				InProjectVersion = 2;
			}

			/*if (!AssetRegistryModule.Get().IsLoadingAssets())
			{
				if (!UEditorAssetLibrary::DoesAssetExist(this->ObjectsInPack.Array()[0]))
				{
					InProjectVersion = 2;
				}
			}*/
		}
	}

	return InProjectVersion;
}

FString FVaultMetadata::CategoryToString(FVaultCategory InCategory)
{
	switch (InCategory)
	{
	case FVaultCategory::ThreeD: 
		return TEXT("3D");
		break;
	case FVaultCategory::Material:
		return TEXT("Material");
		break;
	case FVaultCategory::FX:
		return TEXT("FX");
		break;
	case FVaultCategory::Environment:
		return TEXT("Environment");
		break;
	case FVaultCategory::HDRI:
		return TEXT("HDRI");
		break;
	default:
		return TEXT("Unknown");
		break;
	}
}

FVaultCategory FVaultMetadata::StringToCategory(FString InString)
{
	if (InString.Equals(TEXT("3D"))) return FVaultCategory::ThreeD;
	else if (InString.Equals(TEXT("Material"))) return FVaultCategory::Material;
	else if (InString.Equals(TEXT("FX"))) return FVaultCategory::FX;
	else if (InString.Equals(TEXT("Environment"))) return FVaultCategory::Environment;
	else if (InString.Equals(TEXT("HDRI"))) return FVaultCategory::HDRI;
	else return FVaultCategory::Unknown;
}
