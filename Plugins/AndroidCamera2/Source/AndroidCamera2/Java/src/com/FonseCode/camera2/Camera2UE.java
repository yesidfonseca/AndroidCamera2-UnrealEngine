/* SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025-2026 Yesid Fonseca
 */

package com.FonseCode.camera2;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.TotalCaptureResult;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.SystemClock;
import android.util.Log;
import android.util.Size;
import android.util.Range;
import android.view.Surface;
import androidx.annotation.Nullable;
import android.os.Trace;
import android.graphics.Rect;
import android.os.Build;
import com.epicgames.unreal.GameActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicReference;

public final class Camera2UE {

    
    public class FrameUpdateInfo {
        private ByteBuffer dy, du, dv ;
        private int Width, Height;
        private ByteBuffer dyRot, duRot, dvRot ;
        private int WidthRot, HeightRot;
        private final java.util.concurrent.atomic.AtomicBoolean reading = new java.util.concurrent.atomic.AtomicBoolean(false);
        private final Object resizeLock = new Object();
        public ByteBuffer y, v, u;
        public int imgWidth, imgHeight;
        public int Orientation; //0->0 degress, 1->90 degress, 2->180 degress, 3->270 degress
        public long timeStamp;
        private void setOutputDataAndPointers()
        {
            if(Orientation==0) {
                y = dy;
                u = du;
                v = dv;
            }
            else
            {
                y = dyRot;
                u = duRot;
                v = dvRot;
            }
        }
        private void ensureBufferSize(int w, int h) {
            Height = h; Width = w;
            HeightRot = h; WidthRot = w;
            if(Orientation > 0)
            {
                if(Orientation%2 ==1) {
                    WidthRot = h;
                    HeightRot = w;
                }

                if(dyRot == null || dyRot.capacity() < w*h || duRot == null || duRot.capacity() < w*h/4 || dvRot == null || dvRot.capacity() < w*h/4)
                {
                    synchronized (resizeLock) {
                        if (dyRot == null || dyRot.capacity() < w*h) dyRot = NativeYuv.allocDirect(w*h);
                        if (duRot == null || duRot.capacity() < w*h/4) duRot = NativeYuv.allocDirect(w*h/4);
                        if (dvRot == null || dvRot.capacity() < w*h/4) dvRot = NativeYuv.allocDirect(w*h/4);
                        dyRot.clear(); duRot.clear(); dvRot.clear();
                    }
                }

                imgWidth = WidthRot;
                imgHeight = HeightRot;

            }
            else
            {
                imgWidth = Width;
                imgHeight = Height;
            }

            if(dy == null || dy.capacity() < w*h || du == null || du.capacity() < w*h/4 || dv == null || dv.capacity() < w*h/4)
            {
                synchronized (resizeLock) {
                    if (dy == null || dy.capacity() < w*h) dy = NativeYuv.allocDirect(w*h);
                    if (du == null || du.capacity() < w*h/4) du = NativeYuv.allocDirect(w*h/4);
                    if (dv == null || dv.capacity() < w*h/4) dv = NativeYuv.allocDirect(w*h/4);
                    dy.clear(); du.clear(); dv.clear();
                }
            }

            setOutputDataAndPointers();

        }

    }
    private static final String TAG = "com.FonseCode.camera2.Camera2UE";

    private int ControlMode = CameraMetadata.CONTROL_MODE_OFF;
    private int AF_Mode = CameraMetadata.CONTROL_AF_MODE_OFF; // Modo AF desactivado por defecto
    private int AE_Mode = CameraMetadata.CONTROL_AE_MODE_OFF; // Modo AE desactivado por defecto
    private int AWB_Mode = CameraMetadata.CONTROL_AWB_MODE_OFF; // Modo AWB desactivado por defecto

    private Context appContext;


    private long framecounter = 0;
    FrameUpdateInfo FrameInfo = new FrameUpdateInfo();


    // Camera2
    private CameraManager cameraManager;
    private String cameraId;
    private CameraDevice cameraDevice;
    private CameraCaptureSession captureSession;
    
    // Readers
    private ImageReader yuvReader;
    private ImageReader jpegReader;

    // Hilo de fondo
    private HandlerThread camThread;
    private Handler bgHandler;

    // Requests
    private CaptureRequest.Builder previewBuilder;
    private CaptureRequest previewRequest;

    // ultimos datos capturados
    private final AtomicReference<byte[]> lastJpeg = new AtomicReference<>(null); // still JPEG


    // Tamanos/outputs
    private Size yuvSize;   // para preview YUV
    private Size jpegSize;  // para still JPEG

