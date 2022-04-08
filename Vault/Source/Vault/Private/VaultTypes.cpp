#include "VaultTypes.h"

FSimpleDelegate& FVaultMetadata::OnRenameRequested()
{
	return RenameRequestedEvent;
}

FSimpleDelegate& FVaultMetadata::OnRenameCanceled()
{
	return RenameCanceledEvent;
}