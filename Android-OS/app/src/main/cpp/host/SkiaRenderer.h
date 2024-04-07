//
// Created by konek on 3/4/2024.
//

#ifndef ANDROID_OPENGL_SKIARENDERER_H
#define ANDROID_OPENGL_SKIARENDERER_H

#include <EGL/egl.h>
#include <memory>

#include <thread>
#include <condition_variable>
#include <mutex>

struct android_app;

class SkiaRenderer {
public:
    /*!
     * @param pApp the android_app this Renderer belongs to, needed to configure GL
     */
    inline SkiaRenderer(android_app *pApp) :
            app_(pApp),
            width_(0),
            height_(0)
    {
    }

    void create();
    void handleInput();
    void destroy();
    virtual ~SkiaRenderer();

private:

    std::thread render_thread;
    //std::mutex render_lock;
    //std::condition_variable render_cond;
    bool running = false;
    //bool initialized = false;

    android_app *app_;
    EGLint width_;
    EGLint height_;
};

#endif //ANDROID_OPENGL_SKIARENDERER_H
