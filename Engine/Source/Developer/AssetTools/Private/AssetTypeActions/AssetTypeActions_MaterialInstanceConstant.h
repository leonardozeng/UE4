// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Materials/MaterialInstanceConstant.h"
#include "AssetTypeActions_MaterialInterface.h"

class FAssetTypeActions_MaterialInstanceConstant : public FAssetTypeActions_MaterialInterface
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MaterialInstanceConstant", "Material Instance"); }
	virtual FColor GetTypeColor() const override { return FColor(0,128,0); }
	virtual UClass* GetSupportedClass() const override { return UMaterialInstanceConstant::StaticClass(); }
	virtual void GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder ) override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
	virtual bool CanFilter() override { return true; }

private:
	/** Handler for when FindParent is selected */
	void ExecuteFindParent(TArray<TWeakObjectPtr<UMaterialInstanceConstant>> Objects);
};