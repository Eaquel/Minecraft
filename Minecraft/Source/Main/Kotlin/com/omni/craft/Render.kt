package com.omni.craft

import android.opengl.GLSurfaceView
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class Render(private val engine: Engine) : GLSurfaceView.Renderer {
    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        engine.initializeEngine()
    }
    override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
        engine.resizeViewport(width, height)
    }
    override fun onDrawFrame(gl: GL10?) {
        engine.stepFrame()
    }
}
