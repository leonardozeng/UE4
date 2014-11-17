// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "BlueprintActionMenuUtils.h"
#include "BlueprintActionMenuBuilder.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintEditorUtils.h"	// for DoesSupportComponents()
#include "BlueprintPaletteFavorites.h"
#include "EdGraphSchema_K2.h"		// for bUseLegacyActionMenus 
#include "K2Node_ActorBoundEvent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_MatineeController.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "KismetEditorUtilities.h"	// for CanPasteNodes()
#include "K2ActionMenuBuilder.h"

#define LOCTEXT_NAMESPACE "BlueprintActionMenuUtils"

/*******************************************************************************
* Static FBlueprintActionMenuUtils Helpers
******************************************************************************/

namespace BlueprintActionMenuUtilsImpl
{
	/**
	 * Additional filter rejection test, for menu sections that only contain 
	 * bound actions. Rejects any action that is not bound.
	 * 
	 * @param  Filter			The filter querying this rejection test.
	 * @param  BlueprintAction	The action to test.
	 * @return True if the spawner is unbound (and should be rejected), otherwise false.
	 */
	static bool IsUnBoundSpawner(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * Filter rejection test, for favorite menus. Rejects any actions that are 
	 * not favorited by the user.
	 * 
	 * @param  Filter			The filter querying this rejection test.
	 * @param  BlueprintAction	The action to test.
	 * @return True if the spawner is not favorited (and should be rejected), otherwise false.
	 */
	static bool IsNonFavoritedAction(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction);

	/**
	 * Utility function to find a common base class from a set of given classes.
	 * 
	 * @param  ClassSet	The set of classes that you want a single base class for.
	 * @return A common base class for all the specified classes (fallsback to UObject's class).
	 */
	static UClass* FindCommonBaseClass(TArray<UClass*> const& ClassSet);
}

//------------------------------------------------------------------------------
static bool BlueprintActionMenuUtilsImpl::IsUnBoundSpawner(FBlueprintActionFilter const& /*Filter*/, FBlueprintActionInfo& BlueprintAction)
{
	return (BlueprintAction.GetBindings().Num() <= 0);
}

//------------------------------------------------------------------------------
static bool BlueprintActionMenuUtilsImpl::IsNonFavoritedAction(FBlueprintActionFilter const& Filter, FBlueprintActionInfo& BlueprintAction)
{
	UEditorUserSettings& EditorUserSettings = GEditor->AccessEditorUserSettings();
	// grab the user's favorites
	UBlueprintPaletteFavorites const* BlueprintFavorites = EditorUserSettings.BlueprintFavorites;
	checkSlow(BlueprintFavorites != nullptr);

	return !BlueprintFavorites->IsFavorited(BlueprintAction);
}

//------------------------------------------------------------------------------
static UClass* BlueprintActionMenuUtilsImpl::FindCommonBaseClass(TArray<UClass*> const& ClassSet)
{
	UClass* CommonClass = UObject::StaticClass();
	if (ClassSet.Num() > 0)
	{
		CommonClass = ClassSet[0];
		for (UClass const* Class : ClassSet)
		{
			while (!Class->IsChildOf(CommonClass))
			{
				CommonClass = CommonClass->GetSuperClass();
			}
		}
	}	
	return CommonClass;
}

/*******************************************************************************
 * FBlueprintActionMenuUtils
 ******************************************************************************/

//------------------------------------------------------------------------------
void FBlueprintActionMenuUtils::MakePaletteMenu(FBlueprintActionContext const& Context, UClass* FilterClass, FBlueprintActionMenuBuilder& MenuOut)
{
	MenuOut.Empty();
	
	uint32 FilterFlags = 0x00;
	if (FilterClass != nullptr)
	{
		// make sure we exclude global and static library actions
		FilterFlags |= FBlueprintActionFilter::BPFILTER_RejectGlobalFields;
	}
	
	FBlueprintActionFilter MenuFilter(FilterFlags);
	MenuFilter.Context = Context;
	
	// self member variables can be accessed through the MyBluprint panel (even
	// inherited ones)... external variables can be accessed through the context
	// menu (don't want to clutter the palette, I guess?)
	MenuFilter.RejectedNodeTypes.Add(UK2Node_VariableGet::StaticClass());
	MenuFilter.RejectedNodeTypes.Add(UK2Node_VariableSet::StaticClass());
	
	if (FilterClass != nullptr)
	{
		MenuFilter.TargetClasses.Add(FilterClass);
	}

	MenuOut.AddMenuSection(MenuFilter, LOCTEXT("PaletteRoot", "Library"), /*MenuOrder =*/0, FBlueprintActionMenuBuilder::ConsolidatePropertyActions);
	MenuOut.RebuildActionList();
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuUtils::MakeContextMenu(FBlueprintActionContext const& Context, bool bIsContextSensitive, FBlueprintActionMenuBuilder& MenuOut)
{
	using namespace BlueprintActionMenuUtilsImpl;

	//--------------------------------------
	// Composing Filters
	//--------------------------------------

	FBlueprintActionFilter MenuFilter;
	MenuFilter.Context = Context;
	MenuFilter.Context.SelectedObjects.Empty();

	FBlueprintActionFilter ComponentsFilter;
	ComponentsFilter.Context = Context;
	ComponentsFilter.PermittedNodeTypes.Add(UK2Node_CallFunction::StaticClass());
	ComponentsFilter.PermittedNodeTypes.Add(UK2Node_ComponentBoundEvent::StaticClass());
	// only want bound actions for this menu section
	ComponentsFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(IsUnBoundSpawner));

	FBlueprintActionFilter LevelActorsFilter;
	LevelActorsFilter.Context = Context;
	LevelActorsFilter.PermittedNodeTypes.Add(UK2Node_CallFunction::StaticClass());
	LevelActorsFilter.PermittedNodeTypes.Add(UK2Node_ActorBoundEvent::StaticClass());
	LevelActorsFilter.PermittedNodeTypes.Add(UK2Node_Literal::StaticClass());
	LevelActorsFilter.PermittedNodeTypes.Add(UK2Node_MatineeController::StaticClass());
	// only want bound actions for this menu section
	LevelActorsFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(IsUnBoundSpawner));

