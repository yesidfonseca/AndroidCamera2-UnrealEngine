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
import android.media.Image;
import android.media.ImageReader;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.util.Size;

import com.epicgames.unreal.GameActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.Locale;

public final class Camera2UE {

    private static final String TAG = "Camera2PhotoCapture";
    private static final int IMAGE_BUFFER_SIZE = 3;
    private static final int DEFAULT_WIDTH = 640;
    private static final int DEFAULT_HEIGHT = 480;

    private Context context;
    private CameraManager cameraManager;

    private String cameraId; // ID validado via initializeCamera(String)

    private CameraDevice cameraDevice;
    private CameraCaptureSession captureSession;
    private ImageReader imageReader;

    private HandlerThread backgroundThread;
    private Handler backgroundHandler;

    private byte[] lastCapturedJpeg;
    private Size captureSize;

   
    private Camera2UE() {
        GameActivity ga = GameActivity.Get();
        if (ga == null) {
            Log.e(TAG, "GameActivity.Get() es null (muy temprano en el ciclo de vida).");
            return;
        }

        context = ga.getApplicationContext();
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
    public synchronized boolean initializeCamera(String inputCameraId) {
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

            // Elegir tamano JPEG (mayor disponible)
            captureSize = chooseLargestJpeg(cameraId);
            if (captureSize == null) {
                captureSize = new Size(DEFAULT_WIDTH, DEFAULT_HEIGHT);
            }

            // Preparar ImageReader para JPEG
            imageReader = ImageReader.newInstance(
                    captureSize.getWidth(),
                    captureSize.getHeight(),
                    ImageFormat.JPEG,
                    IMAGE_BUFFER_SIZE
            );

            // Listener que extrae los bytes del frame capturado
            imageReader.setOnImageAvailableListener(reader -> {
                Image image = null;
                try {
                    Image drain;
                    while ((drain = reader.acquireNextImage()) != null) {
                        if (image != null) { drain.close(); continue; }
                        image = drain;
                    }
                    if (image != null) {
                        saveImageToMemory(image);
                    }else
                    {
                        Log.d(TAG, "OnImageAvailable warning: image null ");
                    }
                } catch (Throwable t) {
                    Log.e(TAG, "OnImageAvailable error: " + t.getMessage(), t);
                } finally {
                    if (image != null) image.close();
                }
            }, getBackgroundHandler());

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

    // -------------------------------------------------------
    // 3) Captura de foto (STILL_CAPTURE)
    // -------------------------------------------------------
    public synchronized void takePhoto() {
        if (cameraDevice == null || captureSession == null || imageReader == null) {
            Log.e(TAG, "takePhoto: camara no lista");
            return;
        }
        try {
            final CameraDevice device = cameraDevice;
            final android.hardware.camera2.CaptureRequest.Builder builder =
                    device.createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE);
            builder.addTarget(imageReader.getSurface());
            captureSession.capture(builder.build(), null, getBackgroundHandler());
        } catch (CameraAccessException e) {
            Log.e(TAG, "takePhoto: CameraAccessException " + e.getMessage(), e);
        }
    }

    // -------------------------------------------------------
    // 4) Guardar el ultimo JPEG a disco (opcional)
    // -------------------------------------------------------
    public synchronized String saveResult() {
        if (lastCapturedJpeg == null || lastCapturedJpeg.length == 0) {
            Log.e(TAG, "saveResult: no hay datos de imagen capturada");
            return null;
        }
        try {
            File outFile = createTimestampedFile(context, "jpg");
            writeBytesToFile(lastCapturedJpeg, outFile);
            Log.d(TAG, "Imagen guardada: " + outFile.getAbsolutePath());
            return outFile.getAbsolutePath();
        } catch (IOException io) {
            Log.e(TAG, "saveResult: error escribiendo archivo " + io.getMessage(), io);
            return null;
        }
    }

    // -------------------------------------------------------
    // Utilidades publicas
    // -------------------------------------------------------
    public synchronized byte[] getLastCapturedImage() {
        return lastCapturedJpeg;
    }

    public synchronized void release() {
        try { if (captureSession != null) { captureSession.close(); captureSession = null; } } catch (Throwable ignored) {}
        try { if (cameraDevice != null) { cameraDevice.close(); cameraDevice = null; } } catch (Throwable ignored) {}
        try { if (imageReader != null) { imageReader.close(); imageReader = null; } } catch (Throwable ignored) {}
        stopBackgroundThread();
        lastCapturedJpeg = null;
        Log.d(TAG, "Recursos liberados");
    }

    // -------------------------------------------------------
    // Internos / Helper methods
    // -------------------------------------------------------
    private void ensureManager() {
        if (cameraManager == null) {
            cameraManager = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
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
        if (cameraDevice == null || imageReader == null) {
            Log.e(TAG, "createCaptureSession: faltan recursos");
            return;
        }
        try {
            cameraDevice.createCaptureSession(
                    Collections.singletonList(imageReader.getSurface()),
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

    private Size chooseLargestJpeg(String id) throws CameraAccessException {
        android.hardware.camera2.CameraCharacteristics cc =
                cameraManager.getCameraCharacteristics(id);
        Size[] sizes = cc.get(
                android.hardware.camera2.CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP
        ).getOutputSizes(ImageFormat.JPEG);
        if (sizes == null || sizes.length == 0) return null;
        return Collections.max(Arrays.asList(sizes), new Comparator<Size>() {
            @Override public int compare(Size a, Size b) {
                long ap = (long) a.getWidth() * a.getHeight();
                long bp = (long) b.getWidth() * b.getHeight();
                return Long.compare(ap, bp);
            }
        });
    }

    private synchronized void saveImageToMemory(Image image) {
        try {
            Image.Plane[] planes = image.getPlanes();
            if (planes == null || planes.length == 0) {
                Log.e(TAG, "Imagen sin planes");
                return;
            }
            ByteBuffer buffer = planes[0].getBuffer();
            byte[] data = new byte[buffer.remaining()];
            buffer.get(data);
            lastCapturedJpeg = data;
            Log.d(TAG, "Imagen capturada: " + lastCapturedJpeg.length + " bytes");
        } catch (Throwable t) {
            Log.e(TAG, "saveImageToMemory error: " + t.getMessage(), t);
        }
    }

    private Handler getBackgroundHandler() {
        if (backgroundHandler == null) {
            startBackgroundThread();
        }
        return backgroundHandler;
    }

    private void startBackgroundThread() {
        if (backgroundThread != null && backgroundThread.isAlive()) return;
        backgroundThread = new HandlerThread("Camera2Background");
        backgroundThread.start();
        backgroundHandler = new Handler(backgroundThread.getLooper());
    }

    private void stopBackgroundThread() {
        if (backgroundThread == null) return;
        try {
            backgroundThread.quitSafely();
            backgroundThread.join();
        } catch (InterruptedException ignored) {
        } finally {
            backgroundThread = null;
            backgroundHandler = null;
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
}
