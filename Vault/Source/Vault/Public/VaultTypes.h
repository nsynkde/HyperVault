// Copyright Daniel Orchard 2020

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2DDynamic.h"

UENUM()
enum FVaultCategory
{
	ThreeD		UMETA(DisplayName = "3D"),
	Material	UMETA(DisplayName = "Material"),
	FX			UMETA(DisplayName = "FX"),
	HDRI		UMETA(DisplayName = "HDRI"),
	Environment	UMETA(DisplayName = "Environment"),
	Unknown		UMETA(DisplayName = "Unknown")
};

// Any metadata required for your assets can be added here.
class VAULT_API FVaultMetadata
{
public:
	FName Author;
	// visible packname
	FName PackName;
	// Unique ID for the package used as filename for its .upack, .meta and .png files.
	FName FileId;
	FString Description;
	TSet<FString> Tags;
	TEnumAsByte<FVaultCategory> Category;
	TSoftObjectPtr<UTexture2DDynamic> Thumbnail;

	FDateTime CreationDate;
	FDateTime LastModified;

	FString RelativePath;
	FString MachineID;
	TSet<FString> ObjectsInPack;

	/// <summary>
	/// The higher the value the worse it is.
	/// 0 for good hierarchy
	/// 1 for asset outside of the vault, but all dependencies are in the same subfolder
	/// 2 asset and all dependencies are in the vault, but not in the same subfolder within it
	/// 3 asset in the vault, but dependencies are not in the vault
	/// 4 asset is not in the vault and dependencies are not in the same subfolder as the asset
	/// </summary>
	int32 HierarchyBadness;

	/** Get the event fired whenever a rename is requested */
	FSimpleDelegate& OnRenameRequested();

	/** Get the event fired whenever a rename is canceled */
	FSimpleDelegate& OnRenameCanceled();

	static FString CategoryToString(FVaultCategory InCategory);

	static FVaultCategory StringToCategory(FString InString);

	/// <summary>
	/// Checks if the package is in the current project and if it was imported before
	/// </summary>
	/// <returns>0 for not in project, 1 for in project and updated, -1 if in project but out of date</returns>
	int32 CheckVersion();

	int32 InProjectVersion;

protected:

	FSimpleDelegate RenameRequestedEvent;

	FSimpleDelegate RenameCanceledEvent;

public:
	// Constructor
	FVaultMetadata()
	{
		Author = NAME_None;
		PackName = NAME_None;
		FileId = NAME_None;
		Description = FString();
		CreationDate = FDateTime::UtcNow();
		LastModified = FDateTime::UtcNow();
		RelativePath = FString();
		MachineID = FString();
		Category = FVaultCategory::Unknown;
	}

	bool IsMetaValid()
	{
		return PackName != NAME_None && FileId != NAME_None;
	}

	bool operator==(const FVaultMetadata& V) const;
};

FORCEINLINE uint32 GetTypeHash(const FVaultMetadata& V)
{
	const FString ComboString = V.PackName.ToString() + V.FileId.ToString() + V.Author.ToString() + V.Description + V.CreationDate.ToString();
	return GetTypeHash(ComboString);
}

FORCEINLINE bool FVaultMetadata::operator==(const FVaultMetadata& V) const
{
	return
		Author == V.Author &&
		PackName == V.PackName &&
		FileId == V.FileId &&
		Description == V.Description &&
		CreationDate == V.CreationDate &&
		LastModified == V.LastModified;
}

// Category Filter Struct used for the Loader UI
struct FCategoryFilteringItem
{
	FCategoryFilteringItem() {}
	virtual ~FCategoryFilteringItem() {}
	FVaultCategory Category;
	int UseCount;
};

// Tag Filter Struct used for the Loader UI
struct FTagFilteringItem
{
	FTagFilteringItem() {}
	virtual ~FTagFilteringItem() {}
	FString Tag;
	int UseCount;
};

// Developer Name Struct used for Loader UI
struct FDeveloperFilteringItem
{
	FDeveloperFilteringItem() {}
	virtual ~FDeveloperFilteringItem() {}
	FName Developer;
	bool bFilterflag;
	int UseCount;
};

UENUM()
enum SortingTypes
{
	Filename	UMETA(DisplayName = "Filename"),
	CreationDate UMETA(DisplayName = "Creation Date"),
	ModificationDate UMETA(DisplayName = "Modification Date")
};
