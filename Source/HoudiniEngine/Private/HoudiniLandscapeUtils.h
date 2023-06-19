/*
* Copyright (c) <2023> Side Effects Software Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. The name of Side Effects Software may not be used to endorse or
*    promote products derived from this software without specific prior
*    written permission.
*
* THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE "AS IS" AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
* NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

// This file contains utility functions for translating Houdini Height Fields to Unreal Landscapes.
// Do not include this header in other headers; the code is separated out to speed compilation times.

#include "HoudiniLandscapeTranslator.h"
#include "LandscapeInfo.h"
#include "HAPI/HAPI_Common.h"
#include "UObject/Class.h"

class UHoudiniAssetComponent;
class UHoudiniLandscapeTargetLayerOutput;
class UHoudiniOutput;
class ALandscape;
struct FHoudiniExtents;
class FName;
struct FLandscapeLayer;
struct FHoudiniGeoPartObject;
struct FHoudiniVolumeInfo;
struct FHoudiniPackageParams;
struct FHoudiniEngineOutputStats;
struct FHoudiniEngineOutputStats;
class UMaterialInstance;

struct FHoudiniMinMax
{
    float MinValue = TNumericLimits<float>::Max();
    float MaxValue = TNumericLimits<float>::Min();

    void Add(float Value)
    {
        MinValue = FMath::Min(MinValue, Value);
        MaxValue = FMath::Max(MaxValue, Value);
    }

    float Diff() const
    {
        return MaxValue - MinValue;
    }

    bool IsValid() const
    {
        return MinValue <= MaxValue;
    }
};

struct FHoudiniLandscapeMaterial
{
    FString Material;
    bool bCreateMaterialInstance = false;
    FString HoleMaterial;
    FString PhysicalMaterial;
};

struct FHoudiniHeightFieldData
{
    FIntPoint Dimensions;
    FTransform Transform;
    TArray<float> Values;

    int GetNumPoints() const { return Dimensions.X * Dimensions.Y; }

};

struct FHoudiniTileInfo
{
	FIntPoint TileStart; // Position of this tile
    FIntPoint LandscapeDimensions; // Dimensions of the entire landscape.
};

struct FHoudiniHeightFieldPartData
{
    int ObjectId = 0;
	int GeoId = 0;
	int PartId = 0;

    // The Edit Layer name to use (Empty if applied to a landscape without edit layers)
    FString UnrealLayerName;

    // Target layer, eg. "height" or a layer associated with a material.
    FString TargetLayerName;

    // Houdini Height Field Object
    const FHoudiniGeoPartObject* HeightField;

    // HAPI_UNREAL_LANDSCAPE_EDITLAYER_TYPE_BASE or HAPI_UNREAL_LANDSCAPE_EDITLAYER_TYPE_ADDITIVE
    int EditLayerType;

    // Should the layer be cleared
    bool bClearLayer;

    // Help place new Edit Layers in order.
    FString AfterLayerName;

    // Name of the target landscape.
    FString TargetLandscapeName;

    // Whether to update or create an existing landscape.
    bool bCreateNewLandscape;

    // Will be a subtractive layer.
    bool bSubtractiveEditLayer;

    // Will be a weight blended landscape.
    bool bIsWeightBlended;

    // Materials to assign.
	FHoudiniLandscapeMaterial Materials;

    // Optional Height Range to use
    TOptional<FHoudiniMinMax> HeightRange;

    // Whether to treast data as zero to one.
    bool bIsUnitData;

    // Actual data of the height field, fetch from Houdini.
    TUniquePtr<FHoudiniHeightFieldData> CachedData;

    // Houdini Tile Dimensions.
    TOptional<FHoudiniTileInfo> TileInfo;

    // Existing Layer Info Name
    FString LayerInfoObjectName;

    // Bake Outliner Folder
    FString BakeOutlinerFolder;

    // Size info - number of sections and components.
    FHoudiniLandscapeCreationInfo SizeInfo;

    // Any material instance created.
	UMaterialInterface* MaterialInstance = nullptr;


};

struct FHoudiniUnrealLandscapeTarget
{
    // Pointer to the landscape to where the layers will be cooked. 
	TWeakObjectPtr<ALandscapeProxy> Proxy;

    // If true, the landscape above was a new one, if not it was an existing landscape
    bool bWasCreated = false;

    // Name of the landscape when it it is baked.
    FName BakedName;

    // Dimensions of landscape when created.
    FIntPoint Dimensions;

    // Any created layer info assets.
    TArray<ULandscapeLayerInfoObject*> CreatedLayerInfoObjects;
};

struct FHoudiniLayersToUnrealLandscapeMapping
{
    // All the target landscapes for this output.
    TArray<FHoudiniUnrealLandscapeTarget> TargetLandscapes;

    // Packages created during landscape creation.
    TArray<UPackage*> CreatedPackages;

    // Maps the layer to an entry in TargetLandscapes.
    TMap<FHoudiniHeightFieldPartData *,int> HoudiniLayerToUnrealLandscape;
};



struct HOUDINIENGINE_API FHoudiniLandscapeUtils
{
	
    static TSet<FString> GetCookedLandscapeLayers(UHoudiniAssetComponent& HAC, ALandscape& Landscape);

    static TSet<UHoudiniLandscapeTargetLayerOutput *> GetEditLayers(UHoudiniOutput& Output);

    static void SetNonCookedLayersVisibility(UHoudiniAssetComponent& HAC, ALandscape& Landscape,  bool bVisible);

    static void SetCookedLayersVisibility(UHoudiniAssetComponent& HAC, ALandscape& Landscape, bool bVisible);
    
	static void RealignHeightFieldData(TArray<float>& Data, float ZeroPoint, float Scale);

    static bool ClampHeightFieldData(TArray<float>& Data, float MinValue, float MaxValue);

	static TArray<uint16> QuantizeNormalizedDataTo16Bit(const TArray<float>& Data);

    static float GetLandscapeHeightRangeInCM(ALandscape& Landscape);

    static TArray<uint16> GetHeightData(ALandscape* Landscape, const FHoudiniExtents& Extents, FLandscapeLayer* EditLayer);

    static FLandscapeLayer* GetEditLayerForReading(ALandscape* Landscape, const FName& LayerName);

    static FLandscapeLayer* GetEditLayerForWriting(ALandscape* Landscape, const FName& LayerName, bool bCreateIfMissing = true);

    static FLandscapeLayer* MoveEditLayerAfter(ALandscape* Landscape, const FName& LayerName, const FName& AfterLayerName);

	static TArray<uint8_t> GetLayerData(ALandscape* Landscape, const FHoudiniExtents& Extents, const FName& EditLayerName, const FName& TargetLayerName);

    static FHoudiniLayersToUnrealLandscapeMapping ResolveLandscapes(const FString & CookedLandscapePrefix, 
			const FHoudiniPackageParams& PackageParams, 
            UHoudiniAssetComponent* HAC, 
            TMap<FString,ALandscape*>& LandsscapeMap, 
            TArray<FHoudiniHeightFieldPartData>& Parts, 
            UWorld* World, 
            const TArray<ALandscapeProxy*>& LandscapeInputs);

    static bool CalcLandscapeSizeFromHeightFieldSize(const int32 ProposedUnrealSizeX, const int32 ProposedUnrealSizeY, FHoudiniLandscapeCreationInfo& Info);

    static void CreateDefaultHeightField(ALandscape* LandscapeActor, const FHoudiniLandscapeCreationInfo& Info);

    static ALandscapeProxy* FindTargetLandscapeProxy(const FString& ActorName, UWorld* World, const TArray<ALandscapeProxy*>& LandscapeInputs);

    static FHoudiniHeightFieldPartData* GetPartWithHeightData(TMap<FString, FHoudiniHeightFieldPartData*>& Parts);

    static FTransform GetHeightFieldTransformInUnrealSpace(const FHoudiniVolumeInfo& VolumeInfo);

    static UMaterialInterface* AssignGraphicsMaterialsToLandscape(
				ALandscapeProxy* LandscapeProxy, 
                FHoudiniLandscapeMaterial & Materials,
				const FHoudiniPackageParams& Params,
                TArray<UPackage*> & CreatedPacakages);

    static void AssignPhysicsMaterialsToLandscape(ALandscapeProxy* LandscapeProxy, const FString& LayerName, FHoudiniLandscapeMaterial& Materials);

    static TArray<ULandscapeLayerInfoObject*> CreateTargetLayerInfoAssets(
		ALandscapeProxy* LandscapeProxy, 
        const FHoudiniPackageParams& PackageParams, 
        TMap<FString, FHoudiniHeightFieldPartData*>& PartsForLandscape,
        TArray<UPackage*> & CreatedPackages);

    static ULandscapeLayerInfoObject* FindOrCreateLandscapeLayerInfoObject(const FString& InLayerName, const FString& InPackagePath, const FString& InPackageName, UPackage*& OutPackage);

    static void SetWorldPartitionGridSize(ALandscape* Landscape, int GridSize);

    static UPackage* FindOrCreate(const FString& PackageFullPath);

    static FHoudiniExtents GetExtents(const ALandscape* TargetLandscape, const FHoudiniHeightFieldData& HeightFieldData);

	static FHoudiniHeightFieldData FetchVolumeInUnrealSpace(const FHoudiniGeoPartObject& HeightField, bool bTransposeData);

    static FIntPoint GetVolumeDimensionsInUnrealSpace(const FHoudiniGeoPartObject& HeightField);

    static FHoudiniHeightFieldData ReDimensionLandscape(const FHoudiniHeightFieldData& HeightField, FIntPoint NewDimensions);
    
    static FHoudiniMinMax GetHeightFieldRange(const FHoudiniHeightFieldData& HeightField);

    static float GetAbsRange(const FHoudiniMinMax& Range, float MaxUsableRange);

    static void AdjustLandscapeTransformToLayerHeight(ALandscape& TargetLandscape , const FHoudiniHeightFieldPartData& LayerData, const FHoudiniHeightFieldData & HeightFieldData);

    static TSet<FString> GetNonWeightBlendedLayerNames(const FHoudiniGeoPartObject& InHGPO);

    static FTransform GetLandscapeActorTransformFromTileTransform(const FTransform& Transform, const FHoudiniTileInfo& TileInfo);

    static ULandscapeLayerInfoObject* GetLandscapeLayerInfoForLayer(const FHoudiniGeoPartObject& Part, const FName& InLayerName);

    static bool GetOutputMode(int GeoId, int PartId, HAPI_AttributeOwner Owner, int& LandscapeOutputMode);

    static UMaterialInterface* CreateMaterialInstance(const FString & Prefix, UMaterialInterface * Material, const FHoudiniPackageParams & Params, TArray<UPackage*>& CreatedPackages);

    static void ApplyMaterialsFromParts(
			ALandscapeProxy * LandscapeProxy, 
            TMap<FString, FHoudiniHeightFieldPartData*> & LayerParts, 
            const FHoudiniPackageParams& PackageParams,
            TArray<UPackage *> & CreatedPackages);

};
