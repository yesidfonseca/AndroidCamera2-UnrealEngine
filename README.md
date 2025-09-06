# Android Camera2 API for Unreal Engine (UE 5.6)

# Project Features
- Plugin "Android Camera2" in C++ for **Video Capture with Android Camera2 Api** in Unreal Engine (Vulkan-only). 
- Project sample tha use the plugin "Android Camera2" to get de Luminance data of Android Video Device 2 signal processing purposes:
    1) QR Code detection using the quirc library https://github.com/dlbeer/quirc
    2) Edge Detection using the example explained in https://youtu.be/PXLgkxRizPI
> **Estado**: estable (preview público).  
> **Licencia**: Apache-2.0.

## ✨ "AndroidCamera2" Plugin 
- Video Campture wtih Android Camera2 in format YUV I420. 
- Basic auto controls modes of the camera supported for **Androd Camera2 Api** (3A): Auto focus modes, auto-exposure antibanding modes, auto-white-balance modes (*support on some of this modes depends of the android device*).
- Basic media reproduction functionalities: List available cameras, initialize camera, pause/resume video capturing, stop video capturing.
- 3 TextureRenderTarget2D used to render the camera video in YUV I420 format: **Luma (Y)**, **Chroma U**, **Chroma V**. You can set your own TextureRenderTarget2D in Project Settings → Plugins → Android Camera2.
- RGB Material Instance: A simple example of conversion of data in YUV I420 format to RGB (Full optimized).
- API (BP/C++) for comsuming the raw camera data for custom client porpuse (UAndroidCamera2Subsystem and UAndroidCamera2BlueprintLibrary).
- UI Example with the previues features.

## ✅ Compatibilidad
- **UE**: 5.6  
- **Plataform**: Android (Vulkan-only)  
- **ABI**: arm64-v8a  
- **Tested Devices**: Quest 3 (HorizonOS v74 or later), Xiaomi Poco F5, Xiaomi Poco X7 Pro.

## 🔒 Permission (Meta Quest 3 – optional)

If you are going to access to the passthrough camera of the Meta Quest 3 device you need to enable horizonos.permission.HEADSET_CAMERA pemisson (https://developers.meta.com/horizon/documentation/spatial-sdk/spatial-sdk-pca-overview/). For that you need to check this:

Project Settings → Plugins → Android Camera2 → Permissions | Meta Quest
-    Request Headset Camera Permission ✅

## ⏱️ Rendimiento y límites
- **Vulkan-only** en Android.  
- YUV→RGB en CPU es costoso; usa material/shader (recomendado) o compute si integras.  
- If you set Rotation different to EAndroidCamera2RotationMode::R0 then the Java Class rotates the data using the yub library and can add 1.7ms of lattency per frame for a 1920x1080 video size (measured with Snapdragon 7+ Gen2 Mobile and 12 GB Ram). You can use the seccion trace "packtoI420Lib" in Android to measure the time consumed in the conversion to YUV I420 format and apply rotations (you need to measure the number of frames or estimated them to make a correct measure of time per frame: #total amount of time / # number of frames).


<img width="2200" height="990" alt="image" src="https://github.com/user-attachments/assets/3d884157-ef39-481d-8f02-8b68604be9d3" />
