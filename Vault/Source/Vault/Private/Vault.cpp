// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Vault.h"

#include "VaultTypes.h"
#include "Misc/MessageDialog.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "SVaultRootPanel.h"
#include "MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "VaultSettings.h"
#include "SPublisherWindow.h"
#include "AssetPublisher.h"
#include "LevelEditor.h"
#include "VaultStyle.h"
#include "VaultCommands.h"

#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "ContentBrowserModule.h"
#include "Metadataops.h"

static const FName VaultTabName("VaultOperations");
static const FName VaultPublisherName("VaultPublisher");
static const FName VaultLoaderName("VaultLoader");

void* FVaultModule::LibHandle = nullptr;

#define LOCTEXT_NAMESPACE "FVaultModule"
DEFINE_LOG_CATEGORY(LogVault);


void FVaultModule::StartupModule()
{

	// Init our styles
	FVaultStyle::Initialize();

	FVaultStyle::CacheThumbnailsLocally();

	// Reload Textures
	FVaultStyle::ReloadTextures();

	// Register our Vault Commands into the Engine
	FVaultCommands::Register();

	// Init the Settings system
	FVaultSettings::Get().Initialize();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FVaultCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FVaultModule::SpawnOperationsTab),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FVaultCommands::Get().Rename,
		FExecuteAction::CreateRaw(this, &FVaultModule::HandleRenameAsset),
		FCanExecuteAction());
	
	// Store a Ref to the Level Editor.
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	//{
	//	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
	//	MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FVaultModule::AddMenuExtension));
	//	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	//}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Content", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &FVaultModule::AddToolbarExtension));
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}

	// content browser right click action
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FContentBrowserMenuExtender_SelectedAssets>& MenuExtenderDelegates = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	// Create new delegate that will be called to provide our menu extender
	MenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(this, &FVaultModule::AssetMenuExtender));

	// Setup Operations Tab
	const FText VaultBasePanelWindowTitle = LOCTEXT("OperationsWindowTitleLabel", "The Vault");
	const FText VaultBasePanelWindowTooltip = LOCTEXT("VaultWindowTooltipLabel", "Vault Operations");

	// Init tabs
	TSharedRef<FGlobalTabmanager> TabManager = FGlobalTabmanager::Get();

	TabManager->RegisterNomadTabSpawner(VaultTabName, FOnSpawnTab::CreateRaw(this, &FVaultModule::CreateVaultMajorTab))
		.SetDisplayName(VaultBasePanelWindowTitle)
		.SetTooltipText(VaultBasePanelWindowTooltip)
		.SetIcon(FSlateIcon("VaultStyle", "Vault.Icon16px"))
		.SetMenuType(ETabSpawnerMenuType::Hidden); //Hide root menu from the windows dropdown
}

void FVaultModule::ShutdownModule()
{
	FVaultStyle::Shutdown();
	FVaultCommands::Unregister();
	TSharedRef<FGlobalTabmanager> TabManager = FGlobalTabmanager::Get();
	TabManager->UnregisterTabSpawner(VaultTabName);

	FreeDependency(LibHandle);
}

void FVaultModule::SpawnOperationsTab()
{
	//VaultConnection::Get().Initialize();
	FVaultStyle::CacheThumbnailsLocally();
	TSharedRef<FGlobalTabmanager> TabManager = FGlobalTabmanager::Get();
	TabManager->TryInvokeTab(VaultTabName);
}

void FVaultModule::SpawnOperationsTab(FName SubTabName)
{
	//VaultConnection::Get().Initialize();
	FVaultStyle::CacheThumbnailsLocally();
	TSharedRef<FGlobalTabmanager> TabManager = FGlobalTabmanager::Get();
	TabManager->TryInvokeTab(VaultTabName);
	VaultBasePanelWidget->SetActiveSubTab(SubTabName);
}

FVaultModule& FVaultModule::Get()
{
	static const FName VaultModuleName = "Vault";
	return FModuleManager::LoadModuleChecked<FVaultModule>(VaultModuleName);
}


void FVaultModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FVaultCommands::Get().PluginAction);
}



TSharedRef<SDockTab> FVaultModule::CreateVaultMajorTab(const FSpawnTabArgs& TabSpawnArgs)
{
	const TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
		.TabRole(ETabRole::MajorTab);

	VaultBasePanelWidget = SNew(SVaultRootPanel, SpawnedTab, TabSpawnArgs.GetOwnerWindow());

	SpawnedTab->SetContent(VaultBasePanelWidget.ToSharedRef());

	return SpawnedTab;
}

void FVaultModule::FreeDependency(void*& Handle)
{
	if (Handle != nullptr)
	{
		FPlatformProcess::FreeDllHandle(Handle);
		Handle = nullptr;
	}
}

bool FVaultModule::LoadDependency(const FString& Dir, const FString& Name, void*& Handle)
{
	FString Lib = Name + TEXT(".") + FPlatformProcess::GetModuleExtension();
	FString Path = Dir.IsEmpty() ? *Lib : FPaths::Combine(*Dir, *Lib);

	if (FPaths::FileExists(*Path))
	{
		UE_LOG(LogVault, Log, TEXT("%s exists"), *Name);
	}

	//FPlatformProcess::PushDllDirectory(*Dir);

	Handle = FPlatformProcess::GetDllHandle(*Path);

	if (Handle == nullptr)
	{
		UE_LOG(LogVault, Warning, TEXT("Failed to load required library %s. Plug-in will not be functional."), *Lib);
		return false;
	}
	return true;
}

void FVaultModule::UpdateMetaFilesCache()
{
	MetaFilesCache = FMetadataOps::FindAllMetadataInLibrary();
}

void FVaultModule::HandleRenameAsset()
{
	UE_LOG(LogVault, Display, TEXT("Hello?"));
}

void FVaultModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FVaultCommands::Get().PluginAction);
}

TSharedRef<FExtender> FVaultModule::AssetMenuExtender(const TArray<FAssetData>& Assets)
{
	// Extension variable contains a copy of selected paths array, you must keep Extension somewhere to prevent it from being deleted/garbage collected!
	Extension = MakeShareable(new FContentBrowserMenuExtension(Assets));
	// Create extender that contains a delegate that will be called to get information about new context menu items
	TSharedPtr<FExtender> MenuExtender = MakeShared<FExtender>();
	// Create a Shared-pointer delegate that keeps a weak reference to object
	// "NewFolder" is a hook name that is used by extender to identify extenders that will extend path context menu
	MenuExtender->AddMenuExtension(
		"AssetContextSourceControl", EExtensionHook::After, TSharedPtr<FUICommandList>(),
		FMenuExtensionDelegate::CreateSP(Extension.ToSharedRef(),
		&FContentBrowserMenuExtension::AddMenuEntry));
	return MenuExtender.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FVaultModule, Vault)