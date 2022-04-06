// Copyright Daniel Orchard 2020

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2DDynamic.h"

// Any metadata required for your assets can be added here.
struct FVaultMetadata
{
	FName Author;
	// visible packname
	FName PackName;
	// Unique ID for the package used as filename for its .upack, .meta and .png files.
	FName FileId;
	FString Description;
	TSet<FString> Tags;
	TSoftObjectPtr<UTexture2DDynamic> Thumbnail;

	FDateTime CreationDate;
	FDateTime LastModified;

	FString RelativePath;
	FString MachineID;
	TSet<FString> ObjectsInPack;

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
