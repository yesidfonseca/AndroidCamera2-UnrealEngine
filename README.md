# Android Camera2 API for Unreal Engine (UE 5.6)

This repo contains 
- A plugin "Android Camera2" in C++ for **Video Capture with Android Camera2** in Unreal Engine (Vulkan-only). This plugin include a  UI Widget ("CameraUI") that shows the real time Camera Video Device using the "Android Camera2 Api".  
- A project tha uses the plugin "Android Camera2" to get de Luminance data of the Video Camera for 2 signal processing examples:
    1) QR Code detection using the quirc library https://github.com/dlbeer/quirc
    2) Edge Detection using the example explained in https://youtu.be/PXLgkxRizPI

> **Estado**: estable (preview público).  
> **Licencia**: Apache-2.0.

## ✨ "AndroidCamera2" Plugin 
- Video Campture wtih Android Camera2 in format YUV I420. 
- Basic auto controls modes of the camera supported for Androd Camera2 (3A): Auto focus modes, auto-exposure antibanding modes, auto-white-balance modes (*maybe some or many modes are not supported by the android device*).
- Basic media reproductor functionalities: List Cameras, Initialize Camera, Pause/Resume Video Capturing, Stop Video Capturing.
- 3 TextureRenderTarget2D used to render the camera video in YUV I420 format: **Luma (Y)**, **Chroma U**, **Chroma V**. You can set your own Render Textures in the Proyect Settings of the plugin.
- RGB Material Instance: A simple example of conversion of data in YUV I420 format ro RGB (Full optimized).
- API (BP/C++) for comsuming the raw camera data for custom client porpuse.
- UI Example with the previues features.
