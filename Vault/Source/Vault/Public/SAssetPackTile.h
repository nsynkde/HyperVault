// Copyright Daniel Orchard 2020

#pragma once

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "VaultTypes.h"

DECLARE_DELEGATE_RetVal_FourParams(bool, FOnVerifyRenameCommit, const TSharedPtr<FVaultMetadata>& /*AssetItem*/, const FText& /*NewName*/, const FSlateRect& /*MessageAnchor*/, FText& /*OutErrorMessage*/)

class VAULT_API SAssetTileItem : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SAssetTileItem) {}

	/** Item to use for populating */
	SLATE_ARGUMENT(TSharedPtr<FVaultMetadata>, AssetItem)

	SLATE_END_ARGS()

	~SAssetTileItem();

	// On Construction
	void Construct(const FArguments& InArgs);

	// Create the Tile Thumbnail, Returns Widget ready to use
	TSharedRef<SWidget> CreateTileThumbnail(TSharedPtr<FVaultMetadata> Meta);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:

	// Asset ref passed in on Construct by the Table Generator
	TSharedPtr<FVaultMetadata> AssetItem;

	// Holds the Thumbnail Brush (SlateBrush)
	TSharedPtr<FSlateBrush> Brush;

	/** Stores our resource for the texture used to clear that flags that keep it from GC */
	UObject* TextureResource;

protected:

	TSharedPtr<SInlineEditableTextBlock> InlineRenameWidget;

	/** Handles starting a name change */
	virtual void HandleBeginNameChange(const FText& OriginalText);

	/** Handles committing a name change */
	virtual void HandleNameCommitted(const FText& NewText, ETextCommit::Type CommitInfo);

	/** Handles verifying a name change */
	virtual bool HandleVerifyNameChanged(const FText& NewText, FText& OutErrorMessage);

	/** SAssetViewItem interface */
	virtual float GetNameTextWrapWidth() const { return LastGeometry.GetLocalSize().X - 2.f; }

	/** Check to see if the name should be read-only */
	bool IsNameReadOnly() const;

	/** Delegate for when an asset name has been entered for an item to verify the name before commit */
	FOnVerifyRenameCommit OnVerifyRenameCommit;

	FGeometry LastGeometry;
	
};