    // Estado 3A para takePhoto
    private static enum CaptureState { STATE_IDLE, STATE_WAITING_LOCK, STATE_WAITING_PRECAPTURE, STATE_WAITING_STILLCAPTURE, STATE_PICTURE_TAKEN }
    private CaptureState mCaptureState = CaptureState.STATE_IDLE;

    private Range<Integer> FPS;

    private static final long PRECAPTURE_TIMEOUT_MS = 1000;

    private boolean initialized = false;

    long precaptureTimestamp = 0;

    private static enum RotationMode { R0, R90, R180, R270, RSensor}
    private RotationMode mRotationMode = RotationMode.R0;


    public Camera2UE(Context context) {
        this.appContext = context.getApplicationContext();
    }

    private Camera2UE() {
    GameActivity ga = GameActivity.Get();
    if (ga == null) {
        Log.e(TAG, "GameActivity.Get() es null (muy temprano en el ciclo de vida).");
        return;
    }

    appContext = ga.getApplicationContext();
}

    // -------------------------------------------------------
    // 1) Obtener lista de camaras disponibles (IDs de Camera2)
    // -------------------------------------------------------
    public synchronized String[] getCameraIdList() {
        try {
            ensureManager();
            String[] ids = cameraManager.getCameraIdList();
            return (ids != null) ? ids : new String[0];
        } catch (Throwable t) {
            Log.e(TAG, "getCameraIdList error: " + t.getMessage(), t);
            return new String[0];
        }
    }

