// Copyright Daniel Orchard 2020

#include "SPublisherWindow.h"
#include "AssetPublisher.h"
#include "AssetPublisherTagsCustomization.h"
#include "Vault.h"
#include "VaultSettings.h"
#include "VaultStyle.h"
#include "MetadataOps.h"

#include "Misc/DateTime.h"
#include "Engine/Engine.h"
#include "Slate/SceneViewport.h"
#include "Styling/SlateBrush.h"
#include "Engine/AssetManager.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#include "FileHelpers.h"
#include "ImageUtils.h"
#include "PropertyCustomizationHelpers.h" //Asset Picker
#include "Widgets/Input/SEditableTextBox.h"
#include "EditorFontGlyphs.h"
#include "ImageWriteBlueprintLibrary.h"
#include "Interfaces/IPluginManager.h"
#include "LevelEditorViewport.h" // Enabling GameView for Thumbnail Gen
#include "ObjectTools.h"

#define LOCTEXT_NAMESPACE "FVaultPublisher"

#define VAULT_PUBLISHER_CAPTURE_SIZE 512
#define VAULT_PUBLISHER_THUMBNAIL_SIZE 256

class SVaultLogMessageListRow : public SMultiColumnTableRow<TSharedPtr<FVaultLogMessage>>
{
public:
	SLATE_BEGIN_ARGS(SVaultLogMessageListRow) { }
	SLATE_ARGUMENT(TSharedPtr<FVaultLogMessage>, Message)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		Message = InArgs._Message;
		SMultiColumnTableRow<TSharedPtr<FVaultLogMessage>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		return SNew(SBox)
			.Padding(FMargin(4.0, 0.0))
			.VAlign((VAlign_Center))
			[
				SNew(STextBlock)
				.ColorAndOpacity(HandleGetTextColor())
			.Text(Message->Message)
			];
	}

private:

	// Callback for getting the task's status text.
	FSlateColor HandleGetTextColor() const
	{
		if ((Message->Verbosity == ELogVerbosity::Error) ||
			(Message->Verbosity == ELogVerbosity::Fatal))
		{
			return FLinearColor::Red;
		}
		else if (Message->Verbosity == ELogVerbosity::Warning)
		{
			return FLinearColor::Yellow;
		}
		else
		{
			return FSlateColor::UseForeground();
		}
	}

	// Holds a pointer to the task that is displayed in this row.
	TSharedPtr<FVaultLogMessage> Message;
};

