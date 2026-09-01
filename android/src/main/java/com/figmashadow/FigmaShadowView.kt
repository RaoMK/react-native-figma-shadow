package com.figmashadow

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.os.Handler
import android.os.Looper
import android.util.DisplayMetrics
import com.facebook.react.views.view.ReactViewGroup
import java.util.concurrent.Executors

/**
 * Container that paints a CSS-style shadow behind its children. Every pixel
 * (shadow layers plus the optional fill) is produced by the shared C++ core in
 * [nativeRenderShadow]; this view only blits the resulting bitmap.
 */
class FigmaShadowView(context: Context) : ReactViewGroup(context) {

  private val density: Float =
    context.resources.displayMetrics.densityDpi.toFloat() / DisplayMetrics.DENSITY_DEFAULT

  private var shadowSpec: String = ""
  private var fillSpec: String = ""
  private var radiusTopLeft = 0f
  private var radiusTopRight = 0f
  private var radiusBottomRight = 0f
  private var radiusBottomLeft = 0f
  private var bleedLeft = 0f
  private var bleedTop = 0f
  private var bleedRight = 0f
  private var bleedBottom = 0f
  private var highQuality = false

  private var shadowBitmap: Bitmap? = null
  private var dirty = true
  private var renderGeneration = 0
  private var renderInFlight = -1

  private val bitmapPaint = Paint(Paint.FILTER_BITMAP_FLAG)
  private val dstRect = RectF()

  init {
    // The shadow paints outside the padded content box.
    clipChildren = false
  }

  private fun invalidateShadow() {
    dirty = true
    invalidate()
  }

  fun setShadowSpec(value: String?) {
    val next = value ?: ""
    if (next != shadowSpec) { shadowSpec = next; invalidateShadow() }
  }

  fun setFillColor(value: String?) {
    val next = value ?: ""
    if (next != fillSpec) { fillSpec = next; invalidateShadow() }
  }

  fun setHighQuality(value: Boolean) {
    if (value != highQuality) { highQuality = value; invalidateShadow() }
  }

  fun setRadiusTopLeft(v: Float) { if (v != radiusTopLeft) { radiusTopLeft = v; invalidateShadow() } }
  fun setRadiusTopRight(v: Float) { if (v != radiusTopRight) { radiusTopRight = v; invalidateShadow() } }
  fun setRadiusBottomRight(v: Float) { if (v != radiusBottomRight) { radiusBottomRight = v; invalidateShadow() } }
  fun setRadiusBottomLeft(v: Float) { if (v != radiusBottomLeft) { radiusBottomLeft = v; invalidateShadow() } }

  fun setBleedLeft(v: Float) { if (v != bleedLeft) { bleedLeft = v; invalidateShadow() } }
  fun setBleedTop(v: Float) { if (v != bleedTop) { bleedTop = v; invalidateShadow() } }
  fun setBleedRight(v: Float) { if (v != bleedRight) { bleedRight = v; invalidateShadow() } }
  fun setBleedBottom(v: Float) { if (v != bleedBottom) { bleedBottom = v; invalidateShadow() } }

  override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
    super.onSizeChanged(w, h, oldw, oldh)
    dirty = true
  }

  /** Kicks off an off-thread raster; the current bitmap keeps showing until it lands. */
  private fun rebuildShadow() {
    dirty = false

    val contentW = (width - (bleedLeft + bleedRight) * density) / density
    val contentH = (height - (bleedTop + bleedBottom) * density) / density
    if (contentW <= 0f || contentH <= 0f || (shadowSpec.isEmpty() && fillSpec.isEmpty())) {
      shadowBitmap = null
      return
    }

    val generation = ++renderGeneration
    if (generation == renderInFlight) return
    renderInFlight = generation

    val spec = shadowSpec
    val fill = fillSpec
    val rtl = radiusTopLeft
    val rtr = radiusTopRight
    val rbr = radiusBottomRight
    val rbl = radiusBottomLeft
    val bl = bleedLeft
    val bt = bleedTop
    val br = bleedRight
    val bb = bleedBottom
    val hq = highQuality

    executor.execute {
      val bitmap = nativeRenderShadow(
        contentW, contentH, rtl, rtr, rbr, rbl, spec, fill, bl, bt, br, bb, density, hq,
      )
      mainHandler.post {
        renderInFlight = -1
        if (generation == renderGeneration) {
          shadowBitmap = bitmap
          invalidate()
        }
      }
    }
  }

  override fun dispatchDraw(canvas: Canvas) {
    if (dirty) rebuildShadow()

    shadowBitmap?.let { bmp ->
      dstRect.set(0f, 0f, width.toFloat(), height.toFloat())
      canvas.drawBitmap(bmp, null, dstRect, bitmapPaint)
    }

    super.dispatchDraw(canvas)
  }

  companion object {
    init {
      System.loadLibrary("react-native-figma-shadow")
    }

    private val executor = Executors.newSingleThreadExecutor { r ->
      Thread(r, "figma-shadow-raster").apply { priority = Thread.NORM_PRIORITY - 1 }
    }
    private val mainHandler = Handler(Looper.getMainLooper())

    @JvmStatic
    external fun nativeRenderShadow(
      contentWidth: Float,
      contentHeight: Float,
      radiusTopLeft: Float,
      radiusTopRight: Float,
      radiusBottomRight: Float,
      radiusBottomLeft: Float,
      boxShadow: String,
      fillColor: String,
      bleedLeft: Float,
      bleedTop: Float,
      bleedRight: Float,
      bleedBottom: Float,
      scale: Float,
      highQuality: Boolean,
    ): Bitmap?

    @JvmStatic
    external fun nativeClearCache()
  }
}
