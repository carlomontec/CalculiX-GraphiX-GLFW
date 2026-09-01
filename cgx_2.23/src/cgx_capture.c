/* --------------------------------------------------------------------  */
/* CalculiX GraphiX (GLFW Edition)                                       */
/* File: cgx_capture.c - Native Screen Capture & Video/GIF Engine        */
/*                                                                       */
/* Created by Carlo Monjaraz-Tec (2026) with AI pair-programming         */
/* assistance based on the original work of Klaus Wittig & contributors. */
/*                                                                       */
/* Licensed under GNU General Public License v2 (GPL-2.0 or later).      */
/* --------------------------------------------------------------------  */

#include "cgx_capture.h"
#include "cgx_glut_glfw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define MSF_GIF_IMPL
#include "msf_gif.h"

#if defined(_WIN32) || defined(WIN32)
  #define popen _popen
  #define pclose _pclose
#endif

typedef enum {
  MOVIE_FMT_NONE = 0,
  MOVIE_FMT_MP4,
  MOVIE_FMT_OGV,
  MOVIE_FMT_WEBM,
  MOVIE_FMT_GIF
} MovieFormat;

/* Internal movie recording state */
static int g_movie_active = 0;
static MovieFormat g_movie_format = MOVIE_FMT_NONE;
static char g_movie_filename[512] = {0};
static FILE *g_movie_pipe = NULL;
static MsfGifState g_gif_state;
static int g_movie_w = 0;
static int g_movie_h = 0;
static int g_movie_frame_count = 0;
static int g_movie_target_frames = 0;
static double g_movie_delay_sec = 0.04; /* ~25 fps default */
static int g_movie_custom_delay = 0;

void cgx_capture_init(void)
{
  g_movie_active = 0;
  g_movie_format = MOVIE_FMT_NONE;
  g_movie_pipe = NULL;
  g_movie_frame_count = 0;
  g_movie_custom_delay = 0;
}

/* Save current GLFW OpenGL framebuffer to a crisp PNG image */
int cgx_save_png(const char *filename)
{
  GLFWwindow *win = cgx_glfw_get_window();
  if (!win)
  {
    fprintf(stderr, "[CGX Error] No active GLFW window for screenshot.\n");
    return -1;
  }

  int fb_w = 0, fb_h = 0;
  glfwGetFramebufferSize(win, &fb_w, &fb_h);
  if (fb_w <= 0 || fb_h <= 0)
  {
    fprintf(stderr, "[CGX Error] Invalid framebuffer size (%dx%d).\n", fb_w, fb_h);
    return -1;
  }

  unsigned char *pixels = (unsigned char *)malloc(fb_w * fb_h * 4);
  if (!pixels)
  {
    fprintf(stderr, "[CGX Error] Out of memory allocating screenshot buffer.\n");
    return -1;
  }

  /* Read raw pixels directly from GPU VRAM */
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, fb_w, fb_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  /* Flip vertically on write so OpenGL bottom-up becomes standard top-down */
  stbi_flip_vertically_on_write(1);

  int ok = stbi_write_png(filename, fb_w, fb_h, 4, pixels, fb_w * 4);
  free(pixels);

  if (ok)
  {
    printf("Screenshot saved: %s (%dx%d px)\n", filename, fb_w, fb_h);
    return 0;
  }
  else
  {
    fprintf(stderr, "[CGX Error] Failed to write PNG image to '%s'.\n", filename);
    return -1;
  }
}

/* Clean all numbered hcpy_*.png files in current directory natively */
void cgx_clean_hcpy_files(void)
{
  DIR *dir = opendir(".");
  if (!dir) return;

  struct dirent *entry;
  int count = 0;
  while ((entry = readdir(dir)) != NULL)
  {
    if (strncmp(entry->d_name, "hcpy_", 5) == 0)
    {
      size_t len = strlen(entry->d_name);
      if (len > 4 && strcmp(entry->d_name + len - 4, ".png") == 0)
      {
        unlink(entry->d_name);
        count++;
      }
    }
  }
  closedir(dir);
  printf("Cleaned %d hcpy_*.png file(s).\n", count);
}

int cgx_movie_is_recording(void)
{
  return g_movie_active;
}

void cgx_movie_set_delay(double delay_sec)
{
  if (delay_sec > 0.001)
  {
    g_movie_delay_sec = delay_sec;
    g_movie_custom_delay = 1;
  }
}

