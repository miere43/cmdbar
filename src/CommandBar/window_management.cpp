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

        BYTE currAlpha = 0;
        GetLayeredWindowAttributes(hwnd, nullptr, &currAlpha, nullptr);

        BYTE startAlpha = properties.startAlpha;
        BYTE endAlpha = properties.endAlpha;

        double animDuration = properties.animationDuration;
        if (properties.adjustAnimationDuration)
        {
            BYTE alphaStartDiff = startAlpha > currAlpha
                ? startAlpha - currAlpha
                : currAlpha - startAlpha;
            BYTE alphaEndDiff = endAlpha > currAlpha
                ? endAlpha - currAlpha
                : currAlpha - endAlpha;
            double unadjusted = animDuration;
            BYTE alphaDiff = endAlpha > startAlpha
                ? endAlpha - startAlpha
                : startAlpha - endAlpha;
            double progressCoeff = 1.0 - (double)alphaEndDiff / alphaDiff;
            animDuration -= animDuration * progressCoeff;
            startAlpha = currAlpha;
        }
        
        double currSecs = clockCurrTick / (double)clockFrequency;
        double targetSecs = currSecs + animDuration;
        BYTE alphaDiff = endAlpha > startAlpha
            ? endAlpha - startAlpha
            : startAlpha - endAlpha;

        MSG msg;
        while (currSecs < targetSecs)
        {
            while (PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE) != 0)
            {
                if (msg.message == StopWindowAnimationMessageId && properties.allowToStopAnimation && (int)msg.lParam == properties.stopAnimationMessageFilter)
                    return false;
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }

            QueryPerformanceCounter((LARGE_INTEGER*)&clockCurrTick);
            currSecs = clockCurrTick / (double)clockFrequency;

            if (currSecs >= targetSecs)
            {
                debug("End - set alpha to: %d", endAlpha);
                SetLayeredWindowAttributes(hwnd, 0, endAlpha, LWA_ALPHA);
                break;
            }
            else
            {
                double diff = ((targetSecs - currSecs) / animDuration);
                BYTE alpha;
                if (endAlpha > startAlpha)
                {
                    diff = 1.0 - diff; 
                    alpha = startAlpha + (BYTE)(diff * alphaDiff);
                }
                else
                {
                    alpha = endAlpha + (BYTE)(diff * alphaDiff);
                }

                if (alpha != currAlpha)
                {
                    currAlpha = alpha;
                    //debug("Current alpha: %d  diff: %g", alpha, diff);
                    SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
                }
            }

            Sleep(1);
        }
        return true;
    }
    
    void SendStopAnimationMessage(HWND hwnd, int filter)
    {
        // Must be PostMessage, doesn't work with SendMessage.
        PostMessageW(hwnd, StopWindowAnimationMessageId, 0, filter);
    }
}