void SPublisherWindow::Construct(const FArguments& InArgs)
{
	// Store a shared this
	TWeakPtr<SPublisherWindow> WeakPtr = SharedThis(this);

	VaultOutputLog = MakeShareable(new FVaultOutputLog);

	// Add our Vault icon as the default icon for the thumbnail.
	const FSlateBrush* MyBrush = FVaultStyle::Get().GetBrush("Vault.Icon512px");
	ThumbBrush = *MyBrush;

	// Pre-Fill in the author name from the Local Settings
	
	CurrentAuthorName = FVaultSettings::Get().GetDefaultDeveloperName();

	AssetHierarchyBadness = 0;

	// Attempt to Show the map in the dropdown by default, but not solved yet.
	//const FString VaultPluginContentPath = IPluginManager::Get().FindPlugin(TEXT("Vault"))->GetContentDir();
	//const FString DefaultMapPath = VaultPluginContentPath / "PresetMap.umap";
	//UObject* MapPtr = LoadObject<UObject>(NULL, *DefaultMapPath, NULL, LOAD_None, NULL);
	//MapPtr = LoadObject<UObject>(NULL, TEXT("/Game/Main.umap"));
	//FAssetData PresetMap = FAssetData(MapPtr);
	//CurrentlySelectedScreenshotMap = PresetMap;
	
	// Our Asset Picking Widget
	TSharedRef<SWidget> AssetPickerWidget = SNew(SObjectPropertyEntryBox)
		.ObjectPath(this, &SPublisherWindow::GetCurrentAssetPath)
		.AllowedClass(UObject::StaticClass())
		.OnObjectChanged(this, &SPublisherWindow::OnAssetSelected)
		.AllowClear(false)
		.DisplayUseSelected(true)
		.DisplayBrowse(true)
		.NewAssetFactories(TArray<UFactory*>())
		.IsEnabled(true);

	// Left SideBar
	TSharedRef<SVerticalBox> LeftPanel = SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0, 3.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0, 0, 3, 0))
			[
				SNew(STextBlock)
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.12"))
				.Text_Lambda([this]
				{
					return !CurrentlySelectedAsset.IsValid() ?
						FEditorFontGlyphs::Times_Circle : FEditorFontGlyphs::Check_Circle;
				})
				.ColorAndOpacity_Lambda([this]
				{
					return !CurrentlySelectedAsset.IsValid() ? FLinearColor::Red : FLinearColor::Green;
				})
			]

			+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("PackageObjectLbl", "Package Asset"))
				]

			+ SHorizontalBox::Slot()
			[
				AssetPickerWidget
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0, 3.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0, 0, 3, 0))
			[
				SNew(STextBlock)
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.12"))
				.Text_Lambda([this]
				{
					return PackageNameInput->GetText().IsEmpty() ?
						FEditorFontGlyphs::Times_Circle : FEditorFontGlyphs::Check_Circle;
				})
				.ColorAndOpacity_Lambda([this]
				{
					return PackageNameInput->GetText().IsEmpty() ? FLinearColor::Red : FLinearColor::Green;
				})
			]

			+SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PackageNameLbl", "Package Name"))
			]
			+SHorizontalBox::Slot()
			[
				SAssignNew(PackageNameInput, SEditableTextBox)
				.HintText(LOCTEXT("PackageNameHintTxt", "Package Name"))
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0, 3.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0,0,3,0))
			[
				SNew(STextBlock)
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.12"))
				.Text_Lambda([this] 
				{ 
					return AuthorInput->GetText().IsEmpty() ? 
						FEditorFontGlyphs::Times_Circle : FEditorFontGlyphs::Check_Circle;
				})
				.ColorAndOpacity_Lambda([this]
				{
					return AuthorInput->GetText().IsEmpty() ? FLinearColor::Red : FLinearColor::Green;
				})
			]
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AuthorNameLbl", "Author"))
			]
			+ SHorizontalBox::Slot()
			[
				SAssignNew(AuthorInput, SEditableTextBox)
				.HintText(LOCTEXT("AuthorNameHintTxt", "Author Name"))
				.Text(this, &SPublisherWindow::GetAuthorName)
				.OnTextCommitted(this, &SPublisherWindow::OnAuthorNameTextCommitted)
				.SelectAllTextWhenFocused(true)
				
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(0.0, 3.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(FMargin(0, 0, 3, 0))
			[
				SNew(STextBlock)
				.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.12"))
				.Text_Lambda([this]
				{
					return DescriptionInput->GetText().IsEmpty() ?
						FEditorFontGlyphs::Times_Circle : FEditorFontGlyphs::Check_Circle;
				})
				.ColorAndOpacity_Lambda([this]
				{
					return DescriptionInput->GetText().IsEmpty() ? FLinearColor::Red : FLinearColor::Green;
				})
			]
			+ SHorizontalBox::Slot()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DescriptionNameLbl", "Description"))
			]
			+ SHorizontalBox::Slot()
			[
				SAssignNew(DescriptionInput, SMultiLineEditableTextBox)
				.HintText(LOCTEXT("DescriptionNameHintTxt", "Enter Description"))
			]
		]

		// tags
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 5, 0, 0))
			[
				SAssignNew(TagsWidget, SPublisherTagsWidget)
			]

		// asset issues hierarchy
		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 10.f, 0, 0))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(0,0,3,0))
					[
						SNew(STextBlock)
						.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.12"))
						.Visibility(this, &SPublisherWindow::GetErrorMessageVisibility)
						.Text_Lambda([this]
							{
								if (AssetHierarchyBadness == 0)
								{
									return FEditorFontGlyphs::Check;
								}
								else if (AssetHierarchyBadness > 0 && AssetHierarchyBadness < 3)
								{
									return FEditorFontGlyphs::Exclamation;
								}
								else
								{
									return FEditorFontGlyphs::Times;
								}
							})
						.ColorAndOpacity_Lambda([this]
							{
								if (AssetHierarchyBadness == 0)
								{
									return FLinearColor::Green;
								}
								else if (AssetHierarchyBadness > 0 && AssetHierarchyBadness < 3)
								{
									return FLinearColor::Yellow;
								}
								else
								{
									return FLinearColor::Red;
								}
							})
					]
				+ SHorizontalBox::Slot()
					[
					SNew(SBox)
						.MinDesiredHeight(30.f)
						.MaxDesiredHeight(120.f)
						[
						SAssignNew(ErrorMessageBox, SMultiLineEditableTextBox)
							.IsReadOnly(true)
							.AllowMultiLine(true)
							.AlwaysShowScrollbars(false)
							//.BackgroundColor(FLinearColor::Black)
							.Style(FEditorStyle::Get(), "Log.TextBox")
							.Text(this, &SPublisherWindow::GetErrorMessage)
							.Visibility(this, &SPublisherWindow::GetErrorMessageVisibility)
						]
					]
				
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Padding(0.f, 16.f, 0.f, 0.f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.OnClicked(this, &SPublisherWindow::TryPackage)
				.Text(LOCTEXT("SubmitToVaultLabel", "Submit to Vault"))
				.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
				.TextStyle(FEditorStyle::Get(), "NormalText.Important")
				.ContentPadding(FMargin(10.0, 10.0))
				.ButtonColorAndOpacity_Lambda([this]
					{
						if (AssetHierarchyBadness == 0)
						{
							return FLinearColor::Green;
						}
						else if (AssetHierarchyBadness > 0 && AssetHierarchyBadness < 3)
						{
							return FLinearColor::Yellow;
						}
						else
						{
							return FLinearColor::Red;
						}
					})
				//.IsEnabled(this, &SPublisherWindow::CanPackage) // dont handle it like this until we sort the disabled style
			]

		+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0,20,0,0)
			[
				ConstructOutputLog()
			];

	// Start UI
	TSharedRef<SVerticalBox> RootWidget = SNew(SVerticalBox);

	RootWidget->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.FillHeight(1)
		.Padding(2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1)
			.Padding(FMargin(5.0, 5.0, 10.0, 5.f))
			[
				// Left Side Slot
				LeftPanel
			]
			+SHorizontalBox::Slot()
			.Padding(FMargin(10.0, 5.0, 5.0, 5.f))
			.HAlign(HAlign_Right)
			.AutoWidth()
			[
				// Left Panel
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					ConstructThumbnailWidget().ToSharedRef()
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0,5,0,0))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(FMargin(0.f, 0.f, 1.f, 0.f))
					[
						// Take Screenshot From World
						SNew(SButton)
						.ContentPadding(FMargin(6.f))
						.ButtonStyle(FEditorStyle::Get(), "FlatButton.Primary")
						.OnClicked(this, &SPublisherWindow::OnCaptureImageFromViewport)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.12"))
								.TextStyle(FEditorStyle::Get(), "NormalText.Important")
								.Text(FEditorFontGlyphs::Video_Camera)
							]

							+ SHorizontalBox::Slot()
							.FillWidth(1)
							.Padding(FMargin(5, 0, 0, 0))
							[
								SNew(STextBlock)
								.TextStyle(FEditorStyle::Get(), "NormalText.Important")
								.Text(LOCTEXT("TakeScreenshotLbl", "From Viewport"))
							]
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(FMargin(1.f, 0.f, 0.f, 0.f))
					[
						SNew(SButton)
						.ContentPadding(FMargin(6.f))
						.ButtonStyle(FEditorStyle::Get(), "FlatButton.Primary")
						.OnClicked(this, &SPublisherWindow::OnCaptureImageFromFile)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.12"))
								.TextStyle(FEditorStyle::Get(), "NormalText.Important")
								.Text(FEditorFontGlyphs::File)

							]
							+ SHorizontalBox::Slot()
							.FillWidth(1)
							.Padding(FMargin(5,0,0,0))
							[
								SNew(STextBlock)
								.TextStyle(FEditorStyle::Get(), "NormalText.Important")
								.Text(LOCTEXT("LoadScreenshotFromFileLbl", "Load From File"))
							]
						]
					]
					+ SHorizontalBox::Slot()
					.Padding(FMargin(1.f, 0.f, 0.f, 0.f))
					[
						SNew(SButton)
						.ContentPadding(FMargin(6.f))
						.ButtonStyle(FEditorStyle::Get(), "FlatButton.Primary")
						.OnClicked(this, &SPublisherWindow::OnCaptureImageFromAsset)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.12"))
								.TextStyle(FEditorStyle::Get(), "NormalText.Important")
								.Text(FEditorFontGlyphs::Archive)
							]
							+ SHorizontalBox::Slot()
							.FillWidth(1)
							.Padding(FMargin(5, 0, 0, 0))
							[
								SNew(STextBlock)
								.TextStyle(FEditorStyle::Get(), "NormalText.Important")
								.Text(LOCTEXT("TakeAssetThumbnailLbl", "From Asset"))
							]
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0, 5, 0, 0))
				[
					SNew(SButton)
					.ContentPadding(FMargin(6.f))
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Info")
					.ToolTipText(LOCTEXT("LoadMapFromPresetTooltip", "Load Map from Preset. \n This will load either the Preset map, or if you have a map file to publish, will load this map. \nThis is hard-coded in this release, but future releases will support multiple choices"))
					.OnClicked(this, &SPublisherWindow::GenerateMapFromPreset)
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.Text(LOCTEXT("LoadPresetMapLabel", "Load Preset Map"))
						.TextStyle(FEditorStyle::Get(), "NormalText.Important")
					]
				]
				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(FMargin(0, 5, 0, 0))
					[
						// Our Asset Picking Widget
						SNew(SObjectPropertyEntryBox)
						.ObjectPath(this, &SPublisherWindow::GetCurrentScreenshotMapPath)
						.AllowedClass(UWorld::StaticClass())
						.OnObjectChanged(this, &SPublisherWindow::OnScreenshotMapSelectionChanged)
						.AllowClear(false)
						.DisplayUseSelected(true)
						.DisplayBrowse(true)
						.NewAssetFactories(TArray<UFactory*>())
						.IsEnabled(true)
					]

				+ SVerticalBox::Slot()
				.Padding(0,15,0,3)
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("seecondaryAssetBoxHeaderLbl", "Package List"))
					]
				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SBox)
							.MinDesiredHeight(30.f)
							.MaxDesiredHeight(180.f)
						[
							SAssignNew(SecondaryAssetsBox, SMultiLineEditableTextBox)
							.IsReadOnly(true)
							.AllowMultiLine(true)
							.AlwaysShowScrollbars(false)
							//.BackgroundColor(FLinearColor::Black)
							.Style(FEditorStyle::Get(), "Log.TextBox")
							.Text(this, &SPublisherWindow::GetSecondaryAssetList)
						]
					]
			]
		]; // Close VBox
		
	ChildSlot
	[
		RootWidget
	];

	FVaultModule::Get().OnAssetForExportChosen.BindLambda([this](FAssetData& AssetToExport)
		{
			// Checking that the widget is ready by using any accesible widget variable
			if (ThumbnailWidget.IsValid()) {
				OpenWithAsset(AssetToExport);
			}
		});

	FVaultModule::Get().OnAssetForUpdateChosen.BindLambda([this](FVaultMetadata& AssetMetadata)
		{
			// Checking that the widget is ready by using any accesible widget variable
			if (ThumbnailWidget.IsValid()) {
				OpenWithMetadata(AssetMetadata);
			}
		});
}

