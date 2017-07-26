// Fill out your copyright notice in the Description page of Project Settings.

#include "cTerraformaLandscapeActor.h"



// Sets default values
AcTerraformaLandscapeActor::AcTerraformaLandscapeActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	LandscapeComponent = CreateDefaultSubobject<UcTerraformaLandscapeComponent>(TEXT("Landscape"));
	LandscapeComponent->SetupAttachment(RootComponent);

	ZoneComponent = CreateDefaultSubobject<UcTerraformaZoneComponent>(TEXT("Zone"));
	ZoneComponent->SetupAttachment(RootComponent);
	ZoneComponent->init(LandscapeComponent);
}

// Called when the game starts or when spawned
void AcTerraformaLandscapeActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AcTerraformaLandscapeActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);	
}

