package com.figmashadow

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.os.Handler
import android.os.Looper
import android.util.DisplayMetrics
import com.facebook.react.views.view.ReactViewGroup
import java.util.concurrent.Executors

/**
 * Container that paints a CSS-style shadow behind its children. All shadow
 * geometry is produced by the shared C++ core (see [nativeRenderShadow]); this
 * view only blits the resulting bitmap.
 */
class FigmaShadowView(context: Context) : ReactViewGroup(context) {

  private val density: Float =
    context.resources.displayMetrics.densityDpi.toFloat() / DisplayMetrics.DENSITY_DEFAULT

  private var shadowSpec: String = ""
  private var backgroundSpec: String? = null
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
  private val fillPaint = Paint(Paint.ANTI_ALIAS_FLAG)
  private var hasFill = false
  private val fillPath = Path()
  private val fillRect = RectF()
  private val fillRadii = FloatArray(8)

  init {
    setWillNotDraw(false)
    // The shadow is drawn outside the padded content box.
    clipChildren = false
  }

  fun setShadowSpec(value: String?) {
    val next = value ?: ""
    if (next != shadowSpec) {
      shadowSpec = next
      dirty = true
      invalidate()
    }
  }

  fun setFillColor(value: String?) {
    if (value != backgroundSpec) {
      backgroundSpec = value
      resolveFill()
      invalidate()
    }
  }

  fun setHighQuality(value: Boolean) {
    if (value != highQuality) {
      highQuality = value
      dirty = true
      invalidate()
    }
  }

  private inline fun updateRadius(current: Float, next: Float, apply: (Float) -> Unit) {
    if (current != next) {
      apply(next)
      dirty = true
      invalidate()
    }
  }

  fun setRadiusTopLeft(v: Float) = updateRadius(radiusTopLeft, v) { radiusTopLeft = it }
  fun setRadiusTopRight(v: Float) = updateRadius(radiusTopRight, v) { radiusTopRight = it }
  fun setRadiusBottomRight(v: Float) = updateRadius(radiusBottomRight, v) { radiusBottomRight = it }
  fun setRadiusBottomLeft(v: Float) = updateRadius(radiusBottomLeft, v) { radiusBottomLeft = it }

  fun setBleedLeft(v: Float) = updateRadius(bleedLeft, v) { bleedLeft = it }
  fun setBleedTop(v: Float) = updateRadius(bleedTop, v) { bleedTop = it }
  fun setBleedRight(v: Float) = updateRadius(bleedRight, v) { bleedRight = it }
  fun setBleedBottom(v: Float) = updateRadius(bleedBottom, v) { bleedBottom = it }

  override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
    super.onSizeChanged(w, h, oldw, oldh)
    dirty = true
  }

  private fun contentWidthPx(): Float = width - (bleedLeft + bleedRight) * density
  private fun contentHeightPx(): Float = height - (bleedTop + bleedBottom) * density

  /** Kicks off an off-thread raster; the current bitmap keeps showing until it lands. */
  private fun rebuildShadow() {
    dirty = false

    val contentW = contentWidthPx() / density
    val contentH = contentHeightPx() / density
    if (contentW <= 0f || contentH <= 0f || shadowSpec.isEmpty()) {
      shadowBitmap = null
      return
    }

    val generation = ++renderGeneration
    if (generation == renderInFlight) return
    renderInFlight = generation

    val spec = shadowSpec
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
        contentW, contentH, rtl, rtr, rbr, rbl, spec, bl, bt, br, bb, density, hq,
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

  private fun resolveFill() {
    val spec = backgroundSpec
    val rgba = FloatArray(4)
    hasFill = spec != null && nativeParseColor(spec, rgba) && rgba[3] > 0f
    if (hasFill) {
      fillPaint.setARGB(
        (rgba[3] * 255).toInt(),
        (rgba[0] * 255).toInt(),
        (rgba[1] * 255).toInt(),
        (rgba[2] * 255).toInt(),
      )
    }
  }

  private fun updateFillPath() {
    val left = bleedLeft * density
    val top = bleedTop * density
    fillRect.set(left, top, left + contentWidthPx(), top + contentHeightPx())
    val tl = radiusTopLeft * density
    val tr = radiusTopRight * density
    val br = radiusBottomRight * density
    val bl = radiusBottomLeft * density
    fillRadii[0] = tl; fillRadii[1] = tl
    fillRadii[2] = tr; fillRadii[3] = tr
    fillRadii[4] = br; fillRadii[5] = br
    fillRadii[6] = bl; fillRadii[7] = bl
    fillPath.reset()
    fillPath.addRoundRect(fillRect, fillRadii, Path.Direction.CW)
  }

  override fun dispatchDraw(canvas: Canvas) {
    if (dirty) rebuildShadow()

    if (hasFill) {
      updateFillPath()
      canvas.drawPath(fillPath, fillPaint)
    }

    shadowBitmap?.let { bmp ->
      canvas.drawBitmap(bmp, null, RectF(0f, 0f, width.toFloat(), height.toFloat()), bitmapPaint)
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
      bleedLeft: Float,
      bleedTop: Float,
      bleedRight: Float,
      bleedBottom: Float,
      scale: Float,
      highQuality: Boolean,
    ): Bitmap?

    @JvmStatic
    external fun nativeParseColor(token: String, out: FloatArray): Boolean

    @JvmStatic
    external fun nativeClearCache()
  }
}