SPublisherWindow::~SPublisherWindow()
{
	if (ShotTexture)
	{
		ShotTexture->ClearFlags(RF_Standalone);
	}
}

TSharedPtr<SWidget> SPublisherWindow::ConstructThumbnailWidget()
{
	// Init the thumb brush.
	ThumbBrush.SetImageSize(FVector2D(256.0));

	return ThumbnailWidget = 
		SNew(SBox)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SAssignNew(ThumbnailImage, SImage)
				.Image(&ThumbBrush)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SCircularThrobber)
				.Radius(48)
				.NumPieces(10)
				.Period(0.5f)
				.Visibility(EVisibility::Hidden)
				.Visibility_Lambda([this]()
				{
					if (ShotTexture && ShotTexture->IsValidLowLevel())
					{
						return ShotTexture->IsFullyStreamedIn() ? EVisibility::Hidden : EVisibility::SelfHitTestInvisible;
					}
					return EVisibility::Hidden;
				})
			]
		];
}

FReply SPublisherWindow::OnCaptureImageFromViewport()
{
	ShotTexture = CreateThumbnailFromScene();
	ShotTexture->SetFlags(RF_Standalone);

	if (ShotTexture)
	{
		ThumbBrush.SetResourceObject(ShotTexture);
		ThumbBrush.SetImageSize(FVector2D(VAULT_PUBLISHER_THUMBNAIL_SIZE));
	}
	
	//ThumbBrush = Brush;
	return FReply::Handled();
}

