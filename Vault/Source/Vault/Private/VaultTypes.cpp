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
	for (FVaultMetadata iAsset : FVaultModule::Get().MetaFilesCache)
	{
		if (iAsset.FileId == this->FileId)
		{
			RemoteAsset = iAsset;
			break;
		}
	}

	if (RemoteAsset.IsMetaValid())
	{
		if (RemoteAsset.LastModified >= this->LastModified) InProjectVersion = -1;
		else if (RemoteAsset.LastModified < this->LastModified) InProjectVersion = 1;
	}
	else
	{
		// Remote Asset doesn't exist anymore
		InProjectVersion = -2;
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
