// Copyright Daniel Orchard 2020

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "SlateBasics.h"
#include "VaultTypes.h"
#include "ContentBrowserMenuExtension.h"
#include "SVaultRootPanel.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVault, Log, All);

DECLARE_DELEGATE_OneParam(FExportAssetDelegate, FAssetData&);
DECLARE_DELEGATE_OneParam(FUpdateAssetDelegate, FVaultMetadata&);

class FToolBarBuilder;
class FMenuBuilder;
class UAssetPublisher;

class FVaultModule : public IModuleInterface
{
public:
	// Delegate when asset gets chosen from the content browser.
	FExportAssetDelegate OnAssetForExportChosen;
	// Delegate when asset should be updated (e.g. selected from the loader window)
	FUpdateAssetDelegate OnAssetForUpdateChosen;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void SpawnOperationsTab();

	void SpawnOperationsTab(FName SubTabName);

	// Get Module function
	static FVaultModule& Get();

	TSharedPtr<SVaultRootPanel> VaultBasePanelWidget;

	UAssetPublisher* GetAssetPublisherInstance() { return AssetPublisherInstance; }
	
private:

	void AddToolbarExtension(FToolBarBuilder& Builder);
	TSharedRef<FExtender> AssetMenuExtender(const TArray<FAssetData>& Assets);
	void AddMenuExtension(FMenuBuilder& Builder);

	TSharedRef<SDockTab> CreateVaultMajorTab(const FSpawnTabArgs& TabSpawnArgs);

	TSharedPtr<class FUICommandList> PluginCommands;

	UAssetPublisher* AssetPublisherInstance;

	static void* LibHandle;

	TSharedPtr<FContentBrowserMenuExtension> Extension;

protected:
	static void FreeDependency(void*& Handle);
	static bool LoadDependency(const FString& Dir, const FString& Name, void*& Handle);
};
