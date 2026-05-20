package com.omni.craft

import android.annotation.SuppressLint
import android.app.Activity
import android.content.SharedPreferences
import android.graphics.Color
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.InputType
import android.util.Log
import android.view.*
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.*
import androidx.core.view.isVisible
import org.json.JSONObject
import org.json.JSONArray
import kotlin.math.*

@SuppressLint("ClickableViewAccessibility")
class Activity : Activity() {

    private lateinit var engine: Engine
    private lateinit var glView: Render
    private lateinit var prefs: SharedPreferences

    private lateinit var overlayLayout: FrameLayout
    private lateinit var joystickBase: View
    private lateinit var joystickKnob: View
    private lateinit var cameraArea: View
    private lateinit var btnJump: View
    private lateinit var btnBreak: View
    private lateinit var btnPlace: View
    private lateinit var btnSneak: View
    private lateinit var btnSprint: View
    private lateinit var btnFlyUp: View
    private lateinit var btnFlyDown: View
    private lateinit var btnInventory: View
    private lateinit var btnChat: View
    private lateinit var btnSettings: View
    private lateinit var hotbarLayout: LinearLayout
    private lateinit var chatContainer: LinearLayout
    private lateinit var chatMessages: TextView
    private lateinit var chatInput: EditText
    private lateinit var btnSendChat: Button
    private lateinit var inventoryPanel: LinearLayout
    private lateinit var settingsPanel: LinearLayout
    private lateinit var sensitivitySeekBar: SeekBar
    private lateinit var sensitivityLabel: TextView

    private var joystickPointerId = -1
    private var cameraPointerId   = -1
    private var joystickBaseX = 0f
    private var joystickBaseY = 0f
    private val joystickRadius   = 120f
    private val joystickKnobSize = 60f

    private val handler = Handler(Looper.getMainLooper())
    private val uiUpdateRunnable = object : Runnable {
        override fun run() {
            updateUI()
            handler.postDelayed(this, 100)
        }
    }

