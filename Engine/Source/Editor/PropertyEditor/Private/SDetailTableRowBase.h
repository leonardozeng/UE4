// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SDetailTableRowBase : public STableRow< TSharedPtr< IDetailTreeNode > >
{
public:
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override
	{
		if( OwnerTreeNode.IsValid() && MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !StaticCastSharedRef<STableViewBase>( OwnerTablePtr.Pin()->AsWidget() )->IsRightClickScrolling() )
		{
			FMenuBuilder MenuBuilder( true, NULL );

			TArray< TSharedRef<IDetailTreeNode> > VisibleChildren;
			OwnerTreeNode.Pin()->GetChildren( VisibleChildren );

			bool bShouldOpenMenu = false;
			// Open context menu if this node can be expanded 
			if( VisibleChildren.Num() )
			{
				bShouldOpenMenu = true;

				FUIAction ExpandAllAction( FExecuteAction::CreateSP( this, &SDetailTableRowBase::OnExpandAllClicked ) );
				FUIAction CollapseAllAction( FExecuteAction::CreateSP( this, &SDetailTableRowBase::OnCollapseAllClicked ) );

				MenuBuilder.BeginSection( NAME_None, NSLOCTEXT("PropertyView", "ExpansionHeading", "Expansion") );
					MenuBuilder.AddMenuEntry( NSLOCTEXT("PropertyView", "CollapseAll", "Collapse All"), NSLOCTEXT("PropertyView", "CollapseAll_ToolTip", "Collapses this item and all children"), FSlateIcon(), CollapseAllAction );
					MenuBuilder.AddMenuEntry( NSLOCTEXT("PropertyView", "ExpandAll", "Expand All"), NSLOCTEXT("PropertyView", "ExpandAll_ToolTip", "Expands this item and all children"), FSlateIcon(), ExpandAllAction );
				MenuBuilder.EndSection();

			}

			bShouldOpenMenu |= OnContextMenuOpening(MenuBuilder);

			if( bShouldOpenMenu )
			{
				FSlateApplication::Get().PushMenu(AsShared(), MenuBuilder.MakeWidget(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect::ContextMenu);

				return FReply::Handled();
			}
		}

		return STableRow< TSharedPtr< IDetailTreeNode > >::OnMouseButtonUp( MyGeometry, MouseEvent );
	}

protected:
	/**
	 * Called when the user opens the context menu on this row
	 *
	 * @param MenuBuilder	The menu builder to add menu items to
	 * @return true if menu items were added
	 */
	virtual bool OnContextMenuOpening( FMenuBuilder& MenuBuilder ) { return false; }

private:
	void OnExpandAllClicked()
	{
		TSharedPtr<IDetailTreeNode> OwnerTreeNodePin = OwnerTreeNode.Pin();
		if( OwnerTreeNodePin.IsValid() )
		{
			const bool bRecursive = true;
			const bool bIsExpanded = true;
			OwnerTreeNodePin->GetDetailsView().SetNodeExpansionState( OwnerTreeNodePin.ToSharedRef(), bIsExpanded, bRecursive );
		}
	}

	void OnCollapseAllClicked()
	{
		TSharedPtr<IDetailTreeNode> OwnerTreeNodePin = OwnerTreeNode.Pin();
		if( OwnerTreeNodePin.IsValid() )
		{
			const bool bRecursive = true;
			const bool bIsExpanded = false;
			OwnerTreeNodePin->GetDetailsView().SetNodeExpansionState( OwnerTreeNodePin.ToSharedRef(), bIsExpanded, bRecursive );
		}
	}
protected:
	static float ScrollbarPaddingSize;
	TWeakPtr<IDetailTreeNode> OwnerTreeNode;
};
