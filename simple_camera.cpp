#include "simple_camera.h"

#include <string>

#undef LOG_TAG
#define LOG_TAG "SimpleCamera"
#include <utils/log.h>
#include <media/NdkImage.h>

namespace simple {

static std::string GetBackFacingCameraId(ACameraManager *camera_manager) {
    ACameraIdList *camera_ids = nullptr;
    ACameraManager_getCameraIdList(camera_manager, &camera_ids);

    std::string back_id;
    for (int i = 0; i < camera_ids->numCameras; ++i) {
        const char *id = camera_ids->cameraIds[i];

        ACameraMetadata *metadata;
        ACameraManager_getCameraCharacteristics(camera_manager, id, &metadata);
        ACameraMetadata_const_entry lensInfo = {0};
        ACameraMetadata_getConstEntry(metadata, ACAMERA_LENS_FACING, &lensInfo);

        auto facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(
                lensInfo.data.u8[0]);

        // Found a back-facing camera?
        if (facing == ACAMERA_LENS_FACING_BACK) {
            back_id = id;
            break;
        }
    }
    ACameraManager_deleteCameraIdList(camera_ids);

    return back_id;
}

/***************************
 * Device callbacks
 ***************************/
static void OnDisconnected(void *context, ACameraDevice *device) {
    // TODO Implement it as needed
}

static void OnError(void *context, ACameraDevice *device, int error) {
    // TODO Implement it as needed
}

void Camera::Open() {
    camera_manager_ = ACameraManager_create();
    if (camera_manager_ == nullptr) {
        throw std::runtime_error("failed to create camera manager");
    }

    camera_id_ = GetBackFacingCameraId(camera_manager_);
    ACameraDevice_stateCallbacks device_callbacks{
            .context = this,
            .onDisconnected = OnDisconnected,
            .onError = OnError,
    };
    auto ret = ACameraManager_openCamera(camera_manager_,
                                         camera_id_.c_str(),
                                         &device_callbacks,
                                         &camera_device_);
    if (ret != ACAMERA_OK) {
        throw std::runtime_error("failed to open camera");
    }
}


std::pair<int32_t, int32_t> Camera::GetPreviewSize(int32_t window_width, int32_t window_height) {
    LOGI("Window width=%d, height=%d", window_width, window_height);
    if (window_width < window_height) {
        std::swap(window_width, window_height);
    }

    double min_diff = INT_MAX;
    ACameraMetadata *metadata;
    ACameraManager_getCameraCharacteristics(camera_manager_, camera_id_.c_str(), &metadata);
    ACameraMetadata_const_entry entry;
    ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);
    int32_t match_width = 0;
    int32_t match_height = 0;
    for (int i = 0; i < entry.count; i += 4) {
        int32_t format = entry.data.i32[i + 0];
        int32_t width = entry.data.i32[i + 1];
        int32_t height = entry.data.i32[i + 2];
        int32_t input = entry.data.i32[i + 3];
        if (!input
            && (format == AIMAGE_FORMAT_YUV_420_888 || format == AIMAGE_FORMAT_JPEG)
            && width <= window_width
            && height <= window_height) {
            if (width < height) {
                std::swap(width, height);
            }
            double diff = sqrt((window_width - width) * (window_width - width) +
                               (window_height - height) * (window_height - height));
            if (diff < min_diff) {
                min_diff = diff;
                match_width = width;
                match_height = height;
            }
        }
    }

    LOGI("Preview width=%d, height=%d.", match_width, match_height);

    return {match_width, match_height};
}

void Camera::Close() {
    StopStream();
    if (camera_device_) {
        ACameraDevice_close(camera_device_);
        camera_device_ = nullptr;
    }
    if (camera_manager_) {
        ACameraManager_delete(camera_manager_);
        camera_manager_ = nullptr;
    }
}

/***************************
 * Session state callbacks
 ***************************/
static void OnSessionActive(void *context, ACameraCaptureSession *session) {
    // TODO Implement it as needed
}

static void OnSessionReady(void *context, ACameraCaptureSession *session) {
    // TODO Implement it as needed
}

