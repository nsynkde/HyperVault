// Copyright Daniel Orchard 2020

#pragma once

#include "CoreMinimal.h"
#include "VaultTypes.h"
#include "Dom/JsonObject.h"
#include "VaultSettings.h"

class VAULT_API FMetadataOps
{
public:
	static FVaultMetadata ReadMetadata(FString File);

	static bool WriteMetadata(FVaultMetadata& Metadata);

	static bool DeleteMetadata(FVaultMetadata& Metadata);

	static TArray<FVaultMetadata> FindAllMetadataInLibrary();

	static TArray<FVaultMetadata> FindAllMetadataImportedInProject();

	static TArray<FVaultMetadata> FindAllMetadataInFolder(FString PathToFolder);

	static TSet<FString> GetAllTags();

	static TSet<FTagFilteringItem> GetAllTagFilters();

	static bool CopyMetadataToLocal(FVaultMetadata& Metadata);

private:

	static FVaultMetadata ParseMetaJsonToVaultMetadata(TSharedPtr<FJsonObject> MetaFile);

	static TSharedPtr<FJsonObject> ParseMetadataToJson(FVaultMetadata Metadata);
};