    // -------------------------------------------------------
    // 2) Inicializar con un cameraId explicito (validado)
    // -------------------------------------------------------
    /**
     * Inicializa Camera2 usando un ID especifico.
     * @param inputCameraId Un ID valido retornado por getCameraIdList()
     * @return true si se inicio el flujo de apertura correctamente; false si el ID no es valido o ocurrio un error temprano.
     */
    @SuppressLint("MissingPermission")
    public synchronized boolean initializeCamera(String inputCameraId, int AE_ModeIn, int AF_ModeIn, int AWB_ModeIn, int ControlModeIn, int RotMode, int previewWidth, int previewHeight, int stillCaptureWidth, int stillCaptureHeight, int targetFPS) {
        try {



            ensureManager();
            if (inputCameraId == null || inputCameraId.isEmpty()) {
                Log.e(TAG, "initializeCamera: cameraId vacio o nulo");
                return false;
            }

            // Validar que el ID exista en la lista
            if (!isValidCameraId(inputCameraId)) {
                Log.e(TAG, "initializeCamera: cameraId no encontrado: " + inputCameraId);
                return false;
            }

            cameraId = inputCameraId;

            mRotationMode = RotationMode.values()[RotMode];
            //Log.d(TAG, "rotationMode:" + mRotationMode);
            int mOrientation = RotMode;
            if(mRotationMode==RotationMode.RSensor)
            {
                mOrientation = cameraManager.getCameraCharacteristics(cameraId).get(CameraCharacteristics.SENSOR_ORIENTATION)/90;
            }
            FrameInfo.Orientation = mOrientation;



            // Elegir tamanos
            CameraCharacteristics cc = cameraManager.getCameraCharacteristics(cameraId);
            StreamConfigurationMap map = cc.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) { Log.e(TAG, "initializeCamera:No StreamConfigurationMap"); return false; }

            // Resolver modos 3A
            resolve3AModesSimple(cc, AE_ModeIn, AF_ModeIn, AWB_ModeIn, ControlModeIn);

            // JPEG: elegir el tamano mas grande disponible
            Size[] jpegSizes = map.getOutputSizes(ImageFormat.JPEG);
            jpegSize = pickNearesSize(jpegSizes, stillCaptureWidth, stillCaptureHeight);
            jpegSize = (jpegSize == null)? new Size(1920, 1080) : jpegSize;
            Log.d(TAG, "initializeCamera:jpegSize: w:" +jpegSize.getWidth() + " , h:" + jpegSize.getHeight());

            // YUV: buen tamano para preview (elige 1280x720 si existe, si no el mas cercano <=1080p)
            Size[] yuvSizes = map.getOutputSizes(ImageFormat.YUV_420_888);
            yuvSize = pickNearesSize(yuvSizes, previewWidth, previewHeight);
            Log.d(TAG, "initializeCamera:yuvSize: w:" +yuvSize.getWidth() + " , h:" + yuvSize.getHeight());

            // Readers
            closeReaders();
            yuvReader = ImageReader.newInstance(yuvSize.getWidth(), yuvSize.getHeight(), ImageFormat.YUV_420_888, /*maxImages*/3);
            jpegReader = ImageReader.newInstance(jpegSize.getWidth(), jpegSize.getHeight(), ImageFormat.JPEG, /*maxImages*/2);

            yuvReader.setOnImageAvailableListener(yuvListener, getBackgroundHandler());
            jpegReader.setOnImageAvailableListener(jpegListener, getBackgroundHandler());

            FPS = pickFpsRange(cameraManager.getCameraCharacteristics(cameraId),targetFPS);

            // Abrir camara (asincrono)
            openCamera();
            return true;

        } catch (CameraAccessException e) {
            Log.e(TAG, "initializeCamera: CameraAccessException " + e.getMessage(), e);
            return false;
        } catch (Throwable t) {
            Log.e(TAG, "initializeCamera: error " + t.getMessage(), t);
            return false;
        }
    }

    public synchronized boolean getInitializeCameraState()
    {
        return initialized;
    }

    public synchronized long getLastFrameTimeStamp()
    {
        return FrameInfo.timeStamp;
    }

    /** Empieza/rehace el repeating de preview hacia el ImageReader YUV (necesario para 3A estable). */
    private void startPreviewRepeating() {
        if (cameraDevice == null || captureSession == null || yuvReader == null) return;
        try {
            previewBuilder = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW);
            previewBuilder.addTarget(yuvReader.getSurface());

            // 3A auto continuo
            previewBuilder.set(CaptureRequest.CONTROL_MODE, ControlMode);
            previewBuilder.set(CaptureRequest.CONTROL_AF_MODE, AF_Mode);
            previewBuilder.set(CaptureRequest.CONTROL_AE_MODE, AE_Mode);
            previewBuilder.set(CaptureRequest.CONTROL_AWB_MODE, AWB_Mode);

            // (opcional) ligera compensacion para evitar quemados iniciales
            Range<Integer> evRange = cameraManager.getCameraCharacteristics(cameraId).get(CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE);
            if (evRange != null && evRange.getLower() != null) {
                int minusOneEv = Math.max(evRange.getLower(), -1);
                previewBuilder.set(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION, minusOneEv);
            }

            if(FPS != null) {
                previewBuilder.set(CaptureRequest.CONTROL_AE_TARGET_FPS_RANGE, FPS);
            }
            previewRequest = previewBuilder.build();
            captureSession.setRepeatingRequest(previewRequest, captureCallback, getBackgroundHandler());
            Log.e(TAG, "startPreviewRepeating: start ok");
        } catch (Throwable t) {
            Log.e(TAG, "startPreviewRepeating: failed", t);
        }
    }

    /** Devuelve el último JPEG capturado (puede ser null si aún no has disparado). */
    public byte[] getLastCapturedImage() { return lastJpeg.get(); }

    /** Dispara una foto JPEG con secuencia 3A: AF lock + AE precapture + still. */
    public synchronized boolean takePhoto() {
        if (!initialized || cameraDevice == null || captureSession == null) {
            Log.e(TAG, "takePhoto: camere is no ready");
            return false;
        }
        try {
            mCaptureState = CaptureState.STATE_WAITING_LOCK;

            // 1) AF trigger
            if(AF_Mode != CameraMetadata.CONTROL_AF_MODE_OFF)
            {
                previewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_START);                
            }

            captureSession.capture(previewBuilder.build(), captureCallback, getBackgroundHandler());
            Log.d(TAG, "takePhoto: capture request launched");
            // El resto (AE precapture y still) ocurre en captureCallback cuando los estados reporten listo.
            return true;
        } catch (Throwable t) {
            Log.e(TAG, "takePhoto: failed", t);
            // Reanudar preview si hizo fallo a mitad
            try { 
                if(AF_Mode != CameraMetadata.CONTROL_AF_MODE_OFF)
                {
                    previewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_CANCEL); 
                }
            } catch (Throwable ignored) {}
            startPreviewRepeating();
            return false;
        }
    }

    // Secuencia 3A en el callback de captura
    private final CameraCaptureSession.CaptureCallback captureCallback = new CameraCaptureSession.CaptureCallback() {
        private Integer afState(CaptureResult r) { return r.get(CaptureResult.CONTROL_AF_STATE); }
        private Integer aeState(CaptureResult r) { return r.get(CaptureResult.CONTROL_AE_STATE); }

        @Override public void onCaptureProgressed(CameraCaptureSession session, CaptureRequest request, CaptureResult partialResult) {
            process(partialResult);
        }

        @Override public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request, TotalCaptureResult result) {
            process(result);
        }

        private void process(CaptureResult result) {

            try {


                switch (mCaptureState) {
                    case STATE_WAITING_LOCK: {
                        Integer af = afState(result);
                        Log.d(TAG, "captureCallback: STATE_WAITING_LOCK");

                        if (af == null || shouldSkipAFConvergence() || af == CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED
                                || af == CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED) {
                            // Revisa AE
                            Integer ae = aeState(result);

                            Log.d(TAG, "captureCallback: STATE_WAITING_LOCK > AF success");

                            if (ae == null || shouldSkipAEConvergence() ||ae == CaptureResult.CONTROL_AE_STATE_CONVERGED) {
                                mCaptureState = CaptureState.STATE_PICTURE_TAKEN;
                                Log.d(TAG, "captureCallback: STATE_WAITING_LOCK > AE success");
                                captureStill();
                            } else {
                                // Lanzar precapture
                                Log.d(TAG, "captureCallback: STATE_WAITING_LOCK > AE success");
                                precaptureTimestamp = SystemClock.elapsedRealtime();
                                mCaptureState = CaptureState.STATE_WAITING_PRECAPTURE;
                                previewBuilder.set(CaptureRequest.CONTROL_AE_PRECAPTURE_TRIGGER, CameraMetadata.CONTROL_AE_PRECAPTURE_TRIGGER_START);
                                captureSession.capture(previewBuilder.build(), this, getBackgroundHandler());
                            }
                        }
                        break;
                    }
                    case STATE_WAITING_PRECAPTURE: {
                        Integer ae = aeState(result);
                        boolean timedOut = (SystemClock.elapsedRealtime() - precaptureTimestamp)
                                > PRECAPTURE_TIMEOUT_MS;
                        if (ae == null || ae == CaptureResult.CONTROL_AE_STATE_CONVERGED
                                || ae == CaptureResult.CONTROL_AE_STATE_FLASH_REQUIRED || timedOut) {

                            mCaptureState = CaptureState.STATE_WAITING_STILLCAPTURE;
                            captureStill();
                            Log.d(TAG, "captureCallback: STATE_WAITING_PRECAPTURE > AE success or time out");
                        }
                        break;
                    }
                    
                }
            } catch (Throwable t) {
                Log.e(TAG, "captureCallback:3A process error", t);
                // reanuda preview si algo falla
                try { previewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_CANCEL); } catch (Throwable ignored) {}
                startPreviewRepeating();
                mCaptureState = CaptureState.STATE_IDLE;
            }
        }
    };

    private boolean shouldSkipAFConvergence() {
        return AF_Mode == CameraMetadata.CONTROL_AF_MODE_OFF || AF_Mode == CameraMetadata.CONTROL_AF_MODE_MACRO
                || AF_Mode == CameraMetadata.CONTROL_AF_MODE_EDOF;
    };

    private boolean shouldSkipAEConvergence() {
        return AE_Mode == CameraMetadata.CONTROL_AE_MODE_OFF || AE_Mode == CameraMetadata.CONTROL_AE_MODE_ON_EXTERNAL_FLASH
                || AE_Mode == CameraMetadata.CONTROL_AE_MODE_ON_EXTERNAL_FLASH;
    };

    private void captureStill() {
        try {
            if (cameraDevice == null || captureSession == null || jpegReader == null) return;
			
			// Detener el repeating para un disparo limpio
            captureSession.stopRepeating();
            captureSession.abortCaptures();
			
            // Lock AE/AWB durante el disparo (opcional)
            previewBuilder.set(CaptureRequest.CONTROL_AE_LOCK, true);
            previewBuilder.set(CaptureRequest.CONTROL_AWB_LOCK, true);

            CaptureRequest.Builder still = cameraDevice.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            still.addTarget(jpegReader.getSurface());
            still.set(CaptureRequest.CONTROL_MODE, ControlMode);
            still.set(CaptureRequest.CONTROL_AF_MODE, AF_Mode);
            still.set(CaptureRequest.CONTROL_AE_MODE, AE_Mode);
            still.set(CaptureRequest.CONTROL_AWB_MODE, AWB_Mode);
            still.set(CaptureRequest.JPEG_ORIENTATION, FrameInfo.Orientation*90);

            // Compensa EV igual que preview si configuraste una
            if (previewBuilder.get(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION) != null) {
                still.set(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION,
                        previewBuilder.get(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION));
            }

            
            captureSession.capture(still.build(), new CameraCaptureSession.CaptureCallback() {
                @Override public void onCaptureCompleted(CameraCaptureSession session, CaptureRequest request, TotalCaptureResult result) {
                    // desbloquear y reanudar preview
                    try {
                        previewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_CANCEL);
                        previewBuilder.set(CaptureRequest.CONTROL_AE_LOCK, false);
                        previewBuilder.set(CaptureRequest.CONTROL_AWB_LOCK, false);
                        Log.d(TAG, "captureStill: stillCaputre ready, you can get the data");
                    } catch (Throwable ignored) {}
                    startPreviewRepeating();
                    mCaptureState = CaptureState.STATE_IDLE;
                }
            }, getBackgroundHandler());

        } catch (Throwable t) {
            Log.e(TAG, "captureStill: failed", t);
            try {
                previewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_CANCEL);
                previewBuilder.set(CaptureRequest.CONTROL_AE_LOCK, false);
                previewBuilder.set(CaptureRequest.CONTROL_AWB_LOCK, false);
            } catch (Throwable ignored) {}
            startPreviewRepeating();
            mCaptureState = CaptureState.STATE_IDLE;
        }
    }

    // -------------------------------------------------------
    // 4) Guardar el ultimo JPEG a disco (opcional)
    // -------------------------------------------------------
    public synchronized String saveResult() {
        
        try {
            byte[] lastJpegbytes = lastJpeg.get();

            if (lastJpegbytes == null || lastJpegbytes.length == 0) {
                Log.e(TAG, "saveResult: no data");
                return null;
            }

            File outFile = createTimestampedFile(appContext, "jpg");
            writeBytesToFile(lastJpegbytes, outFile);
            Log.d(TAG, "saveResult: ok. Path:" + outFile.getAbsolutePath());
            return outFile.getAbsolutePath();
        } catch (IOException io) {
            Log.e(TAG, "saveResult: fail write file. Error:" + io.getMessage(), io);
            return null;
        }
    }


    public synchronized void release() {
        try { if (captureSession != null) { captureSession.close(); captureSession = null; } } catch (Throwable ignored) {
            Log.e(TAG, "release: " + ignored.getMessage(), ignored);
        }
        try { if (cameraDevice != null) { cameraDevice.close(); cameraDevice = null; } } catch (Throwable ignored) {
            Log.e(TAG, "release: " + ignored.getMessage(), ignored);
        }
        try { closeReaders();} catch (Throwable ignored) {
            Log.e(TAG, "release: " + ignored.getMessage(), ignored);
        }
        stopBgThread();
        framecounter = 0;
        Log.d(TAG, "release: ok");
    }

    private void closeReaders() {
        try { if (yuvReader != null) { yuvReader.close(); } } catch (Throwable ignored) {}
        try { if (jpegReader != null) { jpegReader.close(); } } catch (Throwable ignored) {}
        yuvReader = null; jpegReader = null;
    }

    // -------------------------------------------------------
    // Internos / Helper methods
    // -------------------------------------------------------
    private void ensureManager() {
        if (cameraManager == null) {
            cameraManager = (CameraManager) appContext.getSystemService(Context.CAMERA_SERVICE);
        }
    }

    private boolean isValidCameraId(String id) throws CameraAccessException {
        String[] ids = cameraManager.getCameraIdList();
        if (ids == null) return false;
        for (String s : ids) if (s.equals(id)) return true;
        return false;
    }

    @SuppressLint("MissingPermission")
    private void openCamera() throws CameraAccessException {
        if (cameraManager == null || cameraId == null) {
            Log.e(TAG, "openCamera: manager or cameraId null");
            return;
        }
        cameraManager.openCamera(cameraId, new CameraDevice.StateCallback() {
            @Override public void onOpened(CameraDevice camera) {
                cameraDevice = camera;
                createCaptureSession();       
                Log.d(TAG, "openCamera: ok");
            }
            @Override public void onDisconnected(CameraDevice camera) {
                Log.w(TAG, "openCamera: Disconected");
                camera.close();
                cameraDevice = null;
                initialized = false;
            }
            @Override public void onError(CameraDevice camera, int error) {
                Log.e(TAG, "openCamera: Error opening camera. Error:" + error);
                camera.close();
                cameraDevice = null;
                initialized = false;
            }
        }, getBackgroundHandler());
    }

    private void createCaptureSession() {
        if (cameraDevice == null || yuvReader == null || jpegReader == null) return;
        try {
            List<Surface> outputs = new ArrayList<>();
            outputs.add(yuvReader.getSurface());
            outputs.add(jpegReader.getSurface());
            cameraDevice.createCaptureSession(outputs,
                    new CameraCaptureSession.StateCallback() {
                        @Override public void onConfigured(CameraCaptureSession session) {
                            captureSession = session;
                            startPreviewRepeating();
                            Log.d(TAG, "createCaptureSession: ok configured");
                        }
                        @Override public void onConfigureFailed(CameraCaptureSession session) {
                            Log.e(TAG, "createCaptureSession: Fail configuration");
                            initialized = false;
                        }
                    },
                    getBackgroundHandler()
            );
        } catch (CameraAccessException e) {
            Log.e(TAG, "createCaptureSession error:" + e.getMessage(), e);
        }
    }

    

    private Handler getBackgroundHandler() {
        if (bgHandler == null) {
            camThread = new HandlerThread("AndroidCamera2");
            camThread.start();
            bgHandler = new Handler(camThread.getLooper());
        }
        return bgHandler;
    }

    

    private void stopBgThread() {
        if (camThread != null) {
            camThread.quitSafely();
            try { camThread.join(); } catch (InterruptedException ignored) {
                Log.e(TAG, "stopBgThread. Error:" + ignored.getMessage(), ignored);
            }
            camThread = null; bgHandler = null;
        }
        initialized = false;
    }

    private static File createTimestampedFile(Context ctx, String ext) {
        String ts = new SimpleDateFormat("yyyy_MM_dd_HH_mm_ss_SSS", Locale.US).format(new Date());
        return new File(ctx.getFilesDir(), "Camera2UE_" + ts + "." + ext);
    }

    private static void writeBytesToFile(byte[] data, File file) throws IOException {
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(file, false);
            fos.write(data);
            fos.flush();
        } finally {
            if (fos != null) try { fos.close(); } catch (IOException ignored) {}
        }
    }

    private Size pickNearesSize(Size[] sizes, int desiredWidth, int desiredHeight) {
        if (sizes == null || sizes.length == 0) return new Size(1280, 720);
        // minimizamos el area de la diferencia simetrica entre el size deseada y los size existentes
        Size nearestSize = new Size(1280, 720);
        long minSymmetricDifferenceArea = 100000000;
        for (Size s : sizes) {
            long currentSymmetricDifferenceArea = Math.abs(s.getWidth()-desiredWidth)*Math.min(s.getHeight(),desiredHeight)+
                    Math.abs(s.getHeight()-desiredHeight)*Math.min(s.getWidth(),desiredWidth)+
                    Math.max(0,(desiredHeight-s.getHeight())*(desiredWidth-s.getWidth()));
            if(currentSymmetricDifferenceArea<= minSymmetricDifferenceArea) {
                minSymmetricDifferenceArea = currentSymmetricDifferenceArea;
                nearestSize = s;
            }
        }

        return nearestSize;
    }

    // Listener JPEG: guardar bytes en lastJpeg
    private final ImageReader.OnImageAvailableListener jpegListener = reader -> {
        Image image = null;
        try {
            image = reader.acquireLatestImage();
            if (image != null) {
                ByteBuffer buf = image.getPlanes()[0].getBuffer();
                byte[] out = new byte[buf.remaining()];
                buf.get(out);
                lastJpeg.set(out);
            }
        } catch (Throwable t) {
            Log.e(TAG, "jpegListener: error:", t);
        } finally {
            if (image != null) try { image.close(); } catch (Throwable ignored) {}
        }
    };

    // Listener YUV: empaquetar a I420 (yuv420p) y guardar en lastI420 (para volcar a archivo bajo demanda)
    private final ImageReader.OnImageAvailableListener yuvListener = reader -> {
        Image image = null;
        try {
            Image drain;
            while ((drain = reader.acquireNextImage()) != null) {
                if (image != null) { drain.close(); continue; }
                image = drain;
            }
            if (image != null) {
                onYuvImage(image);  // respeta rowStride/pixelStride
            }
        } catch (Throwable t) {
            Log.e(TAG, "yuvListener: error:", t);
        } finally {
            if (image != null) try { image.close(); } catch (Throwable ignored) {}
        }
    };

    // ======= Utilidades YUV =======

    private void packtoI420Lib(Image image)
    {
        if (image.getFormat() != ImageFormat.YUV_420_888) throw new IllegalArgumentException("Format must be YUV_420_888");
        Trace.beginSection("packtoI420Lib");

        int w = image.getCropRect().width(), h = image.getCropRect().height();

        //Log.d(TAG, "wc:" + w +", hc:" + h + " --- w:" +image.getWidth() +", h:"+image.getHeight() );
        int ok = NativeYuv.androidImageToI420(image, FrameInfo.dy, FrameInfo.du, FrameInfo.dv);

        if(FrameInfo.Orientation > 0)
        {
            NativeYuv.I420Rotate(FrameInfo.dy, FrameInfo.du, FrameInfo.dv, FrameInfo.Width, FrameInfo.Height, FrameInfo.dyRot, FrameInfo.duRot, FrameInfo.dvRot, FrameInfo.WidthRot, FrameInfo.HeightRot, FrameInfo.Orientation);
        }

        FrameInfo.timeStamp = image.getTimestamp();

        Trace.endSection();
    }


    private static boolean contains(int[] arr, int val) {
        if (arr == null) return false;
        for (int x : arr) if (x == val) return true;
        return false;
    }

    private void resolve3AModesSimple(CameraCharacteristics cc,
                                      int desiredAE, int desiredAF, int desiredAWB, int desiredControl)
    {
        // AF
        int[] afAvail = cc.get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES);
        AF_Mode = contains(afAvail, desiredAF)
                ? desiredAF
                : CameraMetadata.CONTROL_AF_MODE_OFF;

        // AE
        int[] aeAvail = cc.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_MODES);
        AE_Mode = contains(aeAvail, desiredAE)
                ? desiredAE
                : CameraMetadata.CONTROL_AE_MODE_OFF;

        // AWB
        int[] awbAvail = cc.get(CameraCharacteristics.CONTROL_AWB_AVAILABLE_MODES);
        AWB_Mode = contains(awbAvail, desiredAWB)
                ? desiredAWB
                : CameraMetadata.CONTROL_AWB_MODE_OFF;

        // CONTROL_MODE (no hay lista available; aplicamos el pedido tal cual)
        ControlMode = desiredControl;

        Log.d(TAG, "resolve3AModesSimple: Resolved 3A -> AF:" + AF_Mode + " AE:" + AE_Mode + " AWB:" + AWB_Mode + " CTRL:" + ControlMode);
    }

    private static class CompareSizesByArea implements Comparator<Size> {
        @Override public int compare(Size lhs, Size rhs) {
            return Long.signum((long) lhs.getWidth()*lhs.getHeight() - (long) rhs.getWidth()*rhs.getHeight());
        }
    }


    // --- Productor (listener) ---
    private void onYuvImage(Image image) {
        int w = image.getWidth(), h = image.getHeight();



        if (!FrameInfo.reading.get()) {                  // si alguien está leyendo, descarta este frame

            FrameInfo.ensureBufferSize(w, h);

            packtoI420Lib(image);
            framecounter++;
            
            initialized = true;
            //Log.d(TAG, "FrameCounter=" + framecounter);
        }
    }

    @Nullable
    public FrameUpdateInfo getLastFrameInfo() {

        if(FrameInfo.reading.compareAndSet(false, true))
        {
            return FrameInfo;
        }
        return  null;
    }

    public void releaseFrameInfo() {
        FrameInfo.reading.set(false);
    }

    private void changeCaptureStateStateAndNotify(CaptureState state) {
        mCaptureState = state;
        //TODO NOTIFY
    }

    
    public static Range<Integer> pickFpsRange(CameraCharacteristics ch, int targetFps) {
        Range<Integer>[] ranges = ch.get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
        if (ranges == null || ranges.length == 0) return null;

        Range<Integer> exactFixed = null, contains = null;
        for (Range<Integer> r : ranges) {
            Log.d(TAG, "fps range available:" +r);
            if (r.getLower() == targetFps && r.getUpper() == targetFps) {
                exactFixed = r; break;
            }
            if (r.contains(targetFps) && contains == null) contains = r;
        }
        return exactFixed != null ? exactFixed : contains;
    }

    public class Intrinsics {
        // En pixeles (coordenadas del active array)
        public float fx, fy, cx, cy, skew;
        // Tamaño del active array en pixeles
        public int widthPx, heightPx;
        // Datos útiles adicionales
        public float focalLengthMm;        // distancia focal elegida (mm)
        public float sensorWidthMm, sensorHeightMm; // tamaño físico del sensor (mm)
        public int sensorOrientation;      // 90, 270, etc.

        @Override public String toString() {
            return String.format(
                    "fx=%.3f fy=%.3f cx=%.3f cy=%.3f skew=%.3f size=%dx%d, f=%.3fmm, sensor=%.3fx%.3fmm, orient=%d",
                    fx, fy, cx, cy, skew, widthPx, heightPx, focalLengthMm,
                    sensorWidthMm, sensorHeightMm, sensorOrientation
            );
        }
    }

    public class LensPose {
        public float quat_x = 0f, quat_y = 0f, quat_z = 0f, quat_w = 1f;
        public float loc_x = 0f, loc_y = 0f, loc_z = 0f;
        public int lensposeReference = 2; //UNDEFINED:2, PRIMARY_CAMERA:0, GYROSCOPE:1, AUTOMOTIVE:3

        @Override
        public String toString() {
            return String.format(
                    "quat_x=%.3f quat_y=%.3f quat_z=%.3f quat_w=%.3f, loc_x=%.3f loc_y=%.3f loc_z=%.3f, lensposeReference=%d",
                    quat_x, quat_y, quat_z, quat_w, loc_x, loc_y, loc_z, lensposeReference
            );

        }
    }
    @Nullable
    public LensPose getLensPose(String inputCameraId) {
        try {
            if (inputCameraId == null || inputCameraId.isEmpty()) {
                Log.e(TAG, "getLensPose: cameraId vacio o nulo");
                return null;
            }

            ensureManager();
            CameraCharacteristics ch = cameraManager.getCameraCharacteristics(inputCameraId);

            LensPose out = new LensPose();

            // Pose reference (enum int). Disponible desde API 28 (Android P)
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                Integer poseRef = ch.get(CameraCharacteristics.LENS_POSE_REFERENCE);
                out.lensposeReference = (poseRef != null) ? poseRef : 2; // 2 = UNDEFINED
            }

            float[] q = ch.get(CameraCharacteristics.LENS_POSE_ROTATION);
            if (q != null && q.length >= 4) {
                out.quat_x = q[0];
                out.quat_y = q[1];
                out.quat_z = q[2];
                out.quat_w = q[3];
            }

            float[] t = ch.get(CameraCharacteristics.LENS_POSE_TRANSLATION);
            if (t != null && t.length >= 3) {
                out.loc_x = t[0];
                out.loc_y = t[1];
                out.loc_z = t[2];
            }

            return out;

        } catch (CameraAccessException e) {
            Log.e(TAG, "getLensPose: CameraAccessException " + e.getMessage(), e);
            return null;
        }
    }


    @Nullable
    public Intrinsics getIntrinsics( String inputCameraId) {
        Intrinsics out = new Intrinsics();
        try {
            if (inputCameraId == null || inputCameraId.isEmpty()) {
                Log.e(TAG, "getIntrinsics: cameraId vacio o nulo");
                return null;
            }

            ensureManager();
            CameraCharacteristics ch = cameraManager.getCameraCharacteristics(inputCameraId);

            Rect active = ch.get(CameraCharacteristics.SENSOR_INFO_ACTIVE_ARRAY_SIZE);
            if (active == null) return null;

            float[] focalLengths = ch.get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS);
            if (focalLengths == null || focalLengths.length == 0) return null;
            float focalMm = focalLengths[0];

            android.util.SizeF physicalSize = ch.get(CameraCharacteristics.SENSOR_INFO_PHYSICAL_SIZE);
            if (physicalSize == null) return null;

            int widthPx = active.width();
            int heightPx = active.height();
            float sensorWmm = physicalSize.getWidth();
            float sensorHmm = physicalSize.getHeight();


            out.widthPx = widthPx;
            out.heightPx = heightPx;
            out.focalLengthMm = focalMm;
            out.sensorWidthMm = sensorWmm;
            out.sensorHeightMm = sensorHmm;
            out.sensorOrientation = ch.get(CameraCharacteristics.SENSOR_ORIENTATION);

            // 1) Intentar intrinsecas calibradas (en pixeles): [fx, fy, cx, cy, s]
            float[] K = ch.get(CameraCharacteristics.LENS_INTRINSIC_CALIBRATION);
            if (K != null && K.length >= 5) {
                out.fx = K[0];
                out.fy = K[1];
                out.cx = K[2];
                out.cy = K[3];
                out.skew = K[4];
            } else {
                // 2) Fallback: calcular fx/fy en pixeles a partir de focal (mm) y sensor físico (mm)
                float fx = focalMm * (widthPx / sensorWmm);
                float fy = focalMm * (heightPx / sensorHmm);

                out.fx = fx;
                out.fy = fy;
                out.cx = active.left + widthPx * 0.5f;
                out.cy = active.top + heightPx * 0.5f;
                out.skew = 0.0f;
            }
        }catch(CameraAccessException e)
        {
            Log.e(TAG, "getIntrinsics: CameraAccessException " + e.getMessage(), e);
            return null;
        }
        return out;
    }


}
