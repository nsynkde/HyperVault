// Copyright Daniel Orchard 2020

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Slate.h"
#include "SlateExtras.h"
#include "VaultTypes.h"

typedef TSharedPtr<FTagFilteringItem> FTagFilteringItemPtr;
typedef TSharedPtr<FDeveloperFilteringItem> FDeveloperFilteringItemPtr;



class VAULT_API SLoaderWindow : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SLoaderWindow) {}
	SLATE_END_ARGS()

	// On Construct
	void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow);

	// On Tick
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// Populate the Base Asset Tree - No filtering Applied. Called on Construction
	void PopulateBaseAssetList();

	// Populate Base Tags List Array
	void PopulateTagArray();

	// Updates the Tag list with correct Numbers.
	//void UpdateTagArray();

	// Populate Base Developer List Array
	void PopulateDeveloperNameArray();

	// ----  Tables  ---- //

	// Create Individual Tile Widget. Bound to the GenerateTile Event
	TSharedRef<ITableRow> MakeTileViewWidget(TSharedPtr<FVaultMetadata> AssetItem, const TSharedRef<STableViewBase>& OwnerTable);

	// Create the Tag Filter Widget. Bound to the GenerateTile Event
	TSharedRef<ITableRow> MakeTagFilterViewWidget(FTagFilteringItemPtr inTag, const TSharedRef<STableViewBase>& OwnerTable);

	// // Create the Developer Filter Widget. Bound to the GenerateTile Event
	TSharedRef<ITableRow> MakeDeveloperFilterViewWidget(FDeveloperFilteringItemPtr Entry, const TSharedRef<STableViewBase>& OwnerTable);
	// ---- End Tables ----- //

	void OnAssetTileSelectionChanged(TSharedPtr<FVaultMetadata> InItem, ESelectInfo::Type SelectInfo);

	void OnAssetTileDoubleClicked(TSharedPtr<FVaultMetadata> InItem);

	TSharedPtr<SWidget> OnAssetTileContextMenuOpened();

	TSharedRef<SWidget> OnSortingOptionsMenuOpened();

	
	// ---- Thumbnail Scale System ---- //

	// Widget Ref for Scale Slider
	TSharedPtr<SSlider> UserScaleSlider;
	
	float TileUserScale = 1.0f;

	void OnThumbnailSliderValueChanged(float Value);

	// ---- End Thumbnail Scale System ---- 


	// ---- Tag Search System ----

	// Holder for the array of Tags
	TArray<FTagFilteringItemPtr> TagCloud;

	// ---- End Tag Search System ----


	// ---- Developer Name Search System ----

	// Holder for the array of Developer Names
	TArray<FDeveloperFilteringItemPtr> DeveloperCloud;

	// ---- End Developer Name Search System ----
	

	// ---- Search Bar System ----

	// Widget Ref for Search Box
	TSharedPtr<SSearchBox> SearchBox;

	// Widget Ref for Search Box Strict Search Check Box
	TSharedPtr<SCheckBox> StrictSearchCheckBox;

	void OnSearchBoxChanged(const FText& inSearchText);
	
	void OnSearchBoxCommitted(const FText& InFilterText, ETextCommit::Type CommitType);

	// ---- End Search Bar System ----

	// ---- Metadata Zone ---- //

	void ConstructMetadataWidget(TSharedPtr<FVaultMetadata> AssetMeta);
	TSharedPtr<SVerticalBox> MetadataWidget;
	//TSharedPtr<SBox> MetaWrapper;
	
	// GLOBALS

	void LoadAssetPackIntoProject(TSharedPtr<FVaultMetadata> InPack);

	void DeleteAssetPack(TSharedPtr<FVaultMetadata> InPack);

	TSet<FString> ActiveTagFilters;
	TSet<FName> ActiveDevFilters;

	// Force Refresh the File List. Does not call Redraw
	void RefreshAvailableFiles();

	void UpdateFilteredAssets();

	TEnumAsByte<SortingTypes> ActiveSortingType;

	void SortFilteredAssets(TEnumAsByte<SortingTypes> SortingType, bool Reverse = false);
	void SortFilteredAssets();

	TArray<TSharedPtr<FVaultMetadata>> FilteredAssetItems;

	TSharedPtr<STileView<TSharedPtr<FVaultMetadata>>> TileView;

	// Static Base Values
	static const int32 THUMBNAIL_BASE_HEIGHT;
	static const int32 THUMBNAIL_BASE_WIDTH;
	static const int32 TILE_BASE_HEIGHT;
	static const int32 TILE_BASE_WIDTH;

	// Utility

	FText DisplayTotalAssetsInLibrary() const;


	FReply OnRefreshLibraryClicked();

	void RefreshLibrary();

	// Bound to Static delegate in Asset Publisher, so we can update when user pushes a new asset or updates an existing one
	void OnAssetUpdateHappened();

private:
	int32 LastSearchTextLength;

	bool IsConnected;

	bool bSortingReversed;

public:

	void ModifyActiveTagFilters(FString TagModified, bool bFilterThis);

	void ModifyActiveDevFilters(FName DevModified, bool bFilterThis);
};



