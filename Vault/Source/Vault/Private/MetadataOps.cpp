// Copyright Daniel Orchard 2020

#include "MetadataOps.h"
#include "Vault.h"
#include "VaultSettings.h"

#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "JsonUtilities/Public/JsonUtilities.h"
#include "HAL/FileManager.h"


FVaultMetadata FMetadataOps::ReadMetadata(FString File)
{
	// Raw data holder for Json
	FString MetadataRaw;
	FFileHelper::LoadFileToString(MetadataRaw, *File);

	TSharedPtr<FJsonObject> JsonMetadata = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(MetadataRaw);
	FJsonSerializer::Deserialize(JsonReader, JsonMetadata);

	return ParseMetaJsonToVaultMetadata(JsonMetadata);

}

bool FMetadataOps::WriteMetadata(FVaultMetadata& Metadata)
{
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);

	FJsonSerializer::Serialize(ParseMetadataToJson(Metadata).ToSharedRef(), Writer);

	const FString Directory = FVaultSettings::Get().GetAssetLibraryRoot();
	const FString Filepath = Directory / Metadata.FileId.ToString() + ".meta";
	
	return FFileHelper::SaveStringToFile(OutputString, *Filepath);

}

TArray<FVaultMetadata> FMetadataOps::FindAllMetadataInLibrary()
{
	const FString LibraryPath = FVaultSettings::Get().GetAssetLibraryRoot();
	return FindAllMetadataInFolder(LibraryPath);
}

TArray<FVaultMetadata> FMetadataOps::FindAllMetadataImportedInProject() {
	const FString ImportedLibraryPath = FVaultSettings::Get().GetProjectVaultFolder();
	return FindAllMetadataInFolder(ImportedLibraryPath);
}

TArray<FVaultMetadata> FMetadataOps::FindAllMetadataInFolder(FString PathToFolder) {
	TArray<FVaultMetadata> MetaList;

	// Our custom file visitor that seeks out .meta files
	class FFindMetaFilesVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:

		FFindMetaFilesVisitor() {}
		TArray<FString> MetaFilenames;
		TArray<FString> MetaFilepaths;

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			if (!bIsDirectory)
			{
				FString VisitedFile(FilenameOrDirectory);

				if (FPaths::GetExtension(VisitedFile) == TEXT("meta"))
				{
					MetaFilenames.Add(FPaths::GetBaseFilename(VisitedFile, true));
					MetaFilepaths.Add(VisitedFile);
				}
			}
			return true;
		}
	};

	// Create an instance of our custom visitor	   	 
	FFindMetaFilesVisitor Visitor;

	// Iterate Dir. Visitor will populate with the info we need.
	IFileManager::Get().IterateDirectory(*PathToFolder, Visitor);

	// Loop through all meta files we found. Use simple for to have a index
	for (int i = 0; i < Visitor.MetaFilenames.Num(); i++)
	{
		FString MetaRaw;
		FFileHelper::LoadFileToString(MetaRaw, *Visitor.MetaFilepaths[i]);

		TSharedPtr<FJsonObject> MetaJson = MakeShareable(new FJsonObject());
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(MetaRaw);

		FJsonSerializer::Deserialize(JsonReader, MetaJson);

		//MetaList.Add(ParseMetaJsonToVaultMetadata(MetaJson));
		MetaList.Add(ParseMetaJsonToVaultMetadata(MetaJson));
	}

	return MetaList;
}

bool FMetadataOps::CopyMetadataToLocal(FVaultMetadata& Metadata)
{
	const FString SrcDirectory = FVaultSettings::Get().GetAssetLibraryRoot();
	const FString TgtDirectory = FVaultSettings::Get().GetProjectVaultFolder();
	const FString SrcMetaFilepath = SrcDirectory / Metadata.FileId.ToString() + ".meta";
	const FString TgtMetaFilepath = TgtDirectory / Metadata.FileId.ToString() + ".meta";

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!PlatformFile.DirectoryExists(*TgtDirectory))
	{
		PlatformFile.CreateDirectory(*TgtDirectory);
	}

	return PlatformFile.CopyFile(*TgtMetaFilepath, *SrcMetaFilepath, EPlatformFileRead::AllowWrite, EPlatformFileWrite::AllowRead);
}

