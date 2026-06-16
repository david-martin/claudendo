#pragma once
#include <stdbool.h>
typedef struct { char host[128]; char token[160]; } Config;
bool config_load(Config *c);
