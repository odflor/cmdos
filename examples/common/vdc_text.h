#ifndef VDC_TEXT_H
#define VDC_TEXT_H

/* Rasterizador de texto bitmap para el framebuffer 640x200 del VDC. */
void vdc_text_style(unsigned char magx, unsigned char magy, unsigned char dir);
void vdc_text_outtextxy(int x, int y, const char* text);

#endif
