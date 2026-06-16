#pragma once
#include <3ds.h>
#include <stdbool.h>
#define CAM_W 400
#define CAM_H 240
bool camera_init(void);
bool camera_frame(u16 *out); // grab one preview frame (camera already activated); out holds CAM_W*CAM_H u16
void camera_exit(void);