	// make sure the bound menu sections have the proper OwnerClasses specified
	for (UObject* Selection : Context.SelectedObjects)
	{
		if (UObjectProperty* ObjProperty = Cast<UObjectProperty>(Selection))
		{
			ComponentsFilter.TargetClasses.Add(ObjProperty->PropertyClass);
			LevelActorsFilter.Context.SelectedObjects.Remove(Selection);
		}
		else if (AActor* LevelActor = Cast<AActor>(Selection))
		{
			ComponentsFilter.Context.SelectedObjects.Remove(Selection);
			// the loop below (over the editor's selected actors) will add to 
			// LevelActorsFilter's OwnerClasses
		}
		else
		{
			ComponentsFilter.Context.SelectedObjects.Remove(Selection);
			LevelActorsFilter.Context.SelectedObjects.Remove(Selection);
		}
	}

	// make sure all selected level actors are accounted for (in case the caller
	// did not include them in the context)
	for (FSelectionIterator LvlActorIt(*GEditor->GetSelectedActors()); LvlActorIt; ++LvlActorIt)
	{
		AActor* LevelActor = Cast<AActor>(*LvlActorIt);
		LevelActorsFilter.Context.SelectedObjects.AddUnique(LevelActor);
		LevelActorsFilter.TargetClasses.Add(LevelActor->GetClass());
	}

	bool const bOnlyBlueprintMembers = bIsContextSensitive && (MenuFilter.Context.Pins.Num() == 0);
	bool bCanOperateOnLevelActors = bIsContextSensitive;
	bool bCanHaveActorComponents  = bIsContextSensitive;
	// determine if we can operate on certain object selections (level actors, 
	// components, etc.)
	for (UBlueprint* Blueprint : Context.Blueprints)
	{
		UClass* BlueprintClass = Blueprint->SkeletonGeneratedClass;
		if (BlueprintClass != nullptr)
		{
			bCanOperateOnLevelActors &= BlueprintClass->IsChildOf<ALevelScriptActor>();
			if (bOnlyBlueprintMembers)
			{
				MenuFilter.TargetClasses.Add(BlueprintClass);
			}
		}
		bCanHaveActorComponents &= FBlueprintEditorUtils::DoesSupportComponents(Blueprint);
	}

	//--------------------------------------
	// Defining Menu Sections
	//--------------------------------------

	static int32 const MainMenuSectionGroup   = 000;
	static int32 const ComponentsSectionGroup = 100;
	static int32 const LevelActorSectionGroup = 101;

	MenuOut.Empty();

	if (!bIsContextSensitive)
	{
		MenuFilter.Context.Pins.Empty();
	}
	// for backwards compatibility, the main menu section should be added first 
	// (so if we have to, the legacy menu system can be constructed from it)
	MenuOut.AddMenuSection(MenuFilter);

	bool const bAddComponentsSection = bIsContextSensitive && bCanHaveActorComponents && (ComponentsFilter.Context.SelectedObjects.Num() > 0);
	// add the components section to the menu (if we don't have any components
	// selected, then inform the user through a dummy menu entry)
	if (bAddComponentsSection)
	{
		MenuOut.AddMenuSection(ComponentsFilter, FText::GetEmpty(), ComponentsSectionGroup);
	}

	bool const bAddLevelActorsSection = bIsContextSensitive && bCanOperateOnLevelActors && (LevelActorsFilter.Context.SelectedObjects.Num() > 0);
	// add the level actor section to the menu
	if (bAddLevelActorsSection)
	{
		// since we're consolidating all the bound actions, then we have to pick 
		// one common base class to filter by
		UClass* CommonClass = FindCommonBaseClass(LevelActorsFilter.TargetClasses);
		LevelActorsFilter.TargetClasses.Empty();
		LevelActorsFilter.TargetClasses.Add(CommonClass);

		MenuOut.AddMenuSection(LevelActorsFilter, FText::GetEmpty(), LevelActorSectionGroup);
	}

