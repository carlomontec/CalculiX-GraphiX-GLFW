/* --------------------------------------------------------------------  */
/* CalculiX GraphiX (GLFW Edition)                                       */
/* File: cgx_capture.h - Native Screen Capture & Movie Recording Engine  */
/*                                                                       */
/* Created by Carlo Monjaraz-Tec (2026) with AI pair-programming         */
/* assistance based on the original work of Klaus Wittig & contributors. */
/*                                                                       */
/* Licensed under GNU General Public License v2 (GPL-2.0 or later).      */
/* --------------------------------------------------------------------  */

#ifndef CGX_CAPTURE_H
#define CGX_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize capture engine */
void cgx_capture_init(void);

/* Save current viewport / framebuffer to a lossless PNG image */
int cgx_save_png(const char *filename);

/* Clean generated numbered hcpy files */
void cgx_clean_hcpy_files(void);

/* Movie & Video Recording API */
int  cgx_movie_start(const char *filename, int max_frames, double delay_sec);
void cgx_movie_add_frame(void);
int  cgx_movie_finish(void);
void cgx_movie_clean(void);
int  cgx_movie_is_recording(void);
void cgx_movie_set_delay(double delay_sec);
double cgx_movie_get_delay(void);
int  cgx_movie_has_custom_delay(void);

#ifdef __cplusplus
}
#endif

#endif /* CGX_CAPTURE_H */
