// Copyright Daniel Orchard 2020

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <GameFramework/Actor.h>
#include "VaultTypes.h"
#include "AssetPublisher.generated.h"

UCLASS()
class VAULT_API UAssetPublisher : public UObject
{
public:

	GENERATED_BODY()

	// Package Step, Called from UI
	static bool PackageSelected(TSet<FString> PackageObjects, FVaultMetadata& Meta);

	/** Callback that fires after Publishing has completed on an Asset */
	DECLARE_DELEGATE(FOnVaultPackagingCompleted);
	//static FOnVaultPackagingCompleted& OnVaultPackagingCompleted() { return OnVaultPackagingCompletedDelegate; }
	static FOnVaultPackagingCompleted OnVaultPackagingCompletedDelegate;

	static FReply TryPackageAsset(FString PackageName, FAssetData ExportAsset, FVaultMetadata AssetPublishMetadata, UTexture2D* ThumbnailTexture);

	static void GetAssetDependenciesRecursive(const FName AssetPath, TSet<FName>& AllDependencies, const FString& OriginalRoot);
	/// <summary>
	/// Will check if the asset and all its dependencies are according to best practices.
	/// Also Gets all asset dependencies.
	/// Best practices ask for the asset being in the Vault folder and all dependencies to be in the same folder.
	/// </summary>
	/// <param name="AssetData">The asset to be checked</param>
	/// <param name="AllDependencies">Set were all the dependencies will be saved to</param>
	/// <param name="BadAssets">Will contain all the dependencies outside of the first level of folder, or second if within Vault folder</param>
	/// <returns>returns 0 for good, 1 for not in vault folder but in same subfolder, 2 for in vault folder but not in the same subfolder within it, 3 for neither in vault folder nor in same subfolder</returns>
	static int32 CheckForGoodAssetHierarchy(const FAssetData AssetData, TSet<FName>& AllDependencies, TSet<FName>& BadAssets);

	/// <summary>
	/// Attempt to rename an existing package. Will only change the metadata file.
	/// </summary>
	/// <param name="NewPackName">The new name for the package.</param>
	/// <param name="Meta">Metadata of the file we are renaming.</param>
	/// <returns>true if successfull, false if failed due to unknown file id or the new name already existing.</returns>
	static bool RenamePackage(FName NewPackName, FVaultMetadata Meta);

	static void ConvertImageBufferUInt8ToFColor(TArray<uint8>& inputData, TArray<FColor>& outputData);
	static FName CreateUniquePackageFilename(int length = 16);
	static FVaultMetadata FindMetadataByPackName(FName PackName);
	static FVaultCategory GetAssetCategory(FAssetData AssetData);

private:

	static void UpdateSystemMeta(FVaultMetadata& Metadata);

	
};


