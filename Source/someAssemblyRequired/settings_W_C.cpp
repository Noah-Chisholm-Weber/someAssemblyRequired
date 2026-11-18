#include "settings_W_C.h"
#include "Engine/UserInterfaceSettings.h"
#include "Engine/RendererSettings.h"

void Usettings_W_C::setGlobalUIScale(float NewScale)
{
    if (UUserInterfaceSettings* UISettings = GetMutableDefault<UUserInterfaceSettings>())
    {
        UISettings->ApplicationScale = NewScale;
        //UISettings->SaveConfig();
    }
}