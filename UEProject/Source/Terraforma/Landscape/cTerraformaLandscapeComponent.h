// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "Camera/CameraActor.h"
#include "PhysicsEngine/BodySetup.h"
#include "DynamicMeshBuilder.h"
#include "cTerraformaGrid.h"
#include "cCameraGrid.h"
#include "TerraformaLandscapeLoader.h"
#include "cTerraformaMeshSceneProxy.h"
#include "cTerraformaLandscapeComponent.generated.h"

/**
 * 
 */







UCLASS()
class TERRAFORMA_API UcTerraformaLandscapeComponent : public UMeshComponent
{
	GENERATED_BODY()
	friend class FcTerraformaMeshSceneProxy;
protected:
	cTerraformaGrid						m_Landscape;
	cCameraGrid							m_CameraGrid;
	
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*>	m_DynMaterialInstances;
	UPROPERTY()
	TArray<UTexture2D*>	m_HeightmapTextures;
	UPROPERTY()
	TArray<UTexture2D*>	m_NormalmapTextures;
	UPROPERTY()
	TArray<UTexture2D*>	m_ColorTextures;

	bool m_NeedInvalidate;

	int MaterialsReadyCount;

	void ReinitMaterials();
	void loadLevel();
public:
	//UPROPERTY(EditAnywhere, Category = "Landscape")
	//	FName VMPWorld;
	UPROPERTY(EditAnywhere, Category = "Landscape")
		UTerraformaLandscapeLoader* TerraformaWorld;
	UPROPERTY(EditAnywhere, Category = "Landscape")
		bool Wireframe;
	/*Delay between "update graphics" calls*/
	UPROPERTY(EditAnywhere, Category = "Terraforming")
		float ApplyTerraformingDelay;
protected:
	float m_ApplyTerraformingCounter;
	bool m_Terraformed;
public:
	UcTerraformaLandscapeComponent();

	bool needInvalidate() { return m_NeedInvalidate; } 
	void resetInvalidateFlag() { m_NeedInvalidate = false; }

	/** Description of collision */
	UPROPERTY(BlueprintReadOnly, Category = "Collision")
		class UBodySetup* ModelBodySetup;

	// Begin UMeshComponent interface.
	virtual int32 GetNumMaterials() const override;
	// End UMeshComponent interface.

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual class UBodySetup* GetBodySetup() override;
	/**
	* Initializes the component.  Occurs at level startup. This is before BeginPlay (Actor or Component).
	* All Components in the level will be Initialized on load before any Actor/Component gets BeginPlay
	* Requires component to be registered, and bWantsInitializeComponent to be true.
	*/
	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	// End UPrimitiveComponent interface.
#if WITH_EDITOR  
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
private:
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;

};
