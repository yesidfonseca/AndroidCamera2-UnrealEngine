package com.FonseCode.camera2;

import android.annotation.SuppressLint;
import android.app.Activity;
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

    private static final String TAG = "com.FonseCode.camera2.Camera2";

    private int ControlMode = CameraMetadata.CONTROL_MODE_OFF;
    private int AF_Mode = CameraMetadata.CONTROL_AF_MODE_OFF; // Modo AF desactivado por defecto
    private int AE_Mode = CameraMetadata.CONTROL_AE_MODE_OFF; // Modo AE desactivado por defecto
    private int AWB_Mode = CameraMetadata.CONTROL_AWB_MODE_OFF; // Modo AWB desactivado por defecto

    private Context appContext;


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
    private final AtomicReference<byte[]> lastI420 = new AtomicReference<>(null); // preview empaquetado a I420
    private final AtomicReference<byte[]> lastJpeg = new AtomicReference<>(null); // still JPEG


    // Tamanos/outputs
    private Size yuvSize;   // para preview YUV
    private Size jpegSize;  // para still JPEG

    // Estado 3A para takePhoto
    private static final int STATE_IDLE                        = 0;
    private static final int STATE_WAITING_LOCK                = 1;
    private static final int STATE_WAITING_PRECAPTURE          = 2;
    private static final int STATE_WAITING_STILLCAPTURE        = 3;
    private static final int STATE_PICTURE_TAKEN               = 4;
    private int captureState = STATE_IDLE;

    private static final long PRECAPTURE_TIMEOUT_MS = 1000;

    private boolean initialized = false;

    long precaptureTimestamp = 0;
   
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
    public synchronized boolean initializeCamera(String inputCameraId, int AE_ModeIn, int AF_ModeIn, int AWB_ModeIn, int ControlModeIn) {
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

            this.cameraId = inputCameraId;

            

            // Elegir tamanos
            CameraCharacteristics cc = cameraManager.getCameraCharacteristics(cameraId);
            StreamConfigurationMap map = cc.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
            if (map == null) { Log.e(TAG, "No StreamConfigurationMap"); return false; }

            // Resolver modos 3A
            resolve3AModesSimple(cc, AE_ModeIn, AF_ModeIn, AWB_ModeIn, ControlModeIn);

            // JPEG: elegir el tamano mas grande disponible
            Size[] jpegSizes = map.getOutputSizes(ImageFormat.JPEG);
            jpegSize = (jpegSizes != null && jpegSizes.length > 0) ? Collections.max(Arrays.asList(jpegSizes), new CompareSizesByArea()) : new Size(1920, 1080);

            // YUV: buen tamano para preview (elige 1280x720 si existe, si no el mas cercano <=1080p)
            Size[] yuvSizes = map.getOutputSizes(ImageFormat.YUV_420_888);
            yuvSize = pickYuvPreviewSize(yuvSizes);

            // Readers
            closeReaders();
            yuvReader = ImageReader.newInstance(yuvSize.getWidth(), yuvSize.getHeight(), ImageFormat.YUV_420_888, /*maxImages*/3);
            jpegReader = ImageReader.newInstance(jpegSize.getWidth(), jpegSize.getHeight(), ImageFormat.JPEG, /*maxImages*/2);

            yuvReader.setOnImageAvailableListener(yuvListener, getBackgroundHandler());
            jpegReader.setOnImageAvailableListener(jpegListener, getBackgroundHandler());

            // Abrir camara (asincrono)
            openCamera();
            initialized = true;
            return true;

        } catch (CameraAccessException e) {
            Log.e(TAG, "initializeCamera: CameraAccessException " + e.getMessage(), e);
            return false;
        } catch (Throwable t) {
            Log.e(TAG, "initializeCamera: error " + t.getMessage(), t);
            return false;
        }
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

            previewRequest = previewBuilder.build();
            captureSession.setRepeatingRequest(previewRequest, captureCallback, getBackgroundHandler());
        } catch (Throwable t) {
            Log.e(TAG, "startPreviewRepeating failed", t);
        }
    }

    /** Dispara una foto JPEG con secuencia 3A: AF lock + AE precapture + still. */
    public synchronized boolean takePhoto() {
        if (!initialized || cameraDevice == null || captureSession == null) {
            Log.e(TAG, "takePhoto: camara no lista");
            return false;
        }
        try {
            captureState = STATE_WAITING_LOCK;

            // 1) AF trigger
            if(AF_Mode != CameraMetadata.CONTROL_AF_MODE_OFF)
            {
                previewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_START);                
            }

            captureSession.capture(previewBuilder.build(), captureCallback, getBackgroundHandler());

            // El resto (AE precapture y still) ocurre en captureCallback cuando los estados reporten listo.
            return true;
        } catch (Throwable t) {
            Log.e(TAG, "takePhoto failed", t);
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
                switch (captureState) {
                    case STATE_WAITING_LOCK: {
                        Integer af = afState(result);


                        if (af == null || shouldSkipAFConvergence() || af == CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED
                                || af == CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED) {
                            // Revisa AE
                            Integer ae = aeState(result);

                            if (ae == null || shouldSkipAEConvergence() ||ae == CaptureResult.CONTROL_AE_STATE_CONVERGED) {
                                captureState = STATE_PICTURE_TAKEN;
                                captureStill();
                            } else {
                                // Lanzar precapture
                                precaptureTimestamp = SystemClock.elapsedRealtime();
                                captureState = STATE_WAITING_PRECAPTURE;
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

                            captureState = STATE_WAITING_STILLCAPTURE;
                            captureStill();
                        }
                        break;
                    }
                    
                }
            } catch (Throwable t) {
                Log.e(TAG, "3A process error", t);
                // reanuda preview si algo falla
                try { previewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_CANCEL); } catch (Throwable ignored) {}
                startPreviewRepeating();
                captureState = STATE_IDLE;
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
                    } catch (Throwable ignored) {}
                    startPreviewRepeating();
                    captureState = STATE_IDLE;
                }
            }, getBackgroundHandler());

        } catch (Throwable t) {
            Log.e(TAG, "captureStill failed", t);
            try {
                previewBuilder.set(CaptureRequest.CONTROL_AF_TRIGGER, CameraMetadata.CONTROL_AF_TRIGGER_CANCEL);
                previewBuilder.set(CaptureRequest.CONTROL_AE_LOCK, false);
                previewBuilder.set(CaptureRequest.CONTROL_AWB_LOCK, false);
            } catch (Throwable ignored) {}
            startPreviewRepeating();
            captureState = STATE_IDLE;
        }
    }

    // -------------------------------------------------------
    // 4) Guardar el ultimo JPEG a disco (opcional)
    // -------------------------------------------------------
    public synchronized String saveResult() {
        
        try {
            byte[] lastJpegbytes = lastJpeg.get();

            if (lastJpegbytes == null || lastJpegbytes.length == 0) {
                Log.e(TAG, "saveResult: no hay datos de imagen capturada");
                return null;
            }

            File outFile = createTimestampedFile(appContext, "jpg");
            writeBytesToFile(lastJpegbytes, outFile);
            Log.d(TAG, "Imagen guardada: " + outFile.getAbsolutePath());
            return outFile.getAbsolutePath();
        } catch (IOException io) {
            Log.e(TAG, "saveResult: error escribiendo archivo " + io.getMessage(), io);
            return null;
        }
    }


    public synchronized void release() {
        try { if (captureSession != null) { captureSession.close(); captureSession = null; } } catch (Throwable ignored) {}
        try { if (cameraDevice != null) { cameraDevice.close(); cameraDevice = null; } } catch (Throwable ignored) {}
        try { closeReaders();} catch (Throwable ignored) {}
        stopBgThread();
        Log.d(TAG, "Recursos liberados");
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
            Log.e(TAG, "openCamera: manager o cameraId nulos");
            return;
        }
        cameraManager.openCamera(cameraId, new CameraDevice.StateCallback() {
            @Override public void onOpened(CameraDevice camera) {
                cameraDevice = camera;
                createCaptureSession();
                Log.d(TAG, "openCamera: todo bien");
            }
            @Override public void onDisconnected(CameraDevice camera) {
                Log.w(TAG, "Camara desconectada");
                camera.close();
                cameraDevice = null;
            }
            @Override public void onError(CameraDevice camera, int error) {
                Log.e(TAG, "Error al abrir camara: " + error);
                camera.close();
                cameraDevice = null;
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
                            Log.d(TAG, "CaptureSession configurada");
                        }
                        @Override public void onConfigureFailed(CameraCaptureSession session) {
                            Log.e(TAG, "Fallo configuracion de CaptureSession");
                        }
                    },
                    getBackgroundHandler()
            );
        } catch (CameraAccessException e) {
            Log.e(TAG, "createCaptureSession: " + e.getMessage(), e);
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
            try { camThread.join(); } catch (InterruptedException ignored) {}
            camThread = null; bgHandler = null;
        }
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

    private Size pickYuvPreviewSize(Size[] sizes) {
        if (sizes == null || sizes.length == 0) return new Size(1280, 720);
        // preferimos 1280x720 si existe; si no, la mayor <=1080p; si no, la mayor disponible
        Size best720 = null, bestUnder1080 = null, max = sizes[0];
        for (Size s : sizes) {
            if (s.getWidth() == 1280 && s.getHeight() == 720) best720 = s;
            if (s.getWidth() * s.getHeight() > max.getWidth() * max.getHeight()) max = s;
            if (s.getHeight() <= 1080) {
                if (bestUnder1080 == null || (s.getWidth()*s.getHeight() > bestUnder1080.getWidth()*bestUnder1080.getHeight()))
                    bestUnder1080 = s;
            }
        }
        if (best720 != null) return best720;
        if (bestUnder1080 != null) return bestUnder1080;
        return max;
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
            Log.e(TAG, "jpegListener error", t);
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
                byte[] i420 = packToI420(image);  // respeta rowStride/pixelStride
                lastI420.set(i420);
            }
        } catch (Throwable t) {
            Log.e(TAG, "yuvListener error", t);
        } finally {
            if (image != null) try { image.close(); } catch (Throwable ignored) {}
        }
    };

    // ======= Utilidades YUV =======

    /** Empaqueta un Image YUV_420_888 a I420 (yuv420p): Y + U + V (cada uno contiguo). */
    private static byte[] packToI420(Image image) {
        if (image.getFormat() != ImageFormat.YUV_420_888) throw new IllegalArgumentException("Format must be YUV_420_888");
        final int w = image.getWidth();
        final int h = image.getHeight();
        final int ySize = w * h;
        final int cW = w / 2;
        final int cH = h / 2;
        final int uSize = cW * cH;
        final int vSize = uSize;
        byte[] out = new byte[ySize + uSize + vSize];

        Image.Plane[] planes = image.getPlanes();

        // Copiar Y (respetando rowStride)
        copyPlaneToBuffer(planes[0], w, h, out, 0);

        // Determinar si los cromas estan intercalados o planars segun pixelStride
        // Para I420 final, necesitamos U seguido de V, cada uno contiguo (cW x cH).
        copyChromaToI420(planes[1], planes[2], w, h, out, ySize, ySize + uSize);

        return out;
    }

    /** Copia plano (pixelStride==1 esperado) fila a fila; si pixelStride>1, hace muestreo cada pixelStride. */
    private static void copyPlaneToBuffer(Image.Plane plane, int width, int height, byte[] dst, int dstOffset) {
        ByteBuffer buf = plane.getBuffer();
        int rowStride = plane.getRowStride();
        int pixelStride = plane.getPixelStride();
        int dstPos = dstOffset;

        if (pixelStride == 1 && rowStride == width) {
            // bloque contiguo
            buf.get(dst, dstPos, width * height);
            return;
        }

        byte[] row = new byte[rowStride];
        for (int y = 0; y < height; y++) {
            buf.position(y * rowStride);
            int toRead = Math.min(rowStride, buf.remaining());
            buf.get(row, 0, toRead);
            if (pixelStride == 1) {
                System.arraycopy(row, 0, dst, dstPos, width);
                dstPos += width;
            } else {
                // muestrea cada pixelStride
                for (int x = 0; x < width; x++) {
                    dst[dstPos++] = row[x * pixelStride];
                }
            }
        }
    }

    /** Copia U y V (submuestreados) a dos bloques contiguos I420 (U luego V), respetando row/pixel stride. */
    private static void copyChromaToI420(Image.Plane uPlane, Image.Plane vPlane, int width, int height, byte[] dst, int uDstOffset, int vDstOffset) {
        int cW = width / 2;
        int cH = height / 2;

        // Si pixelStride == 1 en ambos -> planar simple
        if (uPlane.getPixelStride() == 1 && vPlane.getPixelStride() == 1) {
            copyPlaneToBuffer(uPlane, cW, cH, dst, uDstOffset);
            copyPlaneToBuffer(vPlane, cW, cH, dst, vDstOffset);
            return;
        }

        // Semi-planar (NV12/NV21 representados via dos planos con pixelStride 2)
        // Leemos cada plano muestreando cada pixelStride y respetando rowStride.
        ByteBuffer ub = uPlane.getBuffer();
        ByteBuffer vb = vPlane.getBuffer();

        int uRowStride = uPlane.getRowStride();
        int vRowStride = vPlane.getRowStride();
        int uPix = uPlane.getPixelStride();
        int vPix = vPlane.getPixelStride();

        byte[] uRow = new byte[uRowStride];
        byte[] vRow = new byte[vRowStride];

        int uPos = uDstOffset;
        int vPos = vDstOffset;

        for (int y = 0; y < cH; y++) {
            ub.position(y * uRowStride);
            vb.position(y * vRowStride);
            ub.get(uRow, 0, Math.min(uRowStride, ub.remaining()));
            vb.get(vRow, 0, Math.min(vRowStride, vb.remaining()));
            for (int x = 0; x < cW; x++) {
                dst[uPos++] = uRow[x * uPix];
                dst[vPos++] = vRow[x * vPix];
            }
        }
    }

    private String makeYuvName() {
        return "preview_" + yuvSize.getWidth() + "x" + yuvSize.getHeight() + "_i420.yuv";
    }

    private static boolean contains(int[] arr, int val) {
        if (arr == null) return false;
        for (int x : arr) if (x == val) return true;
        return false;
    }

    // Llama esto dentro de initializeCamera() despues de leer CameraCharacteristics cc
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

        Log.d(TAG, "Resolved 3A -> AF:" + AF_Mode + " AE:" + AE_Mode + " AWB:" + AWB_Mode + " CTRL:" + ControlMode);
    }

    private static class CompareSizesByArea implements Comparator<Size> {
        @Override public int compare(Size lhs, Size rhs) {
            return Long.signum((long) lhs.getWidth()*lhs.getHeight() - (long) rhs.getWidth()*rhs.getHeight());
        }
    }
}
