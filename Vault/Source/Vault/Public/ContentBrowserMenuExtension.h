// Copyright NSYNK 2022

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class VAULT_API FContentBrowserMenuExtension : public TSharedFromThis<FContentBrowserMenuExtension>
{
public:
	FContentBrowserMenuExtension();
	FContentBrowserMenuExtension(const TArray<FAssetData>& inSelectedAssets);
	~FContentBrowserMenuExtension();
	void AddMenuEntry(FMenuBuilder& MenuBuilder);
	void OnExportToVaultClicked();

	bool CanExportToVault();

private:
	TArray<FAssetData> SelectedAssets;
};
