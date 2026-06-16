#pragma once
#include <3ds.h>
#include <stdbool.h>
#define CAM_W 400
#define CAM_H 240
bool camera_init(void);
bool camera_capture(u16 *out); // out must hold CAM_W*CAM_H u16
void camera_exit(void);
