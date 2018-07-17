#pragma once
#include "common.h"


/** Contains various utilities for window management. */
namespace WindowManagement
{
    /** Represents animations that can be played using AnimateWindow method. */
    enum class WindowAnimation
    {
        /** Fade in from zero opacity to full opacity. */
        Show = 1,

        /** Fade out from full opacity to zero opacity. */
        Hide = 2,
    };

    /** Creates new message loop and does specified window animation. */
    void AnimateWindow(HWND hwnd, WindowAnimation animation);
}
