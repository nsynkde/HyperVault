#include "VaultTypes.h"
#include "Vault.h"

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
	FVaultMetadata RemoteAsset;
	int RemoteAssetIndex = -1;
	for (int i = 0; i < FVaultModule::Get().MetaFilesCache.Num(); i++)
	{
		if (FVaultModule::Get().MetaFilesCache[i].FileId == this->FileId)
		{
			RemoteAssetIndex = i;
			break;
		}
	}

	if (RemoteAssetIndex != -1 && FVaultModule::Get().MetaFilesCache[RemoteAssetIndex].IsMetaValid())
	{
		if (FVaultModule::Get().MetaFilesCache[RemoteAssetIndex].LastModified > this->LastModified) InProjectVersion = -1;
		else if (FVaultModule::Get().MetaFilesCache[RemoteAssetIndex].LastModified <= this->LastModified) InProjectVersion = 1;
	}
	else
	{
		// Remote Asset doesn't exist anymore
		InProjectVersion = -2;
	}

	FVaultModule::Get().MetaFilesCache[RemoteAssetIndex].InProjectVersion = this->InProjectVersion;

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
