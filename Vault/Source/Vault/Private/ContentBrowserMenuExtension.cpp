// Copyright NSYNK 2022


#include "ContentBrowserMenuExtension.h"

#include "ContentBrowserModule.h"
#include "Vault.h"

FContentBrowserMenuExtension::FContentBrowserMenuExtension()
{
}

FContentBrowserMenuExtension::FContentBrowserMenuExtension(const TArray<FAssetData>& inSelectedAssets)
{
	this->SelectedAssets = inSelectedAssets;
}

FContentBrowserMenuExtension::~FContentBrowserMenuExtension()
{
}

#define LOCTEXT_NAMESPACE "Vault"

void FContentBrowserMenuExtension::AddMenuEntry(FMenuBuilder& MenuBuilder)
{
	// Create Section
	MenuBuilder.BeginSection("VaultSection", LOCTEXT("VaultMenuHeading", "Vault"));
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString("Export to Vault"),
			FText::FromString("Export the selected Asset to the Vault. Only one asset can be exported at a time."),
			FSlateIcon("VaultStyle", "Vault.PluginAction"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FContentBrowserMenuExtension::OnExportToVaultClicked),
				FCanExecuteAction::CreateSP(this, &FContentBrowserMenuExtension::CanExportToVault)
			)
		);
	}
	MenuBuilder.EndSection();
}

void FContentBrowserMenuExtension::OnExportToVaultClicked()
{
	if (SelectedAssets.Num() > 0 && SelectedAssets[0].IsValid()) {
		FVaultModule::Get().SpawnOperationsTab("Asset Publisher");
		FVaultModule::Get().OnAssetForExportChosen.ExecuteIfBound(SelectedAssets[0]);
	}
	else if (SelectedAssets.Num() <= 0) {
		UE_LOG(LogVault, Error, TEXT("No asset was selected for publishing!"));
	}
	else {
		UE_LOG(LogVault, Error, TEXT("The selected asset is invalid!"));
	}
}

bool FContentBrowserMenuExtension::CanExportToVault()
{
	return SelectedAssets.Num() == 1;
}

#undef LOCTEXT_NAMESPACE