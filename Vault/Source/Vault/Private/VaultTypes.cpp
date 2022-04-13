#include "VaultTypes.h"

FSimpleDelegate& FVaultMetadata::OnRenameRequested()
{
	return RenameRequestedEvent;
}

FSimpleDelegate& FVaultMetadata::OnRenameCanceled()
{
	return RenameCanceledEvent;
}

FString FVaultMetadata::CategoryToString(FCategory InCategory)
{
	switch (InCategory)
	{
	case FCategory::ThreeD: 
		return TEXT("3D");
		break;
	case FCategory::Material:
		return TEXT("Material");
		break;
	case FCategory::FX:
		return TEXT("FX");
		break;
	case FCategory::Environment:
		return TEXT("Environment");
		break;
	case FCategory::HDRI:
		return TEXT("HDRI");
		break;
	default:
		return TEXT("Unknown");
		break;
	}
}

FCategory FVaultMetadata::StringToCategory(FString InString)
{
	if (InString.Equals(TEXT("3D"))) return FCategory::ThreeD;
	else if (InString.Equals(TEXT("Material"))) return FCategory::Material;
	else if (InString.Equals(TEXT("FX"))) return FCategory::FX;
	else if (InString.Equals(TEXT("Environment"))) return FCategory::Environment;
	else if (InString.Equals(TEXT("HDRI"))) return FCategory::HDRI;
	else return FCategory::Unknown;
}