FReply SPublisherWindow::OnCaptureImageFromFile()
{
	ShotTexture = SelectThumbnailFromFile();
	FSlateBrush Brush;

	if (ShotTexture)
	{
		Brush.SetResourceObject(ShotTexture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetImageSize(FVector2D(VAULT_PUBLISHER_THUMBNAIL_SIZE, VAULT_PUBLISHER_THUMBNAIL_SIZE));
		ThumbBrush = Brush;
	}
	return FReply::Handled();
}

FReply SPublisherWindow::OnCaptureImageFromAsset()
{
	ShotTexture = CreateThumbnailFromAsset();
	FSlateBrush Brush;

	if (ShotTexture)
	{
		Brush.SetResourceObject(ShotTexture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetImageSize(FVector2D(VAULT_PUBLISHER_THUMBNAIL_SIZE, VAULT_PUBLISHER_THUMBNAIL_SIZE));
		ThumbBrush = Brush;
	}
	return FReply::Handled();
}

UTexture2D* SPublisherWindow::CreateThumbnailFromScene()
{
	FViewport* Viewport = GEditor->GetActiveViewport();

	if (!ensure(GCurrentLevelEditingViewportClient || !ensure(Viewport)))
	{
		return nullptr;
	}

	// Cache viewport ptr
	FLevelEditorViewportClient* OldViewportClient = GCurrentLevelEditingViewportClient;

	// Store and enable Game View to keep the screenshot clean
	bool bWasInGameView = GCurrentLevelEditingViewportClient->IsInGameView();
	GCurrentLevelEditingViewportClient->SetGameView(true);
	

	// Clear to remove viewports selection box
	GCurrentLevelEditingViewportClient = nullptr;

	// Redraw viewport
	Viewport->Draw();

	const uint32 SrcWidth = Viewport->GetSizeXY().X;
	const uint32 SrcHeight = Viewport->GetSizeXY().Y;

	TArray<FColor> OrigBitmap;
	if (!Viewport->ReadPixels(OrigBitmap) || OrigBitmap.Num() != SrcWidth * SrcHeight)
	{
		return nullptr;
	}

	TArray<FColor> ScaledBitmap;
	FImageUtils::CropAndScaleImage(SrcWidth, SrcHeight, VAULT_PUBLISHER_CAPTURE_SIZE, VAULT_PUBLISHER_CAPTURE_SIZE, OrigBitmap, ScaledBitmap);

	// Redraw viewport to restore highlight
	GCurrentLevelEditingViewportClient = OldViewportClient;
	GCurrentLevelEditingViewportClient->SetGameView(bWasInGameView);
	Viewport->Draw();

	FScopedTransaction Transaction(LOCTEXT("CapturePackageThumbnail", "Capture Package Image from Viewport"));

	FCreateTexture2DParameters Params;
	Params.bDeferCompression = true;
	Params.CompressionSettings = TC_Default;
	Params.bSRGB = true;

	// Create Image. Note - Image flagged as RF_Standalone, so its not GC'ed with level changes, but must be cleaned up to prevent long term memory use
	UTexture2D* ResizedTexture = FImageUtils::CreateTexture2D(VAULT_PUBLISHER_CAPTURE_SIZE, VAULT_PUBLISHER_CAPTURE_SIZE, ScaledBitmap, GetTransientPackage(), FString(), RF_NoFlags, Params);
	

	if (ResizedTexture)
	{
		return ResizedTexture;
	}
	return nullptr;
}

UTexture2D* SPublisherWindow::SelectThumbnailFromFile()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform == nullptr)
	{
		return nullptr;
	}

	// Formats that IImageWrapper can handle
	const FString Filter = TEXT("Image files (*.jpg; *.png; *.bmp; *.ico; *.exr; *.icns)|*.jpg; *.png; *.bmp; *.ico; *.exr; *.icns|All files (*.*)|*.*");
	TArray<FString> OutFiles;
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

	bool bPickedFile = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		LOCTEXT("LoadThumbnailFromFileTitle", "Choose a file for the thumbnail").ToString(),
		TEXT(""), TEXT(""),
		Filter,
		EFileDialogFlags::None,
		OutFiles);

	if (!bPickedFile || OutFiles.Num() != 1)
	{
		return nullptr;
	}

	FString SourceImagePath = FPaths::ConvertRelativePathToFull(OutFiles[0]);

	return CreateThumbnailFromFile(SourceImagePath);
}

UTexture2D* SPublisherWindow::CreateThumbnailFromFile(FString SourceImagePath)
{
	if (!FPlatformFileManager::Get().GetPlatformFile().FileExists(*SourceImagePath))
	{
		return nullptr;
	}

	UTexture2D* OriginalTexture = FImageUtils::ImportFileAsTexture2D(SourceImagePath);
	if (!OriginalTexture)
	{
		return nullptr;
	}

	FScopedTransaction Transaction(FText::Format(LOCTEXT("LoadThumbnailTransaction", "Load file '{0}' as a package thumbnail"), FText::FromString(SourceImagePath)));

	return OriginalTexture;
}

