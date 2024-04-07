//
// Created by konek on 3/4/2024.
//

#include "SkiaRenderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>

#include "skia_app.h"

#include <tools/window/WindowContext.h>
#include <tools/window/android/WindowContextFactory_android.h>

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

void SkiaRenderer::create() {
    SkDebugf("SkiaRenderer::create()");
    running = true;
    render_thread = std::thread([&]() {
        SkDebugf("[ PENDING  ] Skia Renderer Startup");
        skwindow::DisplayParams dp;
        dp.fColorSpace = SkColorSpace::MakeSRGBLinear();
        auto skia_window = skwindow::MakeGLForAndroid(app_->window, dp);
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

        while (running) {

            // Check to see if the surface has changed size. This is _necessary_ to do every frame when
            // using immersive mode as you'll get no other notification that your renderable area has
            // changed.

            EGLint width = ANativeWindow_getWidth(app_->window);
            EGLint height = ANativeWindow_getHeight(app_->window);

            if (width != width_ || height != height_) {
                width_ = width;
                height_ = height;
                SkDebugf("[ PENDING  ] Skia Renderer Resize");
                skia_window->resize(width, height);
                if (!skia_window->isValid()) {
                    SkDebugf("[ FAILED   ] Skia Renderer Resize");
                    std::this_thread::sleep_for(std::chrono::milliseconds(16));
                    continue;
                }
                SkDebugf("[ COMPLETE ] Skia Renderer Resize");
            }

            if (!skia_window->isValid()) {
                SkDebugf("[ PENDING  ] Skia Renderer Resize");
                skia_window->resize(width, height);
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
}

void SkiaRenderer::handleInput() {
    SkDebugf("[ PENDING  ] Skia Renderer Input");
    // handle all queued inputs
    auto *inputBuffer = android_app_swap_input_buffers(app_);
    if (!inputBuffer) {
        SkDebugf("[ COMPLETE ] Skia Renderer Input");
        // no inputs yet.
        return;
    }

    // handle motion events (motionEventsCounts can be 0).
    for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
        auto &motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;

        // Find the pointer index, mask and bitshift to turn it into a readable value.
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        auto &pointer = motionEvent.pointers[pointerIndex];
        auto x = GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);

        // determine the action type and process the event accordingly.
        switch (action & AMOTION_EVENT_ACTION_MASK) {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                break;

            case AMOTION_EVENT_ACTION_CANCEL:
                // treat the CANCEL as an UP event: doing nothing in the app, except
                // removing the pointer from the cache if pointers are locally saved.
                // code pass through on purpose.
            case AMOTION_EVENT_ACTION_UP:
            case AMOTION_EVENT_ACTION_POINTER_UP:
                break;

            case AMOTION_EVENT_ACTION_MOVE:
                // There is no pointer index for ACTION_MOVE, only a snapshot of
                // all active pointers; app needs to cache previous active pointers
                // to figure out which ones are actually moved.
                for (auto index = 0; index < motionEvent.pointerCount; index++) {
                    pointer = motionEvent.pointers[index];
                    x = GameActivityPointerAxes_getX(&pointer);
                    y = GameActivityPointerAxes_getY(&pointer);
                }
                break;
            default:
                break;
        }
    }
    // clear the motion input count in this buffer for main thread to re-use.
    android_app_clear_motion_events(inputBuffer);

    // handle input key events.
    for (auto i = 0; i < inputBuffer->keyEventsCount; i++) {
        auto &keyEvent = inputBuffer->keyEvents[i];
        // aout << "Key: " << keyEvent.keyCode << " ";
        switch (keyEvent.action) {
            case AKEY_EVENT_ACTION_DOWN:
                break;
            case AKEY_EVENT_ACTION_UP:
                break;
            case AKEY_EVENT_ACTION_MULTIPLE:
                // Deprecated since Android API level 29.
                break;
            default:
                break;
        }
    }
    // clear the key input count too.
    android_app_clear_key_events(inputBuffer);
    SkDebugf("[ COMPLETE ] Skia Renderer Input");
}

void SkiaRenderer::destroy() {
    // A thread that has finished executing code, but has not yet been
    //   joined is still considered an active thread of execution and is
    //   therefore joinable.
    // join() throws invalid_argument if joinable() is false.
    if (render_thread.joinable()) {
        running = false;
        render_thread.join();
    }
}

SkiaRenderer::~SkiaRenderer() {
    destroy();
}
