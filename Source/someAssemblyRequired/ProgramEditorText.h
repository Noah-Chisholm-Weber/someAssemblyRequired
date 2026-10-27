#pragma once

#include "CoreMinimal.h"
#include "Components/MultiLineEditableText.h"
#include "ProgramEditorText.generated.h"

UENUM(BlueprintType)
enum class EProgramLineStatus : uint8
{
    None,
    Selected,
    Error
};

UCLASS(meta = (DisplayName = "Program Editor Text"))
class SOMEASSEMBLYREQUIRED_API UProgramEditorText
    : public UMultiLineEditableText
{
    GENERATED_BODY()

public:

    // LineNumber is 1-based.
    UFUNCTION(BlueprintCallable, Category = "Line Highlighting")
    void SetLineStatus(int32 LineNumber, EProgramLineStatus Status);

    EProgramLineStatus GetLineStatus(int32 LineNumber) const;

protected:

    virtual TSharedRef<SWidget> RebuildWidget() override;

private:

    TMap<int32, EProgramLineStatus> LineStatuses;
};