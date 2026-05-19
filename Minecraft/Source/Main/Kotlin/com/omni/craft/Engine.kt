package com.omni.craft

class Engine {
    companion object {
        init {
            System.loadLibrary("Core")
        }
    }

    external fun initializeEngine(): Boolean
    external fun resizeViewport(width: Int, height: Int)
    external fun stepFrame()
    external fun shutdownEngine()
    
    external fun look(dx: Float, dy: Float)
    external fun move(dir: Int)
    external fun interact()
}