UTexture2D* SPublisherWindow::CreateThumbnailFromAsset()
{
	FObjectThumbnail ObjectThumbnail;
	ThumbnailTools::RenderThumbnail(CurrentlySelectedAsset.GetAsset(), VAULT_PUBLISHER_CAPTURE_SIZE, VAULT_PUBLISHER_CAPTURE_SIZE, ThumbnailTools::EThumbnailTextureFlushMode::NeverFlush, NULL, &ObjectThumbnail);
	//ObjectThumbnail = ThumbnailTools::GetThumbnailForObject(CurrentlySelectedAsset.GetAsset());

	TArray<uint8> thumbnailRawData = ObjectThumbnail.GetUncompressedImageData();
	TArray<FColor> thumbnailOrigBitmap;

	UAssetPublisher::ConvertImageBufferUInt8ToFColor(thumbnailRawData, thumbnailOrigBitmap);

	TArray<FColor> thumbnailScaledBitmap;
	FImageUtils::CropAndScaleImage(ObjectThumbnail.GetImageWidth(), ObjectThumbnail.GetImageHeight(), VAULT_PUBLISHER_CAPTURE_SIZE, VAULT_PUBLISHER_CAPTURE_SIZE, thumbnailOrigBitmap, thumbnailScaledBitmap);

	FCreateTexture2DParameters Params;
	Params.bDeferCompression = true;
	Params.CompressionSettings = TC_Default;
	Params.bSRGB = true;
	UTexture2D* OriginalTexture = FImageUtils::CreateTexture2D(VAULT_PUBLISHER_CAPTURE_SIZE, VAULT_PUBLISHER_CAPTURE_SIZE, thumbnailScaledBitmap, GetTransientPackage(), FString(), RF_NoFlags, Params);

	return OriginalTexture;
}

FReply SPublisherWindow::TryPackage()
{
	OutputLogExpandableBox->SetExpanded(true);
	
	if (!CanPackage())
	{
		UE_LOG(LogVault, Error, TEXT("One or more required fields are empty"));
		return FReply::Handled();
	}
	
#pragma region OldPackageLogic (now in AssetPublisher.cpp)
	//const FString OutputDirectory = FVaultSettings::Get().GetAssetLibraryRoot();
	//const FString ScreenshotPath = OutputDirectory / PackageNameInput->GetText().ToString() + TEXT(".png");

	//// Pack file path, only used here for duplicate detection
	//const FString PackageFileOutput = OutputDirectory / PackageNameInput->GetText().ToString() + TEXT(".upack");

	//FImageWriteOptions Params;
	//Params.bAsync = true;
	//Params.bOverwriteFile = true;
	//Params.CompressionQuality = 90;
	//Params.Format = EDesiredImageFormat::PNG;

	//if (FPaths::FileExists(PackageFileOutput))
	//{

	//	const FText ErrorMsg = LOCTEXT("TryPackageOverwriteMsg", "A Vault item already exists with this pack name, are you sure you want to overwrite it?\nThis action cannot be undone.");
	//	const FText ErrorTitle = LOCTEXT("TryPackageOverwriteTitle", "Existing Pack Detected");

	//	const EAppReturnType::Type Confirmation = FMessageDialog::Open(
	//		EAppMsgType::OkCancel, ErrorMsg, &ErrorTitle);

	//	if (Confirmation == EAppReturnType::Cancel)
	//	{
	//		UE_LOG(LogVault, Error, TEXT("User cancelled packaging operation due to duplicate pack found"));
	//		return FReply::Handled();;
	//	}
	//}

	//UImageWriteBlueprintLibrary::ExportToDisk(ShotTexture, ScreenshotPath, Params);

	//// Store PackageName
	//const FName AssetPath = CurrentlySelectedAsset.PackageName;

	//// Our core publish list, which gets written as a text file to be passed to the Pak Tool.
	//TSet<FString> PublishList;

	//// List of objects that are getting packaged, clean, for writing to the JSON.
	//TSet<FString> ObjectsInPackage;

	//FString RootPath = AssetPath.ToString();
	//FString OrigionalRootString;
	//RootPath.RemoveFromStart(TEXT("/"));
	//RootPath.Split("/", &OrigionalRootString, &RootPath, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	//OrigionalRootString = TEXT("/") + OrigionalRootString;

	//TSet<FName> AssetsToProcess = { AssetPath };
	//GetAssetDependanciesRecursive(CurrentlySelectedAsset.PackageName, AssetsToProcess, OrigionalRootString);


	//// Loop through all Assets, including the root object, and format into the correct absolute system filename for the UnrealPak operation
	//for (const FName Path : AssetsToProcess)
	//{
	//	const FString PathString = Path.ToString();

	//	UE_LOG(LogVault, Display, TEXT("Processing Object Path: %s"), *PathString);

	//	FString Filename;
	//	bool bGotFilename = FPackageName::TryConvertLongPackageNameToFilename(PathString, Filename);

	//	// Find the UPackage to determine the asset type
	//	UPackage* PrimaryAssetPackage = CurrentlySelectedAsset.GetPackage();
	//	//UPackage* Package = FindPackage(nullptr, *CurrentlySelectedAsset.PackageName.ToString());
	//	UPackage* ItemPackage = FindPackage(nullptr, *PathString);

	//	if (!PrimaryAssetPackage || !ItemPackage)
	//	{
	//		UE_LOG(LogVault, Error, TEXT("Unable to find UPackage for %s"), *CurrentlySelectedAsset.PackageName.ToString());
	//		continue;
	//	}

	//	ObjectsInPackage.Add(ItemPackage->FileName.ToString());

	//	// Get and append the asset extension
	//	const FString ExtensionName = ItemPackage->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();

	//	// Append extension into filename
	//	Filename += ExtensionName;

	//	PublishList.Add(Filename);
	//}

	//// Build a Struct from the metadata to pass to the packager
	//FVaultMetadata AssetPublishMetadata;

	//AssetPublishMetadata.Author = FName(*AuthorInput->GetText().ToString());
	//AssetPublishMetadata.PackName = FName(*PackageNameInput->GetText().ToString());
	//AssetPublishMetadata.Description = DescriptionInput->GetText().ToString();
	//AssetPublishMetadata.CreationDate = FDateTime::UtcNow();
	//AssetPublishMetadata.LastModified = FDateTime::UtcNow();
	//AssetPublishMetadata.Tags = TagsWidget->GetUserSelectedTags();
	//AssetPublishMetadata.ObjectsInPack = ObjectsInPackage;

	//// Lets see if we want to append any new tags to our global tags library
	//if (TagsWidget->GetShouldAddNewTagsToLibrary())
	//{
	//	FVaultSettings::Get().SaveVaultTags(TagsWidget->GetUserSelectedTags());
	//}

	//if (UAssetPublisher::PackageSelected(PublishList, AssetPublishMetadata))
	//{
	//	FNotificationInfo PackageResultMessage(LOCTEXT("PackageResultToast", "Packaging Successful"));
	//	PackageResultMessage.ExpireDuration = 5.0f;
	//	PackageResultMessage.bFireAndForget = true;
	//	PackageResultMessage.bUseLargeFont = true;
	//	PackageResultMessage.Image = FCoreStyle::Get().GetBrush(TEXT("NotificationList.SuccessImage"));
	//	FSlateNotificationManager::Get().AddNotification(PackageResultMessage);
	//}
	//return FReply::Handled();

#pragma endregion

	FName FileId = UAssetPublisher::CreateUniquePackageFilename();

	FVaultMetadata AssetPublishMetadata;

	AssetPublishMetadata.Author = FName(*AuthorInput->GetText().ToString());
	AssetPublishMetadata.PackName = FName(*PackageNameInput->GetText().ToString());
	AssetPublishMetadata.FileId = FileId;
	AssetPublishMetadata.Description = DescriptionInput->GetText().ToString();
	AssetPublishMetadata.CreationDate = FDateTime::UtcNow();
	AssetPublishMetadata.LastModified = FDateTime::UtcNow();
	AssetPublishMetadata.Tags = TagsWidget->GetUserSelectedTags();

	// Lets see if we want to append any new tags to our global tags library
	if (TagsWidget->GetShouldAddNewTagsToLibrary())
	{
		FVaultSettings::Get().SaveVaultTags(TagsWidget->GetUserSelectedTags());
	}

	// #todo to abstract UI from functionality, much of this code should be reviewed for splitting
	// this will be important when it comes to python hooks
	// image writing stands out as one that certainly needs to be reviewed, so python can pass its own img

	UE_LOG(LogVault, Display, TEXT("Starting Packaging Operation"));

	return UAssetPublisher::TryPackageAsset(FileId.ToString(), CurrentlySelectedAsset, AssetPublishMetadata, ShotTexture);
}

