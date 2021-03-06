// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 * A backend wrapper that implements a cache hierarchy of backends. 
**/
class FHierarchicalDerivedDataBackend : public FDerivedDataBackendInterface
{
public:

	/**
	 * Constructor
	 * @param	InInnerBackends Backends to call into for actual storage of the cache, first item is the "fastest cache"
	 */
	FHierarchicalDerivedDataBackend(const TArray<FDerivedDataBackendInterface*>& InInnerBackends)
		: InnerBackends(InInnerBackends)
		, bIsWritable(false)
	{
		check(InnerBackends.Num() > 1); // if it is just one, then you don't need this wrapper
		for (int32 CacheIndex = 0; CacheIndex < InnerBackends.Num(); CacheIndex++)
		{
			if (InnerBackends[CacheIndex]->IsWritable())
			{
				bIsWritable = true;
			}
		}
		if (bIsWritable)
		{
			for (int32 CacheIndex = 0; CacheIndex < InnerBackends.Num(); CacheIndex++)
			{
				// async puts to allow us to fill all levels without holding up the engine
				new (AsyncPutInnerBackends) TScopedPointer<FDerivedDataBackendInterface>(new FDerivedDataBackendAsyncPutWrapper(InnerBackends[CacheIndex], false));
			}
		}
	}

	/** Adds inner backend. */
	void AddInnerBackend(FDerivedDataBackendInterface* InInner) 
	{
		InnerBackends.Add(InInner);
	}

	/** Removes inner backend. */
	bool RemoveInnerBackend(FDerivedDataBackendInterface* InInner) 
	{
		const int32 NumRemoved = InnerBackends.Remove(InInner);
		return NumRemoved != 0;
	}

	/** return true if this cache is writable **/
	virtual bool IsWritable() override
	{
		return bIsWritable;
	}

	/**
	 * Synchronous test for the existence of a cache item
	 *
	 * @param	CacheKey	Alphanumeric+underscore key of this cache item
	 * @return				true if the data probably will be found, this can't be guaranteed because of concurrency in the backends, corruption, etc
	 */
	virtual bool CachedDataProbablyExists(const TCHAR* CacheKey) override
	{
		for (int32 CacheIndex = 0; CacheIndex < InnerBackends.Num(); CacheIndex++)
		{
			if (InnerBackends[CacheIndex]->CachedDataProbablyExists(CacheKey))
			{
				return true;
			}
		}
		return false;
	}
	/**
	 * Synchronous retrieve of a cache item
	 *
	 * @param	CacheKey	Alphanumeric+underscore key of this cache item
	 * @param	OutData		Buffer to receive the results, if any were found
	 * @return				true if any data was found, and in this case OutData is non-empty
	 */
	virtual bool GetCachedData(const TCHAR* CacheKey, TArray<uint8>& OutData) override
	{
		for (int32 CacheIndex = 0; CacheIndex < InnerBackends.Num(); CacheIndex++)
		{
			if (InnerBackends[CacheIndex]->CachedDataProbablyExists(CacheKey) && InnerBackends[CacheIndex]->GetCachedData(CacheKey, OutData))
			{
				if (bIsWritable)
				{
					// fill in the higher level caches
					for (int32 PutCacheIndex = CacheIndex - 1; PutCacheIndex >= 0; PutCacheIndex--)
					{
						if (InnerBackends[PutCacheIndex]->IsWritable())
						{
							if (InnerBackends[PutCacheIndex]->BackfillLowerCacheLevels() &&
								InnerBackends[PutCacheIndex]->CachedDataProbablyExists(CacheKey))
							{
								InnerBackends[PutCacheIndex]->RemoveCachedData(CacheKey, /*bTransient=*/ false); // it apparently failed, so lets delete what is there
								AsyncPutInnerBackends[PutCacheIndex]->PutCachedData(CacheKey, OutData, true); // we force a put here because it must have failed
							}
							else
							{
								AsyncPutInnerBackends[PutCacheIndex]->PutCachedData(CacheKey, OutData, false); 
							}
						}
					}
					if (InnerBackends[CacheIndex]->BackfillLowerCacheLevels())
					{
						// fill in the lower level caches
						for (int32 PutCacheIndex = CacheIndex + 1; PutCacheIndex < AsyncPutInnerBackends.Num(); PutCacheIndex++)
						{
							if (!InnerBackends[PutCacheIndex]->IsWritable() && !InnerBackends[PutCacheIndex]->BackfillLowerCacheLevels() && InnerBackends[PutCacheIndex]->CachedDataProbablyExists(CacheKey))
							{
								break; //do not write things that are already in the read only pak file
							}
							if (InnerBackends[PutCacheIndex]->IsWritable())
							{
								AsyncPutInnerBackends[PutCacheIndex]->PutCachedData(CacheKey, OutData, false); // we do not need to force a put here
							}
						}
					}
				}
				return true;
			}
		}
		return false;
	}
	/**
	 * Asynchronous, fire-and-forget placement of a cache item
	 *
	 * @param	CacheKey	Alphanumeric+underscore key of this cache item
	 * @param	InData		Buffer containing the data to cache, can be destroyed after the call returns, immediately
	 * @param	bPutEvenIfExists	If true, then do not attempt skip the put even if CachedDataProbablyExists returns true
	 */
	virtual void PutCachedData(const TCHAR* CacheKey, TArray<uint8>& InData, bool bPutEvenIfExists) override
	{
		if (!bIsWritable)
		{
			return; // no point in continuing down the chain
		}
		bool bSynchronousPutPeformed = false;  // we must do at least one synchronous put to a writable cache before we return
		for (int32 PutCacheIndex = 0; PutCacheIndex < InnerBackends.Num(); PutCacheIndex++)
		{
			if (!InnerBackends[PutCacheIndex]->IsWritable() && !InnerBackends[PutCacheIndex]->BackfillLowerCacheLevels() && InnerBackends[PutCacheIndex]->CachedDataProbablyExists(CacheKey))
			{
				break; //do not write things that are already in the read only pak file
			}
			if (InnerBackends[PutCacheIndex]->IsWritable())
			{
				if (!bSynchronousPutPeformed)
				{
					InnerBackends[PutCacheIndex]->PutCachedData(CacheKey, InData, bPutEvenIfExists);
					bSynchronousPutPeformed = true;
				}
				else
				{
					AsyncPutInnerBackends[PutCacheIndex]->PutCachedData(CacheKey, InData, bPutEvenIfExists);
				}
			}
		}
	}

	virtual void RemoveCachedData(const TCHAR* CacheKey, bool bTransient) override
	{
		if (!bIsWritable)
		{
			return; // no point in continuing down the chain
		}
		for (int32 PutCacheIndex = 0; PutCacheIndex < InnerBackends.Num(); PutCacheIndex++)
		{
			InnerBackends[PutCacheIndex]->RemoveCachedData(CacheKey, bTransient);
		}
	}
private:

	/** Array of backends forming the hierarchical cache...the first element is the fastest cache. **/
	TArray<FDerivedDataBackendInterface*> InnerBackends;
	/** Each of the backends wrapped with an async put **/
	TArray<TScopedPointer<FDerivedDataBackendInterface> > AsyncPutInnerBackends;
	/** As an optimization, we check our writable status at contruction **/
	bool bIsWritable;
};
