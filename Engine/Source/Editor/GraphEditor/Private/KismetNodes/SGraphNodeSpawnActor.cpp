// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "SGraphNode.h"
#include "SGraphNodeK2Base.h"
#include "SGraphNodeK2Default.h"
#include "SGraphNodeSpawnActor.h"
#include "KismetPins/SGraphPinObject.h"
#include "NodeFactory.h"
#include "Editor/ClassViewer/Public/ClassViewerModule.h"
#include "Editor/ClassViewer/Public/ClassViewerFilter.h"

#define LOCTEXT_NAMESPACE "SGraphNodeSpawnActor"

//////////////////////////////////////////////////////////////////////////
// SGraphPinSwitchNodeDefaultCaseExec

/** 
 * GraphPin can select only actor classes generated by blueprint.
 * Instead of asset picker, a class viewer is used, and blueprint is obtained form class.
 */
class SGraphPinActorBasedBlueprintClass : public SGraphPinObject
{
	void OnClassPicked(UClass* InChosenClass)
	{
		AssetPickerAnchor->SetIsOpen(false);

		if(InChosenClass && GraphPinObj)
		{
			check(InChosenClass->ClassGeneratedBy);
			check(InChosenClass->IsChildOf(AActor::StaticClass()));
			if(const UEdGraphSchema* Schema = GraphPinObj->GetSchema())
			{
				Schema->TrySetDefaultObject(*GraphPinObj, InChosenClass->ClassGeneratedBy);
			}
		}
	}

	class FActorBasedBlueprintClassFilter : public IClassViewerFilter
	{
	public:

		virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs ) OVERRIDE
		{
			if(NULL != InClass)
			{
				const bool bGeneratedByBlueprint = NULL != Cast<UBlueprint>(InClass->ClassGeneratedBy);
				const bool bActorBased = InClass->IsChildOf(AActor::StaticClass());
				return bGeneratedByBlueprint && bActorBased;
			}
			return false;
		}

		virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) OVERRIDE
		{
			const bool bActorBased = InUnloadedClassData->IsChildOf(AActor::StaticClass());
			return bActorBased;
		}
	};

protected:

	virtual FText GetDefaultComboText() const OVERRIDE { return LOCTEXT( "DefaultComboText", "Select Blueprint" ); }

	virtual TSharedRef<SWidget> GenerateAssetPicker() OVERRIDE
	{
		FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

		FClassViewerInitializationOptions Options;
		Options.Mode = EClassViewerMode::ClassPicker;
		Options.DisplayMode = EClassViewerDisplayMode::DefaultView;
		//Options.bIsActorsOnly = true;
		//Options.bIsBlueprintBaseOnly = true;
		Options.bShowUnloadedBlueprints = true;
		Options.bShowNoneOption = true;
		Options.bShowObjectRootClass = false;
		TSharedPtr< FActorBasedBlueprintClassFilter > Filter = MakeShareable(new FActorBasedBlueprintClassFilter);
		Options.ClassFilter = Filter;

		return
			SNew(SBox)
			.WidthOverride(280)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(500)
				[ 
					SNew(SBorder)
					.Padding(4)
					.BorderImage( FEditorStyle::GetBrush("ToolPanel.GroupBorder") )
					[
						ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &SGraphPinActorBasedBlueprintClass::OnClassPicked))
					]
				]			
			];
	}
};

//////////////////////////////////////////////////////////////////////////
// SGraphNodeSpawnActor

void SGraphNodeSpawnActor::CreatePinWidgets()
{
	UK2Node_SpawnActor* SpawnActorNode = CastChecked<UK2Node_SpawnActor>(GraphNode);
	UEdGraphPin* BlueprintPin = SpawnActorNode->GetBlueprintPin();

	// Create Pin widgets for each of the pins, except for the Blueprint pin
	for (auto PinIt = GraphNode->Pins.CreateConstIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* CurrentPin = *PinIt;
		if (CurrentPin != BlueprintPin)
		{
			CreateStandardPinWidget(CurrentPin);
		}
	}

	// Handle the Blueprint pin
	if ((BlueprintPin != NULL) && (!BlueprintPin->bHidden || (BlueprintPin->LinkedTo.Num() > 0)))
	{
		TSharedPtr<SGraphPinActorBasedBlueprintClass> NewPin = SNew(SGraphPinActorBasedBlueprintClass, BlueprintPin);
		check(NewPin.IsValid());
		NewPin->SetIsEditable(IsEditable);

		this->AddPin(NewPin.ToSharedRef());
	}
}

#undef LOCTEXT_NAMESPACE