FReply SPublisherWindow::TryUpdateMetadata()
{
	// TODO allow changing the name of a package

	if (PackageNameInput->GetText().IsEmpty() ||
		AuthorInput->GetText().IsEmpty() ||
		DescriptionInput->GetText().IsEmpty() ||
		TagsWidget->GetUserSelectedTags().Num() == 0)
	{
		OutputLogExpandableBox->SetExpanded(true);
		UE_LOG(LogVault, Error, TEXT("One or more required fields are empty! Asset can stay empty when updating metadata only."));
		return FReply::Handled();
	}

	const FString LibraryPath = FVaultSettings::Get().GetAssetLibraryRoot();
	const FString MetaFilePath = LibraryPath / PackageNameInput->GetText().ToString() + ".meta";

	FVaultMetadata AssetPublishMetadata = FMetadataOps::ReadMetadata(MetaFilePath);

	AssetPublishMetadata.Author = FName(*AuthorInput->GetText().ToString());
	AssetPublishMetadata.PackName = FName(*PackageNameInput->GetText().ToString());
	AssetPublishMetadata.Description = DescriptionInput->GetText().ToString();
	AssetPublishMetadata.LastModified = FDateTime::UtcNow();
	AssetPublishMetadata.Tags = TagsWidget->GetUserSelectedTags();

	// Lets see if we want to append any new tags to our global tags library
	if (TagsWidget->GetShouldAddNewTagsToLibrary())
	{
		FVaultSettings::Get().SaveVaultTags(TagsWidget->GetUserSelectedTags());
	}

	UE_LOG(LogVault, Display, TEXT("Updating Metadata"));

	FMetadataOps::WriteMetadata(AssetPublishMetadata);

	return FReply::Handled();
}

void SPublisherWindow::GetAssetDependanciesRecursive(const FName AssetPath, TSet<FName>& AllDependencies, const FString& OriginalRoot) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FName> BaseDependencies;
	AssetRegistryModule.Get().GetDependencies(AssetPath, BaseDependencies);

	for (auto DependsIt = BaseDependencies.CreateConstIterator(); DependsIt; ++DependsIt)
	{
		if (!AllDependencies.Contains(*DependsIt))
		{
			const bool bIsEnginePackage = (*DependsIt).ToString().StartsWith(TEXT("/Engine"));
			const bool bIsScriptPackage = (*DependsIt).ToString().StartsWith(TEXT("/Script"));
			// Skip all packages whose root is different than the source package list root
			const bool bIsInSamePackage = (*DependsIt).ToString().StartsWith(OriginalRoot);
			if (!bIsEnginePackage && !bIsScriptPackage && bIsInSamePackage)
			{
				AllDependencies.Add(*DependsIt);
				GetAssetDependanciesRecursive(*DependsIt, AllDependencies, OriginalRoot);
			}
		}
	}


	//TArray<FString> DependenciesList;

	//for (FName& name : BaseDependencies)
	//{
	//	DependenciesList.Add(name.ToString());
	//}
	//return DependenciesList;

}

