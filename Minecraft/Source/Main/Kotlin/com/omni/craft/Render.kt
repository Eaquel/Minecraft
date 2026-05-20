package com.omni.craft

import android.content.Context
import android.opengl.GLSurfaceView
import android.view.MotionEvent

class Render(context: Context, val engine: Engine) : GLSurfaceView(context) {

    init {
        setEGLContextClientVersion(3)
        setEGLConfigChooser(8, 8, 8, 8, 24, 0)
        setRenderer(engine)
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    var onTouchListener: ((MotionEvent) -> Boolean)? = null

    override fun onTouchEvent(event: MotionEvent): Boolean {
        return onTouchListener?.invoke(event) ?: super.onTouchEvent(event)
    }
}
