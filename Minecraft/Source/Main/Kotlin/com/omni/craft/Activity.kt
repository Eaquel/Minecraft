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

// ══════════════════════════════════════════════════════════════════════════════
// Engine  – JNI köprüsü (Core.cpp)
// ══════════════════════════════════════════════════════════════════════════════
class Engine(@Suppress("UNUSED_PARAMETER") context: Context) {

    var seed: Long = System.currentTimeMillis()

    companion object {
        init { System.loadLibrary("omnicraft") }

        // static JNI → jclass
        @JvmStatic external fun nativeInit(width: Int, height: Int, seed: Long)
        @JvmStatic external fun nativeIsInitialized(): Boolean
        @JvmStatic external fun nativeFrame(dt: Float)
        @JvmStatic external fun nativeResize(w: Int, h: Int)
    }

    fun isInitialized(): Boolean = nativeIsInitialized()

    // ── wrapper metodlar ─────────────────────────────────────────────────────
    fun onJoystickMove(x: Float, y: Float) = nativeJoystick(x, y)
    fun onCameraDrag(dx: Float, dy: Float) = nativeCameraInput(dx, dy)
    fun onJumpPress()                       = nativeJump()
    fun onBreakPress()                      = nativeStartBreak()
    fun onBreakRelease()                    = nativeStopBreak()
    fun onPlace()                           = nativeTap(1)
    fun onSneak(on: Boolean)                = nativeSneak(on)
    fun onSprint(on: Boolean)               = nativeSprint(on)
    fun onFlyUp(on: Boolean)                = nativeFlyUp(on)
    fun onFlyDown(on: Boolean)              = nativeFlyDown(on)
    fun onSelectSlot(slot: Int)             = nativeSelectSlot(slot)
    fun onSendChat(msg: String)             = nativeSendChat(msg)
    fun onSetSensitivity(s: Float)          = nativeSetSensitivity(s)
    fun craftItem(grid: IntArray)           = nativeCraftItem(grid)
    fun getChatLog(): String                = nativeGetChatLog()
    fun getInventory(): String              = nativeGetInventory()
    fun destroy()                           = nativeDestroy()

    // ── instance JNI bildirimleri ────────────────────────────────────────────
    private external fun nativeJoystick(x: Float, y: Float)
    private external fun nativeCameraInput(dx: Float, dy: Float)
    private external fun nativeTap(type: Int)
    private external fun nativeJump()
    private external fun nativeSneak(on: Boolean)
    private external fun nativeSprint(on: Boolean)
    private external fun nativeSelectSlot(slot: Int)
    private external fun nativeSendChat(msg: String)
    private external fun nativeSetSensitivity(sens: Float)
    private external fun nativeFlyUp(on: Boolean)
    private external fun nativeFlyDown(on: Boolean)
    private external fun nativeStartBreak()
    private external fun nativeStopBreak()
    private external fun nativeCraftItem(grid: IntArray)
    private external fun nativeGetChatLog(): String
    private external fun nativeGetInventory(): String
    private external fun nativeDestroy()
}

// ══════════════════════════════════════════════════════════════════════════════
// Render  – GLSurfaceView, her frame'de Engine.nativeFrame() çağırır
// ══════════════════════════════════════════════════════════════════════════════
class Render(context: Context, private val engine: Engine) : GLSurfaceView(context) {

