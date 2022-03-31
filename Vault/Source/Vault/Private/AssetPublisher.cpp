// Copyright Daniel Orchard 2020

#include "AssetPublisher.h"
#include "Vault.h"
#include "VaultSettings.h"
#include "PakFileUtilities.h"
#include "Misc/FileHelper.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "MetadataOps.h"
#include <AssetRegistryModule.h>
#include "Slate.h"
#include "SlateExtras.h"

#define LOCTEXT_NAMESPACE "FVaultPublisher"


UAssetPublisher::FOnVaultPackagingCompleted UAssetPublisher::OnVaultPackagingCompletedDelegate;

bool UAssetPublisher::PackageSelected(TSet<FString> PackageObjects, FVaultMetadata& Meta)
{
	const FString Quote(TEXT("\""));

	UpdateSystemMeta(Meta);

	FScopedSlowTask SubPackageTask(1.0F);

	for (FString obj : PackageObjects) {
		PackageObjects.Add(Quote + obj + Quote);
		PackageObjects.Remove(obj);
	}

	bool bWipePackageList = FVaultSettings::Get().GetVaultLocalSettings()->GetBoolField(TEXT("ClearPackageListOnSuccessfulPackage"));
	// #todo store in a better place, make name procedural, so it can be kept for archive and even analysis by loader.

	// Dated Filename
	const FString PackageListFilename = TEXT("VaultPackageList_") + Meta.LastModified.ToString() + TEXT(".txt");

	FString ListDirectory;
	bool bFoundListDirectory = FVaultSettings::Get().GetVaultLocalSettings()->TryGetStringField(TEXT("PackageListStoragePath"), ListDirectory);

	if (!bFoundListDirectory || !FPaths::DirectoryExists(ListDirectory))
	{
		FString TempPath = FGenericPlatformMisc::GetEnvironmentVariable(TEXT("TEMP"));
		FPaths::NormalizeDirectoryName(TempPath);
		ListDirectory = TempPath;
		UE_LOG(LogVault, Error, TEXT("Unable to use PackageListStoragePath, storing file instead to : %s"), *TempPath);
	}

	FString TextDocFull = ListDirectory / PackageListFilename;
	FPaths::NormalizeDirectoryName(TextDocFull);
	UE_LOG(LogVault, Display, TEXT("Writing File List: %s"), *TextDocFull);
	
	FFileHelper::SaveStringArrayToFile(PackageObjects.Array(), *TextDocFull);
	
	const FString Root = FVaultSettings::Get().GetAssetLibraryRoot();
	
	const FString Filename = Meta.PackName.ToString() + TEXT(".upack");

	// Wrap Our Path in Quotes for use in Command-Line
	
	const FString PackFilePath = Quote + (Root / Filename) + Quote;
	
	// Convert String to parsable command. Ensures path is wrapped in quotes in case of spaces in name
	const FString Command = FString::Printf(TEXT("%s -create=%s -compress"), *PackFilePath, *TextDocFull);
	SubPackageTask.EnterProgressFrame(0.2f);
	UE_LOG(LogVault, Display, TEXT("Running Pack Command: %s"), *Command);
	bool bRanPak = ExecuteUnrealPak(*Command);

	if (!bRanPak)
	{
		return false;
	}
	SubPackageTask.EnterProgressFrame(0.6f);

	// Metadata Writing

	FMetadataOps::WriteMetadata(Meta);
	SubPackageTask.EnterProgressFrame(0.2f);
	OnVaultPackagingCompletedDelegate.ExecuteIfBound();

	return true;

}

void UAssetPublisher::UpdateSystemMeta(FVaultMetadata& Metadata)
{
	Metadata.MachineID = FGenericPlatformMisc::GetLoginId();
}

void UAssetPublisher::GetAssetDependenciesRecursive(const FName AssetPath, TSet<FName>& AllDependencies, const FString& OriginalRoot)
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FName> BaseDependencies;
	AssetRegistryModule.Get().GetDependencies(AssetPath, BaseDependencies);

	for (auto DependsIt = BaseDependencies.CreateConstIterator(); DependsIt; ++DependsIt)
	{
		if (!AllDependencies.Contains(*DependsIt))
		{
			const bool bIsEnginePackage = (*DependsIt).ToString().StartsWith(TEXT("/Engine"));
			const bool bIsScriptPackage = (*DependsIt).ToString().StartsWith(TEXT("/Script"));
			// Skip all packages whose root is different than the source package list root
			const bool bIsInSamePackage = (*DependsIt).ToString().StartsWith(OriginalRoot);
			if (!bIsEnginePackage && !bIsScriptPackage && bIsInSamePackage)
			{
				AllDependencies.Add(*DependsIt);
				GetAssetDependenciesRecursive(*DependsIt, AllDependencies, OriginalRoot);
			}
		}
	}
}

