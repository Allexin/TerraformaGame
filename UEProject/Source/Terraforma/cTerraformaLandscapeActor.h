// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Landscape/cTerraformaLandscapeComponent.h"
#include "Zones/cTerraformaZoneComponent.h"
#include "cTerraformaLandscapeActor.generated.h"

UCLASS()
class TERRAFORMA_API AcTerraformaLandscapeActor : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Landscape")
	UcTerraformaLandscapeComponent*	LandscapeComponent;

	UPROPERTY(EditAnywhere, Category = "Terraforming")
		UcTerraformaZoneComponent*	ZoneComponent;
	
public:	
	// Sets default values for this actor's properties
	AcTerraformaLandscapeActor();

	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	
};
