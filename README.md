# Android Camera2 API for Unreal Engine (UE 5.6)
High-performance Android Camera2 capture for Unreal Engine 5.6 (Vulkan-only), with sample apps for QR detection (quirc) and luma-based (same as grayscale image) edge detection.
> **Status**: stable (public preview).  
> **License**: Apache-2.0.


## ✨ "AndroidCamera2" Plugin 
- **Video Campture** via Android **Camera2** in **YUV I420** format.
- **Basic 3A controls** (device-dependent support): autofocus modes, auto-exposure and antibanding modes, auto-white-balance modes.
- **Device enumeration & control**: list  cameras, initialize camera, pause/resume video capturing, stop video capturing. This simple funcionalities can be used with UAndroidCamera2BlueprintLibrary.
- **GPU-friendly outputs**: three UTextureRenderTarget2D targets for I420 planes: ** Y (Luma)**, ** U (Chroma blue-difference)**, **V (Chroma red-difference)**. You can assign your own UTextureRenderTarget2D in  
  **Project Settings → Plugins → Android Camera2 → Video Output → Data Settings.**
- Fast YUV-RGB: material example using `/Plugin/AndroidCamera2/Private/YUVUtils.ush`.  
  See sample material at:  
  `/AndroidCamera2/Materials/MaterialsSamples/M_AndroidCamera2RGB_SampleUI`
- **Raw access API (BP/C++)**: `UAndroidCamera2BlueprintLibrary` and `UAndroidCamera2Subsystem` to consume Y/U/V buffers directly for custom processing.  
  You can see an example of use in the sample Camera UI at : `/AndroidCamera2/UISample/CameraUI`.

## 🧪 "Sample Project" 
- **`Quirc` module**: for QR detection from Luma data (equivalent to gray scale) using [quirc](https://github.com/dlbeer/quirc)
- **UQRCodeDetectionComp (ActorComponent)**: shows how to pull luma data from `UAndroidCamera2Subsystem` and run QR detection.
- **Signal Processing UI**: simple UI that display edge detection (as in this [video](https://youtu.be/PXLgkxRizPI) ) and QR code detection results (text content and corners locations) from Luma Data.


  <img width="1289" height="730" alt="image" src="https://github.com/user-attachments/assets/10042c53-d72c-45c0-886c-8b5efa2af55e" />



## ✅ Compatibilidad
- **UE**: 5.6  
- **Plataform**: Android (Vulkan-only)  
- **ABI**: arm64-v8a  
- **Tested Devices**: Quest 3 (HorizonOS v74 or later), Xiaomi Poco F5, Xiaomi Poco X7 Pro.

## 🔒 Permissions (Meta Quest 3 – optional)

To access Meta Quest 3 passthrough cameras of you must request `horizonos.permission.HEADSET_CAMERA` pemisson.

Enable in:  
**Project Settings → Plugins → Android Camera2 → Permissions | Meta Quest**
-    ✅ Request Headset Camera Permission  
See [Meta documentation](https://developers.meta.com/horizon/documentation/spatial-sdk/spatial-sdk-pca-overview/).

By default the plugin will require `android.permission.CAMERA` permisson. 

## 🚀 Getting Started

1. **Install**: place the plugin folder under `YourProject/Plugins/AndroidCamera2/`.
2. (Optional) Assign your own Render Targets (Y/U/V) in Project Settings.
3. **Make apk and install** it on an android device.
4. **Make sure to enable all camera permissions**.
5. **Open the apk and open UI sample**: `/AndroidCamera2/UISample/CameraUI` and press InitializeCamera.

## 🧩 API Overview
- **Rendering**  
  The plugin can auto-update the three UTextureRenderTarget2D (Y/U/V). You can disable per-plane rendering updates or point the plugin to custom UTextureRenderTarget2D. 

- **Raw buffers**  
  Use UAndroidCamera2Subsystem to retrieve Y/U/V as tightly-packed byte buffers (ideal for computer vision). Make sure the setting #bCaptureBuffer# be enable if you need to get the buffer. (You can capture the buffer whitout rendering if you need it).

Check this on:  
  **Project Settings → Plugins → Android Camera2 → Video Output | Data Setting**  

  Example consumers in the sample: quirc (QR) and edge detection.  


## ⏱️ Rendimiento y límites
- **Vulkan-only** en Android.  
- Prefer the **shader path** (material using `YUVUtils.ush`) for YUV to RGB conversion instead of CPU conversion.
- If you set a rotation different to `EAndroidCamera2RotationMode::R0` the Java side rotates frames using the **yuvlib**; measured overhead was **~1.7ms/frame** at 1920x1080 resolution on Snapdragon 7+ Gen2 Mobile(12 GB RAM).  
  Use Android ATrace to measure overhead on packaging of raw camera data to yuv I420 + frame rotation:
  - **Seccion name**: `packtoI420Lib` (created with `android.os.Trace.beginSecction(...)`/`endSection()`).
  - Capture with **Perfetto/Systrace** and divide total time by number of frames to estimate per-frame overhead.

