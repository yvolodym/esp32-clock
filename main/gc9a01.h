
#ifndef GC9A01_H
#define GC9A01_H

#include <stdint.h>
#include "driver/spi_master.h"

#define GC9A01_WIDTH  240
#define GC9A01_HEIGHT 240

#define GC9A01_COLOR_BLACK   0x0000
#define GC9A01_COLOR_WHITE   0xFFFF
#define GC9A01_COLOR_RED     0xF800

typedef void* gc9a01_handle_t;

gc9a01_handle_t gc9a01_init();
void gc9a01_draw_pixel(gc9a01_handle_t dev, int x, int y, uint16_t color);
void gc9a01_draw_line(gc9a01_handle_t dev, int x0, int y0, int x1, int y1, uint16_t color);
void gc9a01_draw_circle(gc9a01_handle_t dev, int x, int y, int radius, uint16_t color);
void gc9a01_fill_screen(gc9a01_handle_t dev, uint16_t color);

#endif