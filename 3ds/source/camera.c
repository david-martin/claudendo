#include "camera.h"

// Configure both lenses up front so switching is just a re-activate.
static void configure(u32 cams){
    CAMU_SetSize(cams, SIZE_CTR_TOP_LCD, CONTEXT_A);                 // 400x240
    CAMU_SetOutputFormat(cams, OUTPUT_RGB_565, CONTEXT_A);
    CAMU_SetFrameRate(cams, FRAME_RATE_30);
    CAMU_SetNoiseFilter(cams, true);
    CAMU_SetAutoExposure(cams, true);
    CAMU_SetAutoWhiteBalance(cams, true);
}

bool camera_init(void){
    if (R_FAILED(camInit())) return false;
    configure(SELECT_OUT1 | SELECT_IN1);
    CAMU_Activate(SELECT_OUT1);
    return true;
}

void camera_set_inner(bool inner){
    CAMU_Activate(inner ? SELECT_IN1 : SELECT_OUT1);
}

bool camera_frame(u16 *out){
    u32 bufSize = 0;
    if (R_FAILED(CAMU_GetMaxBytes(&bufSize, CAM_W, CAM_H))) return false;
    CAMU_SetTransferBytes(PORT_CAM1, bufSize, CAM_W, CAM_H);
    CAMU_ClearBuffer(PORT_CAM1);
    CAMU_StartCapture(PORT_CAM1);
    Handle rx = 0; bool ok = false;
    if (R_SUCCEEDED(CAMU_SetReceiving(&rx, out, PORT_CAM1, CAM_W*CAM_H*2, (s16)bufSize))) {
        ok = R_SUCCEEDED(svcWaitSynchronization(rx, 1000000000LL));
        svcCloseHandle(rx);
    }
    CAMU_StopCapture(PORT_CAM1);
    return ok;
}

void camera_exit(void){
    CAMU_Activate(SELECT_NONE);
    camExit();
}