    init {
        setEGLContextClientVersion(3)
        setEGLConfigChooser(8, 8, 8, 8, 24, 0)
        setRenderer(OmniRenderer())
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    private inner class OmniRenderer : Renderer {
        private var lastNano = System.nanoTime()

        override fun onSurfaceCreated(gl: GL10?, config: EGLConfig?) {
            lastNano = System.nanoTime()
            if (engine.isInitialized()) {
                Engine.nativeInit(
                    this@Render.width.coerceAtLeast(1),
                    this@Render.height.coerceAtLeast(1),
                    engine.seed
                )
            }
        }

        override fun onSurfaceChanged(gl: GL10?, width: Int, height: Int) {
            if (engine.isInitialized()) Engine.nativeResize(width, height)
        }

        override fun onDrawFrame(gl: GL10?) {
            if (!engine.isInitialized()) return
            val now   = System.nanoTime()
            val dtSec = ((now - lastNano) / 1_000_000_000.0f).coerceIn(0f, 0.05f)
            lastNano  = now
            Engine.nativeFrame(dtSec)
        }
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// Activity
// ══════════════════════════════════════════════════════════════════════════════
@SuppressLint("ClickableViewAccessibility")
class Activity : Activity() {

    // ── Core ────────────────────────────────────────────────────────────────
    private lateinit var engine: Engine
    private lateinit var glView: Render
    private lateinit var prefs: SharedPreferences

    // ── Overlay root ────────────────────────────────────────────────────────
    private lateinit var overlayLayout: FrameLayout

    // ── HUD controls ────────────────────────────────────────────────────────
    private lateinit var joystickBase: View
    private lateinit var joystickKnob: View
    private lateinit var btnJump: TextView
    private lateinit var btnBreak: TextView
    private lateinit var btnPlace: TextView
    private lateinit var btnSneak: TextView
    private lateinit var btnSprint: TextView
    private lateinit var btnFlyUp: TextView
    private lateinit var btnFlyDown: TextView
    private lateinit var btnInventory: TextView
    private lateinit var btnChat: TextView
    private lateinit var btnSettings: TextView
    private lateinit var hotbarLayout: LinearLayout

    // ── Chat ────────────────────────────────────────────────────────────────
    private lateinit var chatContainer: LinearLayout
    private lateinit var chatMessages: TextView
    private lateinit var chatInput: EditText
    private lateinit var btnSendChat: Button

    // ── Panels ──────────────────────────────────────────────────────────────
    private lateinit var inventoryPanel: LinearLayout
    private lateinit var settingsPanel: LinearLayout
    private lateinit var mainMenuPanel: FrameLayout
    private lateinit var worldListPanel: LinearLayout

    // ── Settings widgets ────────────────────────────────────────────────────
    private lateinit var sensitivitySeekBar: SeekBar
    private lateinit var sensitivityLabel: TextView

    // ── Joystick state ──────────────────────────────────────────────────────
    private var joystickPointerId = -1
    private var cameraPointerId   = -1
    private var joystickBaseX = 0f
    private var joystickBaseY = 0f
    private val joystickRadius   = 120f
    private val joystickKnobSize = 60f

    // ── UI update loop ──────────────────────────────────────────────────────
    private val handler = Handler(Looper.getMainLooper())
    private val uiUpdateRunnable = object : Runnable {
        override fun run() {
            updateHUD()
            handler.postDelayed(this, 100)
        }
    }

    // ── Hotbar ──────────────────────────────────────────────────────────────
    private val hotbarSlots = Array(9) { TextView(this) }
    private var selectedSlot = 0
    private var sensitivity  = 0.3f
    private var sprintToggle = false
    private var gameStarted  = false

    // ════════════════════════════════════════════════════════════════════════
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        requestWindowFeature(Window.FEATURE_NO_TITLE)
        window.setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        )
        applyImmersive()

        prefs       = getSharedPreferences("omnicraft_prefs", MODE_PRIVATE)
        sensitivity = prefs.getFloat("sensitivity", 0.3f)
        engine      = Engine(this)
        glView      = Render(this, engine)

        val root = FrameLayout(this)
        root.addView(glView, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))

        overlayLayout = FrameLayout(this)
        root.addView(overlayLayout, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))

        buildHUD()
        buildMainMenu()
        setContentView(root)

