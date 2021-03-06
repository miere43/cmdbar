#pragma once
#include "common.h"


/** Contains various utilities for window management. */
namespace WindowManagement
{
    const UINT StopWindowAnimationMessageId = WM_USER + 123;

    struct WindowAnimationProperties
    {
        double animationDuration = 0.05;
        bool allowToStopAnimation = false;
        int stopAnimationMessageFilter = 0;

        /** Use current window alpha value to modify animation duration. */
        bool adjustAnimationDuration = false;

        BYTE startAlpha = 0; // Alpha at the start of animation.
        BYTE endAlpha = 255; // Alpha at the end of animation.
        //BYTE targetShowAlpha
    };

    /**
     * Creates new message loop and does specified window animation.
     *
     * If animation was stopped by a stop message, returns false, true otherwise.
     **/
    bool AnimateWindow(HWND hwnd, const WindowAnimationProperties& properties = WindowAnimationProperties());

    /** Sends stop animation message to specified window. */
    void SendStopAnimationMessage(HWND hwnd, int filter);
}