FText SPublisherWindow::GetSecondaryAssetList() const
{
	if (CurrentlySelectedAsset.IsValid())
	{
		// Dependancies includes original item, so lets remove that
		//Dependancies.Remove(CurrentlySelectedAsset.PackageName.ToString());

		FString FormattedList;
		for (FName Dependant : Dependencies)
		{
			FString DependantString = Dependant.ToString();
			DependantString = FPaths::GetCleanFilename(DependantString);
			FormattedList += (DependantString);
			FormattedList += LINE_TERMINATOR;
		}
			   		 	  
		return FText::FromString(FormattedList);
	}
	return FText::GetEmpty();
}

bool SPublisherWindow::CanPackage() const
{
	if (PackageNameInput->GetText().IsEmpty() ||
		AuthorInput->GetText().IsEmpty() ||
		DescriptionInput->GetText().IsEmpty() ||
		TagsWidget->GetUserSelectedTags().Num() == 0 ||
		CurrentlySelectedAsset.IsValid() == false
		)
	{
		return false;
	}
	return true;
}

TSharedRef<SWidget> SPublisherWindow::ConstructOutputLog()
{
	VaultOutputLog->OnVaultMessageReceived.BindRaw(this, &SPublisherWindow::RefreshOutputLogList);

	return SAssignNew(OutputLogExpandableBox, SExpandableArea)
	.InitiallyCollapsed(true)
	.MaxHeight(200.f)
	.HeaderContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("OutputLogHeaderLbl", "Output Log"))
	]
	.BodyContent()
	[
		SNew(SScrollBox)
		.Orientation(Orient_Vertical)
		+ SScrollBox::Slot()
		[
			SNew(SBox)
			.MinDesiredHeight(FOptionalSize(200.f))
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("Menu.Background"))

				[
					SAssignNew(VaultOutputLogList, SListView<TSharedPtr< FVaultLogMessage>>)
					.ListItemsSource(&VaultOutputLog->MessageList)
					.OnGenerateRow(this, &SPublisherWindow::HandleVaultLogGenerateRow)
					.ItemHeight(24)
					.SelectionMode(ESelectionMode::Multi)
					.HeaderRow(SNew(SHeaderRow)
					.Visibility(EVisibility::Collapsed)
					+ SHeaderRow::Column("Log")
					.DefaultLabel(LOCTEXT("VaultLogHeaderLbl", "Output Log")))
				]
			]
		]
	];

}

TSharedRef<ITableRow> SPublisherWindow::HandleVaultLogGenerateRow(TSharedPtr<FVaultLogMessage> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SVaultLogMessageListRow, OwnerTable)
		.Message(InItem);
}

void SPublisherWindow::RefreshOutputLogList()
{
	if (VaultOutputLogList)
	{
		VaultOutputLogList->RebuildList();
	}
}

FString SPublisherWindow::GetCurrentAssetPath() const
{
	return CurrentlySelectedAsset.IsValid() ? CurrentlySelectedAsset.ObjectPath.ToString() : FString("");
}

FString SPublisherWindow::GetCurrentScreenshotMapPath() const
{
	return CurrentlySelectedScreenshotMap.IsValid() ? CurrentlySelectedScreenshotMap.ObjectPath.ToString() : FString("");
}

