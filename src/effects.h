#pragma once
#include <pebble.h>  

// used to pass bimap info to get/set pixel accurately  
typedef struct {
   GBitmap *bitmap;  // actual bitmap for Chalk raw manipulation
   uint8_t *bitmap_data;
   int bytes_per_row;
   GBitmapFormat bitmap_format;
}  BitmapInfo;

typedef void effect_cb(GContext* ctx, GRect position, void* param);

// inverter effect.
// Added by Yuriy Galanter
effect_cb effect_invert;
