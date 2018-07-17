#include "window_management.h"


namespace WindowManagement
{
    void AnimateWindow(HWND hwnd, WindowAnimation animation)
    {
        uint64_t clockFrequency;
        uint64_t clockCurrTick;
        QueryPerformanceFrequency((LARGE_INTEGER*)&clockFrequency);
        QueryPerformanceCounter((LARGE_INTEGER*)&clockCurrTick);

        const double animDuration = 0.05; // @TODO: Move to WindowAnimationProperties struct.
        double currSecs = clockCurrTick / (double)clockFrequency;
        double targetSecs = currSecs + animDuration;

        MSG msg;
        while (currSecs < targetSecs)
        {
            while (PeekMessageW(&msg, hwnd, 0, 0, PM_REMOVE) != 0)
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }

            QueryPerformanceCounter((LARGE_INTEGER*)&clockCurrTick);
            currSecs = clockCurrTick / (double)clockFrequency;

            if (currSecs >= targetSecs)
            {
                SetLayeredWindowAttributes(hwnd, 0, animation == WindowAnimation::Show ? 255 : 0, LWA_ALPHA);
                break;
            }
            else
            {
                double diff = ((targetSecs - currSecs) * (1.0 / animDuration));
                if (animation == WindowAnimation::Show)
                    diff = 1.0 - diff;
                SetLayeredWindowAttributes(hwnd, 0, (BYTE)(diff * 255), LWA_ALPHA);
            }

            Sleep(1);
        }
    }
}
