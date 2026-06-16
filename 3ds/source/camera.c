#include "camera.h"
bool camera_init(void){
    if (R_FAILED(camInit())) return false;
    CAMU_SetSize(SELECT_OUT1, SIZE_CTR_TOP_LCD, CONTEXT_A);          // 400x240
    CAMU_SetOutputFormat(SELECT_OUT1, OUTPUT_RGB_565, CONTEXT_A);
    CAMU_SetFrameRate(SELECT_OUT1, FRAME_RATE_30);
    CAMU_SetNoiseFilter(SELECT_OUT1, true);
    CAMU_SetAutoExposure(SELECT_OUT1, true);
    CAMU_SetAutoWhiteBalance(SELECT_OUT1, true);
    return true;
}
bool camera_capture(u16 *out){
    u32 bufSize = 0;
    if (R_FAILED(CAMU_GetMaxBytes(&bufSize, CAM_W, CAM_H))) return false;
    CAMU_SetTransferBytes(PORT_CAM1, bufSize, CAM_W, CAM_H);
    CAMU_Activate(SELECT_OUT1);
    CAMU_ClearBuffer(PORT_CAM1);
    CAMU_StartCapture(PORT_CAM1);
    Handle rxEvent = 0;
    bool ok = false;
    if (R_SUCCEEDED(CAMU_SetReceiving(&rxEvent, out, PORT_CAM1, CAM_W*CAM_H*2, (s16)bufSize))) {
        ok = R_SUCCEEDED(svcWaitSynchronization(rxEvent, 1000000000LL)); // 1s
        svcCloseHandle(rxEvent);
    }
    CAMU_StopCapture(PORT_CAM1);
    CAMU_Activate(SELECT_NONE);
    return ok;
}
void camera_exit(void){ camExit(); }
