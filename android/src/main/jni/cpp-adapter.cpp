#include <android/bitmap.h>
#include <jni.h>

#include <cstring>
#include <string>

#include "Color.h"
#include "FigmaShadow.h"

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
    jfloat radiusBottomLeft, jstring boxShadow, jfloat bleedLeft, jfloat bleedTop,
    jfloat bleedRight, jfloat bleedBottom, jfloat scale, jboolean highQuality) {
  const std::string shadow = jstringToStd(env, boxShadow);

  figmashadow::Bitmap bmp = figmashadow::render(
      contentWidth, contentHeight, radiusTopLeft, radiusTopRight,
      radiusBottomRight, radiusBottomLeft, shadow, bleedLeft, bleedTop, bleedRight,
      bleedBottom, scale, highQuality == JNI_TRUE);

  if (bmp.empty()) return nullptr;

  jobject bitmap = createArgb8888Bitmap(env, bmp.width, bmp.height);
  if (bitmap == nullptr) return nullptr;

  void *pixels = nullptr;
  if (AndroidBitmap_lockPixels(env, bitmap, &pixels) != ANDROID_BITMAP_RESULT_SUCCESS) {
    return nullptr;
  }
  // figmashadow::Bitmap is premultiplied RGBA8888; Android's ARGB_8888 is stored
  // in the same byte order (R,G,B,A) and premultiplied by default.
  std::memcpy(pixels, bmp.pixels.data(), bmp.pixels.size());
  AndroidBitmap_unlockPixels(env, bitmap);

  return bitmap;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_figmashadow_FigmaShadowView_nativeParseColor(JNIEnv *env, jclass /*clazz*/,
                                                      jstring token, jfloatArray out) {
  figmashadow::Color color;
  if (!figmashadow::parseColor(jstringToStd(env, token), color)) return JNI_FALSE;
  jfloat values[4] = {color.r, color.g, color.b, color.a};
  env->SetFloatArrayRegion(out, 0, 4, values);
  return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_com_figmashadow_FigmaShadowView_nativeClearCache(JNIEnv * /*env*/, jclass /*clazz*/) {
  figmashadow::clearCache();
}
