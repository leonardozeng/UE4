// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DesktopTargetPlatform.h: Declares the FDesktopTargetPlatform class.
=============================================================================*/

#pragma once

#if WITH_ENGINE
#include "StaticMeshResources.h"
#endif // WITH_ENGINE

class FDesktopPlatformProperties : public FGenericPlatformProperties
{
public:
	static FORCEINLINE const char* PlatformName()
	{
		return "Desktop";
	}
	static FORCEINLINE const char* IniPlatformName()
	{
		// this uses generic, non-platform-specific .ini files
		return "";
	}
	static FORCEINLINE bool HasEditorOnlyData()
	{
		return false;
	}
	static FORCEINLINE bool RequiresCookedData()
	{
		return true;
	}
};

/**
 * FDesktopTargetPlatform, abstraction for cooking iOS platforms
 */
class FDesktopTargetPlatform
	: public TTargetPlatformBase<FDesktopPlatformProperties>
{
public:

	/**
	 * Default constructor.
	 */
	FDesktopTargetPlatform();

	/**
	 * Destructor.
	 */
	~FDesktopTargetPlatform();

public:

	// Begin TTargetPlatformBase interface

	virtual bool IsServerOnly( ) const override
	{
		return false;
	}

	// End TTargetPlatformBase interface

public:

	// Begin ITargetPlatform interface

	virtual void EnableDeviceCheck(bool OnOff) override
	{
	}

	virtual void GetAllDevices( TArray<ITargetDevicePtr>& OutDevices ) const override
	{
	}

	virtual ECompressionFlags GetBaseCompressionMethod() const override
	{
		return COMPRESS_ZLIB;
	}

	virtual bool GenerateStreamingInstallManifest(const TMultiMap<FString, int32>& ChunkMap, const TSet<int32>& ChunkIDsInUse) const override
	{
		return true;
	}

	virtual ITargetDevicePtr GetDefaultDevice( ) const override
	{
		return NULL;
	}

	virtual ITargetDevicePtr GetDevice( const FTargetDeviceId& DeviceId ) override
	{
		return NULL;
	}

	virtual bool IsRunningPlatform( ) const override
	{
		return false;
	}

	virtual bool SupportsFeature( ETargetPlatformFeatures Feature ) const override
	{
		return TTargetPlatformBase<FDesktopPlatformProperties>::SupportsFeature(Feature);
	}

	virtual bool IsSdkInstalled(bool bProjectHasCode, FString& OutTutorialPath) const override
	{
		return true;
	}

	virtual int32 CheckRequirements(const FString& ProjectPath, bool bProjectHasCode, FString& OutTutorialPath) const override
	{
		return ETargetPlatformReadyStatus::Ready;
	}


#if WITH_ENGINE

	virtual void GetAllPossibleShaderFormats( TArray<FName>& OutFormats ) const override;

	virtual void GetAllTargetedShaderFormats( TArray<FName>& OutFormats ) const override;

	virtual const class FStaticMeshLODSettings& GetStaticMeshLODSettings( ) const override
	{
		return StaticMeshLODSettings;
	}

	virtual void GetTextureFormats(const UTexture* Texture, TArray<FName>& OutFormats) const override;
	
	virtual const UTextureLODSettings& GetTextureLODSettings() const override
	{
		return *TextureLODSettings;
	}

	virtual void RegisterTextureLODSettings(const UTextureLODSettings* InTextureLODSettings) override
	{
		TextureLODSettings = InTextureLODSettings;
	}

	virtual FName GetWaveFormat( const class USoundWave* Wave ) const override;
#endif // WITH_ENGINE


	DECLARE_DERIVED_EVENT(FDesktopTargetPlatform, ITargetPlatform::FOnTargetDeviceDiscovered, FOnTargetDeviceDiscovered);
	virtual FOnTargetDeviceDiscovered& OnDeviceDiscovered( ) override
	{
		return DeviceDiscoveredEvent;
	}

	DECLARE_DERIVED_EVENT(FDesktopTargetPlatform, ITargetPlatform::FOnTargetDeviceLost, FOnTargetDeviceLost);
	virtual FOnTargetDeviceLost& OnDeviceLost( ) override
	{
		return DeviceLostEvent;
	}

private:

#if WITH_ENGINE
	// Holds the Engine INI settings, for quick use.
	FConfigFile EngineSettings;

	// Holds the cache of the target LOD settings.
	const UTextureLODSettings* TextureLODSettings;

	// Holds the static mesh LOD settings.
	FStaticMeshLODSettings StaticMeshLODSettings;
#endif // WITH_ENGINE

	// Holds an event delegate that is executed when a new target device has been discovered.
	FOnTargetDeviceDiscovered DeviceDiscoveredEvent;

	// Holds an event delegate that is executed when a target device has been lost, i.e. disconnected or timed out.
	FOnTargetDeviceLost DeviceLostEvent;
};