int32 UAssetPublisher::CheckForGoodAssetHierarchy(const FAssetData AssetData, TSet<FName>& AllDependencies, TSet<FName>& BadAssets)
{
	
	bool IsInVaultFolder = true;
	bool IsInSameSubfolder = true;
	FString RootPath = AssetData.PackageName.ToString();
	FString OriginalRootString;
	RootPath.RemoveFromStart(TEXT("/"));
	RootPath.Split("/", &OriginalRootString, &RootPath, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	OriginalRootString = TEXT("/") + OriginalRootString;

	AllDependencies = { AssetData.PackageName };
	GetAssetDependenciesRecursive(AssetData.PackageName, AllDependencies, OriginalRootString);

	TArray<FString> AssetFolders;
	TArray<FString> DependencyFolders;
	RootPath.ParseIntoArray(AssetFolders, TEXT("/"));
	// index 0 is the subfolder as the /Game was already removed from the rootpath
	if (!AssetFolders[0].Equals(TEXT("Vault")))
	{
		IsInVaultFolder = false;
	}
	for (FName Dependency : AllDependencies)
	{
		Dependency.ToString().ParseIntoArray(DependencyFolders, TEXT("/"));

		if (IsInVaultFolder)
		{
			if (!AssetFolders[0].Equals(DependencyFolders[1], ESearchCase::CaseSensitive) || !AssetFolders[1].Equals(DependencyFolders[2], ESearchCase::CaseSensitive))
			{
				IsInSameSubfolder = false;
				BadAssets.Add(Dependency);
			}
		}
		else
		{
			if (!AssetFolders[0].Equals(DependencyFolders[1], ESearchCase::CaseSensitive))
			{
				IsInSameSubfolder = false;
				BadAssets.Add(Dependency);
			}
		}

	}

	// return 0 for good, 1 for not in vault folder but in same subfolder, 2 for in vault folder but not in the same subfolder within it, 3 for neither in vault folder nor in same subfolder
	if (IsInVaultFolder && IsInSameSubfolder)
	{
		return 0;
	}
	else if (!IsInVaultFolder && IsInSameSubfolder)
	{
		return 1;
	}
	else if (IsInVaultFolder && !IsInSameSubfolder)
	{
		return 2;
	}
	else if (!IsInVaultFolder && !IsInSameSubfolder)
	{
		return 3;
	}
	else
	{
		// this should only happen with quantum computing :)
		return -1;
	}
}

FReply UAssetPublisher::TryPackageAsset(FString PackageName, FAssetData ExportAsset, FVaultMetadata AssetPublishMetadata)
{
	const FString OutputDirectory = FVaultSettings::Get().GetAssetLibraryRoot();

	// Pack file path, only used here for duplicate detection
	const FString PackageFileOutput = OutputDirectory / PackageName + TEXT(".upack");


	if (FPaths::FileExists(PackageFileOutput))
	{

		const FText ErrorMsg = LOCTEXT("TryPackageOverwriteMsg", "A Vault item already exists with this pack name, are you sure you want to overwrite it?\nThis action cannot be undone.");
		const FText ErrorTitle = LOCTEXT("TryPackageOverwriteTitle", "Existing Pack Detected");

		const EAppReturnType::Type Confirmation = FMessageDialog::Open(
			EAppMsgType::OkCancel, ErrorMsg, &ErrorTitle);

		if (Confirmation == EAppReturnType::Cancel)
		{
			UE_LOG(LogVault, Error, TEXT("User cancelled packaging operation due to duplicate pack found"));
			return FReply::Handled();;
		}
	}

	
	

	// Store PackageName
	const FName AssetPath = ExportAsset.PackageName;

	// Our core publish list, which gets written as a text file to be passed to the Pak Tool.
	TSet<FString> PublishList;

	// List of objects that are getting packaged, clean, for writing to the JSON.
	TSet<FString> ObjectsInPackage;

	FString RootPath = AssetPath.ToString();
	FString OriginalRootString;
	RootPath.RemoveFromStart(TEXT("/"));
	RootPath.Split("/", &OriginalRootString, &RootPath, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	OriginalRootString = TEXT("/") + OriginalRootString;

	TSet<FName> AssetsToProcess = { AssetPath };
	TSet<FName> BadAssets = {};
	int32 HierarchyBadness = CheckForGoodAssetHierarchy(ExportAsset, AssetsToProcess, BadAssets);
	if (HierarchyBadness != 0)
	{
		FText ErrorMsg;
		FText ErrorTitle;
		if (HierarchyBadness >= 2)
		{
			FString BadAssetsString = "";
			for (FName BadAsset : BadAssets)
			{
				BadAssetsString.Append("\n");
				BadAssetsString.Append(BadAsset.ToString());
			}
			if (HierarchyBadness == 2) {
				ErrorMsg = FText::Format(LOCTEXT("BadAssetsInVaultText", "The Asset is in the Vault folder, but not all of dependencies are located in the same subfolder as the current asset! Please fix this!\nOnly continue if it has to be that way or you hate all your coworkers!\nProblematic Assets:\n{0}"), FText::FromString(BadAssetsString));
			}
			else
			{
				ErrorMsg = FText::Format(LOCTEXT("BadAssetsOutsideVaultText", "Your asset isn't inside the vault folder and not all of its dependencies are located in the same subfolder as the current asset! Please fix this!\nOnly continue if it has to be that way or you hate all your coworkers!\nProblematic Assets:\n{0}"), FText::FromString(BadAssetsString));
			}
			
			ErrorTitle = LOCTEXT("BadAssetsTitle", "Bad Asset Structure Detected");

		}
		else if (HierarchyBadness == 1)
		{
			ErrorMsg = LOCTEXT("NotInVaultFolderText", "The Asset you are trying to export is not in the /Game/Vault folder! Are you sure you want to continue?");
			ErrorTitle = LOCTEXT("NotInVaultFolderTitle", "Not in Vault folder");
		}
		const EAppReturnType::Type Confirmation = FMessageDialog::Open(
			EAppMsgType::OkCancel, ErrorMsg, &ErrorTitle);

		if (Confirmation == EAppReturnType::Cancel)
		{
			UE_LOG(LogVault, Error, TEXT("User cancelled packaging operation due to bad asset structure"));
			return FReply::Handled();
		}
	}
	//GetAssetDependenciesRecursive(ExportAsset.PackageName, AssetsToProcess, OriginalRootString);

	FScopedSlowTask MainPackageTask(1.0F, LOCTEXT("AssetsToPackageText", "Collecting assets for packaging."));
	MainPackageTask.MakeDialog();

	// Loop through all Assets, including the root object, and format into the correct absolute system filename for the UnrealPak operation
	for (const FName Path : AssetsToProcess)
	{
		const FString PathString = Path.ToString();

		UE_LOG(LogVault, Display, TEXT("Processing Object Path: %s"), *PathString);

		FString Filename;
		bool bGotFilename = FPackageName::TryConvertLongPackageNameToFilename(PathString, Filename);

		// Find the UPackage to determine the asset type
		UPackage* PrimaryAssetPackage = ExportAsset.GetPackage();
		UPackage* ItemPackage = FindPackage(nullptr, *PathString);

		if (!PrimaryAssetPackage || !ItemPackage)
		{
			UE_LOG(LogVault, Error, TEXT("Unable to find UPackage for %s"), *ExportAsset.PackageName.ToString());
			continue;
		}

		ObjectsInPackage.Add(ItemPackage->FileName.ToString());

		// Get and append the asset extension
		const FString ExtensionName = ItemPackage->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();

		// Append extension into filename
		Filename += ExtensionName;

		PublishList.Add(Filename);
		MainPackageTask.EnterProgressFrame(0.5F/AssetsToProcess.Num());
	}

	MainPackageTask.DefaultMessage = LOCTEXT("PackagingText", "Packaging Assets.");
	MainPackageTask.EnterProgressFrame(0.05F);

	// Build a Struct from the metadata to pass to the packager
	AssetPublishMetadata.ObjectsInPack = ObjectsInPackage;
	{
		if (UAssetPublisher::PackageSelected(PublishList, AssetPublishMetadata))
		{
			FNotificationInfo PackageResultMessage(LOCTEXT("PackageResultToast", "Packaging Successful"));
			PackageResultMessage.ExpireDuration = 5.0f;
			PackageResultMessage.bFireAndForget = true;
			PackageResultMessage.bUseLargeFont = true;
			PackageResultMessage.Image = FCoreStyle::Get().GetBrush(TEXT("NotificationList.SuccessImage"));
			FSlateNotificationManager::Get().AddNotification(PackageResultMessage);
		}
	}
	MainPackageTask.EnterProgressFrame(0.45F);
	return FReply::Handled();
}

void UAssetPublisher::ConvertImageBufferUInt8ToFColor(TArray<uint8>& inputData, TArray<FColor>& outputData)
{
	outputData.Empty();
	for (int i = 0; i < (inputData.Num()/4); i++)
	{
		int index = i * 4;
		FColor newColor = FColor(inputData[index + 2], inputData[index + 1], inputData[index], inputData[index + 3]);
		outputData.Add(newColor);
	}
}