	//--------------------------------------
	// Building the Menu
	//--------------------------------------

	MenuOut.RebuildActionList();

	UEditorExperimentalSettings const* ExperimentalSettings =  GetDefault<UEditorExperimentalSettings>();
	if (ExperimentalSettings->bUseRefactoredBlueprintMenuingSystem)
	{
		for (UEdGraph const* Graph : Context.Graphs)
		{
			if (FKismetEditorUtilities::CanPasteNodes(Graph))
			{
				// @TODO: Grey out menu option with tooltip if one of the nodes cannot paste into this graph
				TSharedPtr<FEdGraphSchemaAction> PasteHereAction(new FEdGraphSchemaAction_K2PasteHere(TEXT(""), LOCTEXT("PasteHereMenuName", "Paste here"), TEXT(""), MainMenuSectionGroup));
				MenuOut.AddAction(PasteHereAction);
				break;
			}
		}

		if (bIsContextSensitive && bCanHaveActorComponents && !bAddComponentsSection)
		{
			FText SelectComponentMsg = LOCTEXT("SelectComponentForEvents", "Select a Component to see available Events & Functions");
			FText SelectComponentToolTip = LOCTEXT("SelectComponentForEventsTooltip", "Select a Component in the MyBlueprint tab to see available Events and Functions in this menu.");
			TSharedPtr<FEdGraphSchemaAction> MsgAction = TSharedPtr<FEdGraphSchemaAction>(new FEdGraphSchemaAction_Dummy(TEXT(""), SelectComponentMsg, SelectComponentToolTip.ToString(), ComponentsSectionGroup));
			MenuOut.AddAction(MsgAction);
		}

		if (bIsContextSensitive && bCanOperateOnLevelActors && !bAddLevelActorsSection)
		{
			FText SelectActorsMsg = LOCTEXT("SelectActorForEvents", "Select Actor(s) to see available Events & Functions");
			FText SelectActorsToolTip = LOCTEXT("SelectActorForEventsTooltip", "Select Actor(s) in the level to see available Events and Functions in this menu.");
			TSharedPtr<FEdGraphSchemaAction> MsgAction = TSharedPtr<FEdGraphSchemaAction>(new FEdGraphSchemaAction_Dummy(TEXT(""), SelectActorsMsg, SelectActorsToolTip.ToString(), LevelActorSectionGroup));
			MenuOut.AddAction(MsgAction);
		}
	}
}

//------------------------------------------------------------------------------
void FBlueprintActionMenuUtils::MakeFavoritesMenu(FBlueprintActionContext const& Context, FBlueprintActionMenuBuilder& MenuOut)
{
	MenuOut.Empty();

	UEditorExperimentalSettings const* ExperimentalSettings = GetDefault<UEditorExperimentalSettings>();
	if (ExperimentalSettings->bUseRefactoredBlueprintMenuingSystem)
	{
		FBlueprintActionFilter MenuFilter;
		MenuFilter.Context = Context;
		MenuFilter.AddRejectionTest(FBlueprintActionFilter::FRejectionTestDelegate::CreateStatic(BlueprintActionMenuUtilsImpl::IsNonFavoritedAction));

		MenuOut.AddMenuSection(MenuFilter);
		MenuOut.RebuildActionList();
	}
	else
	{
		check(Context.Blueprints.Num() > 0);
		FBlueprintPaletteListBuilder LegacyMenuBuilder(Context.Blueprints[0]);
		UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
		FK2ActionMenuBuilder(K2Schema).GetPaletteActions(LegacyMenuBuilder, /*ClassFilter =*/nullptr);

		UEditorUserSettings& EditorUserSettings = GEditor->AccessEditorUserSettings();
		// grab the user's favorites
		UBlueprintPaletteFavorites const* BlueprintFavorites = EditorUserSettings.BlueprintFavorites;
		check(BlueprintFavorites != nullptr);

		for (int32 ActionIndex = 0; ActionIndex < LegacyMenuBuilder.GetNumActions(); ++ActionIndex)
		{
			FGraphActionListBuilderBase::ActionGroup& ActionGroup = LegacyMenuBuilder.GetAction(ActionIndex);
			if (!ensure(ActionGroup.Actions.Num() == 1))
			{
				continue;
			}

			TSharedPtr<FEdGraphSchemaAction> Action = ActionGroup.Actions[0];
			if (!Action.IsValid())
			{
				continue;
			}

			if (BlueprintFavorites->IsFavorited(Action))
			{
				MenuOut.AddAction(Action);
			}
		}
	} 	
}

#undef LOCTEXT_NAMESPACE