FVaultMetadata FMetadataOps::ParseMetaJsonToVaultMetadata(TSharedPtr<FJsonObject> MetaFile)
{
	// New blank metadata struct
	FVaultMetadata Metadata;
	
	// User Info
	Metadata.Author = FName(*MetaFile->GetStringField("Author"));
	Metadata.PackName = FName(*MetaFile->GetStringField("PackName"));
	Metadata.FileId = FName(*MetaFile->GetStringField("FileId"));
	Metadata.Description = MetaFile->GetStringField("Description");

	// Category
	Metadata.Category = FVaultMetadata::StringToCategory(*MetaFile->GetStringField("Category"));

	// Tags
	TArray<TSharedPtr<FJsonValue>> TagValues = MetaFile->GetArrayField("Tags");
	TSet<FString> Tags;
	for (TSharedPtr<FJsonValue> TagRaw : TagValues)
	{
		Tags.Add(TagRaw->AsString());
	}
	Metadata.Tags = Tags;


	// Dates
	FDateTime CreationDate;
	FDateTime::Parse(MetaFile->GetStringField("CreationDate"), CreationDate);
	Metadata.CreationDate = CreationDate;

	FDateTime ModifiedDate;
	FDateTime::Parse(MetaFile->GetStringField("LastModified"), ModifiedDate );
	Metadata.LastModified = ModifiedDate;

	// System Info 
	Metadata.MachineID = MetaFile->GetStringField("MachineID");

	// Hierarchy Badness
	Metadata.HierarchyBadness = MetaFile->GetNumberField("HierarchyBadness");
	
	// Objects List
	TArray<TSharedPtr<FJsonValue>> ListOfObjects = MetaFile->GetArrayField("ObjectsInPack");
	TSet<FString> Objects;
	for (TSharedPtr<FJsonValue> TagRaw : ListOfObjects)
	{
		Objects.Add(TagRaw->AsString());
	}
	Metadata.ObjectsInPack = Objects;
	
	return Metadata;
}

TSharedPtr<FJsonObject> FMetadataOps::ParseMetadataToJson(FVaultMetadata Metadata)
{
	TSharedPtr<FJsonObject> MetaJson = MakeShareable(new FJsonObject());

	MetaJson->SetStringField("Author", Metadata.Author.ToString());
	MetaJson->SetStringField("PackName", Metadata.PackName.ToString());
	MetaJson->SetStringField("FileId", Metadata.FileId.ToString());
	MetaJson->SetStringField("Description", Metadata.Description);

	// Category
	MetaJson->SetStringField("Category", FVaultMetadata::CategoryToString(Metadata.Category));

	// Tags
	TArray<TSharedPtr<FJsonValue>> TagsToWrite;
	for (FString TagText : Metadata.Tags)
	{
		TagsToWrite.Add(MakeShareable(new FJsonValueString(TagText)));
	}
	MetaJson->SetArrayField("Tags", TagsToWrite);

	// Dates
	MetaJson->SetStringField("CreationDate", Metadata.CreationDate.ToString());
	MetaJson->SetStringField("LastModified", Metadata.LastModified.ToString());

	// Sys info
	MetaJson->SetStringField("MachineID", Metadata.MachineID);

	// Hierarchy Badness
	MetaJson->SetNumberField("HierarchyBadness", Metadata.HierarchyBadness);

	// Objects
	TArray<TSharedPtr<FJsonValue>> ObjectsToWrite;
	for (FString ObjectText : Metadata.ObjectsInPack)
	{
		ObjectsToWrite.Add(MakeShareable(new FJsonValueString(ObjectText)));
	}
	MetaJson->SetArrayField("ObjectsInPack", ObjectsToWrite);

	return MetaJson;
}