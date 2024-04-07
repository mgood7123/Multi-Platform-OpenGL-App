#include <Windows.h>
#include <thread>

#include <gl/GL.h>

#include <tools/window/WindowContext.h>
#include <tools/window/win/WindowContextFactory_win.h>

// we get
//   no member named 'MakeGLForWin' in namespace 'skwindow'
// for some reason
// pre-declare it to shut up the compiler, it SHOULD exist in the build
//
namespace skwindow {
    std::unique_ptr<WindowContext> MakeGLForWin(HWND, const DisplayParams&);
}  // namespace skwindow

#include <include/gpu/gl/egl/GrGLMakeEGLInterface.h>
#include "src/gpu/ganesh/gl/GrGLDefines.h"
#include "src/gpu/ganesh/gl/GrGLUtil.h"
#include "include/gpu/GrTypes.h"
#include "include/gpu/GrDirectContext.h"
#include "src/gpu/ganesh/GrDirectContextPriv.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/GrContextOptions.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

#include <include/core/SkCanvas.h>
#include <include/core/SkSurface.h>

CHAR app_name[] = "Win OpenGL Skia";
HWND  window_handle = nullptr;

#define WIDTH           300 
#define HEIGHT          200 

LONG WINAPI MainWndProc(HWND, UINT, WPARAM, LPARAM);

int main() {
    HINSTANCE hInstance = nullptr;
    int nCmdShow = SW_SHOWNORMAL;

    MSG        msg;
    WNDCLASS   wndclass;

    /* Register the frame class */
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = (WNDPROC)MainWndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(hInstance, app_name);
    wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndclass.lpszMenuName = app_name;
    wndclass.lpszClassName = app_name;

    if (!RegisterClass(&wndclass))
        return FALSE;

    /* Create the frame */
    window_handle = CreateWindowExA(
        0L,
        app_name, app_name,
        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT,
        nullptr, nullptr,
        hInstance, nullptr
    );

    /* make sure window was created */
    if (!window_handle) return FALSE;

    /* show and update main window */
    ShowWindow(window_handle, nCmdShow);

    UpdateWindow(window_handle);

    while (1) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE) == TRUE)
        {
            if (GetMessage(&msg, nullptr, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                return TRUE;
            }
        }
    }
    return TRUE;
}

bool running = false;
std::thread render_thread;

/* main window procedure */
LONG WINAPI MainWndProc(
    HWND    hWnd,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{

    switch (uMsg) {

    case WM_CREATE: {
        SkDebugf("SkiaRenderer::create()");
        running = true;
        render_thread = std::thread([&]() {
            SkDebugf("[ PENDING  ] Skia Renderer Startup");
            skwindow::DisplayParams dp;
            dp.fColorSpace = SkColorSpace::MakeSRGBLinear();
            auto skia_window = skwindow::MakeGLForWin(hWnd, dp);
            if (!skia_window) {
                SkDebugf("[ FAILED   ] Skia Renderer Startup - failed to initialize skia window");
                return;
            }
            if (!skia_window->isValid()) {
                SkDebugf("[ FAILED   ] Skia Renderer Startup - skia window is invalid");
                skia_window.reset();
                return;
            }

            SkDebugf("[ COMPLETE ] Skia Renderer Startup");

            long width_ = -1;
            long height_ = -1;

            while (running) {

                // Check to see if the surface has changed size. This is _necessary_ to do every frame when
                // using immersive mode as you'll get no other notification that your renderable area has
                // changed.

                RECT rect;
                GetClientRect(hWnd, &rect);

                if (rect.right != width_ || rect.bottom != height_) {
                    width_ = rect.right;
                    height_ = rect.bottom;
                    SkDebugf("[ PENDING  ] Skia Renderer Resize");
                    skia_window->resize(width_, height_);
                    if (!skia_window->isValid()) {
                        SkDebugf("[ FAILED   ] Skia Renderer Resize");
                        std::this_thread::sleep_for(std::chrono::milliseconds(16));
                        continue;
                    }
                    SkDebugf("[ COMPLETE ] Skia Renderer Resize");
                }

                if (!skia_window->isValid()) {
                    SkDebugf("[ PENDING  ] Skia Renderer Resize");
                    skia_window->resize(width_, height_);
                    if (!skia_window->isValid()) {
                        SkDebugf("[ FAILED   ] Skia Renderer Resize");
                        std::this_thread::sleep_for(std::chrono::milliseconds(16));
                        continue;
                    }
                    SkDebugf("[ COMPLETE ] Skia Renderer Resize");
                }

                // clear the color buffer to solid black
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                sk_sp<SkSurface> surface = skia_window->getBackbufferSurface();
                SkCanvas* canvas = surface->getCanvas();
                canvas->clear(SkColors::kBlack);
                SkPaint paint;
                paint.setColor(SkColors::kBlue);
                paint.setStroke(true);
                paint.setStrokeWidth(1.0f);
                canvas->drawOval(SkRect::MakeXYWH(100, 100, 200, 200), paint);

                // see include/private/chromium/GrDeferredDisplayListRecorder.h
                // multi-threaded tile-drawing
                // is this better than a SkPicture ?

                skia_window->directContext()->flushAndSubmit(surface.get());
                skia_window->swapBuffers();
            }
            SkDebugf("[ PENDING  ] Skia Renderer Shutdown");
            skia_window.reset();
            SkDebugf("[ COMPLETE ] Skia Renderer Shutdown");
            });
        //std::unique_lock lk(render_lock);
        //render_cond.wait(lk, [&] { return initialized; });
        //lk.unlock();
        // GL has been initialized, it is safe to call destroy() after this

        return TRUE;
    }

    case WM_PAINT: {
        //drawScene();
        return TRUE;
    }

    case WM_SIZE: {
        //RECT rect;
        //GetClientRect(hWnd, &rect);
        //resize(rect.right, rect.bottom);
        //drawScene();
        return TRUE;
    }

    case WM_CLOSE:
        // A thread that has finished executing code, but has not yet been
        //   joined is still considered an active thread of execution and is
        //   therefore joinable.
        // join() throws invalid_argument if joinable() is false.
        if (render_thread.joinable()) {
            running = false;
            render_thread.join();
        }
        DestroyWindow(hWnd);
        return TRUE;

    case WM_DESTROY:
        // A thread that has finished executing code, but has not yet been
        //   joined is still considered an active thread of execution and is
        //   therefore joinable.
        // join() throws invalid_argument if joinable() is false.
        if (render_thread.joinable()) {
            running = false;
            render_thread.join();
        }
        PostQuitMessage(0);
        return TRUE;

    default:
        return (LONG)DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}
