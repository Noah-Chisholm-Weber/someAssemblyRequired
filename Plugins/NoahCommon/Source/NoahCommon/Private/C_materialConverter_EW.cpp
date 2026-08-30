//// Fill out your copyright notice in the Description page of Project Settings.
//
//
//#include "C_materialConverter_EW.h"
//#include "Serialization/ArchiveReplaceObjectRef.h"
//#include "AssetRegistry/AssetRegistryModule.h"
//#include "AssetRegistry/IAssetRegistry.h"
//#include "AssetRegistry/AssetData.h"
//#include "EditorAssetLibrary.h"
//
//bool UC_materialConverter_EW::deleteMaterial(UObject* OldObject, UObject* NewObject) {
//	if (!IsValid(OldObject) || !IsValid(NewObject))
//	{
//		return false;
//	}
//
//	if (OldObject == NewObject)
//	{
//		return false;
//	}
//
//	UPackage* OldPackage = OldObject->GetOutermost();
//	if (!OldPackage)
//	{
//		return false;
//	}
//
//	IAssetRegistry& AssetRegistry =
//		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
//			"AssetRegistry"
//		).Get();
//
//	// Find packages on disk that reference OldObject's package.
//	TArray<FName> ReferencerPackages;
//
//	AssetRegistry.GetReferencers(
//		OldPackage->GetFName(),
//		ReferencerPackages,
//		UE::AssetRegistry::EDependencyCategory::Package,
//		UE::AssetRegistry::FDependencyQuery()
//	);
//
//	// The actual replacement map used by the serialization archive.
//	TMap<UObject*, UObject*> ReplacementMap;
//	ReplacementMap.Add(OldObject, NewObject);
//
//	TArray<UObject*> ModifiedAssets;
//
//	for (const FName& ReferencerPackageName : ReferencerPackages)
//	{
//		TArray<FAssetData> AssetsInPackage;
//
//		AssetRegistry.GetAssetsByPackageName(
//			ReferencerPackageName,
//			AssetsInPackage,
//			false
//		);
//
//		for (const FAssetData& AssetData : AssetsInPackage)
//		{
//			UObject* ReferencerAsset = AssetData.GetAsset();
//
//			if (!IsValid(ReferencerAsset))
//			{
//				continue;
//			}
//
//			// Enables undo and tells the editor we're about to change it.
//			ReferencerAsset->Modify();
//
//			FArchiveReplaceObjectRef<UObject> ReplaceArchive(
//				ReferencerAsset,
//				ReplacementMap,
//				EArchiveReplaceObjectFlags::IgnoreOuterRef |
//				EArchiveReplaceObjectFlags::IgnoreArchetypeRef
//			);
//
//			ReferencerAsset->MarkPackageDirty();
//			ModifiedAssets.AddUnique(ReferencerAsset);
//		}
//	}
//
//	// Persist the changed references before removing OldObject.
//	if (ModifiedAssets.Num() > 0)
//	{
//		if (!UEditorAssetLibrary::SaveLoadedAssets(
//			ModifiedAssets,
//			true
//		))
//		{
//			return false;
//		}
//	}
//
//	// Only after the replacement packages have been saved.
//	return UEditorAssetLibrary::DeleteLoadedAsset(OldObject);
//}