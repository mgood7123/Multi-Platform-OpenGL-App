//
// Created by konek on 3/4/2024.
//

#include <jni.h>

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

#include "SkiaRenderer.h"

extern "C" {

    #include <game-activity/native_app_glue/android_native_app_glue.c>

    /*!
     * Handles commands sent to this Android application
     * @param pApp the app the commands are coming from
     * @param cmd the command to handle
     */
    void handle_cmd(android_app *pApp, int32_t cmd) {
        switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                // A new window is created, associate a renderer with it. You may replace this with a
                // "game" class if that suits your needs. Remember to change all instances of userData
                // if you change the class here as a reinterpret_cast is dangerous this in the
                // android_main function and the APP_CMD_TERM_WINDOW handler case.
                pApp->userData = new SkiaRenderer(pApp);
                reinterpret_cast<SkiaRenderer *>(pApp->userData)->create();
                break;
            case APP_CMD_TERM_WINDOW:
                // The window is being destroyed. Use this to clean up your userData to avoid leaking
                // resources.
                //
                // We have to check if userData is assigned just in case this comes in really quickly
                if (pApp->userData) {
                    auto *pRenderer = reinterpret_cast<SkiaRenderer *>(pApp->userData);
                    pApp->userData = nullptr;
                    pRenderer->destroy();
                    delete pRenderer;
                }
                break;
            default:
                break;
        }
    }


    /*!
     * Enable the motion events you want to handle; not handled events are
     * passed back to OS for further processing.
     *
     * @param motionEvent the newly arrived GameActivityMotionEvent.
     * @return true if the event is from a pointer,
     *         false for all other input devices.
     */
    bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
        auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
        return (sourceClass == AINPUT_SOURCE_CLASS_POINTER);
    }

    /*!
     * This the main entry point for a native activity
     */
    void android_main(struct android_app *pApp) {

        pApp->onAppCmd = handle_cmd;

        android_app_set_motion_event_filter(pApp, motion_event_filter_func);

        int events;
        android_poll_source *pSource;
        do {
            // Process all pending events before running game logic.
            if (ALooper_pollAll(-1, nullptr, &events, (void **) &pSource) >= 0) {
                if (pSource) {
                    pSource->process(pApp, pSource);
                }
            }
            if (pApp->userData) {
                reinterpret_cast<SkiaRenderer *>(pApp->userData)->handleInput();
            }
        } while (!pApp->destroyRequested);
    }
}