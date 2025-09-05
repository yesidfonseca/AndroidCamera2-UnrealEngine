/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025-2026 Yesid Fonseca
 */

package com.FonseCode.camera2;

import android.graphics.Rect;
import android.media.Image;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public final class NativeYuv {

  public static native int yuv420888ToI420(
      ByteBuffer y, int yStride, int yOffset,
      ByteBuffer u, int uStride, int uPixStride, int uOffset,
      ByteBuffer v, int vStride, int vPixStride, int vOffset,
      ByteBuffer dstY, int dstYStride,
      ByteBuffer dstU, int dstUStride,
      ByteBuffer dstV, int dstVStride,
      int width, int height);

  public static native int i420ToNv12(
      ByteBuffer srcY, int srcYStride,
      ByteBuffer srcU, int srcUStride,
      ByteBuffer srcV, int srcVStride,
      ByteBuffer dstY, int dstYStride,
      ByteBuffer dstUV, int dstUVStride,
      int width, int height);

  public static native int I420ToARGB(
    ByteBuffer y,
    ByteBuffer u,
    ByteBuffer v,
    int width, int height,
    ByteBuffer outBuf);

  public static native int I420Rotate(
          ByteBuffer y,
          ByteBuffer u,
          ByteBuffer v,
          int width, int height,
          ByteBuffer yRot,
          ByteBuffer uRot,
          ByteBuffer vRot,
          int widthRot, int heightRot,
          int Orientation);

  // Helper: prepara DirectByteBuffers y llama al JNI
  public static int androidImageToI420(Image img, ByteBuffer dy, ByteBuffer du, ByteBuffer dv) {
    Image.Plane[] p = img.getPlanes();
    Rect crop = img.getCropRect();
    int w = crop.width(), h = crop.height();

    ByteBuffer y = p[0].getBuffer().duplicate();
    ByteBuffer u = p[1].getBuffer().duplicate();
    ByteBuffer v = p[2].getBuffer().duplicate();

    y.order(ByteOrder.nativeOrder());
    u.order(ByteOrder.nativeOrder());
    v.order(ByteOrder.nativeOrder());

    int yStride   = p[0].getRowStride();
    int uStride   = p[1].getRowStride();
    int vStride   = p[2].getRowStride();
    int uPix      = p[1].getPixelStride();
    int vPix      = p[2].getPixelStride();

    int yOff = crop.top * yStride + crop.left * 1;
    int uOff = (crop.top/2) * uStride + (crop.left/2) * uPix;
    int vOff = (crop.top/2) * vStride + (crop.left/2) * vPix;

    int r = yuv420888ToI420(
        y, yStride, yOff,
        u, uStride, uPix, uOff,
        v, vStride, vPix, vOff,
        dy, w, du, w/2, dv, w/2,
        w, h);

    return r;
  }

  public static ByteBuffer allocDirect(int size) {
    ByteBuffer b = ByteBuffer.allocateDirect(size);
    b.order(ByteOrder.nativeOrder());
    return b;
  }


}