double cgx_movie_get_delay(void)
{
  return g_movie_delay_sec;
}

int cgx_movie_has_custom_delay(void)
{
  return g_movie_custom_delay;
}

/* Start recording video (MP4/OGV/WebM) or animated GIF */
int cgx_movie_start(const char *filename, int max_frames, double delay_sec)
{
  if (g_movie_active)
  {
    printf("[CGX Notice] Movie recording already active. Finishing previous movie first...\n");
    cgx_movie_finish();
  }

  GLFWwindow *win = cgx_glfw_get_window();
  if (!win)
  {
    fprintf(stderr, "[CGX Error] No active GLFW window for movie recording.\n");
    return -1;
  }

  glfwGetFramebufferSize(win, &g_movie_w, &g_movie_h);
  if (g_movie_w <= 0 || g_movie_h <= 0)
  {
    fprintf(stderr, "[CGX Error] Invalid framebuffer size for movie recording.\n");
    return -1;
  }

  /* Make dimensions even for H.264 video encoders */
  if (g_movie_w % 2 != 0) g_movie_w--;
  if (g_movie_h % 2 != 0) g_movie_h--;

  if (delay_sec > 0.001) g_movie_delay_sec = delay_sec;
  g_movie_target_frames = max_frames;
  g_movie_frame_count = 0;

  /* Determine target format */
  char target[512];
  if (!filename || strlen(filename) == 0)
  {
    strcpy(target, "movie.mp4");
  }
  else
  {
    strncpy(target, filename, sizeof(target) - 1);
    target[sizeof(target) - 1] = '\0';
  }

  /* Check extension */
  size_t len = strlen(target);
  if (len > 4 && strcasecmp(target + len - 4, ".gif") == 0)
  {
    g_movie_format = MOVIE_FMT_GIF;
  }
  else if (len > 4 && strcasecmp(target + len - 4, ".ogv") == 0)
  {
    g_movie_format = MOVIE_FMT_OGV;
  }
  else if (len > 5 && strcasecmp(target + len - 5, ".webm") == 0)
  {
    g_movie_format = MOVIE_FMT_WEBM;
  }
  else
  {
    if (len <= 4 || strcasecmp(target + len - 4, ".mp4") != 0)
    {
      strcat(target, ".mp4");
    }
    g_movie_format = MOVIE_FMT_MP4;
  }

  strncpy(g_movie_filename, target, sizeof(g_movie_filename) - 1);

  int fps = (int)(1.0 / g_movie_delay_sec + 0.5);
  if (fps < 1) fps = 25;
  if (fps > 60) fps = 60;

  if (g_movie_format == MOVIE_FMT_GIF)
  {
    /* Native pure-C GIF recording */
    memset(&g_gif_state, 0, sizeof(g_gif_state));
    msf_gif_begin(&g_gif_state, g_movie_w, g_movie_h);
    g_movie_active = 1;
    printf("[CGX] Started recording animated GIF: %s (%dx%d px @ %d fps)...\n",
           g_movie_filename, g_movie_w, g_movie_h, fps);
    return 0;
  }
  else
  {
    /* Real-time video streaming via ffmpeg pipe */
    char cmd[1024];
    if (g_movie_format == MOVIE_FMT_OGV)
    {
      snprintf(cmd, sizeof(cmd),
               "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgba -s %dx%d -r %d -i - "
               "-vf vflip -c:v libtheora -qscale:v 7 \"%s\" 2>/dev/null",
               g_movie_w, g_movie_h, fps, g_movie_filename);
    }
    else if (g_movie_format == MOVIE_FMT_WEBM)
    {
      snprintf(cmd, sizeof(cmd),
               "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgba -s %dx%d -r %d -i - "
               "-vf vflip -c:v libvpx-vp9 -b:v 2M \"%s\" 2>/dev/null",
               g_movie_w, g_movie_h, fps, g_movie_filename);
    }
    else
    {
      snprintf(cmd, sizeof(cmd),
               "ffmpeg -y -f rawvideo -vcodec rawvideo -pix_fmt rgba -s %dx%d -r %d -i - "
               "-vf vflip -c:v libx264 -preset medium -crf 20 -pix_fmt yuv420p \"%s\" 2>/dev/null",
               g_movie_w, g_movie_h, fps, g_movie_filename);
    }

    g_movie_pipe = popen(cmd, "w");
    if (!g_movie_pipe)
    {
      fprintf(stderr, "\n==================================================================\n");
      fprintf(stderr, "[CGX Error] Could not launch ffmpeg for video encoding.\n");
      fprintf(stderr, "To record MP4 / OGV / WebM videos, please install ffmpeg:\n");
      fprintf(stderr, "  - macOS:   brew install ffmpeg\n");
      fprintf(stderr, "  - Linux:   sudo apt install ffmpeg (or dnf/pacman)\n");
      fprintf(stderr, "  - Windows: winget install Gyan.FFmpeg\n");
      fprintf(stderr, "             (or place ffmpeg.exe next to cgx_glfw.exe)\n");
      fprintf(stderr, "See exporting_videos.md for complete guidance.\n");
      fprintf(stderr, "Tip: You can also record animated GIFs directly without ffmpeg:\n");
      fprintf(stderr, "     movie start movie.gif\n");
      fprintf(stderr, "==================================================================\n\n");
      return -1;
    }

    g_movie_active = 1;
    printf("[CGX] Started recording video: %s (%dx%d px @ %d fps)...\n",
           g_movie_filename, g_movie_w, g_movie_h, fps);
    return 0;
  }
}

