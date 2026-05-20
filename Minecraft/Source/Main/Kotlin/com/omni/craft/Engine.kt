package com.omni.craft

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.Log
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class Engine(private val context: Context) : GLSurfaceView.Renderer {

    companion object {
        private const val TAG = "OmniCraft_Engine"

        init {
            System.loadLibrary("omnicraft")
        }

        @JvmStatic external fun nativeInit(width: Int, height: Int, seed: Long)
        @JvmStatic external fun nativeResize(width: Int, height: Int)
        @JvmStatic external fun nativeFrame(dt: Float)
        @JvmStatic external fun nativeJoystick(x: Float, y: Float)
        @JvmStatic external fun nativeCameraInput(dx: Float, dy: Float)
        @JvmStatic external fun nativeTap(type: Int)
        @JvmStatic external fun nativeJump()
        @JvmStatic external fun nativeSneak(on: Boolean)
        @JvmStatic external fun nativeSprint(on: Boolean)
        @JvmStatic external fun nativeSelectSlot(slot: Int)
        @JvmStatic external fun nativeSendChat(msg: String)
        @JvmStatic external fun nativeGetChatLog(): String
        @JvmStatic external fun nativeGetPlayerInfo(): String
        @JvmStatic external fun nativeGetInventory(): String
        @JvmStatic external fun nativeSetSensitivity(sens: Float)
        @JvmStatic external fun nativeToggleFly()
        @JvmStatic external fun nativeFlyUp(on: Boolean)
        @JvmStatic external fun nativeFlyDown(on: Boolean)
        @JvmStatic external fun nativeStartBreak()
        @JvmStatic external fun nativeStopBreak()
        @JvmStatic external fun nativeBreakContinue(dt: Float)
        @JvmStatic external fun nativeCraftItem(grid: IntArray)
        @JvmStatic external fun nativeDestroy()
        @JvmStatic external fun nativeIsInitialized(): Boolean
    }

    private var lastFrameTime = 0L
    private var width  = 0
    private var height = 0
    var seed: Long = System.currentTimeMillis()

    private var breakHeld    = false
    private var jumpHeld     = false
    private var flyUpHeld    = false
    private var flyDownHeld  = false
    private var lastBreakTime = 0L

    override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
        Log.i(TAG, "Surface created")
        lastFrameTime = System.nanoTime()
    }

    override fun onSurfaceChanged(gl: GL10?, w: Int, h: Int) {
        Log.i(TAG, "Surface changed: ${w}x${h}")
        width = w; height = h
        if (!nativeIsInitialized()) {
            nativeInit(w, h, seed)
        } else {
            nativeResize(w, h)
        }
    }

    override fun onDrawFrame(gl: GL10?) {
        val now = System.nanoTime()
        val dt  = ((now - lastFrameTime) / 1_000_000_000.0).toFloat()
        lastFrameTime = now
        val clampedDt = dt.coerceIn(0.001f, 0.05f)

        if (breakHeld) {
            nativeBreakContinue(clampedDt)
        }

        nativeFrame(clampedDt)
    }

    fun onJoystickMove(x: Float, y: Float) {
        nativeJoystick(x, y)
    }

    fun onCameraDrag(dx: Float, dy: Float) {
        nativeCameraInput(dx, dy)
    }

    fun onBreakPress() {
        breakHeld = true
        nativeStartBreak()
    }

    fun onBreakRelease() {
        breakHeld = false
        nativeStopBreak()
    }

    fun onPlace() {
        nativeTap(1)
    }

    fun onJumpPress() {
        nativeJump()
    }

    fun onJumpRelease() {
        jumpHeld = false
    }

    fun onSneak(on: Boolean)   { nativeSneak(on) }
    fun onSprint(on: Boolean)  { nativeSprint(on) }
    fun onSelectSlot(s: Int)   { nativeSelectSlot(s) }
    fun onSendChat(msg: String){ nativeSendChat(msg) }
    fun onSetSensitivity(s: Float) { nativeSetSensitivity(s) }
    fun onToggleFly() { nativeToggleFly() }
    fun onFlyUp(on: Boolean)   { nativeFlyUp(on) }
    fun onFlyDown(on: Boolean) { nativeFlyDown(on) }

    fun getChatLog()     = nativeGetChatLog()
    fun getPlayerInfo()  = nativeGetPlayerInfo()
    fun getInventory()   = nativeGetInventory()

    fun craftItem(grid: IntArray) { nativeCraftItem(grid) }

    fun destroy() {
        if (nativeIsInitialized()) nativeDestroy()
    }
}
