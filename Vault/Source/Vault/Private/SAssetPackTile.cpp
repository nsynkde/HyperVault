// Copyright Daniel Orchard 2020

#include "SAssetPackTile.h"
#include "VaultSettings.h"
#include "MetadataOps.h"
#include "Vault.h"

#include "SlateBasics.h"
#include "ImageUtils.h"
#include "Engine/Texture2D.h"

#include "Styling/SlateBrush.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Images/SThrobber.h"
#include "AssetPublisher.h"

#include "Internationalization/BreakIterator.h"

#define LOCTEXT_NAMESPACE "VaultListsDefinitions"

SAssetTileItem::~SAssetTileItem()
{
	if (TextureResource)
	{
		TextureResource->ClearFlags(RF_Standalone);
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAssetTileItem::Construct(const FArguments& InArgs)
{
	AssetItem = InArgs._AssetItem;

	TSharedRef<SWidget> ThumbnailWidget = CreateTileThumbnail(AssetItem);
		
	// Clear Old
	this->ChildSlot [ SNullWidget::NullWidget ];
	
	TSharedPtr<SVerticalBox> StandardWidget =
		SNew(SVerticalBox)

		// Image Area
		+SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.FillHeight(0.9f)
		.Padding(FMargin(0.0f, 3.0f, 0.0f, 0.0f))
		[
			SNew(SScaleBox)
			.Stretch(EStretch::ScaleToFit)
			[
				// Optional Overlay Box to help additional meta later in pipe. 
				SNew(SOverlay)
				+SOverlay::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				[
					ThumbnailWidget
				]
			]
		]

		// File Name
		/*+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			.Padding(FMargin(8.0f, 11.0f, 3.0f, 0.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromName(InArgs._AssetItem->PackName.IsNone() ? TEXT("Unknown Pack") : AssetItem->PackName))
					.WrapTextAt(300)
					.Justification(ETextJustify::Left)
				]
		];*/

		// File Name
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			.Padding(FMargin(8.0f, 11.0f, 3.0f, 0.0f))
			[
				SAssignNew(InlineRenameWidget, SInlineEditableTextBlock)
				.Font(FEditorStyle::GetFontStyle("ContentBrowser.AssetTileViewNameFont"))
				.Text(FText::FromName(InArgs._AssetItem->PackName.IsNone() ? TEXT("Unknown Pack") : AssetItem->PackName))
				.WrapTextAt(300)
				.OnBeginTextEdit(this, &SAssetTileItem::HandleBeginNameChange)
				.OnTextCommitted(this, &SAssetTileItem::HandleNameCommitted)
				.OnVerifyTextChanged(this, &SAssetTileItem::HandleVerifyNameChanged)
				.IsReadOnly(this, &SAssetTileItem::IsNameReadOnly)
				.Justification(ETextJustify::Center)
				.LineBreakPolicy(FBreakIterator::CreateCamelCaseBreakIterator())
				.ModiferKeyForNewLine(EModifierKey::None)
			]
		];
		
		ChildSlot
		[
			StandardWidget->AsShared()
		];

		if (AssetItem.IsValid())
		{
			AssetItem->OnRenameRequested().BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode);
			AssetItem->OnRenameCanceled().BindSP(InlineRenameWidget.Get(), &SInlineEditableTextBlock::ExitEditingMode);
		}


}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAssetTileItem::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	const float PrevSizeX = LastGeometry.Size.X;

	LastGeometry = AllottedGeometry;

	// Set cached wrap text width based on new "LastGeometry" value. 
	// We set this only when changed because binding a delegate to text wrapping attributes is expensive
	if (PrevSizeX != AllottedGeometry.Size.X && InlineRenameWidget.IsValid())
	{
		InlineRenameWidget->SetWrapTextAt(GetNameTextWrapWidth());
	}
}

TSharedRef<SWidget> SAssetTileItem::CreateTileThumbnail(TSharedPtr<FVaultMetadata> Meta)
{
	const FString Root = FVaultSettings::Get().GetThumbnailCacheRoot();
	const FString FileId = Meta->FileId.ToString();
	const FString Filepath = Root / FileId + TEXT(".png");

	if (FPaths::FileExists(Filepath) == false)
	{
		return SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage) 
				.Image(FEditorStyle::GetDefaultBrush())
			];
	}
	
	UTexture2D* ThumbTexture = FImageUtils::ImportFileAsTexture2D(Filepath);
	ThumbTexture->SetFlags(RF_Standalone);

	Brush = MakeShareable(new FSlateBrush());

	if (ThumbTexture)
	{
		Brush->SetResourceObject(ThumbTexture);
		Brush->DrawAs = ESlateBrushDrawType::Image;
		TextureResource = Brush->GetResourceObject();
	}

	return SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(Brush.Get())
			.Visibility(EVisibility::SelfHitTestInvisible)
		];
}

void SAssetTileItem::HandleBeginNameChange(const FText& OriginalText)
{

}

void SAssetTileItem::HandleNameCommitted(const FText& NewText, ETextCommit::Type CommitInfo)
{
	FVaultMetadata RenameMetaData;

	RenameMetaData.Author = AssetItem->Author;
	RenameMetaData.PackName = AssetItem->PackName;
	RenameMetaData.FileId = AssetItem->FileId;
	RenameMetaData.Description = AssetItem->Description;
	RenameMetaData.CreationDate = AssetItem->CreationDate;
	RenameMetaData.LastModified = FDateTime::UtcNow();
	RenameMetaData.Tags = AssetItem->Tags;
	RenameMetaData.MachineID = AssetItem->MachineID;
	RenameMetaData.ObjectsInPack = AssetItem->ObjectsInPack;

	if (UAssetPublisher::RenamePackage(FName(NewText.ToString()), RenameMetaData))
	{
		FVaultModule::Get().OnAssetWasUpdated.ExecuteIfBound();
	}
}

bool SAssetTileItem::HandleVerifyNameChanged(const FText& NewText, FText& OutErrorMessage)
{
	if (AssetItem->PackName != FName(*NewText.ToString()) && UAssetPublisher::FindMetadataByPackName(FName(*NewText.ToString())).IsMetaValid())
	{
		InlineRenameWidget->SetColorAndOpacity(FLinearColor::Red);
	}
	else
	{
		InlineRenameWidget->SetColorAndOpacity(FLinearColor::White);
	}

	return !OnVerifyRenameCommit.IsBound() || OnVerifyRenameCommit.Execute(AssetItem, NewText, LastGeometry.GetLayoutBoundingRect(), OutErrorMessage);
}

bool SAssetTileItem::IsNameReadOnly() const
{
	return false;
}

#undef LOCTEXT_NAMESPACE