/* Capture current OpenGL frame and append to active movie stream */
void cgx_movie_add_frame(void)
{
  if (!g_movie_active) return;

  unsigned char *pixels = (unsigned char *)malloc(g_movie_w * g_movie_h * 4);
  if (!pixels) return;

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, g_movie_w, g_movie_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  if (g_movie_format == MOVIE_FMT_GIF)
  {
    /* Flip vertically in memory for GIF encoder */
    unsigned char *flipped = (unsigned char *)malloc(g_movie_w * g_movie_h * 4);
    if (flipped)
    {
      int row_stride = g_movie_w * 4;
      for (int y = 0; y < g_movie_h; y++)
      {
        memcpy(flipped + y * row_stride,
               pixels + (g_movie_h - 1 - y) * row_stride,
               row_stride);
      }
      int centisec = (int)(g_movie_delay_sec * 100.0 + 0.5);
      if (centisec < 1) centisec = 4;
      msf_gif_frame(&g_gif_state, flipped, centisec, 16, row_stride);
      free(flipped);
    }
  }
  else if (g_movie_pipe)
  {
    /* Video stream pipe (vflip is handled in hardware filter by ffmpeg) */
    fwrite(pixels, 1, g_movie_w * g_movie_h * 4, g_movie_pipe);
  }

  free(pixels);
  g_movie_frame_count++;

  if (g_movie_target_frames > 0 && g_movie_frame_count >= g_movie_target_frames)
  {
    cgx_movie_finish();
  }
}

/* Finish active movie recording and write output file */
int cgx_movie_finish(void)
{
  if (!g_movie_active) return 0;

  int total_frames = g_movie_frame_count;
  char out_file[512];
  strncpy(out_file, g_movie_filename, sizeof(out_file) - 1);
  out_file[sizeof(out_file) - 1] = '\0';

  if (g_movie_format == MOVIE_FMT_GIF)
  {
    MsfGifResult result = msf_gif_end(&g_gif_state);
    if (result.data)
    {
      FILE *fp = fopen(out_file, "wb");
      if (fp)
      {
        fwrite(result.data, 1, result.dataSize, fp);
        fclose(fp);
        printf("[CGX] Animated GIF movie saved: %s (%d frames, %.2f MB)\n",
               out_file, total_frames, (double)result.dataSize / (1024.0 * 1024.0));
      }
      else
      {
        fprintf(stderr, "[CGX Error] Could not write GIF output file '%s'.\n", out_file);
      }
      msf_gif_free(result);
    }
  }
  else if (g_movie_pipe)
  {
    pclose(g_movie_pipe);
    g_movie_pipe = NULL;
    printf("[CGX] Video movie saved: %s (%d frames)\n", out_file, total_frames);
  }

  g_movie_active = 0;
  g_movie_format = MOVIE_FMT_NONE;
  g_movie_frame_count = 0;
  g_movie_target_frames = 0;
  return 0;
}

/* Remove default movie files in directory */
void cgx_movie_clean(void)
{
  unlink("movie.mp4");
  unlink("movie.gif");
  unlink("movie.ogv");
  unlink("movie.webm");
  printf("Cleaned movie video and GIF output files.\n");
}
