#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "settings_W_C.generated.h"

UCLASS()
class SOMEASSEMBLYREQUIRED_API Usettings_W_C : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void setGlobalUIScale(float NewScale);
};
