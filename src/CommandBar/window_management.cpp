#include "window_management.h"
#include <stdio.h>

namespace WindowManagement
{
    void debug(const char* format, ...)
    {
        char buf[2048];
        va_list args;
        va_start(args, format);
        int nchars = vsprintf_s(buf, format, args);
        va_end(args);
        buf[nchars] = '\n';
        buf[nchars+1] = '\0';
        OutputDebugStringA(buf);
    }

    // 0 -> 255.
    // 

    bool AnimateWindow(HWND hwnd, const WindowAnimationProperties& properties)
    {
        uint64_t clockFrequency = 0;
        uint64_t clockCurrTick = 0;
        QueryPerformanceFrequency((LARGE_INTEGER*)&clockFrequency);
        QueryPerformanceCounter((LARGE_INTEGER*)&clockCurrTick);

        double animDuration = properties.animationDuration;
        double currSecs = clockCurrTick / (double)clockFrequency;
        double targetSecs = currSecs + animDuration;
        BYTE alphaDiff = properties.endAlpha > properties.startAlpha
            ? properties.endAlpha - properties.startAlpha
            : properties.startAlpha - properties.endAlpha;
        BYTE prevAlpha = 0;
        GetLayeredWindowAttributes(hwnd, nullptr, &prevAlpha, nullptr);

        MSG msg;
        while (currSecs < targetSecs)
        {
            while (PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE) != 0)
            {
                if (msg.message == StopWindowAnimationMessageId && properties.allowToStopAnimation)
                    return false;
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }

            QueryPerformanceCounter((LARGE_INTEGER*)&clockCurrTick);
            currSecs = clockCurrTick / (double)clockFrequency;

            if (currSecs >= targetSecs)
            {
                //debug("End - set alpha to: %d", endAlpha);
                SetLayeredWindowAttributes(hwnd, 0, properties.endAlpha, LWA_ALPHA);
                break;
            }
            else
            {
                double diff = ((targetSecs - currSecs) / animDuration);
                BYTE alpha;
                if (properties.endAlpha > properties.startAlpha)
                {
                    diff = 1.0 - diff; 
                    alpha = properties.startAlpha + (BYTE)(diff * alphaDiff);
                }
                else
                {
                    alpha = properties.endAlpha + (BYTE)(diff * alphaDiff);
                }

                if (alpha != prevAlpha)
                {
                    prevAlpha = alpha;
                    debug("Current alpha: %d  diff: %g", alpha, diff);
                    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
                }
            }

            Sleep(1);
        }
        return true;
    }
    
    void SendStopAnimationMessage(HWND hwnd)
    {
        // Must be PostMessage, doesn't work with SendMessage.
        PostMessageW(hwnd, StopWindowAnimationMessageId, 0, 0);
    }
}
