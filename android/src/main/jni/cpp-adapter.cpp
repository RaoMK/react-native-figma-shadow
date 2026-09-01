#include <android/bitmap.h>
#include <jni.h>

#include <cstring>
#include <string>

#include "figmashadow/FigmaShadow.h"

namespace {

std::string jstringToStd(JNIEnv *env, jstring value) {
  if (value == nullptr) return {};
  const char *chars = env->GetStringUTFChars(value, nullptr);
  std::string out(chars ? chars : "");
  if (chars) env->ReleaseStringUTFChars(value, chars);
  return out;
}

jobject createArgb8888Bitmap(JNIEnv *env, int width, int height) {
  jclass bitmapClass = env->FindClass("android/graphics/Bitmap");
  jclass configClass = env->FindClass("android/graphics/Bitmap$Config");
  jfieldID argb8888Field = env->GetStaticFieldID(
      configClass, "ARGB_8888", "Landroid/graphics/Bitmap$Config;");
  jobject config = env->GetStaticObjectField(configClass, argb8888Field);
  jmethodID createBitmap = env->GetStaticMethodID(
      bitmapClass, "createBitmap",
      "(IILandroid/graphics/Bitmap$Config;)Landroid/graphics/Bitmap;");
  return env->CallStaticObjectMethod(bitmapClass, createBitmap, width, height, config);
}

}  // namespace

extern "C" JNIEXPORT jobject JNICALL
Java_com_figmashadow_FigmaShadowView_nativeRenderShadow(
    JNIEnv *env, jclass /*clazz*/, jfloat contentWidth, jfloat contentHeight,
    jfloat radiusTopLeft, jfloat radiusTopRight, jfloat radiusBottomRight,
    jfloat radiusBottomLeft, jstring boxShadow, jstring fillColor, jfloat bleedLeft,
    jfloat bleedTop, jfloat bleedRight, jfloat bleedBottom, jfloat scale) {
  const std::string shadow = jstringToStd(env, boxShadow);
  const std::string fill = jstringToStd(env, fillColor);

  figmashadow::Bitmap bmp = figmashadow::render(
      contentWidth, contentHeight, radiusTopLeft, radiusTopRight,
      radiusBottomRight, radiusBottomLeft, shadow, fill, bleedLeft, bleedTop,
      bleedRight, bleedBottom, scale);

  if (bmp.empty()) return nullptr;

  jobject bitmap = createArgb8888Bitmap(env, bmp.width, bmp.height);
  if (bitmap == nullptr) return nullptr;

  void *pixels = nullptr;
  if (AndroidBitmap_lockPixels(env, bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS) {
    return nullptr;
  }

  // figmashadow::Bitmap is premultiplied RGBA8888; Android's ARGB_8888 is stored
  // in the same byte order (R,G,B,A) and premultiplied by default. Copy row by
  // row in case the Android bitmap uses a padded stride.
  AndroidBitmapInfo info;
  if (AndroidBitmap_getInfo(env, bitmap, &info) == ANDROID_BITMAP_RESULT_SUCCESS &&
      info.stride == static_cast<uint32_t>(bmp.width) * 4) {
    std::memcpy(pixels, bmp.pixels.data(), bmp.pixels.size());
  } else {
    const size_t rowBytes = static_cast<size_t>(bmp.width) * 4;
    auto *dst = static_cast<uint8_t *>(pixels);
    for (int y = 0; y < bmp.height; ++y) {
      std::memcpy(dst + static_cast<size_t>(y) * info.stride,
                  bmp.pixels.data() + static_cast<size_t>(y) * rowBytes, rowBytes);
    }
  }

  AndroidBitmap_unlockPixels(env, bitmap);
  return bitmap;
}

extern "C" JNIEXPORT void JNICALL
Java_com_figmashadow_FigmaShadowView_nativeClearCache(JNIEnv * /*env*/, jclass /*clazz*/) {
  figmashadow::clearCache();
}