void SPublisherWindow::OnAssetSelected(const FAssetData& InAssetData)
{
	// #todo lambda this for simpler code?
	CurrentlySelectedAsset = InAssetData;

	// Build our list of dependencies
	CheckDependencies();

	if (!ShotTexture)
	{
		ShotTexture = CreateThumbnailFromAsset();
	}

	FSlateBrush Brush;

	if (ShotTexture)
	{
		Brush.SetResourceObject(ShotTexture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetImageSize(FVector2D(VAULT_PUBLISHER_THUMBNAIL_SIZE, VAULT_PUBLISHER_THUMBNAIL_SIZE));
		ThumbBrush = Brush;
	}
}

void SPublisherWindow::OnScreenshotMapSelectionChanged(const FAssetData& InAssetData)
{
	// #todo lambda this for simpler code?
	if (InAssetData.IsValid())
	{
		CurrentlySelectedScreenshotMap = InAssetData;
	}
}

bool SPublisherWindow::IsPythonMapGenAvailable() const
{
	return false;
}

FReply SPublisherWindow::GenerateMapFromPython()
{
	return FReply::Handled();
}

FReply SPublisherWindow::GenerateMapFromPreset()
{
	/* 
		#todo. Hard coded Path to content, In future want to make this flexible, copying the map from a central location.
		We also want to unify the python and pre-made maps options, 
		as the end user doesnt really need to care the source, just the result
	*/

	FString MapPath;
	if (CurrentlySelectedScreenshotMap.IsValid())
	{
		MapPath = CurrentlySelectedScreenshotMap.ObjectPath.ToString();
	}
	else
	{
		const FString ContentPath = IPluginManager::Get().FindPlugin(TEXT("Vault"))->GetContentDir();
		MapPath = ContentPath / "PresetMap.umap";
	}

	if (CurrentlySelectedAsset.IsValid())
	{
		if (CurrentlySelectedAsset.GetPackage()->ContainsMap())
		{
			FEditorFileUtils::LoadMap(CurrentlySelectedAsset.GetPackage()->FileName.ToString(), true, true);
			return FReply::Handled();
			
		}
	}

	FEditorFileUtils::LoadMap(MapPath, true, true);
	return FReply::Handled();
}

FText SPublisherWindow::GetAuthorName() const
{
	return CurrentAuthorName;
}

void SPublisherWindow::OnAuthorNameTextCommitted(const FText& InText, ETextCommit::Type InCommitType)
{
	CurrentAuthorName = InText;
}

FText SPublisherWindow::GetErrorMessage() const
{
	FText ErrorMsg;
	if (CurrentlySelectedAsset.IsValid() && AssetHierarchyBadness > 0)
	{
		FString FormattedList;
		for (FName Dependant : Dependencies)
		{
			FString DependantString = Dependant.ToString();
			FormattedList += (DependantString);
			FormattedList += LINE_TERMINATOR;
		}

		if (AssetHierarchyBadness == 1) 
		{
			ErrorMsg = LOCTEXT("NotInVaultFolderText", "The Asset you are trying to export is not in the /Game/Vault folder!");
		}
		else if (AssetHierarchyBadness == 2) {
			ErrorMsg = FText::Format(LOCTEXT("BadAssetsInVaultText", "The Asset is in the Vault folder, but not all of dependencies are located in the same subfolder as the current asset! Please fix this!\nOnly continue if it has to be that way or you hate all your coworkers!\nProblematic Assets:\n{0}"), FText::FromString(FormattedList));
		}
		else if (AssetHierarchyBadness == 3)
		{
			ErrorMsg = FText::Format(LOCTEXT("BadAssetsInAndOutsideVaultText", "Your asset is inside the vault folder, but not all of its dependencies are inside the vault! Some assets might also be in the vault, but not in the same subfolder. Please fix this!\nOnly continue if it has to be that way or you hate all your coworkers!\nProblematic Assets:\n{0}"), FText::FromString(FormattedList));
		}
		else if (AssetHierarchyBadness == 4)
		{
			ErrorMsg = FText::Format(LOCTEXT("BadAssetsOutsideVaultText", "Your asset isn't inside the vault folder and not all of its dependencies are located in the same subfolder as the current asset! Please fix this!\nOnly continue if it has to be that way or you hate all your coworkers!\nProblematic Assets:\n{0}"), FText::FromString(FormattedList));
		}

		return ErrorMsg;
	}
	return FText();
}

EVisibility SPublisherWindow::GetErrorMessageVisibility() const
{
	if (AssetHierarchyBadness == 0)
	{
		return EVisibility::Collapsed;
	}
	else
	{
		return EVisibility::Visible;
	}
}

FSlateColor SPublisherWindow::GetSubmitButtonColor() const
{
	if (AssetHierarchyBadness == 0)
	{
		return FSlateColor(FLinearColor::FromSRGBColor(FColor::FromHex("52A852FF")));
	}
	else if (AssetHierarchyBadness > 0 && AssetHierarchyBadness < 3)
	{
		return FSlateColor(FLinearColor::FromSRGBColor(FColor::FromHex("DA9E45FF")));
	}
	else
	{
		return FSlateColor(FLinearColor::FromSRGBColor(FColor::FromHex("C64945FF")));
	}
}

void SPublisherWindow::CheckDependencies()
{
	AssetHierarchyBadness = UAssetPublisher::CheckForGoodAssetHierarchy(CurrentlySelectedAsset, Dependencies, BadDependencies);
}

// logic to execute when the vault gets opened by right clicking an asset in the content browser.
void SPublisherWindow::OpenWithAsset(FAssetData AssetForExport)
{
	CurrentlySelectedAsset = AssetForExport;
	this->OnAssetSelected(AssetForExport);
}

bool SPublisherWindow::OpenWithMetadata(FVaultMetadata AssetMetadata)
{
	FString ObjectPath = *(AssetMetadata.ObjectsInPack.begin());
	TArray<FString> temp;
	ObjectPath.ParseIntoArray(temp, FGenericPlatformMisc::GetDefaultPathSeparator(), true);
	FString FullObjectPath = ObjectPath + "." + temp.Last();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(FName(FullObjectPath), true);

	if (AssetData.IsValid())
	{
		this->OnAssetSelected(AssetData);
	}
	else
	{
		CurrentlySelectedAsset = nullptr;
		OutputLogExpandableBox->SetExpanded(true);
		UE_LOG(LogVault, Warning, TEXT("Original root asset (%s) of the selected package doesn't exist in this project!"), *ObjectPath);
	}

	FString SourceImagePath = FVaultSettings::Get().GetAssetLibraryRoot() + FGenericPlatformMisc::GetDefaultPathSeparator() + AssetMetadata.FileId.ToString() + ".png";

	ShotTexture = CreateThumbnailFromFile(SourceImagePath);
	FSlateBrush Brush;

	if (ShotTexture)
	{
		Brush.SetResourceObject(ShotTexture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.SetImageSize(FVector2D(VAULT_PUBLISHER_THUMBNAIL_SIZE, VAULT_PUBLISHER_THUMBNAIL_SIZE));
		ThumbBrush = Brush;
	}
	else
	{
		const FSlateBrush* MyBrush = FVaultStyle::Get().GetBrush("Vault.Icon512px");
		ThumbBrush = *MyBrush;
		ThumbBrush.SetImageSize(FVector2D(VAULT_PUBLISHER_THUMBNAIL_SIZE));
	}

	PackageNameInput->SetText(FText::FromName(AssetMetadata.PackName));
	DescriptionInput->SetText(FText::FromString(AssetMetadata.Description));
	FString Tags = "";
	for (FString ATag : AssetMetadata.Tags)
	{
		if (Tags.IsEmpty())
		{
			Tags += ATag;
		}
		else
		{
			Tags += "," + ATag;
		}
	}

	TagsWidget->TagsCustomBox->SetText(FText::FromString(Tags));

	return true;
}

#undef LOCTEXT_NAMESPACE
#undef VAULT_PUBLISHER_THUMBNAIL_SIZE

