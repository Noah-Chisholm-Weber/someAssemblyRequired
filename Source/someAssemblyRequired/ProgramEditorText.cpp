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
        if (Editor.IsValid())
        {
            const FSlateBrush* Brush =
                FCoreStyle::Get().GetBrush("WhiteBrush");

            for (const FTextLayout::FLineView& Line : LineViews)
            {
                // Convert Slate's 0-based index to 1-based.
                const int32 LineNumber = Line.ModelIndex + 1;

                const EProgramLineStatus Status =
                    Editor->GetLineStatus(LineNumber);

                if (Status == EProgramLineStatus::None)
                {
                    continue;
                }

                const FLinearColor Color =
                    Status == EProgramLineStatus::Error
                    ? FLinearColor(0.65f, 0.12f, 0.12f, 0.7f)
                    : FLinearColor(0.3f, 0.3f, 0.3f, 0.7f);

                const FVector2D Position(
                    0.0f,
                    Line.Offset.Y - ScrollOffset.Y
                );

                const FVector2D Size(
                    AllottedGeometry.GetLocalSize().X,
                    Line.Size.Y
                );

                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    LayerId,
                    AllottedGeometry.ToPaintGeometry(Position, Size),
                    Brush,
                    ESlateDrawEffect::None,
                    Color * InWidgetStyle.GetColorAndOpacityTint()
                );
            }
        }

        // Draw the actual text, cursor and selection above backgrounds.
        return FSlateTextLayout::OnPaint(
            Args,
            AllottedGeometry,
            MyCullingRect,
            OutDrawElements,
            LayerId + 1,
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