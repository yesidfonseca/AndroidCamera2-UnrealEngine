// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 Yesid Fonseca


#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
#include <android/log.h>

THIRD_PARTY_INCLUDES_START
#include "libyuv.h"
THIRD_PARTY_INCLUDES_END

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "NativeYUV_JNI", __VA_ARGS__)

extern "C" JNIEXPORT jint JNICALL
Java_com_FonseCode_camera2_NativeYuv_yuv420888ToI420(
        JNIEnv* env, jclass,
        jobject yBuf, jint yStride, jint yOffset,
        jobject uBuf, jint uStride, jint uPixStride, jint uOffset,
        jobject vBuf, jint vStride, jint vPixStride, jint vOffset,
        jobject dstY, jint dstYStride,
        jobject dstU, jint dstUStride,
        jobject dstV, jint dstVStride,
        jint width, jint height)
{
    const uint8_t* Y = (const uint8_t*)env->GetDirectBufferAddress(yBuf) + yOffset;
    const uint8_t* U = (const uint8_t*)env->GetDirectBufferAddress(uBuf) + uOffset;
    const uint8_t* V = (const uint8_t*)env->GetDirectBufferAddress(vBuf) + vOffset;

    uint8_t* DY = (uint8_t*)env->GetDirectBufferAddress(dstY);
    uint8_t* DU = (uint8_t*)env->GetDirectBufferAddress(dstU);
    uint8_t* DV = (uint8_t*)env->GetDirectBufferAddress(dstV);

    // libyuv hace todo: respeta rowStride y pixelStride de UV (1 ó 2).
    int r = libyuv::Android420ToI420(
        Y, yStride,
        U, uStride,
        V, uStride, vPixStride,
        DY, dstYStride,
        DU, dstUStride,
        DV, dstVStride,
        width, height);

    return r; // 0 = OK
}

// Opcional: si prefieres UV intercalado (NV12) para subir PF_R8G8 a UE:
extern "C" JNIEXPORT jint JNICALL
Java_com_FonseCode_camera2_NativeYuv_i420ToNv12(
        JNIEnv* env, jclass,
        jobject srcY, jint srcYStride,
        jobject srcU, jint srcUStride,
        jobject srcV, jint srcVStride,
        jobject dstY, jint dstYStride,
        jobject dstUV, jint dstUVStride,
        jint width, jint height)
{
    const uint8_t* SY = (const uint8_t*)env->GetDirectBufferAddress(srcY);
    const uint8_t* SU = (const uint8_t*)env->GetDirectBufferAddress(srcU);
    const uint8_t* SV = (const uint8_t*)env->GetDirectBufferAddress(srcV);
    uint8_t* DY = (uint8_t*)env->GetDirectBufferAddress(dstY);
    uint8_t* DUV = (uint8_t*)env->GetDirectBufferAddress(dstUV);

    // Copia Y tal cual
    libyuv::I420Copy(SY, srcYStride, SU, srcUStride, SV, srcVStride,
                     DY, dstYStride, /*dummy*/nullptr,0, /*dummy*/nullptr,0,
                     width, height);

    // Mezcla U y V en UV intercalado
    return libyuv::I420ToNV12(SY, dstYStride, SU, srcUStride, SV, srcVStride,
                              DY, dstYStride, DUV, dstUVStride,
                              width, height);
}


extern "C" JNIEXPORT jint JNICALL
Java_com_FonseCode_camera2_NativeYuv_I420ToARGB(
        JNIEnv* env, jclass,
        jobject yBuf,
        jobject uBuf,
        jobject vBuf,
        jint w, jint h,
        jobject outBuf)
{
    const uint8_t* Y = (const uint8_t*)env->GetDirectBufferAddress(yBuf) ;
    const uint8_t* U = (const uint8_t*)env->GetDirectBufferAddress(uBuf);
    const uint8_t* V = (const uint8_t*)env->GetDirectBufferAddress(vBuf);
    uint8_t* out = ( uint8_t*)env->GetDirectBufferAddress(outBuf);

    int bytes = libyuv::I420ToABGR(     Y, w,
                                        U, w/2,
                                        V,w/2,
                                        out, w*4,
                                        w,h);

    return bytes; 
}

extern "C" JNIEXPORT jint JNICALL
Java_com_FonseCode_camera2_NativeYuv_I420Rotate(JNIEnv* env, jclass,
        jobject yBuf,
        jobject uBuf,
        jobject vBuf,
        jint w, jint h,
        jobject yBufRot,
        jobject uBufRot,
        jobject vBufRot,
        jint wr, jint hr,
        jint Orientation
        )
{
    const uint8_t* Y = (const uint8_t*)env->GetDirectBufferAddress(yBuf) ;
    const uint8_t* U = (const uint8_t*)env->GetDirectBufferAddress(uBuf);
    const uint8_t* V = (const uint8_t*)env->GetDirectBufferAddress(vBuf);

    uint8_t* Yr = (uint8_t*)env->GetDirectBufferAddress(yBufRot) ;
    uint8_t* Ur = (uint8_t*)env->GetDirectBufferAddress(uBufRot);
    uint8_t* Vr = (uint8_t*)env->GetDirectBufferAddress(vBufRot);
    libyuv::RotationMode out;
    switch (Orientation) {
        case 0:   out = libyuv::kRotate0; break;
        case 1:  out = libyuv::kRotate90; break;
        case 2: out = libyuv::kRotate180; break;
        case 3: out = libyuv::kRotate270; break;
        default:  out = libyuv::kRotate0; break;
    }

    int r = libyuv::I420Rotate(Y,w, U, w/2, V, w/2, Yr, wr, Ur, wr/2, Vr, wr/2, w,h,out);

    return r;

}


