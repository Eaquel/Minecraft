package com.omni.craft

import android.app.Activity
import android.graphics.Color
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.view.Gravity
import android.view.MotionEvent
import android.view.WindowInsets
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout

class Activity : Activity() {

    private lateinit var glView: GLSurfaceView
    private val engine = Engine()
    private var lastX = 0f
    private var lastY = 0f
    private var isLobby = true

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        val rootLayout = FrameLayout(this)
        
        glView = object : GLSurfaceView(this@Activity) {
            override fun onTouchEvent(event: MotionEvent): Boolean {
                if (isLobby) return false
                when (event.action) {
                    MotionEvent.ACTION_DOWN -> {
                        lastX = event.x
                        lastY = event.y
                    }
                    MotionEvent.ACTION_MOVE -> {
                        val dx = event.x - lastX
                        val dy = event.y - lastY
                        engine.look(dx, dy)
                        lastX = event.x
                        lastY = event.y
                    }
                    MotionEvent.ACTION_UP -> {
                        engine.interact()
                    }
                }
                return true
            }
        }
        glView.setEGLContextClientVersion(3)
        glView.setRenderer(Render(engine))
        rootLayout.addView(glView)

        val controlsLayout = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.BOTTOM or Gravity.START
            setPadding(50, 50, 50, 50)
        }

        val btnForward = Button(this).apply { text = "W"; setOnClickListener { engine.move(0) } }
        val btnBack = Button(this).apply { text = "S"; setOnClickListener { engine.move(1) } }
        val btnLeft = Button(this).apply { text = "A"; setOnClickListener { engine.move(2) } }
        val btnRight = Button(this).apply { text = "D"; setOnClickListener { engine.move(3) } }

        controlsLayout.addView(btnLeft)
        controlsLayout.addView(btnBack)
        controlsLayout.addView(btnForward)
        controlsLayout.addView(btnRight)

        val lobbyLayout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER
            setBackgroundColor(Color.parseColor("#88000000"))
        }

        val btnPlay = Button(this).apply {
            text = "Oyuna Gir (Dünya Yarat)"
            textSize = 24f
            setOnClickListener {
                lobbyLayout.visibility = android.view.View.GONE
                rootLayout.addView(controlsLayout)
                isLobby = false
            }
        }
        lobbyLayout.addView(btnPlay)
        rootLayout.addView(lobbyLayout)

        setContentView(rootLayout)
    }

    override fun onPause() {
        super.onPause()
        glView.onPause()
    }

    override fun onResume() {
        super.onResume()
        glView.onResume()
        window.decorView.windowInsetsController?.hide(
            WindowInsets.Type.statusBars() or WindowInsets.Type.navigationBars()
        )
    }

    override fun onDestroy() {
        super.onDestroy()
        engine.shutdownEngine()
    }
}
