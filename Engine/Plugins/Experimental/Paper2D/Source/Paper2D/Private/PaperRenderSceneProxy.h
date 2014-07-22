// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#define TEST_BATCHING 0

//////////////////////////////////////////////////////////////////////////
// FPaperSpriteVertex

/** A Paper2D sprite vertex. */
struct FPaperSpriteVertex
{
	FVector Position;
	FPackedNormal TangentX;
	FPackedNormal TangentZ;
	FLinearColor Color;
	FVector2D TexCoords;

	FPaperSpriteVertex() {}

	FPaperSpriteVertex(const FVector& InPosition, const FVector2D& InTextureCoordinate, const FLinearColor& InColor)
		: Position(InPosition)
		, TangentX(PackedNormalX)
		, TangentZ(PackedNormalZ)
		, Color(InColor)
		, TexCoords(InTextureCoordinate)
	{}

	static void SetTangentsFromPaperAxes();

	static FPackedNormal PackedNormalX;
	static FPackedNormal PackedNormalZ;
};

//////////////////////////////////////////////////////////////////////////
// FSpriteRenderSection

struct FSpriteRenderSection
{
	int32 VertexOffset;
	int32 NumVertices;

	UMaterialInterface* Material;
	UTexture2D* Texture;

	FSpriteRenderSection()
		: VertexOffset(INDEX_NONE)
		, NumVertices(0)
	{
	}

#if 0
	template <typename SourceArrayType>
	void AddTriangles(const FSpriteDrawCallRecord& Record, SourceArrayType& Vertices)
	{
		if (NumVertices == 0)
		{
			VertexOffset = Vertices.Num();
			Texture = Record.Texture;
		}
		else
		{
			checkSlow((VertexOffset + NumVertices) == Vertices.Num());
		}

		NumVertices += Record.RenderVerts.Num();
		Vertices.Reserve(NumVertices);

		for (int32 SVI = 0; SVI < Record.RenderVerts.Num(); ++SVI)
		{
			const FVector4& SourceVert = Record.RenderVerts[SVI];

			const FVector Pos((PaperAxisX * SourceVert.X) + (PaperAxisY * SourceVert.Y) + Record.Destination);
			const FVector2D UV(SourceVert.Z, SourceVert.W);

			new (Vertices) FPaperSpriteVertex(Pos, UV, Record.Color);
		}
	}
#endif
};

//////////////////////////////////////////////////////////////////////////
// FPaperRenderSceneProxy

class FPaperRenderSceneProxy : public FPrimitiveSceneProxy
{
public:
	FPaperRenderSceneProxy(const UPrimitiveComponent* InComponent);
	~FPaperRenderSceneProxy();

	// FPrimitiveSceneProxy interface.
	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) override;
	virtual void OnTransformChanged() override;
	virtual uint32 GetMemoryFootprint() const override;
	virtual bool CanBeOccluded() const override;
	virtual void CreateRenderThreadResources() override;
	// End of FPrimitiveSceneProxy interface.

	void SetDrawCall_RenderThread(const FSpriteDrawCallRecord& NewDynamicData);

	class UMaterialInterface* GetMaterial() const
	{
		return Material;
	}

protected:
	virtual void DrawDynamicElements_RichMesh(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor);

	void DrawBatch(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor, class UMaterialInterface* BatchMaterial, const TArray<FSpriteDrawCallRecord>& Batch);
	void DrawNewBatches(FPrimitiveDrawInterface* PDI, const FSceneView* View, bool bUseOverrideColor, const FLinearColor& OverrideColor);

	bool IsCollisionView(const FSceneView* View, bool& bDrawSimpleCollision, bool& bDrawComplexCollision) const;

	friend class FPaperBatchSceneProxy;

	FVertexFactory* GetPaperSpriteVertexFactory() const;

protected:
	TArray<FPaperSpriteVertex> BatchedVertices;
	TArray<FSpriteRenderSection> BatchedSections;

	TArray<FSpriteDrawCallRecord> BatchedSprites;
	class UMaterialInterface* Material;
	FVector Origin;
	AActor* Owner;

	// The color to draw as in wireframe mode
	FLinearColor WireframeColor;

	// The view relevance for the associated material
	FMaterialRelevance MaterialRelevance;

	// The Collision Response of the component being proxied
	FCollisionResponseContainer CollisionResponse;
};
