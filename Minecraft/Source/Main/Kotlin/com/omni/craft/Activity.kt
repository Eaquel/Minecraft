package com.omni.craft

import android.annotation.SuppressLint
import android.app.Activity
import android.app.AlertDialog
import android.content.Context
import android.content.SharedPreferences
import android.graphics.Color
import android.opengl.GLSurfaceView
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.text.InputType
import android.view.*
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.*
import androidx.core.view.isVisible
import org.json.JSONArray
import kotlin.math.*
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class Engine(@Suppress("UNUSED_PARAMETER") ctx: Context) {

    var seed: Long = System.currentTimeMillis()

    companion object {
        init { System.loadLibrary("omnicraft") }
        @JvmStatic external fun nativeInit(width: Int, height: Int, seed: Long)
        @JvmStatic external fun nativeIsInitialized(): Boolean
        @JvmStatic external fun nativeFrame(dt: Float)
        @JvmStatic external fun nativeResize(w: Int, h: Int)
    }

    fun isInitialized(): Boolean = nativeIsInitialized()

    fun onJoystick(x: Float, y: Float)          = nativeJoystick(x, y)
    fun onCameraDrag(dx: Float, dy: Float)       = nativeCameraInput(dx, dy)
    fun onJump()                                 = nativeJump()
    fun onBreakStart()                           = nativeStartBreak()
    fun onBreakStop()                            = nativeStopBreak()
    fun onPlace()                                = nativeTap(1)
    fun onSneak(on: Boolean)                     = nativeSneak(on)
    fun onSprint(on: Boolean)                    = nativeSprint(on)
    fun onFlyUp(on: Boolean)                     = nativeFlyUp(on)
    fun onFlyDown(on: Boolean)                   = nativeFlyDown(on)
    fun onSlot(slot: Int)                        = nativeSelectSlot(slot)
    fun onChat(msg: String)                      = nativeSendChat(msg)
    fun onSensitivity(s: Float)                  = nativeSetSensitivity(s)
    fun onCraft(grid: IntArray)                  = nativeCraftItem(grid)
    fun chatLog(): String                        = nativeGetChatLog()
    fun inventory(): String                      = nativeGetInventory()
    fun destroy()                                = nativeDestroy()

    private external fun nativeJoystick(x: Float, y: Float)
    private external fun nativeCameraInput(dx: Float, dy: Float)
    private external fun nativeTap(type: Int)
    private external fun nativeJump()
    private external fun nativeSneak(on: Boolean)
    private external fun nativeSprint(on: Boolean)
    private external fun nativeSelectSlot(slot: Int)
    private external fun nativeSendChat(msg: String)
    private external fun nativeSetSensitivity(s: Float)
    private external fun nativeFlyUp(on: Boolean)
    private external fun nativeFlyDown(on: Boolean)
    private external fun nativeStartBreak()
    private external fun nativeStopBreak()
    private external fun nativeCraftItem(grid: IntArray)
    private external fun nativeGetChatLog(): String
    private external fun nativeGetInventory(): String
    private external fun nativeDestroy()
}

class GameSurface(ctx: Context, private val engine: Engine) : GLSurfaceView(ctx) {
    init {
        setEGLContextClientVersion(3)
        setEGLConfigChooser(8, 8, 8, 8, 24, 8)
        setRenderer(OmniRenderer())
        renderMode = RENDERMODE_CONTINUOUSLY
    }
    private inner class OmniRenderer : Renderer {
        private var lastNano = System.nanoTime()
        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            lastNano = System.nanoTime()
            if (engine.isInitialized())
                Engine.nativeInit(width.coerceAtLeast(1), height.coerceAtLeast(1), engine.seed)
        }
        override fun onSurfaceChanged(gl: GL10?, w: Int, h: Int) {
            if (engine.isInitialized()) Engine.nativeResize(w, h)
        }
        override fun onDrawFrame(gl: GL10?) {
            if (!engine.isInitialized()) return
            val now  = System.nanoTime()
            val dt   = ((now - lastNano) / 1_000_000_000f).coerceIn(0f, 0.05f)
            lastNano = now
            Engine.nativeFrame(dt)
        }
    }
}

@SuppressLint("ClickableViewAccessibility")
class Activity : Activity() {

    private lateinit var engine:       Engine
    private lateinit var surface:      GameSurface
    private lateinit var prefs:        SharedPreferences
    private lateinit var overlay:      FrameLayout

