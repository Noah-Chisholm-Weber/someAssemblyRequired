#include "ProgramEditorText.h"

#include "Framework/Text/SlateTextLayout.h"
#include "Framework/Text/SlateTextLayoutFactory.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Text/SMultiLineEditableText.h"


// Custom text layout that paints line backgrounds.
class FProgramLineTextLayout : public FSlateTextLayout
{
public:

    FProgramLineTextLayout(
        SWidget* Owner,
        FTextBlockStyle Style,
        TWeakObjectPtr<UProgramEditorText> InEditor
    )
        : FSlateTextLayout(Owner, Style)
        , Editor(InEditor)
    {
    }

    virtual int32 OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled
    ) const override
    {
        const float InverseScale = 1.0f / AllottedGeometry.Scale;

        const FSlateBrush* Brush =
            FCoreStyle::Get().GetBrush("WhiteBrush");

        const FSlateFontInfo& NumberFont =
            GetDefaultTextStyle().Font;

        for (int32 i = 0; i < LineViews.Num(); ++i)
        {
            const FTextLayout::FLineView& Line = LineViews[i];

            const int32 LineNumber = Line.ModelIndex + 1;

            // Calculate the full distance occupied by this line.
            float RowHeight = Line.Size.Y;

            if (LineViews.IsValidIndex(i + 1))
            {
                RowHeight =
                    LineViews[i + 1].Offset.Y - Line.Offset.Y;
            }
            else if (LineViews.IsValidIndex(i - 1))
            {
                // Use the previous line's spacing for the final line.
                RowHeight =
                    Line.Offset.Y - LineViews[i - 1].Offset.Y;
            }

            // Convert the layout's pre-scaled coordinates
            // into local widget coordinates.
            const float Y = Line.Offset.Y * InverseScale;
            const float H = FMath::Max(
                1.0f,
                RowHeight * InverseScale
            );

            // --------------------------------------------------
            // Line background
            // --------------------------------------------------

            if (Editor.IsValid())
            {
                const EProgramLineStatus Status =
                    Editor->GetLineStatus(LineNumber);

                if (Status != EProgramLineStatus::None)
                {
                    const FLinearColor Color =
                        Status == EProgramLineStatus::Error
                        ? FLinearColor(0.147027, 0.004391, 0.005605, 1.000000)
                        : FLinearColor(0.002125, 0.030713, 0.038204, 1.000000);

                    FSlateDrawElement::MakeBox(
                        OutDrawElements,
                        LayerId,
                        AllottedGeometry.ToPaintGeometry(
                            FVector2D(0.0f, Y),
                            FVector2D(
                                AllottedGeometry.GetLocalSize().X,
                                H
                            )
                        ),
                        Brush,
                        ESlateDrawEffect::None,
                        Color * InWidgetStyle.GetColorAndOpacityTint()
                    );
                }
            }

            // --------------------------------------------------
            // Line number
            // --------------------------------------------------

            FSlateDrawElement::MakeText(
                OutDrawElements,
                LayerId + 1,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(8.0f, Y),
                    FVector2D(44.0f, H)
                ),
                FString::FromInt(LineNumber),
                NumberFont,
                ESlateDrawEffect::None,
                FLinearColor(0.55f, 0.55f, 0.55f, 1.0f)
            );
        }

        // Draw the editor text, cursor, and selection
        // above our backgrounds and line numbers.
        return FSlateTextLayout::OnPaint(
            Args,
            AllottedGeometry,
            MyCullingRect,
            OutDrawElements,
            LayerId + 2,
            InWidgetStyle,
            bParentEnabled
        );
    }

private:

    TWeakObjectPtr<UProgramEditorText> Editor;
};


// -------------------------------------------------------------
// UProgramEditorText
// -------------------------------------------------------------

TSharedRef<SWidget> UProgramEditorText::RebuildWidget()
{
    TWeakObjectPtr<UProgramEditorText> WeakEditor(this);

    MyMultiLineEditableText = SNew(SMultiLineEditableText)
        .TextStyle(&WidgetStyle)
        .AutoWrapText(false)
        .Margin(Margin)
        .AllowContextMenu(AllowContextMenu)
        .IsReadOnly(GetIsReadOnly())
        .SelectAllTextWhenFocused(GetSelectAllTextWhenFocused())
        .ClearTextSelectionOnFocusLoss(GetClearTextSelectionOnFocusLoss())
        .RevertTextOnEscape(GetRevertTextOnEscape())
        .ClearKeyboardFocusOnCommit(GetClearKeyboardFocusOnCommit())
        .VirtualKeyboardOptions(VirtualKeyboardOptions)
        .VirtualKeyboardDismissAction(VirtualKeyboardDismissAction)
        .OnTextChanged(
            BIND_UOBJECT_DELEGATE(FOnTextChanged, HandleOnTextChanged)
        )
        .OnTextCommitted(
            BIND_UOBJECT_DELEGATE(FOnTextCommitted, HandleOnTextCommitted)
        )
        .CreateSlateTextLayout(
            FCreateSlateTextLayout::CreateLambda(
                [WeakEditor](
                    SWidget* Owner,
                    const FTextBlockStyle& Style
                    ) -> TSharedRef<FSlateTextLayout>
                {
                    return MakeShared<FProgramLineTextLayout>(
                        Owner,
                        Style,
                        WeakEditor
                    );
                }
            )
        );

    return MyMultiLineEditableText.ToSharedRef();
}


void UProgramEditorText::SetLineStatus(
    int32 LineNumber,
    EProgramLineStatus Status
)
{
    if (LineNumber < 1)
    {
        return;
    }

    if (Status == EProgramLineStatus::None)
    {
        LineStatuses.Remove(LineNumber);
    }
    else
    {
        LineStatuses.Add(LineNumber, Status);
    }

    if (MyMultiLineEditableText.IsValid())
    {
        MyMultiLineEditableText->Invalidate(
            EInvalidateWidgetReason::Paint
        );
    }
}


EProgramLineStatus UProgramEditorText::GetLineStatus(
    int32 LineNumber
) const
{
    if (const EProgramLineStatus* Status =
        LineStatuses.Find(LineNumber))
    {
        return *Status;
    }

    return EProgramLineStatus::None;
}