    private var hotbarSlots = Array<TextView>(9) { TextView(this) }
    private var selectedSlot = 0
    private var sensitivity = 0.3f

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN)
        window.decorView.systemUiVisibility = (
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            or View.SYSTEM_UI_FLAG_FULLSCREEN
        )

        prefs = getSharedPreferences("omnicraft_prefs", MODE_PRIVATE)
        sensitivity = prefs.getFloat("sensitivity", 0.3f)

        engine = Engine(this)
        engine.seed = prefs.getLong("world_seed", System.currentTimeMillis())
        glView = Render(this, engine)

        val root = FrameLayout(this)
        root.addView(glView, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))

        overlayLayout = FrameLayout(this)
        root.addView(overlayLayout, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))

        buildUI()
        setContentView(root)
    }

    private fun dp(v: Float) = (v * resources.displayMetrics.density).toInt()
    private fun dpf(v: Float) = v * resources.displayMetrics.density

    private fun buildUI() {
        val sw = resources.displayMetrics.widthPixels
        val sh = resources.displayMetrics.heightPixels

        joystickBase = View(this).apply {
            setBackgroundColor(Color.argb(60, 255, 255, 255))
            val r = dp(joystickRadius)
            clipToOutline = true
            background = resources.getDrawable(android.R.drawable.btn_default, null)
        }
        val jbSize = dp(joystickRadius * 2)
        val jbParams = FrameLayout.LayoutParams(jbSize, jbSize).apply {
            leftMargin = dp(40f); bottomMargin = dp(120f)
            gravity = Gravity.BOTTOM or Gravity.START
        }
        joystickBase.setBackgroundColor(Color.argb(50, 200, 200, 200))
        overlayLayout.addView(joystickBase, jbParams)

        joystickKnob = View(this).apply {
            setBackgroundColor(Color.argb(120, 255, 255, 255))
        }
        val jkSize = dp(joystickKnobSize * 2)
        val jkParams = FrameLayout.LayoutParams(jkSize, jkSize).apply {
            leftMargin = dp(40f) + jbSize / 2 - jkSize / 2
            bottomMargin = dp(120f) + jbSize / 2 - jkSize / 2
            gravity = Gravity.BOTTOM or Gravity.START
        }
        overlayLayout.addView(joystickKnob, jkParams)

        cameraArea = View(this).apply { setBackgroundColor(Color.TRANSPARENT) }
        val caParams = FrameLayout.LayoutParams(sw / 2, sh).apply {
            gravity = Gravity.END
        }
        overlayLayout.addView(cameraArea, caParams)

        btnJump = buildButton("⬆", Color.argb(150, 100, 180, 100), dp(70f), dp(70f)).apply {
            val p = FrameLayout.LayoutParams(dp(70f), dp(70f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(200f); rightMargin = dp(30f)
            }
            overlayLayout.addView(this, p)
        }

        btnBreak = buildButton("⛏", Color.argb(150, 200, 80, 80), dp(80f), dp(80f)).apply {
            val p = FrameLayout.LayoutParams(dp(80f), dp(80f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(120f); rightMargin = dp(120f)
            }
            overlayLayout.addView(this, p)
        }

        btnPlace = buildButton("🏗", Color.argb(150, 80, 80, 200), dp(80f), dp(80f)).apply {
            val p = FrameLayout.LayoutParams(dp(80f), dp(80f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(120f); rightMargin = dp(30f)
            }
            overlayLayout.addView(this, p)
        }

        btnSneak = buildButton("⬇", Color.argb(130, 180, 160, 80), dp(60f), dp(60f)).apply {
            val p = FrameLayout.LayoutParams(dp(60f), dp(60f)).apply {
                gravity = Gravity.BOTTOM or Gravity.START
                bottomMargin = dp(120f); leftMargin = dp(dp(40f) + jbSize + dp(10f).toFloat())
            }
            overlayLayout.addView(this, p)
        }

        btnSprint = buildButton("🏃", Color.argb(130, 180, 100, 180), dp(60f), dp(60f)).apply {
            val p = FrameLayout.LayoutParams(dp(60f), dp(60f)).apply {
                gravity = Gravity.BOTTOM or Gravity.START
                bottomMargin = dp(190f); leftMargin = dp(dp(40f) + jbSize + dp(10f).toFloat())
            }
            overlayLayout.addView(this, p)
        }

        btnFlyUp = buildButton("↑↑", Color.argb(130, 100, 200, 200), dp(55f), dp(55f)).apply {
            val p = FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(280f); rightMargin = dp(30f)
            }
            overlayLayout.addView(this, p)
        }

        btnFlyDown = buildButton("↓↓", Color.argb(130, 200, 200, 100), dp(55f), dp(55f)).apply {
            val p = FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(215f); rightMargin = dp(30f)
            }
            overlayLayout.addView(this, p)
        }

        btnInventory = buildButton("🎒", Color.argb(150, 150, 120, 80), dp(55f), dp(55f)).apply {
            val p = FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.TOP or Gravity.END
                topMargin = dp(10f); rightMargin = dp(75f)
            }
            overlayLayout.addView(this, p)
        }

        btnChat = buildButton("💬", Color.argb(150, 80, 130, 200), dp(55f), dp(55f)).apply {
            val p = FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.TOP or Gravity.END
                topMargin = dp(10f); rightMargin = dp(140f)
            }
            overlayLayout.addView(this, p)
        }

        btnSettings = buildButton("⚙", Color.argb(150, 130, 130, 130), dp(55f), dp(55f)).apply {
            val p = FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.TOP or Gravity.END
                topMargin = dp(10f); rightMargin = dp(10f)
            }
            overlayLayout.addView(this, p)
        }

        hotbarLayout = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            setBackgroundColor(Color.argb(0, 0, 0, 0))
        }
        val hbParams = FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(55f)).apply {
            gravity = Gravity.BOTTOM
            bottomMargin = dp(60f)
        }
        for (i in 0..8) {
            val slot = TextView(this).apply {
                setBackgroundColor(if (i == 0) Color.argb(180, 255, 255, 100) else Color.argb(100, 50, 50, 50))
                setTextColor(Color.WHITE)
                textSize = 10f
                gravity = Gravity.CENTER
                text = ""
                setPadding(2, 2, 2, 2)
            }
            hotbarSlots[i] = slot
            hotbarLayout.addView(slot, LinearLayout.LayoutParams(0, dp(55f), 1f))
        }
        overlayLayout.addView(hotbarLayout, hbParams)

        chatContainer = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(0, 0, 0, 0))
            isVisible = false
        }
        val chatParams = FrameLayout.LayoutParams(sw * 2 / 3, dp(250f)).apply {
            gravity = Gravity.BOTTOM or Gravity.START
            bottomMargin = dp(120f); leftMargin = dp(10f)
        }
        chatMessages = TextView(this).apply {
            setTextColor(Color.WHITE)
            textSize = 11f
            setBackgroundColor(Color.argb(120, 0, 0, 0))
            setPadding(dp(6f), dp(4f), dp(6f), dp(4f))
            text = ""
        }
        val chatInputRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        chatInput = EditText(this).apply {
            hint = "Message / /gamemode /tp /give..."
            setHintTextColor(Color.GRAY)
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.argb(180, 30, 30, 30))
            inputType = InputType.TYPE_CLASS_TEXT
            imeOptions = EditorInfo.IME_ACTION_SEND
            setPadding(dp(8f), dp(6f), dp(8f), dp(6f))
        }
        btnSendChat = Button(this).apply {
            text = "➤"
            setBackgroundColor(Color.argb(200, 50, 150, 50))
            setTextColor(Color.WHITE)
        }
        chatInputRow.addView(chatInput, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        chatInputRow.addView(btnSendChat, LinearLayout.LayoutParams(dp(50f), ViewGroup.LayoutParams.WRAP_CONTENT))
        chatContainer.addView(chatMessages, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        chatContainer.addView(chatInputRow, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))
        overlayLayout.addView(chatContainer, chatParams)

        inventoryPanel = buildInventoryPanel()
        val invParams = FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
            ViewGroup.LayoutParams.MATCH_PARENT)
        overlayLayout.addView(inventoryPanel, invParams)
        inventoryPanel.isVisible = false

        settingsPanel = buildSettingsPanel()
        overlayLayout.addView(settingsPanel, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        settingsPanel.isVisible = false

        wireListeners()
    }

    private fun buildButton(label: String, bgColor: Int, w: Int, h: Int): TextView {
        return TextView(this).apply {
            text = label
            textSize = 22f
            gravity = Gravity.CENTER
            setTextColor(Color.WHITE)
            setBackgroundColor(bgColor)
            layoutParams = FrameLayout.LayoutParams(w, h)
        }
    }

    private fun buildInventoryPanel(): LinearLayout {
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(220, 20, 20, 20))
            setPadding(dp(16f), dp(16f), dp(16f), dp(16f))
            gravity = Gravity.CENTER
        }
        val title = TextView(this).apply {
            text = "🎒 Inventory"
            textSize = 18f
            setTextColor(Color.WHITE)
            gravity = Gravity.CENTER
        }
        panel.addView(title)

        val grid = GridLayout(this).apply {
            columnCount = 9
            rowCount = 4
        }
        for (r in 0..3) for (c in 0..8) {
            val cell = TextView(this).apply {
                setBackgroundColor(Color.argb(150, 60, 60, 60))
                setTextColor(Color.WHITE)
                textSize = 9f
                gravity = Gravity.CENTER
                text = ""
                setPadding(4, 4, 4, 4)
            }
            val lp = GridLayout.LayoutParams().apply {
                width = dp(40f); height = dp(40f)
                setMargins(2, 2, 2, 2)
            }
            grid.addView(cell, lp)
        }
        panel.addView(grid)

        val craftTitle = TextView(this).apply {
            text = "⚒ Crafting (2×2 → result)"
            textSize = 14f
            setTextColor(Color.LTGRAY)
            gravity = Gravity.CENTER
            setPadding(0, dp(12f), 0, dp(4f))
        }
        panel.addView(craftTitle)

        val craftRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL; gravity = Gravity.CENTER }
        val craftGrid = GridLayout(this).apply { columnCount = 3; rowCount = 3 }
        for (i in 0..8) {
            val cell = EditText(this).apply {
                hint = "ID"
                setTextColor(Color.WHITE)
                setHintTextColor(Color.GRAY)
                setBackgroundColor(Color.argb(150, 80, 80, 80))
                textSize = 10f
                inputType = InputType.TYPE_CLASS_NUMBER
                gravity = Gravity.CENTER
                setPadding(2, 2, 2, 2)
            }
            craftGrid.addView(cell, GridLayout.LayoutParams().apply { width = dp(38f); height = dp(38f); setMargins(2, 2, 2, 2) })
        }
        val arrow = TextView(this).apply { text = "➤"; textSize = 20f; setTextColor(Color.WHITE); gravity = Gravity.CENTER; setPadding(dp(8f), 0, dp(8f), 0) }
        val resultSlot = TextView(this).apply {
            setBackgroundColor(Color.argb(180, 100, 160, 100))
            setTextColor(Color.WHITE)
            textSize = 12f
            gravity = Gravity.CENTER
            text = "?"
            tag = "craft_result"
        }
        val craftBtn = Button(this).apply {
            text = "Craft"
            setBackgroundColor(Color.argb(200, 60, 140, 60))
            setTextColor(Color.WHITE)
            setOnClickListener {
                val grid2 = Array(9) { i ->
                    val child = craftGrid.getChildAt(i)
                    if (child is EditText) child.text.toString().toIntOrNull() ?: 0 else 0
                }
                engine.craftItem(grid2.toIntArray())
            }
        }
        craftRow.addView(craftGrid)
        craftRow.addView(arrow)
        craftRow.addView(resultSlot)
        panel.addView(craftRow)
        panel.addView(craftBtn)

        val closeBtn = Button(this).apply {
            text = "✕ Close"
            setBackgroundColor(Color.argb(200, 160, 50, 50))
            setTextColor(Color.WHITE)
            setOnClickListener { inventoryPanel.isVisible = false }
        }
        panel.addView(closeBtn, LinearLayout.LayoutParams(dp(150f), ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = dp(8f)
            gravity = Gravity.CENTER_HORIZONTAL
        })

        return panel
    }

    private fun buildSettingsPanel(): LinearLayout {
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(230, 15, 15, 15))
            setPadding(dp(30f), dp(40f), dp(30f), dp(40f))
            gravity = Gravity.TOP or Gravity.CENTER_HORIZONTAL
        }
        val title = TextView(this).apply {
            text = "⚙ Settings"
            textSize = 22f
            setTextColor(Color.WHITE)
            gravity = Gravity.CENTER
        }
        panel.addView(title)

        sensitivityLabel = TextView(this).apply {
            text = "Camera Sensitivity: ${String.format("%.2f", sensitivity)}"
            textSize = 14f
            setTextColor(Color.LTGRAY)
            setPadding(0, dp(20f), 0, dp(6f))
        }
        panel.addView(sensitivityLabel)

        sensitivitySeekBar = SeekBar(this).apply {
            max = 200
            progress = (sensitivity * 100).toInt()
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(sb: SeekBar?, p: Int, user: Boolean) {
                    sensitivity = p / 100f + 0.05f
                    sensitivityLabel.text = "Camera Sensitivity: ${String.format("%.2f", sensitivity)}"
                    engine.onSetSensitivity(sensitivity)
                }
                override fun onStartTrackingTouch(sb: SeekBar?) {}
                override fun onStopTrackingTouch(sb: SeekBar?) {
                    prefs.edit().putFloat("sensitivity", sensitivity).apply()
                }
            })
        }
        panel.addView(sensitivitySeekBar, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        val gamemodeTitle = TextView(this).apply {
            text = "Game Mode"; textSize = 16f; setTextColor(Color.WHITE)
            setPadding(0, dp(20f), 0, dp(8f))
        }
        panel.addView(gamemodeTitle)

        val gmRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        val modes = listOf("Survival" to "/gamemode survival", "Creative" to "/gamemode creative",
            "Adventure" to "/gamemode adventure", "Spectator" to "/gamemode spectator")
        for ((label, cmd) in modes) {
            val btn = Button(this).apply {
                text = label; textSize = 11f
                setBackgroundColor(Color.argb(200, 50, 80, 120))
                setTextColor(Color.WHITE)
                setOnClickListener { engine.onSendChat(cmd); settingsPanel.isVisible = false }
            }
            gmRow.addView(btn, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f).apply {
                setMargins(4, 0, 4, 0)
            })
        }
        panel.addView(gmRow, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        val newSeed = Button(this).apply {
            text = "🌍 New World (New Seed)"
            setBackgroundColor(Color.argb(200, 60, 120, 60))
            setTextColor(Color.WHITE)
            setOnClickListener {
                val newS = System.currentTimeMillis()
                prefs.edit().putLong("world_seed", newS).apply()
                engine.destroy()
                engine.seed = newS
                glView.onResume()
                Engine.nativeInit(resources.displayMetrics.widthPixels, resources.displayMetrics.heightPixels, newS)
                settingsPanel.isVisible = false
            }
        }
        panel.addView(newSeed, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = dp(16f)
        })

        val closeBtn = Button(this).apply {
            text = "✕ Close Settings"
            setBackgroundColor(Color.argb(200, 160, 50, 50))
            setTextColor(Color.WHITE)
            setOnClickListener { settingsPanel.isVisible = false }
        }
        panel.addView(closeBtn, LinearLayout.LayoutParams(dp(200f), ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = dp(12f)
            gravity = Gravity.CENTER_HORIZONTAL
        })

        return panel
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun wireListeners() {
        btnJump.setOnTouchListener { _, ev ->
            when (ev.actionMasked) {
                MotionEvent.ACTION_DOWN -> glView.queueEvent { engine.onJumpPress() }
            }
            true
        }

        btnBreak.setOnTouchListener { _, ev ->
            when (ev.actionMasked) {
                MotionEvent.ACTION_DOWN -> glView.queueEvent { engine.onBreakPress() }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> glView.queueEvent { engine.onBreakRelease() }
            }
            true
        }

        btnPlace.setOnClickListener { glView.queueEvent { engine.onPlace() } }

        btnSneak.setOnTouchListener { _, ev ->
            glView.queueEvent { engine.onSneak(ev.actionMasked == MotionEvent.ACTION_DOWN) }
            true
        }

        var sprintToggle = false
        btnSprint.setOnClickListener {
            sprintToggle = !sprintToggle
            engine.onSprint(sprintToggle)
            btnSprint.setBackgroundColor(if (sprintToggle) Color.argb(200, 220, 80, 220) else Color.argb(130, 180, 100, 180))
        }

        btnFlyUp.setOnTouchListener { _, ev ->
            when (ev.actionMasked) {
                MotionEvent.ACTION_DOWN -> glView.queueEvent { engine.onFlyUp(true) }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> glView.queueEvent { engine.onFlyUp(false) }
            }
            true
        }

        btnFlyDown.setOnTouchListener { _, ev ->
            when (ev.actionMasked) {
                MotionEvent.ACTION_DOWN -> glView.queueEvent { engine.onFlyDown(true) }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> glView.queueEvent { engine.onFlyDown(false) }
            }
            true
        }

        btnInventory.setOnClickListener {
            inventoryPanel.isVisible = !inventoryPanel.isVisible
            settingsPanel.isVisible = false
        }

        btnChat.setOnClickListener {
            val visible = !chatContainer.isVisible
            chatContainer.isVisible = visible
            if (visible) {
                chatInput.requestFocus()
                val imm = getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
                imm.showSoftInput(chatInput, InputMethodManager.SHOW_IMPLICIT)
            }
        }

        btnSettings.setOnClickListener {
            settingsPanel.isVisible = !settingsPanel.isVisible
            inventoryPanel.isVisible = false
        }

        btnSendChat.setOnClickListener { sendChatMessage() }
        chatInput.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_SEND) { sendChatMessage(); true } else false
        }

        joystickBase.setOnTouchListener { _, _ -> true }

        for (i in 0..8) {
            val slotIndex = i
            hotbarSlots[i].setOnClickListener {
                selectedSlot = slotIndex
                glView.queueEvent { engine.onSelectSlot(slotIndex) }
                updateHotbarHighlight()
            }
        }

        wireCameraAndJoystick()
    }

    private fun sendChatMessage() {
        val msg = chatInput.text.toString().trim()
        if (msg.isNotEmpty()) {
            glView.queueEvent { engine.onSendChat(msg) }
            chatInput.text.clear()
        }
    }

    private fun updateHotbarHighlight() {
        for (i in 0..8) {
            hotbarSlots[i].setBackgroundColor(
                if (i == selectedSlot) Color.argb(200, 255, 255, 100)
                else Color.argb(100, 50, 50, 50))
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun wireCameraAndJoystick() {
        val sw = resources.displayMetrics.widthPixels.toFloat()
        val sh = resources.displayMetrics.heightPixels.toFloat()
        val jbL = dpf(40f)
        val jbB = sh - dpf(120f) - dpf(joystickRadius * 2)
        joystickBaseX = jbL + dpf(joystickRadius)
        joystickBaseY = jbB + dpf(joystickRadius)

        val cameraZoneLeft = sw / 2f

        overlayLayout.setOnTouchListener { _, event ->
            val action = event.actionMasked
            val pIdx  = event.actionIndex
            val pId   = event.getPointerId(pIdx)
            val rawX  = event.getX(pIdx)
            val rawY  = event.getY(pIdx)

            when (action) {
                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                    if (rawX < cameraZoneLeft && joystickPointerId < 0) {
                        joystickPointerId = pId
                        moveJoystick(rawX, rawY)
                    } else if (rawX >= cameraZoneLeft && cameraPointerId < 0) {
                        cameraPointerId = pId
                    }
                }
                MotionEvent.ACTION_MOVE -> {
                    for (i in 0 until event.pointerCount) {
                        val id = event.getPointerId(i)
                        val x  = event.getX(i)
                        val y  = event.getY(i)
                        if (id == joystickPointerId) moveJoystick(x, y)
                        else if (id == cameraPointerId) {
                            val hist = event.historySize
                            val prevX = if (hist > 0) event.getHistoricalX(i, hist - 1) else x
                            val prevY = if (hist > 0) event.getHistoricalY(i, hist - 1) else y
                            val dx = (x - prevX) * sensitivity
                            val dy = (y - prevY) * sensitivity
                            glView.queueEvent { engine.onCameraDrag(dx, dy) }
                        }
                    }
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_CANCEL -> {
                    if (pId == joystickPointerId) {
                        joystickPointerId = -1
                        resetJoystick()
                    } else if (pId == cameraPointerId) {
                        cameraPointerId = -1
                    }
                }
            }
            true
        }
    }

    private fun moveJoystick(rawX: Float, rawY: Float) {
        val dx = rawX - joystickBaseX
        val dy = rawY - joystickBaseY
        val dist = sqrt(dx * dx + dy * dy)
        val maxR = dpf(joystickRadius)
        val nx = if (dist > maxR) dx / dist else dx / maxR
        val ny = if (dist > maxR) dy / dist else dy / maxR
        val kx = joystickBaseX + nx * maxR - dpf(joystickKnobSize)
        val ky = joystickBaseY + ny * maxR - dpf(joystickKnobSize)
        joystickKnob.translationX = kx - (dpf(40f) + dpf(joystickRadius) - dpf(joystickKnobSize))
        joystickKnob.translationY = kx
        glView.queueEvent { engine.onJoystickMove(nx, ny) }
    }

    private fun resetJoystick() {
        joystickKnob.translationX = 0f
        joystickKnob.translationY = 0f
        glView.queueEvent { engine.onJoystickMove(0f, 0f) }
    }

    private fun updateUI() {
        if (!Engine.nativeIsInitialized()) return
        try {
            val log = engine.getChatLog()
            chatMessages.text = log.trim()

            val inv = JSONArray(engine.getInventory())
            for (i in 0 until minOf(9, inv.length())) {
                val slot = inv.getJSONObject(i)
                val id = slot.getInt("id")
                val count = slot.getInt("count")
                hotbarSlots[i].text = if (id > 0) "$id\n×$count" else ""
            }
        } catch (_: Exception) {}
    }

    override fun onResume() {
        super.onResume()
        glView.onResume()
        handler.post(uiUpdateRunnable)
    }

    override fun onPause() {
        super.onPause()
        glView.onPause()
        handler.removeCallbacks(uiUpdateRunnable)
        prefs.edit().putFloat("sensitivity", sensitivity).apply()
    }

    override fun onDestroy() {
        super.onDestroy()
        glView.queueEvent { engine.destroy() }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) {
            window.decorView.systemUiVisibility = (
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                or View.SYSTEM_UI_FLAG_FULLSCREEN)
        }
    }
}