        setHudVisible(false)
        mainMenuPanel.isVisible = true
    }

    // ════════════════════════════════════════════════════════════════════════
    // dp helpers
    // ════════════════════════════════════════════════════════════════════════
    private fun dp(v: Float): Int   = (v * resources.displayMetrics.density + 0.5f).toInt()
    private fun dpf(v: Float): Float = v * resources.displayMetrics.density

    // ════════════════════════════════════════════════════════════════════════
    // Ana Menü
    // ════════════════════════════════════════════════════════════════════════
    private fun buildMainMenu() {
        mainMenuPanel = FrameLayout(this).apply {
            setBackgroundColor(Color.argb(245, 10, 10, 20))
        }

        val title = TextView(this).apply {
            text = getString(R.string.app_name)
            textSize = 48f
            setTextColor(Color.WHITE)
            gravity = Gravity.CENTER
            setShadowLayer(8f, 4f, 4f, Color.argb(180, 0, 200, 255))
            layoutParams = FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(80f)
            ).apply { topMargin = dp(60f) }
        }
        mainMenuPanel.addView(title)

        val subtitle = TextView(this).apply {
            text = getString(R.string.menu_tagline)
            textSize = 14f
            setTextColor(Color.argb(200, 150, 200, 255))
            gravity = Gravity.CENTER
            layoutParams = FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(30f)
            ).apply { topMargin = dp(148f) }
        }
        mainMenuPanel.addView(subtitle)

        val btnContainer = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
            layoutParams = FrameLayout.LayoutParams(dp(300f), ViewGroup.LayoutParams.WRAP_CONTENT)
                .apply { gravity = Gravity.CENTER }
        }

        val btnPlay    = buildMenuButton(getString(R.string.menu_play),    Color.argb(220, 40, 140, 40))
        val btnWorlds  = buildMenuButton(getString(R.string.menu_worlds),  Color.argb(220, 40, 80,  160))
        val btnSetMenu = buildMenuButton(getString(R.string.btn_settings), Color.argb(220, 80, 80,  80))
        val btnQuit    = buildMenuButton(getString(R.string.menu_quit),    Color.argb(220, 160, 40, 40))

        btnPlay.setOnClickListener {
            val seed = prefs.getLong("world_seed", System.currentTimeMillis())
            val name = prefs.getString("world_name", getString(R.string.world_default_name))
                ?: getString(R.string.world_default_name)
            startGame(seed, name)
        }
        btnWorlds.setOnClickListener  { mainMenuPanel.isVisible = false; showWorldListPanel() }
        btnSetMenu.setOnClickListener { mainMenuPanel.isVisible = false; settingsPanel.isVisible = true }
        btnQuit.setOnClickListener    { finishAffinity() }

        for (btn in listOf(btnPlay, btnWorlds, btnSetMenu, btnQuit)) {
            btnContainer.addView(btn, LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, dp(56f)
            ).apply { setMargins(0, dp(10f), 0, 0) })
        }
        mainMenuPanel.addView(btnContainer)

        val ver = TextView(this).apply {
            text = getString(R.string.menu_version, "1.0.0")
            textSize = 10f
            setTextColor(Color.GRAY)
            gravity = Gravity.END or Gravity.BOTTOM
            setPadding(0, 0, dp(12f), dp(8f))
            layoutParams = FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT)
        }
        mainMenuPanel.addView(ver)

        overlayLayout.addView(mainMenuPanel, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
    }

    private fun buildMenuButton(label: String, bgColor: Int) = Button(this).apply {
        text = label; textSize = 18f; setTextColor(Color.WHITE); setBackgroundColor(bgColor)
    }

    // ════════════════════════════════════════════════════════════════════════
    // Dünya Listesi
    // ════════════════════════════════════════════════════════════════════════
    private fun showWorldListPanel() {
        val worlds = mutableListOf<Pair<Long, String>>()
        val count  = prefs.getInt("world_count", 0)
        for (i in 0 until count) {
            val s = prefs.getLong("world_seed_$i", 0L)
            val n = prefs.getString("world_name_$i",
                "${getString(R.string.world_default_name)} ${i + 1}") ?: ""
            if (s != 0L) worlds.add(s to n)
        }

        worldListPanel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(245, 10, 10, 20))
            setPadding(dp(20f), dp(30f), dp(20f), dp(20f))
        }

        // Başlık satırı
        val titleRow = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL; gravity = Gravity.CENTER_VERTICAL
        }
        titleRow.addView(TextView(this).apply {
            text = getString(R.string.menu_worlds); textSize = 24f; setTextColor(Color.WHITE)
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
        })
        titleRow.addView(Button(this).apply {
            text = getString(R.string.menu_back); textSize = 13f
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(200, 100, 50, 50))
            setOnClickListener { worldListPanel.isVisible = false; mainMenuPanel.isVisible = true }
        }, LinearLayout.LayoutParams(dp(100f), dp(44f)))
        worldListPanel.addView(titleRow)

        // Dünya listesi
        val scroll = ScrollView(this)
        val container = LinearLayout(this).apply { orientation = LinearLayout.VERTICAL }
        if (worlds.isEmpty()) {
            container.addView(TextView(this).apply {
                text = getString(R.string.world_no_worlds); textSize = 14f
                setTextColor(Color.GRAY); gravity = Gravity.CENTER
                setPadding(0, dp(40f), 0, 0)
            })
        } else {
            for ((seed, name) in worlds) container.addView(buildWorldRow(seed, name))
        }
        scroll.addView(container)
        worldListPanel.addView(scroll, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))

        // Yeni dünya butonu
        worldListPanel.addView(Button(this).apply {
            text = getString(R.string.world_new); textSize = 16f
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(220, 40, 130, 40))
            setOnClickListener { showNewWorldDialog() }
        }, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(52f)).apply { topMargin = dp(12f) })

        overlayLayout.addView(worldListPanel, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
    }

    private fun buildWorldRow(seed: Long, name: String) = LinearLayout(this).apply {
        orientation = LinearLayout.HORIZONTAL
        setBackgroundColor(Color.argb(150, 30, 40, 60))
        setPadding(dp(12f), dp(10f), dp(12f), dp(10f))
        val lp = LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
        lp.setMargins(0, dp(6f), 0, 0); layoutParams = lp

        val info = LinearLayout(this@Activity).apply { orientation = LinearLayout.VERTICAL
            layoutParams = LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f) }
        info.addView(TextView(this@Activity).apply {
            text = name; textSize = 16f; setTextColor(Color.WHITE) })
        info.addView(TextView(this@Activity).apply {
            text = getString(R.string.world_seed_label, seed); textSize = 11f; setTextColor(Color.GRAY) })
        addView(info)
        addView(Button(this@Activity).apply {
            text = getString(R.string.world_load); textSize = 12f
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(200, 40, 100, 180))
            setOnClickListener { startGame(seed, name) }
        }, LinearLayout.LayoutParams(dp(90f), dp(44f)))
    }

    private fun showNewWorldDialog() {
        val layout = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(20f), dp(20f), dp(20f), dp(20f))
            setBackgroundColor(Color.argb(240, 20, 20, 35))
        }
        val dialog = AlertDialog.Builder(this).setView(layout).create()

        val nameInput = EditText(this).apply {
            hint = getString(R.string.world_name_hint); setHintTextColor(Color.GRAY)
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(180, 40, 40, 60))
            inputType = InputType.TYPE_CLASS_TEXT
            setPadding(dp(8f), dp(8f), dp(8f), dp(8f))
        }
        val seedInput = EditText(this).apply {
            hint = getString(R.string.world_seed_hint); setHintTextColor(Color.GRAY)
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(180, 40, 40, 60))
            inputType = InputType.TYPE_CLASS_NUMBER or InputType.TYPE_NUMBER_FLAG_SIGNED
            setPadding(dp(8f), dp(8f), dp(8f), dp(8f))
        }

        val btnRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL; gravity = Gravity.END }
        btnRow.addView(Button(this).apply {
            text = getString(R.string.menu_back)
            setBackgroundColor(Color.argb(200, 120, 50, 50)); setTextColor(Color.WHITE)
            setOnClickListener { dialog.dismiss() }
        }, LinearLayout.LayoutParams(dp(100f), dp(44f)).apply { rightMargin = dp(8f) })
        btnRow.addView(Button(this).apply {
            text = getString(R.string.world_create)
            setBackgroundColor(Color.argb(200, 40, 130, 40)); setTextColor(Color.WHITE)
            setOnClickListener {
                val n = nameInput.text.toString().trim()
                    .ifEmpty { getString(R.string.world_default_name) }
                val s = seedInput.text.toString().toLongOrNull() ?: System.currentTimeMillis()
                saveNewWorld(s, n); dialog.dismiss()
                worldListPanel.isVisible = false
                (worldListPanel.parent as? ViewGroup)?.removeView(worldListPanel)
                showWorldListPanel()
            }
        }, LinearLayout.LayoutParams(dp(100f), dp(44f)))

        layout.addView(TextView(this).apply {
            text = getString(R.string.world_new); textSize = 20f
            setTextColor(Color.WHITE); gravity = Gravity.CENTER })
        layout.addView(nameInput, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(50f)).apply { topMargin = dp(12f) })
        layout.addView(seedInput, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(50f)).apply { topMargin = dp(8f) })
        layout.addView(btnRow, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            .apply { topMargin = dp(16f) })
        dialog.show()
    }

    private fun saveNewWorld(seed: Long, name: String) {
        val count = prefs.getInt("world_count", 0)
        prefs.edit()
            .putLong("world_seed_$count", seed)
            .putString("world_name_$count", name)
            .putInt("world_count", count + 1)
            .putLong("world_seed", seed)
            .putString("world_name", name)
            .apply()
    }

    // ════════════════════════════════════════════════════════════════════════
    // Oyun Başlat
    // ════════════════════════════════════════════════════════════════════════
    private fun startGame(seed: Long, worldName: String) {
        prefs.edit().putLong("world_seed", seed).putString("world_name", worldName).apply()
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
        glView.onResume()
        handler.post(uiUpdateRunnable)
    }

    private fun setHudVisible(visible: Boolean) {
        joystickBase.isVisible   = visible
        joystickKnob.isVisible   = visible
        btnJump.isVisible        = visible
        btnBreak.isVisible       = visible
        btnPlace.isVisible       = visible
        btnSneak.isVisible       = visible
        btnSprint.isVisible      = visible
        btnFlyUp.isVisible       = visible
        btnFlyDown.isVisible     = visible
        btnInventory.isVisible   = visible
        btnChat.isVisible        = visible
        btnSettings.isVisible    = visible
        hotbarLayout.isVisible   = visible
    }

    // ════════════════════════════════════════════════════════════════════════
    // HUD Build
    // ════════════════════════════════════════════════════════════════════════
    private fun buildHUD() {
        val sw    = resources.displayMetrics.widthPixels
        val sh    = resources.displayMetrics.heightPixels
        val jbSize = dp(joystickRadius * 2)
        val jkSize = dp(joystickKnobSize * 2)

        // Joystick taban
        joystickBase = View(this).apply { setBackgroundColor(Color.argb(50, 200, 200, 200)) }
        overlayLayout.addView(joystickBase, FrameLayout.LayoutParams(jbSize, jbSize).apply {
            leftMargin = dp(40f); bottomMargin = dp(120f)
            gravity = Gravity.BOTTOM or Gravity.START
        })

        // Joystick knob – başlangıç pozisyonu: tabanın merkezi
        joystickKnob = View(this).apply { setBackgroundColor(Color.argb(120, 255, 255, 255)) }
        overlayLayout.addView(joystickKnob, FrameLayout.LayoutParams(jkSize, jkSize).apply {
            leftMargin   = dp(40f) + jbSize / 2 - jkSize / 2
            bottomMargin = dp(120f) + jbSize / 2 - jkSize / 2
            gravity = Gravity.BOTTOM or Gravity.START
        })

        // Aksiyon butonları
        btnJump = buildIconBtn("⬆", Color.argb(150, 100, 180, 100)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(70f), dp(70f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(200f); rightMargin = dp(30f)
            })
        }
        btnBreak = buildIconBtn("⛏", Color.argb(150, 200, 80, 80)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(80f), dp(80f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(120f); rightMargin = dp(120f)
            })
        }
        btnPlace = buildIconBtn("🏗", Color.argb(150, 80, 80, 200)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(80f), dp(80f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(120f); rightMargin = dp(30f)
            })
        }

        val sneakLeft = dp(40f) + jbSize + dp(10f)
        btnSneak = buildIconBtn("⬇", Color.argb(130, 180, 160, 80)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(60f), dp(60f)).apply {
                gravity = Gravity.BOTTOM or Gravity.START
                bottomMargin = dp(120f); leftMargin = sneakLeft
            })
        }
        btnSprint = buildIconBtn("🏃", Color.argb(130, 180, 100, 180)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(60f), dp(60f)).apply {
                gravity = Gravity.BOTTOM or Gravity.START
                bottomMargin = dp(190f); leftMargin = sneakLeft
            })
        }
        btnFlyUp = buildIconBtn("↑↑", Color.argb(130, 100, 200, 200)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(280f); rightMargin = dp(30f)
            })
        }
        btnFlyDown = buildIconBtn("↓↓", Color.argb(130, 200, 200, 100)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.BOTTOM or Gravity.END
                bottomMargin = dp(215f); rightMargin = dp(30f)
            })
        }

        // Üst toolbar
        btnInventory = buildIconBtn("🎒", Color.argb(150, 150, 120, 80)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.TOP or Gravity.END
                topMargin = dp(10f); rightMargin = dp(75f)
            })
        }
        btnChat = buildIconBtn("💬", Color.argb(150, 80, 130, 200)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.TOP or Gravity.END
                topMargin = dp(10f); rightMargin = dp(140f)
            })
        }
        btnSettings = buildIconBtn("⚙", Color.argb(150, 130, 130, 130)).also {
            overlayLayout.addView(it, FrameLayout.LayoutParams(dp(55f), dp(55f)).apply {
                gravity = Gravity.TOP or Gravity.END
                topMargin = dp(10f); rightMargin = dp(10f)
            })
        }

        // Hotbar
        hotbarLayout = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        for (i in 0..8) {
            val slot = TextView(this).apply {
                setBackgroundColor(if (i == 0) Color.argb(180, 255, 255, 100)
                                   else Color.argb(100, 50, 50, 50))
                setTextColor(Color.WHITE); textSize = 10f; gravity = Gravity.CENTER; text = ""
                setPadding(2, 2, 2, 2)
            }
            hotbarSlots[i] = slot
            hotbarLayout.addView(slot, LinearLayout.LayoutParams(0, dp(55f), 1f))
        }
        overlayLayout.addView(hotbarLayout, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(55f)).apply {
            gravity = Gravity.BOTTOM; bottomMargin = dp(60f)
        })

        // Chat
        chatContainer = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL; isVisible = false
        }
        chatMessages = TextView(this).apply {
            setTextColor(Color.WHITE); textSize = 11f
            setBackgroundColor(Color.argb(120, 0, 0, 0))
            setPadding(dp(6f), dp(4f), dp(6f), dp(4f)); text = ""
        }
        val chatInputRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        chatInput = EditText(this).apply {
            hint = getString(R.string.chat_hint); setHintTextColor(Color.GRAY)
            setTextColor(Color.WHITE); setBackgroundColor(Color.argb(180, 30, 30, 30))
            inputType = InputType.TYPE_CLASS_TEXT; imeOptions = EditorInfo.IME_ACTION_SEND
            setPadding(dp(8f), dp(6f), dp(8f), dp(6f))
        }
        btnSendChat = Button(this).apply {
            text = getString(R.string.chat_send)
            setBackgroundColor(Color.argb(200, 50, 150, 50)); setTextColor(Color.WHITE)
        }
        chatInputRow.addView(chatInput,
            LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        chatInputRow.addView(btnSendChat,
            LinearLayout.LayoutParams(dp(50f), ViewGroup.LayoutParams.WRAP_CONTENT))
        chatContainer.addView(chatMessages,
            LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, 0, 1f))
        chatContainer.addView(chatInputRow)
        overlayLayout.addView(chatContainer, FrameLayout.LayoutParams(sw * 2 / 3, dp(250f)).apply {
            gravity = Gravity.BOTTOM or Gravity.START
            bottomMargin = dp(120f); leftMargin = dp(10f)
        })

        // Paneller
        inventoryPanel = buildInventoryPanel()
        overlayLayout.addView(inventoryPanel, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        inventoryPanel.isVisible = false

        settingsPanel = buildSettingsPanel()
        overlayLayout.addView(settingsPanel, FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT))
        settingsPanel.isVisible = false

        wireListeners()
    }

    private fun buildIconBtn(label: String, bgColor: Int) = TextView(this).apply {
        text = label; textSize = 22f; gravity = Gravity.CENTER
        setTextColor(Color.WHITE); setBackgroundColor(bgColor)
    }

    // ════════════════════════════════════════════════════════════════════════
    // Envanter Paneli
    // ════════════════════════════════════════════════════════════════════════
    private fun buildInventoryPanel(): LinearLayout {
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(220, 20, 20, 20))
            setPadding(dp(16f), dp(16f), dp(16f), dp(16f))
            gravity = Gravity.CENTER
        }
        panel.addView(TextView(this).apply {
            text = getString(R.string.inventory_title)
            textSize = 18f; setTextColor(Color.WHITE); gravity = Gravity.CENTER
        })

        val grid = GridLayout(this).apply { columnCount = 9; rowCount = 4 }
        for (r in 0..3) for (c in 0..8) {
            grid.addView(TextView(this).apply {
                setBackgroundColor(Color.argb(150, 60, 60, 60))
                setTextColor(Color.WHITE); textSize = 9f; gravity = Gravity.CENTER
                text = ""; setPadding(4, 4, 4, 4)
            }, GridLayout.LayoutParams().apply { width = dp(40f); height = dp(40f); setMargins(2, 2, 2, 2) })
        }
        panel.addView(grid)

        panel.addView(TextView(this).apply {
            text = getString(R.string.crafting_title)
            textSize = 14f; setTextColor(Color.LTGRAY); gravity = Gravity.CENTER
            setPadding(0, dp(12f), 0, dp(4f))
        })

        val craftRow  = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL; gravity = Gravity.CENTER }
        val craftGrid = GridLayout(this).apply { columnCount = 3; rowCount = 3 }
        for (i in 0..8) {
            craftGrid.addView(EditText(this).apply {
                hint = getString(R.string.crafting_id_hint)
                setTextColor(Color.WHITE); setHintTextColor(Color.GRAY)
                setBackgroundColor(Color.argb(150, 80, 80, 80))
                textSize = 10f; inputType = InputType.TYPE_CLASS_NUMBER
                gravity = Gravity.CENTER; setPadding(2, 2, 2, 2)
            }, GridLayout.LayoutParams().apply { width = dp(38f); height = dp(38f); setMargins(2, 2, 2, 2) })
        }
        craftRow.addView(craftGrid)
        craftRow.addView(TextView(this).apply {
            text = "➤"; textSize = 20f; setTextColor(Color.WHITE)
            gravity = Gravity.CENTER; setPadding(dp(8f), 0, dp(8f), 0)
        })
        craftRow.addView(TextView(this).apply {
            setBackgroundColor(Color.argb(180, 100, 160, 100))
            setTextColor(Color.WHITE); textSize = 12f; gravity = Gravity.CENTER; text = "?"
        })
        panel.addView(craftRow)

        panel.addView(Button(this).apply {
            text = getString(R.string.crafting_button)
            setBackgroundColor(Color.argb(200, 60, 140, 60)); setTextColor(Color.WHITE)
            setOnClickListener {
                val ids = Array(9) { i ->
                    val child = craftGrid.getChildAt(i)
                    if (child is EditText) child.text.toString().toIntOrNull() ?: 0 else 0
                }
                glView.queueEvent { engine.craftItem(ids.toIntArray()) }
            }
        })
        panel.addView(Button(this).apply {
            text = getString(R.string.close_button)
            setBackgroundColor(Color.argb(200, 160, 50, 50)); setTextColor(Color.WHITE)
            setOnClickListener { inventoryPanel.isVisible = false }
        }, LinearLayout.LayoutParams(dp(150f), ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = dp(8f); gravity = Gravity.CENTER_HORIZONTAL
        })
        return panel
    }

    // ════════════════════════════════════════════════════════════════════════
    // Ayarlar Paneli
    // ════════════════════════════════════════════════════════════════════════
    private fun buildSettingsPanel(): LinearLayout {
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setBackgroundColor(Color.argb(230, 15, 15, 15))
            setPadding(dp(30f), dp(40f), dp(30f), dp(40f))
            gravity = Gravity.TOP or Gravity.CENTER_HORIZONTAL
        }
        panel.addView(TextView(this).apply {
            text = getString(R.string.settings_title)
            textSize = 22f; setTextColor(Color.WHITE); gravity = Gravity.CENTER
        })

        sensitivityLabel = TextView(this).apply {
            text = "${getString(R.string.sensitivity_label)}: ${String.format("%.2f", sensitivity)}"
            textSize = 14f; setTextColor(Color.LTGRAY)
            setPadding(0, dp(20f), 0, dp(6f))
        }
        panel.addView(sensitivityLabel)

        sensitivitySeekBar = SeekBar(this).apply {
            max = 200; progress = (sensitivity * 100).toInt()
            setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
                override fun onProgressChanged(sb: SeekBar?, p: Int, user: Boolean) {
                    sensitivity = p / 100f + 0.05f
                    sensitivityLabel.text = "${getString(R.string.sensitivity_label)}: ${String.format("%.2f", sensitivity)}"
                    glView.queueEvent { engine.onSetSensitivity(sensitivity) }
                }
                override fun onStartTrackingTouch(sb: SeekBar?) {}
                override fun onStopTrackingTouch(sb: SeekBar?) {
                    prefs.edit().putFloat("sensitivity", sensitivity).apply()
                }
            })
        }
        panel.addView(sensitivitySeekBar, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        panel.addView(TextView(this).apply {
            text = getString(R.string.settings_gamemode); textSize = 16f; setTextColor(Color.WHITE)
            setPadding(0, dp(20f), 0, dp(8f))
        })
        val gmRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        listOf(
            getString(R.string.gamemode_survival)  to "/gamemode survival",
            getString(R.string.gamemode_creative)  to "/gamemode creative",
            getString(R.string.gamemode_adventure) to "/gamemode adventure",
            getString(R.string.gamemode_spectator) to "/gamemode spectator"
        ).forEach { (label, cmd) ->
            gmRow.addView(Button(this).apply {
                text = label; textSize = 11f
                setBackgroundColor(Color.argb(200, 50, 80, 120)); setTextColor(Color.WHITE)
                setOnClickListener { glView.queueEvent { engine.onSendChat(cmd) }; settingsPanel.isVisible = false }
            }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
                .apply { setMargins(4, 0, 4, 0) })
        }
        panel.addView(gmRow, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT))

        panel.addView(Button(this).apply {
            text = getString(R.string.world_new)
            setBackgroundColor(Color.argb(200, 60, 120, 60)); setTextColor(Color.WHITE)
            setOnClickListener {
                val newSeed = System.currentTimeMillis()
                val newName = "${getString(R.string.world_default_name)} ${prefs.getInt("world_count", 0) + 1}"
                saveNewWorld(newSeed, newName)
                glView.queueEvent { engine.destroy() }
                settingsPanel.isVisible = false
                startGame(newSeed, newName)
            }
        }, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            .apply { topMargin = dp(16f) })

        panel.addView(Button(this).apply {
            text = getString(R.string.settings_main_menu)
            setBackgroundColor(Color.argb(200, 120, 80, 20)); setTextColor(Color.WHITE)
            setOnClickListener {
                settingsPanel.isVisible = false
                if (gameStarted) {
                    glView.onPause()
                    handler.removeCallbacks(uiUpdateRunnable)
                    gameStarted = false
                    setHudVisible(false)
                    chatContainer.isVisible = false
                    inventoryPanel.isVisible = false
                }
                mainMenuPanel.isVisible = true
            }
        }, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT)
            .apply { topMargin = dp(8f) })

        panel.addView(Button(this).apply {
            text = getString(R.string.close_button)
            setBackgroundColor(Color.argb(200, 160, 50, 50)); setTextColor(Color.WHITE)
            setOnClickListener { settingsPanel.isVisible = false }
        }, LinearLayout.LayoutParams(dp(200f), ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            topMargin = dp(12f); gravity = Gravity.CENTER_HORIZONTAL
        })
        return panel
    }

    // ════════════════════════════════════════════════════════════════════════
    // Listener bağlantıları
    // ════════════════════════════════════════════════════════════════════════
    @SuppressLint("ClickableViewAccessibility")
    private fun wireListeners() {
        btnJump.setOnTouchListener { _, ev ->
            if (ev.actionMasked == MotionEvent.ACTION_DOWN) glView.queueEvent { engine.onJumpPress() }
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
        btnSprint.setOnClickListener {
            sprintToggle = !sprintToggle
            glView.queueEvent { engine.onSprint(sprintToggle) }
            btnSprint.setBackgroundColor(
                if (sprintToggle) Color.argb(200, 220, 80, 220) else Color.argb(130, 180, 100, 180))
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
        btnSendChat.setOnClickListener { sendChatMessage() }
        chatInput.setOnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_SEND) { sendChatMessage(); true } else false
        }
        for (i in 0..8) {
            val idx = i
            hotbarSlots[i].setOnClickListener {
                selectedSlot = idx
                glView.queueEvent { engine.onSelectSlot(idx) }
                updateHotbarHighlight()
            }
        }
        wireCameraAndJoystick()
    }

    private fun sendChatMessage() {
        val msg = chatInput.text.toString().trim()
        if (msg.isNotEmpty()) { glView.queueEvent { engine.onSendChat(msg) }; chatInput.text.clear() }
    }

    private fun updateHotbarHighlight() {
        for (i in 0..8) hotbarSlots[i].setBackgroundColor(
            if (i == selectedSlot) Color.argb(200, 255, 255, 100) else Color.argb(100, 50, 50, 50))
    }

    // ════════════════════════════════════════════════════════════════════════
    // Kamera + Joystick dokunma
    // ════════════════════════════════════════════════════════════════════════
    @SuppressLint("ClickableViewAccessibility")
    private fun wireCameraAndJoystick() {
        val sh        = resources.displayMetrics.heightPixels.toFloat()
        val jbSize    = dpf(joystickRadius * 2)
        joystickBaseX = dpf(40f) + dpf(joystickRadius)
        joystickBaseY = sh - dpf(120f) - jbSize + dpf(joystickRadius)
        val camZoneLeft = resources.displayMetrics.widthPixels / 2f

        overlayLayout.setOnTouchListener { _, event ->
            if (!gameStarted) return@setOnTouchListener false
            val action = event.actionMasked
            val pIdx   = event.actionIndex
            val pId    = event.getPointerId(pIdx)
            val rawX   = event.getX(pIdx)
            val rawY   = event.getY(pIdx)

            when (action) {
                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                    if (rawX < camZoneLeft && joystickPointerId < 0) {
                        joystickPointerId = pId; moveJoystick(rawX, rawY)
                    } else if (rawX >= camZoneLeft && cameraPointerId < 0) {
                        cameraPointerId = pId
                    }
                }
                MotionEvent.ACTION_MOVE -> {
                    for (i in 0 until event.pointerCount) {
                        val id = event.getPointerId(i)
                        val x  = event.getX(i); val y = event.getY(i)
                        if (id == joystickPointerId) {
                            moveJoystick(x, y)
                        } else if (id == cameraPointerId) {
                            val hist  = event.historySize
                            val prevX = if (hist > 0) event.getHistoricalX(i, hist - 1) else x
                            val prevY = if (hist > 0) event.getHistoricalY(i, hist - 1) else y
                            glView.queueEvent { engine.onCameraDrag((x - prevX) * sensitivity, (y - prevY) * sensitivity) }
                        }
                    }
                }
                MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_CANCEL -> {
                    if (pId == joystickPointerId) { joystickPointerId = -1; resetJoystick() }
                    else if (pId == cameraPointerId) cameraPointerId = -1
                }
            }
            true
        }
    }

    private fun moveJoystick(rawX: Float, rawY: Float) {
        val dx   = rawX - joystickBaseX
        val dy   = rawY - joystickBaseY
        val dist = sqrt(dx * dx + dy * dy)
        val maxR = dpf(joystickRadius)
        val nx   = if (dist > maxR) dx / dist else dx / maxR
        val ny   = if (dist > maxR) dy / dist else dy / maxR

        // BUG FIX: translationY kullandı kx'i, şimdi doğru: kx→X, ky→Y
        val kx = joystickBaseX + nx * maxR - dpf(joystickKnobSize)
        val ky = joystickBaseY + ny * maxR - dpf(joystickKnobSize)
        val baseOffX = dpf(40f) + dpf(joystickRadius) - dpf(joystickKnobSize)
        val sh       = resources.displayMetrics.heightPixels.toFloat()
        val baseOffY = sh - dpf(120f) - dpf(joystickRadius * 2) + dpf(joystickRadius) - dpf(joystickKnobSize)

        joystickKnob.translationX = kx - baseOffX
        joystickKnob.translationY = ky - baseOffY

        glView.queueEvent { engine.onJoystickMove(nx, ny) }
    }

    private fun resetJoystick() {
        joystickKnob.translationX = 0f; joystickKnob.translationY = 0f
        glView.queueEvent { engine.onJoystickMove(0f, 0f) }
    }

    // ════════════════════════════════════════════════════════════════════════
    // HUD güncelleme (100ms döngü)
    // ════════════════════════════════════════════════════════════════════════
    private fun updateHUD() {
        if (!engine.isInitialized()) return
        try {
            chatMessages.text = engine.getChatLog().trim()
            val inv = JSONArray(engine.getInventory())
            for (i in 0 until minOf(9, inv.length())) {
                val slot  = inv.getJSONObject(i)
                val id    = slot.getInt("id")
                val count = slot.getInt("count")
                hotbarSlots[i].text = if (id > 0) "$id\n×$count" else ""
            }
        } catch (_: Exception) {}
    }

    // ════════════════════════════════════════════════════════════════════════
    // Lifecycle
    // ════════════════════════════════════════════════════════════════════════
    override fun onResume() {
        super.onResume()
        if (gameStarted) { glView.onResume(); handler.post(uiUpdateRunnable) }
    }

    override fun onPause() {
        super.onPause()
        if (gameStarted) { glView.onPause(); handler.removeCallbacks(uiUpdateRunnable) }
        prefs.edit().putFloat("sensitivity", sensitivity).apply()
    }

    override fun onDestroy() {
        super.onDestroy()
        glView.queueEvent { engine.destroy() }
    }

    override fun onWindowFocusChanged(hasFocus: Boolean) {
        super.onWindowFocusChanged(hasFocus)
        if (hasFocus) applyImmersive()
    }

    @Suppress("DEPRECATION")
    private fun applyImmersive() {
        window.decorView.systemUiVisibility = (
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            or View.SYSTEM_UI_FLAG_FULLSCREEN
        )
    }
}
