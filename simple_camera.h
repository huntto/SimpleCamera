#ifndef SIMPLE_CAMERA_H
#define SIMPLE_CAMERA_H

#include <camera/NdkCameraManager.h>
#include <string>

namespace simple {

class Camera {
public:
    void Open();

    void Close();

    std::pair<int32_t, int32_t> GetPreviewSize(int32_t window_width, int32_t window_height);

    void StartStream(void *surface);

    void StopStream();

private:
    ACameraManager *camera_manager_;
    std::string camera_id_;
    ACameraDevice *camera_device_;
    ANativeWindow *surface_;
    ACaptureRequest *capture_request_;

    ACameraOutputTarget *output_target_;
    ACameraCaptureSession *capture_session_;
    ACaptureSessionOutput *capture_session_output_;
    ACaptureSessionOutputContainer *capture_session_output_container_;
};

} // namespace simple

#endif //SIMPLE_CAMERA_H