static void OnSessionClosed(void *context, ACameraCaptureSession *session) {
    // TODO Implement it as needed
}

/***************************
 * Capture callbacks
 ***************************/
static void OnCaptureStarted(
        void *context,
        ACameraCaptureSession *session,
        const ACaptureRequest *request,
        int64_t timestamp) {
    // TODO Implement it as needed
}

static void OnCaptureProgressed(
        void *context,
        ACameraCaptureSession *session,
        ACaptureRequest *request,
        const ACameraMetadata *result) {
    // TODO Implement it as needed
}

static void OnCaptureFailed(
        void *context,
        ACameraCaptureSession *session,
        ACaptureRequest *request,
        ACameraCaptureFailure *failure) {
    // TODO Implement it as needed
}

static void OnCaptureSequenceCompleted(
        void *context,
        ACameraCaptureSession *session,
        int sequence_id,
        int64_t frame_number) {
    // TODO Implement it as needed
}

static void OnCaptureSequenceAborted(
        void *context,
        ACameraCaptureSession *session,
        int sequence_id) {
    // TODO Implement it as needed
}

static void OnCaptureCompleted(
        void *context,
        ACameraCaptureSession *session,
        ACaptureRequest *request,
        const ACameraMetadata *result) {
    // TODO Implement it as needed
}

static void OnCaptureBufferLost(
        void *context,
        ACameraCaptureSession *session,
        ACaptureRequest *request,
        ACameraWindowType *window,
        int64_t frame_number) {
    // TODO Implement it as needed
}

void Camera::StartStream(void *surface) {
    surface_ = static_cast<ANativeWindow *>(surface);
    if (camera_device_ && surface_) {
        // Prepare request
        ACameraDevice_createCaptureRequest(camera_device_,
                                           TEMPLATE_PREVIEW,
                                           &capture_request_);

        // Prepare outputs for session
        ACaptureSessionOutput_create(surface_, &capture_session_output_);
        ACaptureSessionOutputContainer_create(&capture_session_output_container_);
        ACaptureSessionOutputContainer_add(capture_session_output_container_,
                                           capture_session_output_);

        // Prepare output target surface
        ANativeWindow_acquire(surface_);
        ACameraOutputTarget_create(surface_, &output_target_);
        ACaptureRequest_addTarget(capture_request_, output_target_);

        // CreateGraphicsPipeline the session
        ACameraCaptureSession_stateCallbacks session_state_callbacks{
                .context = this,
                .onClosed = OnSessionClosed,
                .onReady = OnSessionReady,
                .onActive = OnSessionActive,
        };
        ACameraDevice_createCaptureSession(camera_device_,
                                           capture_session_output_container_,
                                           &session_state_callbacks,
                                           &capture_session_);

        // Start capturing continuously
        ACameraCaptureSession_captureCallbacks capture_callbacks{
                .context = this,
                .onCaptureStarted = OnCaptureStarted,
                .onCaptureProgressed = OnCaptureProgressed,
                .onCaptureCompleted = OnCaptureCompleted,
                .onCaptureFailed = OnCaptureFailed,
                .onCaptureSequenceCompleted = OnCaptureSequenceCompleted,
                .onCaptureSequenceAborted = OnCaptureSequenceAborted,
                .onCaptureBufferLost = OnCaptureBufferLost,
        };
        ACameraCaptureSession_setRepeatingRequest(capture_session_,
                                                  &capture_callbacks,
                                                  1,
                                                  &capture_request_,
                                                  nullptr);
    } else {
        throw std::runtime_error("failed to start stream");
    }
}

void Camera::StopStream() {
    if (surface_ != nullptr) {
        // Stop recording and do some cleanup
        ACameraCaptureSession_stopRepeating(capture_session_);
        ACameraCaptureSession_close(capture_session_);
        ACaptureSessionOutputContainer_free(capture_session_output_container_);
        ACaptureSessionOutput_free(capture_session_output_);
        ANativeWindow_release(surface_);
        ACaptureRequest_free(capture_request_);
        surface_ = nullptr;
    }
}

} //namespace simple