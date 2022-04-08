// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Styling/CoreStyle.h"
#include "Framework/Commands/Commands.h"
#include "VaultStyle.h"

/**
 * 
 */
class VAULT_API FVaultCommands : public TCommands<FVaultCommands>
{
public:
	FVaultCommands()
		: TCommands<FVaultCommands>( TEXT("Vault"), NSLOCTEXT("Vault", "Vault Plugin", "Vault Plugin"), NAME_None, FVaultStyle::GetStyleSetName())
	{
	}

	virtual ~FVaultCommands()
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> PluginAction;
	TSharedPtr<FUICommandInfo> VaultExportAsset;
	TSharedPtr<FUICommandInfo> Rename;
};
