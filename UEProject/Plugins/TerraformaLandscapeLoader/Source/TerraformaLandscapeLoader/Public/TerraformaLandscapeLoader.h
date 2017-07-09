// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

/** 
 * A sound module file
 */

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Serialization/BulkData.h"
#include "TerraformaLandscapeLoader.generated.h"


UCLASS(hidecategories=Object, MinimalAPI, BlueprintType)
class UTerraformaLandscapeLoader : public UObject
{
	GENERATED_UCLASS_BODY()

	TArray<uint8>			TFAFileData;

private:

public:	
	//~ Begin UObject Interface. 
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface. 


};

