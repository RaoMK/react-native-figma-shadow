package com.figmashadow

import com.facebook.react.module.annotations.ReactModule
import com.facebook.react.uimanager.SimpleViewManager
import com.facebook.react.uimanager.ThemedReactContext
import com.facebook.react.uimanager.ViewManagerDelegate
import com.facebook.react.uimanager.annotations.ReactProp
import com.facebook.react.viewmanagers.FigmaShadowViewManagerDelegate
import com.facebook.react.viewmanagers.FigmaShadowViewManagerInterface

@ReactModule(name = FigmaShadowViewManager.NAME)
class FigmaShadowViewManager :
  SimpleViewManager<FigmaShadowView>(),
  FigmaShadowViewManagerInterface<FigmaShadowView> {

  private val delegate: ViewManagerDelegate<FigmaShadowView> =
    FigmaShadowViewManagerDelegate(this)

  override fun getDelegate(): ViewManagerDelegate<FigmaShadowView> = delegate

  override fun getName(): String = NAME

  override fun createViewInstance(context: ThemedReactContext): FigmaShadowView =
    FigmaShadowView(context)

  @ReactProp(name = "shadow")
  override fun setShadow(view: FigmaShadowView, value: String?) = view.setShadowSpec(value)

  @ReactProp(name = "fillColor")
  override fun setFillColor(view: FigmaShadowView, value: String?) = view.setFillColor(value)

  @ReactProp(name = "highQuality")
  override fun setHighQuality(view: FigmaShadowView, value: Boolean) = view.setHighQuality(value)

  @ReactProp(name = "pixelRatio")
  override fun setPixelRatio(view: FigmaShadowView, value: Float) {
    // Android derives density from resources; the JS-supplied value is unused.
  }

  @ReactProp(name = "borderTopLeftRadius", defaultFloat = 0f)
  override fun setBorderTopLeftRadius(view: FigmaShadowView, value: Float) =
    view.setRadiusTopLeft(value)

  @ReactProp(name = "borderTopRightRadius", defaultFloat = 0f)
  override fun setBorderTopRightRadius(view: FigmaShadowView, value: Float) =
    view.setRadiusTopRight(value)

  @ReactProp(name = "borderBottomRightRadius", defaultFloat = 0f)
  override fun setBorderBottomRightRadius(view: FigmaShadowView, value: Float) =
    view.setRadiusBottomRight(value)

  @ReactProp(name = "borderBottomLeftRadius", defaultFloat = 0f)
  override fun setBorderBottomLeftRadius(view: FigmaShadowView, value: Float) =
    view.setRadiusBottomLeft(value)

  @ReactProp(name = "bleedLeft", defaultFloat = 0f)
  override fun setBleedLeft(view: FigmaShadowView, value: Float) = view.setBleedLeft(value)

  @ReactProp(name = "bleedTop", defaultFloat = 0f)
  override fun setBleedTop(view: FigmaShadowView, value: Float) = view.setBleedTop(value)

  @ReactProp(name = "bleedRight", defaultFloat = 0f)
  override fun setBleedRight(view: FigmaShadowView, value: Float) = view.setBleedRight(value)

  @ReactProp(name = "bleedBottom", defaultFloat = 0f)
  override fun setBleedBottom(view: FigmaShadowView, value: Float) = view.setBleedBottom(value)

  companion object {
    const val NAME = "FigmaShadowView"
  }
}