    private lateinit var joystickBase: View
    private lateinit var joystickKnob: View
    private lateinit var btnJump:      TextView
    private lateinit var btnBreak:     TextView
    private lateinit var btnPlace:     TextView
    private lateinit var btnSneak:     TextView
    private lateinit var btnSprint:    TextView
    private lateinit var btnFlyUp:     TextView
    private lateinit var btnFlyDown:   TextView
    private lateinit var btnInventory: TextView
    private lateinit var btnChat:      TextView
    private lateinit var btnSettings:  TextView
    private lateinit var hotbarRow:    LinearLayout

    private lateinit var chatContainer: LinearLayout
    private lateinit var chatMessages:  TextView
    private lateinit var chatInput:     EditText

    private lateinit var inventoryPanel: LinearLayout
    private lateinit var settingsPanel:  LinearLayout
    private lateinit var mainMenuPanel:  FrameLayout
    private lateinit var worldListPanel: LinearLayout

    private lateinit var sensitivityBar:   SeekBar
    private lateinit var sensitivityLabel: TextView

    private val hotbarSlots   = Array(9) { TextView(this) }
    private var selectedSlot  = 0
    private var sensitivity   = 0.25f
    private var sprintToggle  = false
    private var gameStarted   = false

    private var joyPointerId  = -1
    private var camPointerId  = -1
    private var joyBaseX      = 0f
    private var joyBaseY      = 0f
    private val joyRadius     = 110f
    private val joyKnobHalf   = 55f

    private val handler         = Handler(Looper.getMainLooper())
    private val hudUpdateTick   = object : Runnable {
        override fun run() { refreshHud(); handler.postDelayed(this, 80) }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        window.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN)
        applyImmersive()

        prefs       = getSharedPreferences("omnicraft", MODE_PRIVATE)
        sensitivity = prefs.getFloat("sensitivity", 0.25f)
        engine      = Engine(this)
        surface     = GameSurface(this, engine)

        val root = FrameLayout(this)
        root.addView(surface, matchFull())
        overlay = FrameLayout(this)
        root.addView(overlay, matchFull())

        buildHud()
        buildMainMenu()
        setContentView(root)

        setHudVisible(false)
        mainMenuPanel.isVisible = true
    }

    private fun dp(v: Float): Int   = (v * resources.displayMetrics.density + .5f).toInt()
    private fun dpf(v: Float): Float = v * resources.displayMetrics.density
    private fun matchFull() = FrameLayout.LayoutParams(
        ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)

    private fun buildMainMenu() {
        mainMenuPanel = FrameLayout(this).apply { setBackgroundColor(Color.argb(248, 8, 8, 18)) }

        mainMenuPanel.addView(TextView(this).apply {
            text = getString(R.string.app_name); textSize = 46f; setTextColor(Color.WHITE)
            gravity = Gravity.CENTER
            setShadowLayer(10f, 4f, 4f, Color.argb(200, 0, 180, 255))
            layoutParams = FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(80f))
                .apply { topMargin = dp(55f) }
        })

        mainMenuPanel.addView(TextView(this).apply {
            text = getString(R.string.menu_tagline); textSize = 13f
            setTextColor(Color.argb(190, 140, 200, 255)); gravity = Gravity.CENTER
            layoutParams = FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(28f))
                .apply { topMargin = dp(142f) }
        })

        val col = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL; gravity = Gravity.CENTER_HORIZONTAL
            layoutParams = FrameLayout.LayoutParams(dp(310f), ViewGroup.LayoutParams.WRAP_CONTENT)
                .apply { gravity = Gravity.CENTER }
        }

        fun menuBtn(label: String, bg: Int) = Button(this).apply {
            text = label; textSize = 17f; setTextColor(Color.WHITE); setBackgroundColor(bg)
        }
        val btnPlay   = menuBtn(getString(R.string.menu_play),    Color.argb(220, 38, 130, 38))
        val btnWorlds = menuBtn(getString(R.string.menu_worlds),  Color.argb(220, 38, 75,  155))
        val btnSet    = menuBtn(getString(R.string.btn_settings), Color.argb(220, 75, 75,  75))
        val btnQuit   = menuBtn(getString(R.string.menu_quit),    Color.argb(220, 155, 38, 38))

        btnPlay.setOnClickListener {
            val s = prefs.getLong("world_seed", System.currentTimeMillis())
            val n = prefs.getString("world_name", getString(R.string.world_default_name))
                ?: getString(R.string.world_default_name)
            startGame(s, n)
        }
        btnWorlds.setOnClickListener { mainMenuPanel.isVisible = false; openWorldList() }
        btnSet.setOnClickListener   { mainMenuPanel.isVisible = false; settingsPanel.isVisible = true }
        btnQuit.setOnClickListener  { finishAffinity() }

        for (btn in listOf(btnPlay, btnWorlds, btnSet, btnQuit))
            col.addView(btn, LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(54f)).apply { topMargin = dp(10f) })
        mainMenuPanel.addView(col)

        mainMenuPanel.addView(TextView(this).apply {
            text = getString(R.string.menu_version, "2.0.0"); textSize = 10f
            setTextColor(Color.GRAY); gravity = Gravity.END or Gravity.BOTTOM
            setPadding(0, 0, dp(12f), dp(8f))
            layoutParams = matchFull()
        })

        overlay.addView(mainMenuPanel, matchFull())
    }

    private fun openWorldList() {
        val count  = prefs.getInt("world_count", 0)
        val worlds = (0 until count).mapNotNull { i ->
            val s = prefs.getLong("world_seed_$i", 0L)
            val n = prefs.getString("world_name_$i", "World ${i+1}") ?: ""
            if (s != 0L) s to n else null
        }

        worldListPanel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(248, 8, 8, 18))
            setPadding(dp(20f), dp(28f), dp(20f), dp(18f))
        }

        val hdr = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL; gravity = Gravity.CENTER_VERTICAL }
        hdr.addView(TextView(this).apply {
            text = getString(R.string.menu_worlds); textSize = 22f; setTextColor(Color.WHITE)
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
        })
        hdr.addView(Button(this).apply {
            text = getString(R.string.menu_back); textSize = 12f
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(200, 100, 48, 48))
            setOnClickListener { worldListPanel.isVisible = false; mainMenuPanel.isVisible = true }
        }, LinearLayout.LayoutParams(dp(100f), dp(44f)))
        worldListPanel.addView(hdr)

        val scroll = ScrollView(this)
        val list   = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        if (worlds.isEmpty()) {
            list.addView(TextView(this).apply {
                text = getString(R.string.world_no_worlds); textSize = 13f
                setTextColor(Color.GRAY); gravity = Gravity.CENTER
                setPadding(0, dp(38f), 0, 0)
            })
        } else { worlds.forEach { (s, n) -> list.addView(worldRow(s, n)) } }
        scroll.addView(list)
        worldListPanel.addView(scroll, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))

        worldListPanel.addView(Button(this).apply {
            text = getString(R.string.world_new); textSize = 15f
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(220, 38, 125, 38))
            setOnClickListener { showNewWorldDialog() }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(50f))
            .apply { topMargin = dp(10f) })

        overlay.addView(worldListPanel, matchFull())
    }

    private fun worldRow(seed: Long, name: String) = LinearLayout(this).apply {
        orientation = LinearLayout.HORIZONTAL
        setBackgroundColor(Color.argb(145, 28, 38, 58))
        setPadding(dp(12f), dp(10f), dp(12f), dp(10f))
        val lp = LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
        lp.topMargin = dp(6f); layoutParams = lp

        val info = LinearLayout(this@Activity).apply {
            orientation = LinearLayout.VERTICAL
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
        }
        info.addView(TextView(this@Activity).apply { text = name; textSize = 15f; setTextColor(Color.WHITE) })
        info.addView(TextView(this@Activity).apply {
            text = getString(R.string.world_seed_label, seed); textSize = 10f; setTextColor(Color.GRAY)
        })
        addView(info)
        addView(Button(this@Activity).apply {
            text = getString(R.string.world_load); textSize = 11f
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(200, 38, 95, 175))
            setOnClickListener { startGame(seed, name) }
        }, LinearLayout.LayoutParams(dp(88f), dp(44f)))
    }

    private fun showNewWorldDialog() {
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(20f), dp(20f), dp(20f), dp(20f))
            setBackgroundColor(Color.argb(242, 18, 18, 32))
        }
        val dialog = AlertDialog.Builder(this).setView(layout).create()

        val nameEt = EditText(this).apply {
            hint = getString(R.string.world_name_hint); setHintTextColor(Color.GRAY)
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(175, 38, 38, 58))
            inputType = InputType.TYPE_CLASS_TEXT; setPadding(dp(8f), dp(8f), dp(8f), dp(8f))
        }
        val seedEt = EditText(this).apply {
            hint = getString(R.string.world_seed_hint); setHintTextColor(Color.GRAY)
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(175, 38, 38, 58))
            inputType = InputType.TYPE_CLASS_NUMBER or InputType.TYPE_NUMBER_FLAG_SIGNED
            setPadding(dp(8f), dp(8f), dp(8f), dp(8f))
        }

        val btnRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL; gravity = Gravity.END }
        btnRow.addView(Button(this).apply {
            text = getString(R.string.menu_back)
            setBackgroundColor(Color.argb(200, 115, 48, 48)); setTextColor(Color.WHITE)
            setOnClickListener { dialog.dismiss() }
        }, LinearLayout.LayoutParams(dp(100f), dp(44f)).apply { rightMargin = dp(8f) })
        btnRow.addView(Button(this).apply {
            text = getString(R.string.world_create)
            setBackgroundColor(Color.argb(200, 38, 125, 38)); setTextColor(Color.WHITE)
            setOnClickListener {
                val n = nameEt.text.toString().trim().ifEmpty { getString(R.string.world_default_name) }
                val s = seedEt.text.toString().toLongOrNull() ?: System.currentTimeMillis()
                saveWorld(s, n); dialog.dismiss()
                worldListPanel.isVisible = false
                (worldListPanel.parent as? ViewGroup)?.removeView(worldListPanel)
                openWorldList()
            }
        }, LinearLayout.LayoutParams(dp(100f), dp(44f)))

        layout.addView(TextView(this).apply {
            text = getString(R.string.world_new); textSize = 19f
            setTextColor(Color.WHITE); gravity = Gravity.CENTER
        })
        layout.addView(nameEt, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(50f)).apply { topMargin = dp(12f) })
        layout.addView(seedEt, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(50f)).apply { topMargin = dp(8f) })
        layout.addView(btnRow, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            .apply { topMargin = dp(14f) })
        dialog.show()
    }

    private fun saveWorld(seed: Long, name: String) {
        val count = prefs.getInt("world_count", 0)
        prefs.edit()
            .putLong("world_seed_$count", seed).putString("world_name_$count", name)
            .putInt("world_count", count + 1)
            .putLong("world_seed", seed).putString("world_name", name)
            .apply()
    }

    private fun startGame(seed: Long, name: String) {
        prefs.edit().putLong("world_seed", seed).putString("world_name", name).apply()
        engine.seed = seed
        mainMenuPanel.isVisible = false
        if (::worldListPanel.isInitialized) worldListPanel.isVisible = false
        settingsPanel.isVisible = false

        Engine.nativeInit(
            resources.displayMetrics.widthPixels,
            resources.displayMetrics.heightPixels,
            seed
        )
        setHudVisible(true)
        gameStarted = true
        surface.onResume()
        handler.post(hudUpdateTick)
    }

    private fun setHudVisible(v: Boolean) {
        listOf(joystickBase, joystickKnob, btnJump, btnBreak, btnPlace,
               btnSneak, btnSprint, btnFlyUp, btnFlyDown,
               btnInventory, btnChat, btnSettings, hotbarRow)
            .forEach { it.isVisible = v }
    }

    private fun buildHud() {
        val sw    = resources.displayMetrics.widthPixels
        val jbSz  = dp(joyRadius * 2)
        val jkSz  = dp(joyKnobHalf * 2)

        joystickBase = View(this).apply { setBackgroundColor(Color.argb(48, 200, 200, 200)) }
        overlay.addView(joystickBase, FrameLayout.LayoutParams(jbSz, jbSz).apply {
            gravity = Gravity.BOTTOM or Gravity.START
            leftMargin = dp(36f); bottomMargin = dp(110f)
        })
        joystickKnob = View(this).apply { setBackgroundColor(Color.argb(110, 255, 255, 255)) }
        overlay.addView(joystickKnob, FrameLayout.LayoutParams(jkSz, jkSz).apply {
            gravity = Gravity.BOTTOM or Gravity.START
            leftMargin   = dp(36f) + jbSz / 2 - jkSz / 2
            bottomMargin = dp(110f) + jbSz / 2 - jkSz / 2
        })

        fun iconBtn(label: String, bg: Int) = TextView(this).apply {
            text = label; textSize = 21f; gravity = Gravity.CENTER
            setTextColor(Color.WHITE); setBackgroundColor(bg)
        }

        btnJump = iconBtn("⬆", Color.argb(145, 95, 175, 95)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(68f), dp(68f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END; bottomMargin = dp(195f); rightMargin = dp(28f)
            })
        }
        btnBreak = iconBtn("⛏", Color.argb(145, 195, 75, 75)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(78f), dp(78f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END; bottomMargin = dp(110f); rightMargin = dp(116f)
            })
        }
        btnPlace = iconBtn("🏗", Color.argb(145, 75, 75, 195)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(78f), dp(78f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END; bottomMargin = dp(110f); rightMargin = dp(28f)
            })
        }

        val sneakLeft = dp(36f) + jbSz + dp(10f)
        btnSneak = iconBtn("⬇", Color.argb(125, 175, 155, 75)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(58f), dp(58f)).apply {
                gravity = Gravity.BOTTOM or Gravity.START; bottomMargin = dp(110f); leftMargin = sneakLeft
            })
        }
        btnSprint = iconBtn("🏃", Color.argb(125, 175, 95, 175)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(58f), dp(58f)).apply {
                gravity = Gravity.BOTTOM or Gravity.START; bottomMargin = dp(178f); leftMargin = sneakLeft
            })
        }
        btnFlyUp = iconBtn("↑↑", Color.argb(125, 95, 195, 195)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(54f), dp(54f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END; bottomMargin = dp(272f); rightMargin = dp(28f)
            })
        }
        btnFlyDown = iconBtn("↓↓", Color.argb(125, 195, 195, 95)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(54f), dp(54f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END; bottomMargin = dp(208f); rightMargin = dp(28f)
            })
        }

        btnInventory = iconBtn("🎒", Color.argb(145, 145, 115, 75)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(54f), dp(54f)).apply {
                gravity = Gravity.TOP or Gravity.END; topMargin = dp(10f); rightMargin = dp(73f)
            })
        }
        btnChat = iconBtn("💬", Color.argb(145, 75, 125, 195)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(54f), dp(54f)).apply {
                gravity = Gravity.TOP or Gravity.END; topMargin = dp(10f); rightMargin = dp(136f)
            })
        }
        btnSettings = iconBtn("⚙", Color.argb(145, 125, 125, 125)).also {
            overlay.addView(it, FrameLayout.LayoutParams(dp(54f), dp(54f)).apply {
                gravity = Gravity.TOP or Gravity.END; topMargin = dp(10f); rightMargin = dp(10f)
            })
        }

        hotbarRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        for (i in 0..8) {
            val slot = TextView(this).apply {
                setBackgroundColor(if (i == 0) Color.argb(175, 255, 255, 100) else Color.argb(95, 48, 48, 48))
                setTextColor(Color.WHITE); textSize = 9f; gravity = Gravity.CENTER; text = ""
                setPadding(2, 2, 2, 2)
            }
            hotbarSlots[i] = slot
            hotbarRow.addView(slot, LinearLayout.LayoutParams(0, dp(54f), 1f))
        }
        overlay.addView(hotbarRow, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(54f)).apply {
            gravity = Gravity.BOTTOM; bottomMargin = dp(52f)
        })

        chatContainer = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL; isVisible = false }
        chatMessages  = TextView(this).apply {
            setTextColor(Color.WHITE); textSize = 10f
            setBackgroundColor(Color.argb(115, 0, 0, 0))
            setPadding(dp(6f), dp(4f), dp(6f), dp(4f)); text = ""
        }
        val chatRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        chatInput   = EditText(this).apply {
            hint = getString(R.string.chat_hint); setHintTextColor(Color.GRAY)
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(175, 28, 28, 28))
            inputType = InputType.TYPE_CLASS_TEXT; imeOptions = EditorInfo.IME_ACTION_SEND
            setPadding(dp(8f), dp(6f), dp(8f), dp(6f))
        }
        val sendBtn = Button(this).apply {
            text = getString(R.string.chat_send)
            setBackgroundColor(Color.argb(195, 48, 145, 48)); setTextColor(Color.WHITE)
            setOnClickListener { sendChat() }
        }
        chatInput.setOnEditorActionListener { _, id, _ ->
            if (id == EditorInfo.IME_ACTION_SEND) { sendChat(); true } else false
        }
        chatRow.addView(chatInput, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        chatRow.addView(sendBtn, LinearLayout.LayoutParams(dp(50f), ViewGroup.LayoutParams.WRAP_CONTENT))
        chatContainer.addView(chatMessages, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        chatContainer.addView(chatRow)
        overlay.addView(chatContainer, FrameLayout.LayoutParams(sw * 2 / 3, dp(240f)).apply {
            gravity = Gravity.BOTTOM or Gravity.START; bottomMargin = dp(110f); leftMargin = dp(10f)
        })

        inventoryPanel = buildInventoryPanel()
        overlay.addView(inventoryPanel, matchFull()); inventoryPanel.isVisible = false

        settingsPanel = buildSettingsPanel()
        overlay.addView(settingsPanel, matchFull()); settingsPanel.isVisible = false

        wireControls()
    }

    private fun buildInventoryPanel(): LinearLayout {
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(215, 18, 18, 18))
            setPadding(dp(16f), dp(16f), dp(16f), dp(16f))
            gravity = Gravity.CENTER
        }
        panel.addView(TextView(this).apply {
            text = getString(R.string.inventory_title)
            textSize = 17f; setTextColor(Color.WHITE); gravity = Gravity.CENTER
        })

        val invGrid = GridLayout(this).apply { columnCount = 9; rowCount = 4 }
        for (r in 0..3) for (c in 0..8)
            invGrid.addView(TextView(this).apply {
                setBackgroundColor(Color.argb(145, 58, 58, 58))
                setTextColor(Color.WHITE); textSize = 9f; gravity = Gravity.CENTER
                text = ""; setPadding(4, 4, 4, 4)
            }, GridLayout.LayoutParams().apply { width = dp(40f); height = dp(40f); setMargins(2, 2, 2, 2) })
        panel.addView(invGrid)

        panel.addView(TextView(this).apply {
            text = getString(R.string.crafting_title)
            textSize = 13f; setTextColor(Color.LTGRAY); gravity = Gravity.CENTER
            setPadding(0, dp(10f), 0, dp(4f))
        })

        val craftRow  = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL; gravity = Gravity.CENTER }
        val craftGrid = GridLayout(this).apply { columnCount = 3; rowCount = 3 }
        for (i in 0..8)
            craftGrid.addView(EditText(this).apply {
                hint = getString(R.string.crafting_id_hint)
                setTextColor(Color.WHITE); setHintTextColor(Color.GRAY)
                setBackgroundColor(Color.argb(145, 78, 78, 78))
                textSize = 9f; inputType = InputType.TYPE_CLASS_NUMBER
                gravity = Gravity.CENTER; setPadding(2, 2, 2, 2)
            }, GridLayout.LayoutParams().apply { width = dp(36f); height = dp(36f); setMargins(2, 2, 2, 2) })
        craftRow.addView(craftGrid)
        craftRow.addView(TextView(this).apply {
            text = "➤"; textSize = 18f; setTextColor(Color.WHITE)
            gravity = Gravity.CENTER; setPadding(dp(8f), 0, dp(8f), 0)
        })
        craftRow.addView(TextView(this).apply {
            setBackgroundColor(Color.argb(175, 95, 155, 95))
            setTextColor(Color.WHITE); textSize = 11f; gravity = Gravity.CENTER; text = "?"
        })
        panel.addView(craftRow)

        panel.addView(Button(this).apply {
            text = getString(R.string.crafting_button)
            setBackgroundColor(Color.argb(195, 58, 135, 58)); setTextColor(Color.WHITE)
            setOnClickListener {
                val ids = Array(9) { i ->
                    val child = craftGrid.getChildAt(i)
                    if (child is EditText) child.text.toString().toIntOrNull() ?: 0 else 0
                }
                surface.queueEvent { engine.onCraft(ids.toIntArray()) }
            }
        })
        panel.addView(Button(this).apply {
            text = getString(R.string.close_button)
            setBackgroundColor(Color.argb(195, 155, 48, 48)); setTextColor(Color.WHITE)
            setOnClickListener { inventoryPanel.isVisible = false }
        }, LinearLayout.LayoutParams(dp(145f), ViewGroup.LayoutParams.WRAP_CONTENT)
            .apply { topMargin = dp(8f); gravity = Gravity.CENTER_HORIZONTAL })
        return panel
    }

    private fun buildSettingsPanel(): LinearLayout {
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(228, 12, 12, 12))
            setPadding(dp(28f), dp(38f), dp(28f), dp(38f))
            gravity = Gravity.TOP or Gravity.CENTER_HORIZONTAL
        }
        panel.addView(TextView(this).apply {
            text = getString(R.string.settings_title)
            textSize = 20f; setTextColor(Color.WHITE); gravity = Gravity.CENTER
        })

        sensitivityLabel = TextView(this).apply {
            text = "${getString(R.string.sensitivity_label)}: ${String.format("%.2f", sensitivity)}"
            textSize = 13f; setTextColor(Color.LTGRAY); setPadding(0, dp(18f), 0, dp(5f))
        }
        panel.addView(sensitivityLabel)

        sensitivityBar = SeekBar(this).apply {
            max = 200; progress = ((sensitivity - 0.05f) * 100).toInt()
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(sb: SeekBar?, p: Int, user: Boolean) {
                    sensitivity = p / 100f + 0.05f
                    sensitivityLabel.text = "${getString(R.string.sensitivity_label)}: ${String.format("%.2f", sensitivity)}"
                    surface.queueEvent { engine.onSensitivity(sensitivity) }
                }
                override fun onStartTrackingTouch(sb: SeekBar?) {}
                override fun onStopTrackingTouch(sb: SeekBar?) { prefs.edit().putFloat("sensitivity", sensitivity).apply() }
            })
        }
        panel.addView(sensitivityBar, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        panel.addView(TextView(this).apply {
            text = getString(R.string.settings_gamemode); textSize = 15f; setTextColor(Color.WHITE)
            setPadding(0, dp(18f), 0, dp(7f))
        })

        val gmRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        listOf(
            getString(R.string.gamemode_survival)  to "/gamemode survival",
            getString(R.string.gamemode_creative)  to "/gamemode creative",
            getString(R.string.gamemode_adventure) to "/gamemode adventure",
            getString(R.string.gamemode_spectator) to "/gamemode spectator"
        ).forEach { (lbl, cmd) ->
            gmRow.addView(Button(this).apply {
                text = lbl; textSize = 10f
                setBackgroundColor(Color.argb(195, 48, 75, 115)); setTextColor(Color.WHITE)
                setOnClickListener { surface.queueEvent { engine.onChat(cmd) }; settingsPanel.isVisible = false }
            }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
                .apply { setMargins(3, 0, 3, 0) })
        }
        panel.addView(gmRow, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        panel.addView(Button(this).apply {
            text = getString(R.string.settings_main_menu)
            setBackgroundColor(Color.argb(195, 115, 75, 18)); setTextColor(Color.WHITE)
            setOnClickListener {
                settingsPanel.isVisible = false
                if (gameStarted) {
                    surface.onPause(); handler.removeCallbacks(hudUpdateTick)
                    gameStarted = false; setHudVisible(false)
                    chatContainer.isVisible = false; inventoryPanel.isVisible = false
                }
                mainMenuPanel.isVisible = true
            }
        }, LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            .apply { topMargin = dp(14f) })

        panel.addView(Button(this).apply {
            text = getString(R.string.close_button)
            setBackgroundColor(Color.argb(195, 155, 48, 48)); setTextColor(Color.WHITE)
            setOnClickListener { settingsPanel.isVisible = false }
        }, LinearLayout.LayoutParams(dp(195f), ViewGroup.LayoutParams.WRAP_CONTENT)
            .apply { topMargin = dp(10f); gravity = Gravity.CENTER_HORIZONTAL })
        return panel
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun wireControls() {
        btnJump.setOnTouchListener { _, e ->
            if (e.actionMasked == MotionEvent.ACTION_DOWN) surface.queueEvent { engine.onJump() }
            true
        }
        btnBreak.setOnTouchListener { _, e ->
            when (e.actionMasked) {
                MotionEvent.ACTION_DOWN -> surface.queueEvent { engine.onBreakStart() }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> surface.queueEvent { engine.onBreakStop() }
            }; true
        }
        btnPlace.setOnClickListener { surface.queueEvent { engine.onPlace() } }
        btnSneak.setOnTouchListener { _, e ->
            surface.queueEvent { engine.onSneak(e.actionMasked == MotionEvent.ACTION_DOWN) }; true
        }
        btnSprint.setOnClickListener {
            sprintToggle = !sprintToggle
            surface.queueEvent { engine.onSprint(sprintToggle) }
            btnSprint.setBackgroundColor(if (sprintToggle) Color.argb(200, 215, 75, 215)
                                         else Color.argb(125, 175, 95, 175))
        }
        btnFlyUp.setOnTouchListener { _, e ->
            when (e.actionMasked) {
                MotionEvent.ACTION_DOWN -> surface.queueEvent { engine.onFlyUp(true) }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> surface.queueEvent { engine.onFlyUp(false) }
            }; true
        }
        btnFlyDown.setOnTouchListener { _, e ->
            when (e.actionMasked) {
                MotionEvent.ACTION_DOWN -> surface.queueEvent { engine.onFlyDown(true) }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> surface.queueEvent { engine.onFlyDown(false) }
            }; true
        }
        btnInventory.setOnClickListener {
            inventoryPanel.isVisible = !inventoryPanel.isVisible
            settingsPanel.isVisible = false
        }
        btnChat.setOnClickListener {
            chatContainer.isVisible = !chatContainer.isVisible
            if (chatContainer.isVisible) {
                chatInput.requestFocus()
                (getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager)
                    .showSoftInput(chatInput, InputMethodManager.SHOW_IMPLICIT)
            }
        }
        btnSettings.setOnClickListener {
            settingsPanel.isVisible = !settingsPanel.isVisible
            inventoryPanel.isVisible = false
        }
        for (i in 0..8) {
            val idx = i
            hotbarSlots[i].setOnClickListener {
                selectedSlot = idx; surface.queueEvent { engine.onSlot(idx) }; updateHotbarHighlight()
            }
        }
        wireTouchInput()
    }

    private fun sendChat() {
        val msg = chatInput.text.toString().trim()
        if (msg.isNotEmpty()) { surface.queueEvent { engine.onChat(msg) }; chatInput.text.clear() }
    }

    private fun updateHotbarHighlight() {
        for (i in 0..8) hotbarSlots[i].setBackgroundColor(
            if (i == selectedSlot) Color.argb(195, 255, 255, 95) else Color.argb(95, 48, 48, 48))
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun wireTouchInput() {
        val sh     = resources.displayMetrics.heightPixels.toFloat()
        val jbSz   = dpf(joyRadius * 2)
        joyBaseX   = dpf(36f) + dpf(joyRadius)
        joyBaseY   = sh - dpf(110f) - jbSz + dpf(joyRadius)
        val camLeft= resources.displayMetrics.widthPixels / 2f

        overlay.setOnTouchListener { _, event ->
            if (!gameStarted) return@setOnTouchListener false
            val action = event.actionMasked
            val pIdx   = event.actionIndex
            val pId    = event.getPointerId(pIdx)
            val rawX   = event.getX(pIdx)
            val rawY   = event.getY(pIdx)

            when (action) {
                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                    if (rawX < camLeft && joyPointerId < 0) {
                        joyPointerId = pId; moveJoystick(rawX, rawY)
                    } else if (rawX >= camLeft && camPointerId < 0) {
                        camPointerId = pId
                    }
                }
                MotionEvent.ACTION_MOVE -> {
                    for (i in 0 until event.pointerCount) {
                        val id = event.getPointerId(i)
                        val x  = event.getX(i); val y = event.getY(i)
                        if (id == joyPointerId) { moveJoystick(x, y) }
                        else if (id == camPointerId) {
                            val hist  = event.historySize
                            val prevX = if (hist > 0) event.getHistoricalX(i, hist - 1) else x
                            val prevY = if (hist > 0) event.getHistoricalY(i, hist - 1) else y
                            val dx = (x - prevX) * sensitivity
                            val dy = (y - prevY) * sensitivity
                            surface.queueEvent { engine.onCameraDrag(dx, dy) }
                        }
                    }
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_CANCEL -> {
                    if (pId == joyPointerId) { joyPointerId = -1; resetJoystick() }
                    else if (pId == camPointerId) camPointerId = -1
                }
            }
            true
        }
    }

    private fun moveJoystick(rx: Float, ry: Float) {
        val dx = rx - joyBaseX; val dy = ry - joyBaseY
        val dist = sqrt(dx * dx + dy * dy)
        val maxR = dpf(joyRadius)
        val nx = if (dist > maxR) dx / dist else dx / maxR
        val ny = if (dist > maxR) dy / dist else dy / maxR
        val kx = joyBaseX + nx * maxR - dpf(joyKnobHalf)
        val ky = joyBaseY + ny * maxR - dpf(joyKnobHalf)
        val offX = dpf(36f) + dpf(joyRadius) - dpf(joyKnobHalf)
        val sh   = resources.displayMetrics.heightPixels.toFloat()
        val offY = sh - dpf(110f) - dpf(joyRadius * 2) + dpf(joyRadius) - dpf(joyKnobHalf)
        joystickKnob.translationX = kx - offX
        joystickKnob.translationY = ky - offY
        surface.queueEvent { engine.onJoystick(nx, ny) }
    }

    private fun resetJoystick() {
        joystickKnob.translationX = 0f; joystickKnob.translationY = 0f
        surface.queueEvent { engine.onJoystick(0f, 0f) }
    }

    private fun refreshHud() {
        if (!engine.isInitialized()) return
        try {
            chatMessages.text = engine.chatLog().trim()
            val inv = JSONArray(engine.inventory())
            for (i in 0 until minOf(9, inv.length())) {
                val slot  = inv.getJSONObject(i)
                val id    = slot.getInt("id")
                val count = slot.getInt("count")
                hotbarSlots[i].text = if (id > 0) "$id\n×$count" else ""
            }
        } catch (_: Exception) {}
    }

    override fun onResume() {
        super.onResume()
        if (gameStarted) { surface.onResume(); handler.post(hudUpdateTick) }
    }

    override fun onPause() {
        super.onPause()
        if (gameStarted) { surface.onPause(); handler.removeCallbacks(hudUpdateTick) }
        prefs.edit().putFloat("sensitivity", sensitivity).apply()
    }

    override fun onDestroy() {
        super.onDestroy()
        surface.queueEvent { engine.destroy() }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) applyImmersive()
    }

    @Suppress("DEPRECATION")
    private fun applyImmersive() {
        window.decorView.systemUiVisibility =
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_FULLSCREEN
    }
}
