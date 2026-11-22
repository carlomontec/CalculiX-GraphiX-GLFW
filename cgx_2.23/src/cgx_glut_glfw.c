/* --------------------------------------------------------------------  */
/* CalculiX GraphiX (GLFW Edition)                                       */
/* File: cgx_glut_glfw.c - Modern GLFW3 & OpenGL Backend for CGX         */
/*                                                                       */
/* Created by Carlo Monjaraz-Tec (2026) with AI pair-programming         */
/* assistance as an academic exercise based on the original work of      */
/* Klaus Wittig and contributors.                                        */
/*                                                                       */
/* Licensed under GNU General Public License v2 (GPL-2.0 or later).      */
/* --------------------------------------------------------------------  */

#include "cgx_glut_glfw.h"
#include "cgx_app_icon.h"
#include "cgx_shaders.h"
#include "cgx_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_WINDOWS 16
#define MAX_MENUS   128
#define MAX_MENU_ITEMS 128
#define MAX_MENU_DEPTH 8
#define CMD_BAR_HEIGHT 38
#define MENU_ITEM_HEIGHT 25
#define MENU_BOX_WIDTH 250

/* External references from CGX */
extern int mainmenu;
extern int activWindow;
extern int w0, w1, w2, w3;
extern void cgx_execute_command_string(const char *cmd_str);
extern void createHardcopy(int selection, char *fileName);
extern void rot_r(double a);
extern int cgx_movie_is_recording(void);
extern void cgx_movie_add_frame(void);
extern GLdouble R[4][4];
extern int perspectiveFlag;
extern int backgrndcol;
extern char elemEdgeFlag;
extern char addDispFlag;
extern double anim_faktor;
extern void frame(void);
extern void selectView(int selection);
extern void tuneAnimation(int selection);

/* Window definition */
typedef struct {
  int id;
  int parent_id; /* 0 for root */
  int x, y, width, height; /* coordinates in window units */
  int visible;
  
  /* Callbacks */
  void (*display_func)(void);
  void (*reshape_func)(int width, int height);
  void (*keyboard_func)(unsigned char key, int x, int y);
  void (*special_func)(int key, int x, int y);
  void (*mouse_func)(int button, int state, int x, int y);
  void (*motion_func)(int x, int y);
  void (*passive_motion_func)(int x, int y);
  void (*visibility_func)(int state);
  void (*entry_func)(int state);
  
  /* Attached menu (button -> menu_id) */
  int attached_menu[5];
} CGXWindow;

/* Menu item definition */
typedef struct {
  char label[128];
  int value;
  int submenu_id; /* -1 if regular entry */
  int is_submenu;
} CGXMenuItem;

/* Menu definition */
typedef struct {
  int id;
  void (*callback)(int value);
  CGXMenuItem items[MAX_MENU_ITEMS];
  int num_items;
} CGXMenu;

/* Cascade menu level */
typedef struct {
  int menu_id;
  int x, y;
  int width, height;
  int hovered_item;
} CGXMenuCascade;

/* Global state */
static GLFWwindow *g_glfw_window = NULL;
static int g_init_w = 800, g_init_h = 600;
static unsigned int g_display_mode = GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH;
static void (*g_idle_func)(void) = NULL;

static CGXWindow g_windows[MAX_WINDOWS];
static int g_num_windows = 0;
static int g_current_window_id = 1;
static volatile int g_need_redisplay = 1;

static CGXMenu g_menus[MAX_MENUS];
static int g_num_menus = 0;
static int g_current_menu_id = 0;

/* Multi-level recursive cascade popup menu */
static CGXMenuCascade g_cascade[MAX_MENU_DEPTH];
static int g_cascade_depth = 0;

/* Interactive Command Bar state */
static int  g_cmd_bar_visible = 1;
static char g_cmd_buf[256] = "";
static int  g_cmd_len = 0;
static int  g_cmd_focused = 1;
static int  g_send_hovered = 0;
static char g_last_cmd_echo[256] = "Ready";
static void init_truetype_fonts(void);

/* Command History */
#define MAX_CMD_HIST 64
static char g_cmd_history[MAX_CMD_HIST][256];
static int  g_cmd_hist_count = 0;
static int  g_cmd_hist_pos = 0;

/* Mouse state */
static double g_last_mouse_x = 0.0;
static double g_last_mouse_y = 0.0;
static int g_mouse_buttons[5] = { GLUT_UP, GLUT_UP, GLUT_UP, GLUT_UP, GLUT_UP };

/* Lightweight Interactive HUD & Turntable Auto-Rotate (all hidden/off by default) */
static int    g_auto_rotate_active = 0;
static double g_auto_rotate_speed = 25.0; /* degrees per second */
static double g_auto_rotate_last_time = 0.0;
static int    g_turntable_rec_remaining = 0;
static double g_turntable_step_deg = 3.0;

static int    g_hud_toolbar_visible = 0;
static int    g_gizmo_visible = 0;
static int    g_hud_hovered_btn = -1;
static int    g_hud_tab_hovered = 0;
static int    g_gizmo_hovered_idx = -1;
static int    g_is_wireframe = 0;

/* Helper: find window by id */
static CGXWindow *get_window(int id)
{
  for (int i = 0; i < g_num_windows; i++)
  {
    if (g_windows[i].id == id) return &g_windows[i];
  }
  return NULL;
}

/* Helper: find menu by id */
static CGXMenu *get_menu(int id)
{
  for (int i = 0; i < g_num_menus; i++)
  {
    if (g_menus[i].id == id) return &g_menus[i];
  }
  return NULL;
}

/* Helper: compute absolute screen rectangle for hierarchical windows */
static void get_window_screen_rect(CGXWindow *win, int *out_x, int *out_y, int *out_w, int *out_h)
{
  int x = win->x;
  int y = win->y;
  int parent_id = win->parent_id;
  while (parent_id > 0)
  {
    CGXWindow *parent = get_window(parent_id);
    if (parent)
    {
      x += parent->x;
      y += parent->y;
      parent_id = parent->parent_id;
    }
    else break;
  }
  *out_x = x;
  *out_y = y;
  *out_w = win->width;
  *out_h = win->height;
}

/* Helper: find window at screen position (x, y) */
static CGXWindow *find_window_at(int x, int y)
{
  for (int i = g_num_windows - 1; i >= 0; i--)
  {
    if (g_windows[i].parent_id != 0 && g_windows[i].visible)
    {
      int sx, sy, sw, sh;
      get_window_screen_rect(&g_windows[i], &sx, &sy, &sw, &sh);
      if (x >= sx && x < sx + sw && y >= sy && y < sy + sh)
      {
        return &g_windows[i];
      }
    }
  }
  for (int i = 0; i < g_num_windows; i++)
  {
    if (g_windows[i].parent_id == 0 && g_windows[i].visible) return &g_windows[i];
  }
  return (g_num_windows > 0) ? &g_windows[0] : NULL;
}

GLFWwindow *cgx_glfw_get_window(void)
{
  return g_glfw_window;
}

void cgx_glfw_toggle_command_bar(void)
{
  g_cmd_bar_visible = !g_cmd_bar_visible;
  if (g_cmd_bar_visible) printf("\n Command Line Bar shown.\n\n");
  else printf("\n Command Line Bar hidden.\n\n");
  g_need_redisplay = 1;
}

int cgx_glfw_is_command_bar_visible(void)
{
  return g_cmd_bar_visible;
}

void cgx_glfw_set_auto_rotate(int enable, double deg_per_sec)
{
  g_auto_rotate_active = enable ? 1 : 0;
  if (deg_per_sec != 0.0)
  {
    g_auto_rotate_speed = deg_per_sec;
  }
  g_auto_rotate_last_time = g_glfw_window ? glfwGetTime() : 0.0;
  if (g_auto_rotate_active)
  {
    char msg[128];
    snprintf(msg, sizeof(msg), "Auto-rotate ON (%.1f deg/s) - click model to stop", g_auto_rotate_speed);
    printf("\n %s\n\n", msg);
    cgx_set_gui_status(msg);
  }
  else
  {
    printf("\n Auto-rotate OFF.\n\n");
    cgx_set_gui_status("Auto-rotate stopped");
  }
  g_need_redisplay = 1;
  if (g_glfw_window) glfwPostEmptyEvent();
}

void cgx_glfw_toggle_auto_rotate(void)
{
  cgx_glfw_set_auto_rotate(!g_auto_rotate_active, 0.0);
}

int cgx_glfw_is_auto_rotate_active(void)
{
  return g_auto_rotate_active;
}

void cgx_glfw_start_turntable_recording(int total_frames)
{
  if (total_frames < 12) total_frames = 120;
  g_auto_rotate_active = 0;
  g_turntable_rec_remaining = total_frames;
  g_turntable_step_deg = 360.0 / (double)total_frames;
  g_need_redisplay = 1;
  if (g_glfw_window) glfwPostEmptyEvent();
}

void cgx_glfw_set_hud_toolbar_visible(int visible)
{
  g_hud_toolbar_visible = visible ? 1 : 0;
  g_hud_hovered_btn = -1;
  printf("\n HUD Toolbar %s (F2).\n\n", g_hud_toolbar_visible ? "shown" : "hidden");
  cgx_set_gui_status(g_hud_toolbar_visible ? "HUD Toolbar ON (F2)" : "HUD Toolbar OFF (F2)");
  g_need_redisplay = 1;
  if (g_glfw_window) glfwPostEmptyEvent();
}

void cgx_glfw_toggle_hud_toolbar(void)
{
  cgx_glfw_set_hud_toolbar_visible(!g_hud_toolbar_visible);
}

int cgx_glfw_is_hud_toolbar_visible(void)
{
  return g_hud_toolbar_visible;
}

void cgx_glfw_set_gizmo_visible(int visible)
{
  g_gizmo_visible = visible ? 1 : 0;
  g_gizmo_hovered_idx = -1;
  printf("\n Interactive Axes %s (F3).\n\n", g_gizmo_visible ? "shown" : "hidden");
  cgx_set_gui_status(g_gizmo_visible ? "Interactive Axes ON (F3)" : "Interactive Axes OFF (F3)");
  g_need_redisplay = 1;
  if (g_glfw_window) glfwPostEmptyEvent();
}

void cgx_glfw_toggle_gizmo(void)
{
  cgx_glfw_set_gizmo_visible(!g_gizmo_visible);
}

int cgx_glfw_is_gizmo_visible(void)
{
  return g_gizmo_visible;
}

/* --------------------------------------------------------------------  */
/* Stdin Background Listener Thread for Interactive CLI Commands         */
/* --------------------------------------------------------------------  */
#include <pthread.h>

#define MAX_QUEUED_CMDS 64
static char g_cmd_queue[MAX_QUEUED_CMDS][512];
static int g_cmd_queue_head = 0;
static int g_cmd_queue_tail = 0;
static pthread_mutex_t g_cmd_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

static void queue_command(const char *cmd) {
    pthread_mutex_lock(&g_cmd_queue_mutex);
    int next = (g_cmd_queue_tail + 1) % MAX_QUEUED_CMDS;
    if (next != g_cmd_queue_head) {
        strncpy(g_cmd_queue[g_cmd_queue_tail], cmd, 511);
        g_cmd_queue[g_cmd_queue_tail][511] = '\0';
        g_cmd_queue_tail = next;
    }
    pthread_mutex_unlock(&g_cmd_queue_mutex);
    if (g_glfw_window) glfwPostEmptyEvent();
}

/* Deferred hardcopy: a hcpy arriving through the stdin queue only records a
   request; the capture is done in glutMainLoop after the complete frame
   (3D + overlays) has been drawn, just before the buffer swap. */
static int  g_in_queued_cmd = 0;
static int  g_hcpy_pending = 0;
static char g_hcpy_name[512];

int cgx_defer_hardcopy(const char *name)
{
    if (!g_in_queued_cmd || g_hcpy_pending) return 0;  /* caller captures immediately */
    g_hcpy_name[0] = '\0';
    if (name) { strncpy(g_hcpy_name, name, sizeof(g_hcpy_name) - 1); g_hcpy_name[sizeof(g_hcpy_name) - 1] = '\0'; }
    g_hcpy_pending = 1;
    g_need_redisplay = 1;
    return 1;
}

static void process_queued_commands(void) {
    pthread_mutex_lock(&g_cmd_queue_mutex);
    /* stop draining once a capture is pending: the frame must be rendered first */
    while (g_cmd_queue_head != g_cmd_queue_tail && !g_hcpy_pending) {
        char cmd[512];
        strncpy(cmd, g_cmd_queue[g_cmd_queue_head], 512);
        g_cmd_queue_head = (g_cmd_queue_head + 1) % MAX_QUEUED_CMDS;
        pthread_mutex_unlock(&g_cmd_queue_mutex);
        
        g_in_queued_cmd = 1;
        cgx_execute_command_string(cmd);
        g_in_queued_cmd = 0;
        g_need_redisplay = 1;
        
        pthread_mutex_lock(&g_cmd_queue_mutex);
    }
    pthread_mutex_unlock(&g_cmd_queue_mutex);
}

static void *stdin_listener_thread(void *arg)
{
  (void)arg;
  char line[512];

  while (fgets(line, sizeof(line), stdin))
  {
    queue_command(line);

    strncpy(g_last_cmd_echo, line, sizeof(g_last_cmd_echo) - 1);
    int elen = (int)strlen(g_last_cmd_echo);
    while (elen > 0 && (g_last_cmd_echo[elen-1] == '\n' || g_last_cmd_echo[elen-1] == '\r'))
    {
      g_last_cmd_echo[--elen] = '\0';
    }
  }
  return NULL;
}

/* GLUT Initialization */
void glutInit(int *argcp, char **argv)
{
  (void)argcp; (void)argv;
  if (!glfwInit())
  {
    fprintf(stderr, "ERROR: Failed to initialize GLFW\n");
    exit(EXIT_FAILURE);
  }
}

void glutInitDisplayMode(unsigned int mode)
{
  g_display_mode = mode;
}

void glutInitWindowPosition(int x, int y) { (void)x; (void)y; }

void glutInitWindowSize(int width, int height)
{
  g_init_w = (width > 0) ? width : 800;
  g_init_h = (height > 0) ? height : 600;
}

/* Window Creation & Management */
int glutCreateWindow(const char *title)
{
  int id;
  CGXWindow *win;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
  glfwWindowHint(GLFW_DEPTH_BITS, 24);

  g_glfw_window = glfwCreateWindow(g_init_w, g_init_h, title, NULL, NULL);
  if (!g_glfw_window)
  {
    /* Fallback for remote X11 / XQuartz / software rasterizers: relax framebuffer hints */
    glfwDefaultWindowHints();
    g_glfw_window = glfwCreateWindow(g_init_w, g_init_h, title, NULL, NULL);
  }

  if (!g_glfw_window)
  {
    fprintf(stderr, "\n===============================================================\n");
    fprintf(stderr, "ERROR: Failed to create GLFW window.\n");
    fprintf(stderr, "If using SSH with XQuartz, try running on your Mac:\n");
    fprintf(stderr, "  defaults write org.xquartz.X11 enable_iglx 1\n");
    fprintf(stderr, "and on Linux before running:\n");
    fprintf(stderr, "  export LIBGL_ALWAYS_INDIRECT=1\n");
    fprintf(stderr, "===============================================================\n\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(g_glfw_window);
  glfwSwapInterval(1);
  init_truetype_fonts();
  cgx_shaders_init();

  /* Set runtime window icon for titlebar, dock, and taskbar */
  {
    GLFWimage icon;
    icon.width = CGX_ICON_WIDTH;
    icon.height = CGX_ICON_HEIGHT;
    icon.pixels = (unsigned char *)cgx_app_icon_rgba;
    glfwSetWindowIcon(g_glfw_window, 1, &icon);
  }

  id = 1;
  win = &g_windows[g_num_windows++];
  memset(win, 0, sizeof(CGXWindow));
  win->id = id;
  win->parent_id = 0;
  win->x = 0;
  win->y = 0;
  win->width = g_init_w;
  win->height = g_init_h;
  win->visible = 1;
  for (int b = 0; b < 5; b++) win->attached_menu[b] = 0;

  g_current_window_id = id;

  pthread_t tid;
  pthread_create(&tid, NULL, stdin_listener_thread, NULL);
  pthread_detach(tid);

  return id;
}

int glutCreateSubWindow(int parent, int x, int y, int width, int height)
{
  int id = g_num_windows + 1;
  CGXWindow *win = &g_windows[g_num_windows++];
  memset(win, 0, sizeof(CGXWindow));
  win->id = id;
  win->parent_id = parent;
  win->x = x;
  win->y = y;
  win->width = width;
  win->height = height;
  win->visible = 1;
  for (int b = 0; b < 5; b++) win->attached_menu[b] = 0;

  g_current_window_id = id;
  return id;
}

void glutDestroyWindow(int win) { (void)win; }
int glutGetWindow(void) { return g_current_window_id; }

void glutSetWindow(int win)
{
  CGXWindow *w = get_window(win);
  if (w) g_current_window_id = win;
}

void glutPostRedisplay(void)
{
  g_need_redisplay = 1;
}

void glutSwapBuffers(void)
{
  /* No-op: Frame buffer swap is performed centrally in glutMainLoop after all passes */
}

void glutPositionWindow(int x, int y)
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win && win->parent_id != 0)
  {
    win->x = x;
    win->y = y;
    g_need_redisplay = 1;
  }
  else if (g_glfw_window)
  {
    glfwSetWindowPos(g_glfw_window, x, y);
  }
}

void glutReshapeWindow(int width, int height)
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win && win->parent_id != 0)
  {
    win->width = width;
    win->height = height;
    if (win->reshape_func) win->reshape_func(width, height);
    g_need_redisplay = 1;
  }
  else if (g_glfw_window)
  {
    glfwSetWindowSize(g_glfw_window, width, height);
  }
}

void glutSetWindowTitle(const char *title)
{
  if (g_glfw_window) glfwSetWindowTitle(g_glfw_window, title);
}

void glutSetIconTitle(const char *title) { (void)title; }
void glutPopWindow(void) {}
void glutPushWindow(void) {}
void glutIconifyWindow(void) {}
void glutShowWindow(void) {}
void glutHideWindow(void) {}
void glutFullScreen(void) {}
void glutSetCursor(int cursor) { (void)cursor; }

/* Callback Registrations */
void glutDisplayFunc(void (*func)(void))
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win) win->display_func = func;
}

void glutReshapeFunc(void (*func)(int width, int height))
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win) win->reshape_func = func;
}

void glutKeyboardFunc(void (*func)(unsigned char key, int x, int y))
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win) win->keyboard_func = func;
}

void glutSpecialFunc(void (*func)(int key, int x, int y))
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win) win->special_func = func;
}

void glutMouseFunc(void (*func)(int button, int state, int x, int y))
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win) win->mouse_func = func;
}

void glutMotionFunc(void (*func)(int x, int y))
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win) win->motion_func = func;
}

void glutPassiveMotionFunc(void (*func)(int x, int y))
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win) win->passive_motion_func = func;
}

void glutEntryFunc(void (*func)(int state))
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win) win->entry_func = func;
}

void glutVisibilityFunc(void (*func)(int state))
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win) win->visibility_func = func;
}

void glutIdleFunc(void (*func)(void))
{
  g_idle_func = func;
}

/* Menu Management */
int glutCreateMenu(void (*func)(int value))
{
  int id = g_num_menus + 1;
  CGXMenu *menu = &g_menus[g_num_menus++];
  memset(menu, 0, sizeof(CGXMenu));
  menu->id = id;
  menu->callback = func;
  menu->num_items = 0;
  g_current_menu_id = id;
  return id;
}

void glutDestroyMenu(int menu_id) { (void)menu_id; }
int glutGetMenu(void) { return g_current_menu_id; }
void glutSetMenu(int menu) { g_current_menu_id = menu; }

void glutAddMenuEntry(const char *label, int value)
{
  CGXMenu *menu = get_menu(g_current_menu_id);
  if (menu && menu->num_items < MAX_MENU_ITEMS)
  {
    CGXMenuItem *item = &menu->items[menu->num_items++];
    strncpy(item->label, label, sizeof(item->label) - 1);
    item->value = value;
    item->submenu_id = -1;
    item->is_submenu = 0;
  }
}

void glutAddSubMenu(const char *label, int submenu)
{
  CGXMenu *menu = get_menu(g_current_menu_id);
  if (menu && menu->num_items < MAX_MENU_ITEMS)
  {
    CGXMenuItem *item = &menu->items[menu->num_items++];
    strncpy(item->label, label, sizeof(item->label) - 1);
    item->value = 0;
    item->submenu_id = submenu;
    item->is_submenu = 1;
  }
}

void glutChangeToMenuEntry(int item_idx, const char *label, int value)
{
  CGXMenu *menu = get_menu(g_current_menu_id);
  if (menu && item_idx >= 1 && item_idx <= menu->num_items)
  {
    CGXMenuItem *item = &menu->items[item_idx - 1];
    strncpy(item->label, label, sizeof(item->label) - 1);
    item->value = value;
    item->submenu_id = -1;
    item->is_submenu = 0;
  }
}

void glutChangeToSubMenu(int item_idx, const char *label, int submenu)
{
  CGXMenu *menu = get_menu(g_current_menu_id);
  if (menu && item_idx >= 1 && item_idx <= menu->num_items)
  {
    CGXMenuItem *item = &menu->items[item_idx - 1];
    strncpy(item->label, label, sizeof(item->label) - 1);
    item->value = 0;
    item->submenu_id = submenu;
    item->is_submenu = 1;
  }
}

void glutRemoveMenuItem(int item_idx)
{
  CGXMenu *menu = get_menu(g_current_menu_id);
  if (menu && item_idx >= 1 && item_idx <= menu->num_items)
  {
    for (int i = item_idx - 1; i < menu->num_items - 1; i++)
    {
      menu->items[i] = menu->items[i + 1];
    }
    menu->num_items--;
  }
}

void glutAttachMenu(int button)
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win && button >= 0 && button < 5)
  {
    win->attached_menu[button] = g_current_menu_id;
  }
}

void glutDetachMenu(int button)
{
  CGXWindow *win = get_window(g_current_window_id);
  if (win && button >= 0 && button < 5)
  {
    win->attached_menu[button] = 0;
  }
}

void glutSetColor(int cell, GLfloat red, GLfloat green, GLfloat blue)
{
  (void)cell; (void)red; (void)green; (void)blue;
}

GLfloat glutGetColor(int cell, int component) { (void)cell; (void)component; return 0.0f; }
void glutCopyColormap(int win) { (void)win; }

int glutGet(GLenum type)
{
  CGXWindow *win = get_window(g_current_window_id);
  switch (type)
  {
    case GLUT_WINDOW_X: return win ? win->x : 0;
    case GLUT_WINDOW_Y: return win ? win->y : 0;
    case GLUT_WINDOW_WIDTH: return win ? win->width : g_init_w;
    case GLUT_WINDOW_HEIGHT: return win ? win->height : g_init_h;
    case GLUT_WINDOW_RGBA: return 1;
    case GLUT_WINDOW_DOUBLEBUFFER: return 1;
    case GLUT_WINDOW_DEPTH_SIZE: return 24;
    case GLUT_SCREEN_WIDTH: return 1920;
    case GLUT_SCREEN_HEIGHT: return 1080;
    case GLUT_INIT_WINDOW_WIDTH: return g_init_w;
    case GLUT_INIT_WINDOW_HEIGHT: return g_init_h;
    case GLUT_ELAPSED_TIME: return (int)(glfwGetTime() * 1000.0);
    default: return 0;
  }
}

int glutDeviceGet(GLenum type) { (void)type; return 0; }

static int g_current_mods = 0;
static int g_right_down = 0;
static double g_right_down_x = 0.0;
static double g_right_down_y = 0.0;
static int g_right_dragging = 0;
static int g_left_mapped_btn = GLUT_LEFT_BUTTON;

int glutGetModifiers(void)
{
  int m = 0;
  if (g_current_mods & GLFW_MOD_SHIFT) m |= GLUT_ACTIVE_SHIFT;
  if (g_current_mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER)) m |= GLUT_ACTIVE_CTRL;
  if (g_current_mods & GLFW_MOD_ALT) m |= GLUT_ACTIVE_ALT;
  return m;
}

int glutLayerGet(GLenum type) { (void)type; return 0; }

/* DPI and Line Width Utilities */
float cgx_get_fb_scale(void)
{
  if (!g_glfw_window) return 1.0f;
  int win_w = 1, win_h = 1, fb_w = 1, fb_h = 1;
  glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
  glfwGetFramebufferSize(g_glfw_window, &fb_w, &fb_h);
  if (win_w <= 0) win_w = 1;
  float scale = (float)fb_w / (float)win_w;
  return (scale > 0.5f) ? scale : 1.0f;
}

void cgx_glLineWidth(float width)
{
  float scale = cgx_get_fb_scale();
  float final_w = width * scale;
  if (final_w < 1.0f) final_w = 1.0f;
  glLineWidth(final_w);
}

void cgx_glPointSize(float size)
{
  float scale = cgx_get_fb_scale();
  float final_sz = size * scale * 1.5f;
  if (final_sz < 2.0f) final_sz = 2.0f;
  glPointSize(final_sz);
  glEnable(GL_POINT_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void cgx_enable_smooth_lines(int enable)
{
  if (enable)
  {
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glDisable(GL_LINE_SMOOTH);
  }
}

static char g_gui_status_msg[128] = {0};
static double g_gui_status_time = 0.0;

void cgx_set_gui_status(const char *msg)
{
  if (!msg) { g_gui_status_msg[0] = '\0'; return; }
  strncpy(g_gui_status_msg, msg, sizeof(g_gui_status_msg) - 1);
  g_gui_status_msg[sizeof(g_gui_status_msg) - 1] = '\0';
  g_gui_status_time = glfwGetTime();
  g_need_redisplay = 1;
}

/* --------------------------------------------------------------------  */
/* Execute Command from Command Bar                                      */
/* --------------------------------------------------------------------  */
static void submit_command_bar(void)
{
  if (g_cmd_len > 0)
  {
    if (g_cmd_hist_count < MAX_CMD_HIST)
    {
      strncpy(g_cmd_history[g_cmd_hist_count++], g_cmd_buf, 255);
    }
    else
    {
      for (int i = 0; i < MAX_CMD_HIST - 1; i++)
      {
        strcpy(g_cmd_history[i], g_cmd_history[i + 1]);
      }
      strncpy(g_cmd_history[MAX_CMD_HIST - 1], g_cmd_buf, 255);
    }
    g_cmd_hist_pos = g_cmd_hist_count;

    strncpy(g_last_cmd_echo, g_cmd_buf, sizeof(g_last_cmd_echo) - 1);

    cgx_execute_command_string(g_cmd_buf);

    g_cmd_buf[0] = '\0';
    g_cmd_len = 0;
    g_need_redisplay = 1;
  }
}

/* Handle Clipboard Paste (Cmd+V on macOS, Ctrl+V on Linux/Windows) */
static void handle_command_bar_paste(void)
{
  if (!g_glfw_window) return;
  const char *clip = glfwGetClipboardString(g_glfw_window);
  if (!clip || !*clip) return;

  /* Check if clipboard contains any newline characters */
  int has_newlines = (strchr(clip, '\n') != NULL || strchr(clip, '\r') != NULL);

  g_cmd_bar_visible = 1;
  g_cmd_focused = 1;

  if (!has_newlines)
  {
    /* Single-line text: append directly into the command bar buffer */
    while (*clip && g_cmd_len < (int)sizeof(g_cmd_buf) - 2)
    {
      if ((unsigned char)*clip >= 32 && (unsigned char)*clip < 127)
      {
        g_cmd_buf[g_cmd_len++] = *clip;
      }
      clip++;
    }
    g_cmd_buf[g_cmd_len] = '\0';
    cgx_set_gui_status("Pasted text into command bar");
    g_need_redisplay = 1;
    return;
  }

  /* Multi-line script paste: sequentially parse and execute line by line */
  char *clip_copy = strdup(clip);
  if (!clip_copy) return;

  char *p = clip_copy;
  int executed_count = 0;
  char single_line[512];

  while (*p)
  {
    /* Find end of line */
    char *eol = p;
    while (*eol && *eol != '\n' && *eol != '\r') eol++;

    int is_last_line = (*eol == '\0');
    char delim = *eol;
    *eol = '\0';

    /* Trim leading and trailing whitespace */
    char *start = p;
    while (*start == ' ' || *start == '\t') start++;
    int len = (int)strlen(start);
    while (len > 0 && (start[len - 1] == ' ' || start[len - 1] == '\t' || start[len - 1] == '\r'))
    {
      start[--len] = '\0';
    }

    if (len > 0)
    {
      /* Combine with existing command buffer if on the very first line */
      if (executed_count == 0 && g_cmd_len > 0)
      {
        snprintf(single_line, sizeof(single_line), "%s%s", g_cmd_buf, start);
        g_cmd_buf[0] = '\0';
        g_cmd_len = 0;
      }
      else
      {
        strncpy(single_line, start, sizeof(single_line) - 1);
        single_line[sizeof(single_line) - 1] = '\0';
      }

      /* Add to command history */
      if (g_cmd_hist_count < MAX_CMD_HIST)
      {
        strncpy(g_cmd_history[g_cmd_hist_count++], single_line, 255);
      }
      else
      {
        for (int i = 0; i < MAX_CMD_HIST - 1; i++)
        {
          strcpy(g_cmd_history[i], g_cmd_history[i + 1]);
        }
        strncpy(g_cmd_history[MAX_CMD_HIST - 1], single_line, 255);
      }
      g_cmd_hist_pos = g_cmd_hist_count;

      cgx_execute_command_string(single_line);
      executed_count++;
    }

    if (is_last_line) break;
    p = eol + 1;
    if (delim == '\r' && *p == '\n') p++; /* handle CRLF */
  }

  free(clip_copy);

  g_cmd_buf[0] = '\0';
  g_cmd_len = 0;

  if (executed_count > 0)
  {
    char status[128];
    snprintf(status, sizeof(status), "Pasted & executed %d command%s", executed_count, (executed_count == 1) ? "" : "s");
    cgx_set_gui_status(status);
  }
  g_need_redisplay = 1;
}

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

typedef struct {
  GLuint tex_id;
  stbtt_bakedchar cdata[96];
  float font_size;
} CGXFontAtlas;

static CGXFontAtlas g_font_atlas[3]; /* 0: 14pt (Small), 1: 20pt (Medium / Default), 2: 32pt (Big) */
static int g_font_atlas_ready = 0;

static int get_ui_font_tier(void)
{
  extern int draw_font;
  if (draw_font <= 4) return 0; /* Small (14pt) */
  if (draw_font == 5) return 1; /* Medium / Default (20pt) */
  return 2; /* Big (32pt) */
}

static int get_menu_item_height(void)
{
  int t = get_ui_font_tier();
  if (t == 0) return 24;
  if (t == 1) return 30;
  return 44;
}

static int get_menu_box_width(void)
{
  int t = get_ui_font_tier();
  if (t == 0) return 240;
  if (t == 1) return 280;
  return 400;
}

static int get_cmd_bar_height(void)
{
  int t = get_ui_font_tier();
  if (t == 0) return 34;
  if (t == 1) return 42;
  return 58;
}

extern const unsigned char g_iosevka_ss05_ttf[];
extern const unsigned int g_iosevka_ss05_ttf_len;

static void init_truetype_fonts(void)
{
  if (g_font_atlas_ready) return;

  const unsigned char *ttf_buffer = NULL;
  unsigned char *allocated_ttf = NULL;

  if (g_iosevka_ss05_ttf_len > 0)
  {
    ttf_buffer = g_iosevka_ss05_ttf;
    printf(" [Font] Typography Engine: stb_truetype loaded embedded 'Iosevka SS05 Medium' (%u KB)\n", g_iosevka_ss05_ttf_len / 1024);
  }
  else
  {
    const char *home = getenv("HOME");
    char user_font_mac[512] = "";
    char user_font_linux[512] = "";
    if (home)
    {
      snprintf(user_font_mac, sizeof(user_font_mac), "%s/Library/Fonts/IosevkaSS05-Medium.ttf", home);
      snprintf(user_font_linux, sizeof(user_font_linux), "%s/.local/share/fonts/IosevkaSS05-Medium.ttf", home);
    }

    const char *paths[] = {
      "font/IosevkaSS05-Medium.ttf",
      "../font/IosevkaSS05-Medium.ttf",
      "../../font/IosevkaSS05-Medium.ttf",
      "cgx/CalculiX-CGX-New3D/font/IosevkaSS05-Medium.ttf",
      "src/font/IosevkaSS05-Medium.ttf",
      user_font_mac,
      user_font_linux,
      "/Library/Fonts/IosevkaSS05-Medium.ttf",
      "/System/Library/Fonts/Helvetica.ttc",
      "/System/Library/Fonts/Supplemental/Arial.ttf",
      "/Library/Fonts/Arial.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
      "C:\\Windows\\Fonts\\arial.ttf",
      NULL
    };

    const char *loaded_font_path = NULL;
    FILE *f = NULL;
    for (int i = 0; paths[i] != NULL; i++)
    {
      if (paths[i][0] == '\0') continue;
      f = fopen(paths[i], "rb");
      if (f) {
        loaded_font_path = paths[i];
        break;
      }
    }
    if (!f) return;
    printf(" [Font] Typography Engine: stb_truetype loaded '%s'\n", loaded_font_path);

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) { fclose(f); return; }

    allocated_ttf = (unsigned char *)malloc(size);
    if (!allocated_ttf) { fclose(f); return; }

    if (fread(allocated_ttf, 1, size, f) != (size_t)size)
    {
      free(allocated_ttf);
      fclose(f);
      return;
    }
    fclose(f);
    ttf_buffer = allocated_ttf;
  }

  int offset = stbtt_GetFontOffsetForIndex(ttf_buffer, 0);
  if (offset < 0) offset = 0;

  int win_w = 800, win_h = 600, fb_w = 800, fb_h = 600;
  if (g_glfw_window)
  {
    glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
    glfwGetFramebufferSize(g_glfw_window, &fb_w, &fb_h);
  }
  float fb_scale = (win_w > 0) ? (float)fb_w / (float)win_w : 1.0f;
  if (fb_scale < 1.0f) fb_scale = 1.0f;

  float sizes[3] = { 14.0f * fb_scale, 20.0f * fb_scale, 32.0f * fb_scale };
  int tex_dim = 1024;

  for (int tier = 0; tier < 3; tier++)
  {
    unsigned char *temp_bitmap = (unsigned char *)calloc(tex_dim * tex_dim, 1);
    if (!temp_bitmap) continue;

    int res = stbtt_BakeFontBitmap(ttf_buffer, offset, sizes[tier], temp_bitmap, tex_dim, tex_dim, 32, 96, g_font_atlas[tier].cdata);
    if (res > 0)
    {
      glGenTextures(1, &g_font_atlas[tier].tex_id);
      glBindTexture(GL_TEXTURE_2D, g_font_atlas[tier].tex_id);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, tex_dim, tex_dim, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      g_font_atlas[tier].font_size = sizes[tier];
    }
    free(temp_bitmap);
  }

  if (allocated_ttf) free(allocated_ttf);
  g_font_atlas_ready = 1;
}

/* --------------------------------------------------------------------  */
/* Modern Anti-Aliased Vector Typography Helpers                         */
/* --------------------------------------------------------------------  */

void glutBitmapCharacter(void *font, int character)
{
  (void)font;
  if (character < 32 || character >= 128) return;

  GLboolean valid = GL_FALSE;
  glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);
  if (!valid) return;

  GLfloat rpos[4];
  glGetFloatv(GL_CURRENT_RASTER_POSITION, rpos);

  int tier = get_ui_font_tier();
  if (!g_font_atlas_ready || g_font_atlas[tier].tex_id == 0) return;

  int tex_dim = 1024;

  int win_w = 800, win_h = 600, fb_w = 800, fb_h = 600;
  if (g_glfw_window)
  {
    glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
    glfwGetFramebufferSize(g_glfw_window, &fb_w, &fb_h);
  }
  if (fb_w <= 0 || fb_h <= 0) return;

  glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT | GL_POLYGON_BIT);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g_font_atlas[tier].tex_id);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glViewport(0, 0, fb_w, fb_h);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, fb_w, 0, fb_h);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  float cur_x = rpos[0];
  float cur_y = (float)fb_h - rpos[1];

  stbtt_aligned_quad q;
  stbtt_GetBakedQuad(g_font_atlas[tier].cdata, tex_dim, tex_dim, character - 32, &cur_x, &cur_y, &q, 1);

  glBegin(GL_QUADS);
    glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, (float)fb_h - q.y0);
    glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, (float)fb_h - q.y0);
    glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, (float)fb_h - q.y1);
    glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, (float)fb_h - q.y1);
  glEnd();

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glPopAttrib();

  /* Advance raster position in framebuffer pixels for next character */
  float adv = g_font_atlas[tier].cdata[character - 32].xadvance;
  glBitmap(0, 0, 0.0f, 0.0f, adv, 0.0f, NULL);
}

int glutBitmapWidth(void *font, int character)
{
  (void)font;
  if (character < 32 || character >= 128) return 8;
  int tier = get_ui_font_tier();
  if (!g_font_atlas_ready) return 10;
  return (int)ceil(g_font_atlas[tier].cdata[character - 32].xadvance);
}

int glutBitmapLength(void *font, const unsigned char *string)
{
  (void)font;
  if (!string || !*string) return 0;
  int tier = get_ui_font_tier();
  if (!g_font_atlas_ready) return (int)strlen((const char*)string) * 10;
  float width = 0.0f;
  while (*string)
  {
    int ch = (unsigned char)*string;
    if (ch >= 32 && ch < 128)
      width += g_font_atlas[tier].cdata[ch - 32].xadvance;
    else
      width += g_font_atlas[tier].font_size * 0.5f;
    string++;
  }
  return (int)ceil(width);
}

static void draw_ui_text_dynamic(float x, float y, const char *str, float r, float g, float b, int win_h)
{
  if (!str || !*str) return;
  int tier = get_ui_font_tier();
  if (!g_font_atlas_ready || g_font_atlas[tier].tex_id == 0) return;

  int win_w = 800, fb_w = 800, fb_h = 600;
  if (g_glfw_window)
  {
    glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
    glfwGetFramebufferSize(g_glfw_window, &fb_w, &fb_h);
  }
  float scale_x = (win_w > 0) ? (float)fb_w / (float)win_w : 1.0f;
  float scale_y = (win_h > 0) ? (float)fb_h / (float)win_h : 1.0f;

  int tex_dim = 1024;

  glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_POLYGON_BIT);
  glViewport(0, 0, fb_w, fb_h);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, g_font_atlas[tier].tex_id);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(r, g, b, 1.0f);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  gluOrtho2D(0, fb_w, 0, fb_h);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  float cur_x = x * scale_x;
  float cur_y = y * scale_y;

  glBegin(GL_QUADS);
  while (*str)
  {
    int ch = (unsigned char)*str;
    if (ch >= 32 && ch < 128)
    {
      stbtt_aligned_quad q;
      stbtt_GetBakedQuad(g_font_atlas[tier].cdata, tex_dim, tex_dim, ch - 32, &cur_x, &cur_y, &q, 1);

      glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, (float)fb_h - q.y0);
      glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, (float)fb_h - q.y0);
      glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, (float)fb_h - q.y1);
      glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, (float)fb_h - q.y1);
    }
    else
    {
      cur_x += g_font_atlas[tier].font_size * 0.5f;
    }
    str++;
  }
  glEnd();

  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  glPopAttrib();
}

static float get_text_width_dynamic(const char *str)
{
  if (!str || !*str) return 0.0f;
  int tier = get_ui_font_tier();
  if (!g_font_atlas_ready || g_font_atlas[tier].tex_id == 0) return 0.0f;

  int win_w = 800, fb_w = 800;
  if (g_glfw_window)
  {
    int win_h;
    glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
    glfwGetFramebufferSize(g_glfw_window, &fb_w, &win_h);
  }
  float scale_x = (win_w > 0) ? (float)fb_w / (float)win_w : 1.0f;

  float width = 0.0f;
  while (*str)
  {
    int ch = (unsigned char)*str;
    if (ch >= 32 && ch < 128)
      width += g_font_atlas[tier].cdata[ch - 32].xadvance;
    else
      width += g_font_atlas[tier].font_size * 0.5f;
    str++;
  }
  return width / scale_x;
}

void cgx_glfw_toggle_tree(void)
{
  cgx_tree_toggle();
}

int cgx_glfw_is_tree_visible(void)
{
  return cgx_tree_is_visible();
}

void cgx_glfw_set_tree_visible(int visible)
{
  cgx_tree_set_visible(visible);
}

void cgx_glfw_queue_command(const char *cmd)
{
  if (cmd && cmd[0] != '\0') queue_command(cmd);
}

void cgx_ui_draw_text(float x, float y, const char *str, float r, float g, float b, int win_h)
{
  draw_ui_text_dynamic(x, y, str, r, g, b, win_h);
}

float cgx_ui_text_width(const char *str)
{
  return get_text_width_dynamic(str);
}

int cgx_ui_font_tier(void)
{
  return get_ui_font_tier();
}

int cgx_ui_cmd_bar_height(void)
{
  return get_cmd_bar_height();
}

int cgx_ui_top_tab_height(void)
{
  int tier = get_ui_font_tier();
  int btn_h = (tier == 0) ? 24 : (tier == 1 ? 30 : 42);
  int outer = (tier == 0) ? 4  : (tier == 1 ? 5  : 7);
  return btn_h + outer * 2;
}

void cgx_ui_get_viewport_rect(int win_w, int win_h, int *out_x, int *out_y, int *out_w, int *out_h)
{
  CGXWindow *vp = (w1 > 0) ? get_window(w1) : get_window(2);
  if (vp && vp->width > 10 && vp->height > 10)
  {
    int x, y, w, h;
    get_window_screen_rect(vp, &x, &y, &w, &h);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > win_w) w = win_w - x;
    if (y + h > win_h) h = win_h - y;
    if (w > 10 && h > 10)
    {
      *out_x = x;
      *out_y = y;
      *out_w = w;
      *out_h = h;
      return;
    }
  }
  *out_x = 0;
  *out_y = 0;
  *out_w = win_w;
  *out_h = win_h;
}

/* --------------------------------------------------------------------  */
/* Multi-Level Recursive Popup Menu Engine                               */
/* --------------------------------------------------------------------  */

static void open_cascade_root(int menu_id, int x, int y, int win_w, int win_h)
{
  CGXMenu *menu = get_menu(menu_id);
  if (!menu) return;

  int item_h = get_menu_item_height();
  int menu_w = get_menu_box_width();
  int bar_h = g_cmd_bar_visible ? get_cmd_bar_height() : 0;
  int total_h = menu->num_items * item_h;
  if (x + menu_w > win_w) x = win_w - menu_w - 6;
  if (y + total_h > win_h - bar_h) y = win_h - bar_h - total_h - 6;
  if (x < 6) x = 6;
  if (y < 6) y = 6;

  g_cascade[0].menu_id = menu_id;
  g_cascade[0].x = x;
  g_cascade[0].y = y;
  g_cascade[0].width = menu_w;
  g_cascade[0].height = total_h;
  g_cascade[0].hovered_item = -1;
  g_cascade_depth = 1;
  g_need_redisplay = 1;
}

static void update_cascade_hover(int mouse_x, int mouse_y, int win_w, int win_h)
{
  if (g_cascade_depth <= 0) return;

  int item_h = get_menu_item_height();
  int menu_w = get_menu_box_width();
  int bar_h = g_cmd_bar_visible ? get_cmd_bar_height() : 0;

  int match_lvl = -1;
  int match_item = -1;

  for (int lvl = g_cascade_depth - 1; lvl >= 0; lvl--)
  {
    CGXMenuCascade *c = &g_cascade[lvl];
    if (mouse_x >= c->x && mouse_x <= c->x + c->width &&
        mouse_y >= c->y && mouse_y <= c->y + c->height)
    {
      match_lvl = lvl;
      match_item = (mouse_y - c->y) / item_h;
      break;
    }
  }

  if (match_lvl >= 0)
  {
    CGXMenuCascade *c = &g_cascade[match_lvl];
    CGXMenu *menu = get_menu(c->menu_id);
    if (menu && match_item >= 0 && match_item < menu->num_items)
    {
      c->hovered_item = match_item;
      g_cascade_depth = match_lvl + 1;

      if (menu->items[match_item].is_submenu && match_lvl + 1 < MAX_MENU_DEPTH)
      {
        int sub_id = menu->items[match_item].submenu_id;
        CGXMenu *sub_menu = get_menu(sub_id);
        if (sub_menu && sub_menu->num_items > 0)
        {
          int next_lvl = match_lvl + 1;
          int sub_w = menu_w;
          int sub_h = sub_menu->num_items * item_h;

          int sub_x = c->x + c->width;
          if (sub_x + sub_w > win_w - 6)
          {
            sub_x = c->x - sub_w;
            if (sub_x < 6) sub_x = 6;
          }

          int sub_y = c->y + match_item * item_h;
          if (sub_y + sub_h > win_h - bar_h)
          {
            sub_y = win_h - bar_h - sub_h - 6;
          }
          if (sub_y < 6) sub_y = 6;

          g_cascade[next_lvl].menu_id = sub_id;
          g_cascade[next_lvl].x = sub_x;
          g_cascade[next_lvl].y = sub_y;
          g_cascade[next_lvl].width = sub_w;
          g_cascade[next_lvl].height = sub_h;
          g_cascade[next_lvl].hovered_item = -1;
          g_cascade_depth = next_lvl + 1;
        }
      }
      g_need_redisplay = 1;
    }
  }
}

static int handle_cascade_click(int mouse_x, int mouse_y)
{
  if (g_cascade_depth <= 0) return 0;
  int item_h = get_menu_item_height();

  for (int lvl = g_cascade_depth - 1; lvl >= 0; lvl--)
  {
    CGXMenuCascade *c = &g_cascade[lvl];
    if (mouse_x >= c->x && mouse_x <= c->x + c->width &&
        mouse_y >= c->y && mouse_y <= c->y + c->height)
    {
      int item_idx = (mouse_y - c->y) / item_h;
      CGXMenu *menu = get_menu(c->menu_id);
      if (menu && item_idx >= 0 && item_idx < menu->num_items)
      {
        if (!menu->items[item_idx].is_submenu && menu->callback)
        {
          int val = menu->items[item_idx].value;
          void (*cb)(int) = menu->callback;

          g_cascade_depth = 0;

          glutSetWindow(2);
          activWindow = 2;

          cb(val);

          g_need_redisplay = 1;
          return 1;
        }
      }
    }
  }

  g_cascade_depth = 0;
  g_need_redisplay = 1;
  return 1;
}

static void draw_cascade_menu(CGXMenuCascade *c, int win_h)
{
  CGXMenu *menu = get_menu(c->menu_id);
  if (!menu) return;

  int item_h = get_menu_item_height();
  int start_x = c->x;
  int start_y = c->y;
  int menu_w = c->width;
  int total_h = c->height;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  /* Drop Shadow */
  glColor4f(0.0f, 0.0f, 0.0f, 0.50f);
  glRectf(start_x + 6, win_h - (start_y + total_h + 6), start_x + menu_w + 6, win_h - (start_y + 6));

  /* Dark Teal / Slate Glass Background (#0E151E) */
  glColor4f(0.05f, 0.07f, 0.10f, 0.98f);
  glRectf(start_x, win_h - (start_y + total_h), start_x + menu_w, win_h - start_y);

  /* Sleek Border (#223A52) */
  glColor4f(0.18f, 0.28f, 0.38f, 1.0f);
  glLineWidth(1.2f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(start_x, win_h - start_y);
    glVertex2f(start_x + menu_w, win_h - start_y);
    glVertex2f(start_x + menu_w, win_h - (start_y + total_h));
    glVertex2f(start_x, win_h - (start_y + total_h));
  glEnd();

  int text_baseline_offset = (int)(item_h * 0.70f);

  for (int i = 0; i < menu->num_items; i++)
  {
    int item_y = start_y + i * item_h;
    
    if (i == c->hovered_item)
    {
      /* Electric Blue / Teal Hover Gradient */
      glColor4f(0.12f, 0.38f, 0.70f, 1.0f);
      glRectf(start_x + 2, win_h - (item_y + item_h), start_x + menu_w - 2, win_h - item_y);

      /* Left Accent Pill */
      glColor4f(0.25f, 0.75f, 1.0f, 1.0f);
      glRectf(start_x + 2, win_h - (item_y + item_h), start_x + 5, win_h - item_y);

      draw_ui_text_dynamic(start_x + 12, item_y + text_baseline_offset, menu->items[i].label, 1.0f, 1.0f, 1.0f, win_h);
      if (menu->items[i].is_submenu)
      {
        draw_ui_text_dynamic(start_x + menu_w - 22, item_y + text_baseline_offset, ">", 0.40f, 0.85f, 1.0f, win_h);
      }
    }
    else
    {
      draw_ui_text_dynamic(start_x + 12, item_y + text_baseline_offset, menu->items[i].label, 0.90f, 0.94f, 0.98f, win_h);
      if (menu->items[i].is_submenu)
      {
        draw_ui_text_dynamic(start_x + menu_w - 22, item_y + text_baseline_offset, ">", 0.45f, 0.58f, 0.72f, win_h);
      }
    }
  }
}

/* --------------------------------------------------------------------  */
/* Modern In-Window Command Bar (Bottom Strip)                           */
/* --------------------------------------------------------------------  */
static void render_command_bar(int win_w, int win_h)
{
  int bar_h = get_cmd_bar_height();
  int bar_y = win_h - bar_h;
  int baseline_y = bar_y + (int)(bar_h * 0.68f);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  /* Bar Background - Deep Charcoal Dark Slate (#05070A) */
  glColor4f(0.05f, 0.07f, 0.10f, 0.98f);
  glRectf(0, 0, win_w, bar_h);

  /* Top Border (#1E2D3E) */
  glColor4f(0.18f, 0.28f, 0.38f, 1.0f);
  glLineWidth(1.4f);
  glBegin(GL_LINES);
    glVertex2f(0, bar_h);
    glVertex2f(win_w, bar_h);
  glEnd();

  /* Prompt Symbol (Bright Cyan >) */
  draw_ui_text_dynamic(12, baseline_y, ">", 0.25f, 0.80f, 1.0f, win_h);

  /* Input Text, Status Feedback, or Placeholder */
  if (g_cmd_len > 0)
  {
    draw_ui_text_dynamic(30, baseline_y, g_cmd_buf, 1.0f, 1.0f, 1.0f, win_h);
  }
  else if ((glfwGetTime() - g_gui_status_time) < 6.0 && g_gui_status_msg[0] != '\0')
  {
    draw_ui_text_dynamic(30, baseline_y, g_gui_status_msg, 1.0f, 0.78f, 0.35f, win_h);
  }
  else
  {
    draw_ui_text_dynamic(30, baseline_y, "Type command (e.g. ds 1 e 4, plot f all, view sh, frame, help)...", 0.38f, 0.46f, 0.56f, win_h);
  }

  /* Send Button */
  int btn_w = (int)(get_text_width_dynamic("SEND") + 28);
  if (btn_w < 80) btn_w = 80;
  int btn_x1 = win_w - btn_w - 10;
  int btn_x2 = win_w - 10;
  int btn_y1 = bar_y + 4;
  int btn_y2 = bar_y + bar_h - 4;

  if (g_send_hovered)
  {
    glColor4f(0.18f, 0.52f, 0.95f, 1.0f);
  }
  else
  {
    glColor4f(0.12f, 0.35f, 0.68f, 1.0f);
  }
  glRectf(btn_x1, win_h - btn_y2, btn_x2, win_h - btn_y1);

  /* Button Border */
  if (g_send_hovered)
    glColor4f(0.40f, 0.75f, 1.0f, 1.0f);
  else
    glColor4f(0.30f, 0.65f, 1.0f, 1.0f);
  glLineWidth(1.4f);
  glBegin(GL_LINE_LOOP);
    glVertex2f(btn_x1, win_h - btn_y1);
    glVertex2f(btn_x2, win_h - btn_y1);
    glVertex2f(btn_x2, win_h - btn_y2);
    glVertex2f(btn_x1, win_h - btn_y2);
  glEnd();

  float send_tw = get_text_width_dynamic("SEND");
  draw_ui_text_dynamic(btn_x1 + (btn_w - send_tw) / 2.0f, baseline_y, "SEND", 1.0f, 1.0f, 1.0f, win_h);
}

/* --------------------------------------------------------------------  */
/* Lightweight Collapsible HUD Toolbar (Top-Center, Hidden by Default)   */
/* --------------------------------------------------------------------  */

#define HUD_BTN_COUNT 11

static void draw_chevron_lr(float cx, float cy_gl, float half_w, float half_h, int point_right)
{
  float dx = point_right ? half_w : -half_w;
  glBegin(GL_LINE_STRIP);
    glVertex2f(cx - dx, cy_gl - half_h);
    glVertex2f(cx + dx, cy_gl);
    glVertex2f(cx - dx, cy_gl + half_h);
  glEnd();
}

static void get_hud_pull_tab_rect(int win_w, int win_h, int *x1, int *y1, int *x2, int *y2)
{
  int vx, vy, vw, vh;
  cgx_ui_get_viewport_rect(win_w, win_h, &vx, &vy, &vw, &vh);
  int tier = get_ui_font_tier();
  int btn_h = (tier == 0) ? 24 : (tier == 1 ? 30 : 42);
  int outer = (tier == 0) ? 4  : (tier == 1 ? 5  : 7);
  int pad_x = (tier == 0) ? 9  : (tier == 1 ? 11 : 15);
  int gap_chev = (tier == 0) ? 6 : (tier == 1 ? 7 : 10);
  int chev_w = (tier == 0) ? 6 : (tier == 1 ? 8 : 11);
  int h = btn_h + outer * 2;
  float tw = get_text_width_dynamic("Tools");
  int w = (int)ceil(tw) + gap_chev + chev_w + pad_x * 2;
  *x2 = vx + vw;
  *x1 = *x2 - w;
  *y1 = vy;
  *y2 = vy + h;
}

static int hit_test_hud_pull_tab(int mx, int my, int win_w, int win_h)
{
  if (win_w < 200 || win_h < 120) return 0;
  int x1, y1, x2, y2;
  get_hud_pull_tab_rect(win_w, win_h, &x1, &y1, &x2, &y2);
  return (mx >= x1 && mx <= x2 && my >= y1 && my <= y2) ? 1 : 0;
}

static void render_hud_pull_tab(int win_w, int win_h)
{
  if (win_w < 200 || win_h < 120) return;

  int x1, y1, x2, y2;
  get_hud_pull_tab_rect(win_w, win_h, &x1, &y1, &x2, &y2);
  int tier = get_ui_font_tier();
  int pad_x = (tier == 0) ? 9 : (tier == 1 ? 11 : 15);
  int gap_chev = (tier == 0) ? 6 : (tier == 1 ? 7 : 10);
  int chev_w = (tier == 0) ? 6 : (tier == 1 ? 8 : 11);
  int tab_h = y2 - y1;
  int dark = (backgrndcol == 0);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  if (g_hud_tab_hovered)
  {
    glColor4f(0.18f, 0.48f, 0.82f, 0.96f);
  }
  else if (g_hud_toolbar_visible)
  {
    if (dark) glColor4f(0.05f, 0.08f, 0.12f, 0.88f);
    else      glColor4f(0.94f, 0.96f, 0.98f, 0.92f);
  }
  else if (dark)
  {
    glColor4f(0.05f, 0.08f, 0.12f, 0.48f);
  }
  else
  {
    glColor4f(0.90f, 0.93f, 0.96f, 0.60f);
  }
  glRectf((float)x1, (float)(win_h - y2), (float)x2, (float)(win_h - y1));

  if (g_hud_tab_hovered)
  {
    glColor4f(0.35f, 0.78f, 1.0f, 0.95f);
  }
  else if (g_hud_toolbar_visible)
  {
    if (dark) glColor4f(0.20f, 0.30f, 0.42f, 0.95f);
    else      glColor4f(0.68f, 0.74f, 0.82f, 0.95f);
  }
  else if (dark)
  {
    glColor4f(0.24f, 0.34f, 0.46f, 0.55f);
  }
  else
  {
    glColor4f(0.62f, 0.68f, 0.76f, 0.68f);
  }

  glLineWidth(1.0f);
  glBegin(GL_LINE_STRIP);
    glVertex2f((float)x1, (float)(win_h - y1));
    glVertex2f((float)x1, (float)(win_h - y2));
    glVertex2f((float)x2, (float)(win_h - y2));
    glVertex2f((float)x2, (float)(win_h - y1));
  glEnd();

  float tr, tg, tb, ta = 1.0f;
  if (g_hud_tab_hovered)
  {
    tr = 1.0f; tg = 1.0f; tb = 1.0f;
  }
  else if (g_hud_toolbar_visible)
  {
    if (dark) { tr = 0.92f; tg = 0.95f; tb = 0.98f; }
    else      { tr = 0.10f; tg = 0.14f; tb = 0.20f; }
  }
  else if (dark)
  {
    tr = 0.86f; tg = 0.91f; tb = 0.97f; ta = 0.82f;
  }
  else
  {
    tr = 0.15f; tg = 0.20f; tb = 0.28f; ta = 0.85f;
  }

  float chev_cx = (float)x1 + (float)pad_x + (float)chev_w * 0.5f;
  float chev_cy_gl = (float)win_h - ((float)y1 + (float)tab_h * 0.50f);
  glColor4f(tr, tg, tb, ta);
  glLineWidth(1.5f);
  draw_chevron_lr(chev_cx, chev_cy_gl, (float)chev_w * 0.40f, (float)tab_h * 0.16f, g_hud_toolbar_visible);

  int baseline_y = y1 + (int)(tab_h * 0.68f);
  float text_x = (float)x1 + (float)pad_x + (float)chev_w + (float)gap_chev;
  draw_ui_text_dynamic(text_x, (float)baseline_y, "Tools", tr, tg, tb, win_h);
}

static const char *get_hud_button_label(int idx, int for_width)
{
  switch (idx)
  {
    case 0: return "Fit";
    case 1: return "Iso";
    case 2: return for_width ? "Wire" : (g_is_wireframe ? "Fill" : "Wire");
    case 3: return "Elem";
    case 4: return "Persp";
    case 5: return "Theme";
    case 6: return "Disp";
    case 7: return "1/2";
    case 8: return "2x";
    case 9: return "Spin";
    case 10: return "Axes";
    default: return "";
  }
}

static int is_hud_button_disabled(int idx)
{
  if (idx == 3 && g_is_wireframe) return 1;
  return 0;
}

static int is_hud_button_active(int idx)
{
  switch (idx)
  {
    case 2: return g_is_wireframe ? 1 : 0;
    case 3: return (!g_is_wireframe && elemEdgeFlag) ? 1 : 0;
    case 4: return perspectiveFlag ? 1 : 0;
    case 6: return addDispFlag ? 1 : 0;
    case 9: return g_auto_rotate_active ? 1 : 0;
    case 10: return g_gizmo_visible ? 1 : 0;
    default: return 0;
  }
}

static void compute_hud_toolbar_layout(int win_w, int win_h,
                                       int out_x1[HUD_BTN_COUNT],
                                       int out_x2[HUD_BTN_COUNT],
                                       int *out_y1, int *out_y2,
                                       int *out_bar_x1, int *out_bar_x2,
                                       int *out_bar_y1, int *out_bar_y2)
{
  int vx, vy, vw, vh;
  cgx_ui_get_viewport_rect(win_w, win_h, &vx, &vy, &vw, &vh);

  int tier = get_ui_font_tier();
  int btn_h = (tier == 0) ? 24 : (tier == 1 ? 30 : 42);
  int pad_x = (tier == 0) ? 8  : (tier == 1 ? 11 : 15);
  int gap   = (tier == 0) ? 4  : (tier == 1 ? 5  : 7);
  int outer = (tier == 0) ? 4  : (tier == 1 ? 5  : 7);

  int widths[HUD_BTN_COUNT];
  int total_btn_w = 0;
  for (int i = 0; i < HUD_BTN_COUNT; i++)
  {
    float tw = get_text_width_dynamic(get_hud_button_label(i, 1));
    widths[i] = (int)ceil(tw) + pad_x * 2;
    total_btn_w += widths[i];
  }
  total_btn_w += gap * (HUD_BTN_COUNT - 1);

  int tx1, ty1, tx2, ty2;
  get_hud_pull_tab_rect(win_w, win_h, &tx1, &ty1, &tx2, &ty2);

  int bar_w = total_btn_w + outer * 2;
  int bar_x2 = tx1;
  int bar_x1 = bar_x2 - bar_w;
  if (bar_x1 < vx)
  {
    bar_x1 = vx;
    bar_x2 = bar_x1 + bar_w;
  }
  int bar_y1 = ty1;

  int cur_x = bar_x1 + outer;
  int y1 = bar_y1 + outer;
  int y2 = y1 + btn_h;
  for (int i = 0; i < HUD_BTN_COUNT; i++)
  {
    out_x1[i] = cur_x;
    out_x2[i] = cur_x + widths[i];
    cur_x = out_x2[i] + gap;
  }

  *out_y1 = y1;
  *out_y2 = y2;
  *out_bar_x1 = bar_x1;
  *out_bar_x2 = bar_x2;
  *out_bar_y1 = bar_y1;
  *out_bar_y2 = ty2;
}

static int hit_test_hud_toolbar(int mx, int my, int win_w, int win_h)
{
  if (!g_hud_toolbar_visible || win_w < 200 || win_h < 120) return -1;

  int bx1[HUD_BTN_COUNT], bx2[HUD_BTN_COUNT];
  int y1, y2, bar_x1, bar_x2, bar_y1, bar_y2;
  compute_hud_toolbar_layout(win_w, win_h, bx1, bx2, &y1, &y2, &bar_x1, &bar_x2, &bar_y1, &bar_y2);

  if (mx < bar_x1 || mx > bar_x2 || my < bar_y1 || my > bar_y2) return -1;
  if (my >= y1 && my <= y2)
  {
    for (int i = 0; i < HUD_BTN_COUNT; i++)
    {
      if (mx >= bx1[i] && mx <= bx2[i]) return i;
    }
  }
  return -2; /* inside toolbar frame padding */
}

static void dispatch_hud_button(int idx)
{
  if (is_hud_button_disabled(idx)) return;
  switch (idx)
  {
    case 0: frame(); break;
    case 1: queue_command("rot iso"); break;
    case 2:
      if (!g_is_wireframe)
      {
        if (elemEdgeFlag) elemEdgeFlag = 0;
        selectView(4);
      }
      else
      {
        selectView(3);
      }
      break;
    case 3: selectView(8); break;
    case 4: queue_command(perspectiveFlag ? "view ortho" : "view persp"); break;
    case 5: queue_command(backgrndcol ? "view bg k" : "view bg w"); break;
    case 6: selectView(13); break;
    case 7:
    case 8:
    {
      char status[64];
      tuneAnimation((idx == 7) ? 4 : 3);
      snprintf(status, sizeof(status), "Disp scale: %.3gx", anim_faktor);
      cgx_set_gui_status(status);
      break;
    }
    case 9: cgx_glfw_toggle_auto_rotate(); break;
    case 10: cgx_glfw_toggle_gizmo(); break;
    default: break;
  }
}

static void render_hud_toolbar(int win_w, int win_h)
{
  if (!g_hud_toolbar_visible || win_w < 200 || win_h < 120) return;

  int bx1[HUD_BTN_COUNT], bx2[HUD_BTN_COUNT];
  int y1, y2, bar_x1, bar_x2, bar_y1, bar_y2;
  compute_hud_toolbar_layout(win_w, win_h, bx1, bx2, &y1, &y2, &bar_x1, &bar_x2, &bar_y1, &bar_y2);

  int dark = (backgrndcol == 0);
  int btn_h = y2 - y1;
  int baseline_y = y1 + (int)(btn_h * 0.70f);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  /* Outer toolbar strip */
  if (dark) glColor4f(0.05f, 0.08f, 0.12f, 0.88f);
  else      glColor4f(0.94f, 0.96f, 0.98f, 0.92f);
  glRectf((float)bar_x1, (float)(win_h - bar_y2), (float)bar_x2, (float)(win_h - bar_y1));

  if (dark) glColor4f(0.20f, 0.30f, 0.42f, 0.95f);
  else      glColor4f(0.68f, 0.74f, 0.82f, 0.95f);
  glLineWidth(1.1f);
  glBegin(GL_LINE_LOOP);
    glVertex2f((float)bar_x1, (float)(win_h - bar_y1));
    glVertex2f((float)bar_x2, (float)(win_h - bar_y1));
    glVertex2f((float)bar_x2, (float)(win_h - bar_y2));
    glVertex2f((float)bar_x1, (float)(win_h - bar_y2));
  glEnd();

  for (int i = 0; i < HUD_BTN_COUNT; i++)
  {
    int disabled = is_hud_button_disabled(i);
    int active = is_hud_button_active(i);
    int hovered = (!disabled && g_hud_hovered_btn == i);
    const char *label = get_hud_button_label(i, 0);

    if (disabled)
    {
      if (dark) glColor4f(0.08f, 0.11f, 0.16f, 0.60f);
      else      glColor4f(0.88f, 0.90f, 0.93f, 0.65f);
    }
    else if (hovered)
    {
      glColor4f(0.18f, 0.48f, 0.82f, 0.96f);
    }
    else if (active)
    {
      if (dark) glColor4f(0.12f, 0.34f, 0.56f, 0.92f);
      else      glColor4f(0.16f, 0.46f, 0.78f, 0.92f);
    }
    else
    {
      if (dark) glColor4f(0.10f, 0.14f, 0.20f, 0.85f);
      else      glColor4f(0.86f, 0.89f, 0.94f, 0.90f);
    }
    glRectf((float)bx1[i], (float)(win_h - y2), (float)bx2[i], (float)(win_h - y1));

    if (active)
    {
      glColor4f(0.25f, 0.80f, 1.0f, 1.0f);
      glRectf((float)bx1[i], (float)(win_h - y2), (float)bx2[i], (float)(win_h - (y2 - 2)));
    }

    float tr = 1.0f, tg = 1.0f, tb = 1.0f;
    if (disabled)
    {
      if (dark) { tr = 0.42f; tg = 0.48f; tb = 0.56f; }
      else      { tr = 0.55f; tg = 0.60f; tb = 0.66f; }
    }
    else if (!hovered && !active)
    {
      if (dark) { tr = 0.90f; tg = 0.94f; tb = 0.98f; }
      else      { tr = 0.10f; tg = 0.14f; tb = 0.20f; }
    }
    float tw = get_text_width_dynamic(label);
    float tx = (float)bx1[i] + ((float)(bx2[i] - bx1[i]) - tw) * 0.5f;
    draw_ui_text_dynamic(tx, (float)baseline_y, label, tr, tg, tb, win_h);
  }
}

/* --------------------------------------------------------------------  */
/* Clickable 3D Orientation Gizmo (Top-Right, Hidden by Default)         */
/* --------------------------------------------------------------------  */

typedef struct {
  int   id;
  float ax, ay, az;
  float sx, sy, sz;
  float px_top, py_top;
  float px_gl, py_gl;
  float r, g, b;
  const char *label;
  const char *cmd;
  int   is_pos;
} CGXGizmoHandle;

static void get_gizmo_metrics(int win_w, int win_h, float *cx, float *cy_top, float *radius, float *arm, float *dot_r)
{
  int vx, vy, vw, vh;
  cgx_ui_get_viewport_rect(win_w, win_h, &vx, &vy, &vw, &vh);
  int tier = get_ui_font_tier();
  int tx1, ty1, tx2, ty2;
  get_hud_pull_tab_rect(win_w, win_h, &tx1, &ty1, &tx2, &ty2);
  *radius = (tier == 0) ? 40.0f : (tier == 1 ? 48.0f : 60.0f);
  *arm    = (*radius) * 0.68f;
  *dot_r  = (tier == 0) ? 8.5f : (tier == 1 ? 10.5f : 13.5f);
  *cx     = (float)(vx + vw) - (*radius) - 12.0f;
  *cy_top = (float)ty2 + (*radius) + 10.0f;
}

static void build_gizmo_handles(int win_w, int win_h, CGXGizmoHandle handles[6])
{
  static const struct {
    int id; float ax, ay, az; float r, g, b;
    const char *label; const char *cmd; int is_pos;
  } defs[6] = {
    { 0,  1.0f,  0.0f,  0.0f, 0.92f, 0.26f, 0.26f, "X",  "rot x",  1 },
    { 1, -1.0f,  0.0f,  0.0f, 0.55f, 0.22f, 0.22f, "-X", "rot -x", 0 },
    { 2,  0.0f,  1.0f,  0.0f, 0.22f, 0.82f, 0.36f, "Y",  "rot y",  1 },
    { 3,  0.0f, -1.0f,  0.0f, 0.18f, 0.48f, 0.24f, "-Y", "rot -y", 0 },
    { 4,  0.0f,  0.0f,  1.0f, 0.26f, 0.66f, 0.98f, "Z",  "rot z",  1 },
    { 5,  0.0f,  0.0f, -1.0f, 0.20f, 0.40f, 0.62f, "-Z", "rot -z", 0 }
  };

  float cx, cy_top, radius, arm, dot_r;
  get_gizmo_metrics(win_w, win_h, &cx, &cy_top, &radius, &arm, &dot_r);
  float cy_gl = (float)win_h - cy_top;

  for (int i = 0; i < 6; i++)
  {
    handles[i].id = defs[i].id;
    handles[i].ax = defs[i].ax;
    handles[i].ay = defs[i].ay;
    handles[i].az = defs[i].az;
    handles[i].r  = defs[i].r;
    handles[i].g  = defs[i].g;
    handles[i].b  = defs[i].b;
    handles[i].label  = defs[i].label;
    handles[i].cmd    = defs[i].cmd;
    handles[i].is_pos = defs[i].is_pos;

    double sx = R[0][0] * defs[i].ax + R[1][0] * defs[i].ay + R[2][0] * defs[i].az;
    double sy = R[0][1] * defs[i].ax + R[1][1] * defs[i].ay + R[2][1] * defs[i].az;
    double sz = R[0][2] * defs[i].ax + R[1][2] * defs[i].ay + R[2][2] * defs[i].az;

    handles[i].sx = (float)sx;
    handles[i].sy = (float)sy;
    handles[i].sz = (float)sz;
    handles[i].px_top = cx + (float)sx * arm;
    handles[i].py_top = cy_top - (float)sy * arm;
    handles[i].px_gl  = cx + (float)sx * arm;
    handles[i].py_gl  = cy_gl + (float)sy * arm;
  }

  /* Sort back-to-front by sz (6-element insertion sort) */
  for (int i = 1; i < 6; i++)
  {
    CGXGizmoHandle key = handles[i];
    int j = i - 1;
    while (j >= 0 && handles[j].sz > key.sz)
    {
      handles[j + 1] = handles[j];
      j--;
    }
    handles[j + 1] = key;
  }
}

static void draw_filled_circle(float cx, float cy, float r, int segs)
{
  glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segs; i++)
    {
      float a = (float)i * (6.2831853f / (float)segs);
      glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
  glEnd();
}

static void draw_circle_outline(float cx, float cy, float r, int segs)
{
  glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segs; i++)
    {
      float a = (float)i * (6.2831853f / (float)segs);
      glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
  glEnd();
}

static int hit_test_orientation_gizmo(int mx, int my, int win_w, int win_h)
{
  if (!g_gizmo_visible || win_w < 200 || win_h < 200) return -1;

  float cx, cy_top, radius, arm, dot_r;
  get_gizmo_metrics(win_w, win_h, &cx, &cy_top, &radius, &arm, &dot_r);

  float dx_c = (float)mx - cx;
  float dy_c = (float)my - cy_top;
  if (dx_c * dx_c + dy_c * dy_c > (radius + 6.0f) * (radius + 6.0f)) return -1;

  CGXGizmoHandle handles[6];
  build_gizmo_handles(win_w, win_h, handles);

  /* Check front-to-back across axis handles */
  float hit_r = dot_r * 1.15f;
  for (int i = 5; i >= 0; i--)
  {
    float dx = (float)mx - handles[i].px_top;
    float dy = (float)my - handles[i].py_top;
    if (dx * dx + dy * dy <= hit_r * hit_r) return handles[i].id;
  }

  /* Center Iso button */
  if (dx_c * dx_c + dy_c * dy_c <= (dot_r * 0.9f) * (dot_r * 0.9f)) return 6;

  return -2; /* inside gizmo disc backdrop */
}

static void dispatch_gizmo_click(int idx)
{
  static const char *cmds[7] = {
    "rot x", "rot -x", "rot y", "rot -y", "rot z", "rot -z", "rot iso"
  };
  if (idx >= 0 && idx < 7) queue_command(cmds[idx]);
}

static void render_orientation_gizmo(int win_w, int win_h)
{
  if (!g_gizmo_visible || win_w < 200 || win_h < 200) return;

  float cx, cy_top, radius, arm, dot_r;
  get_gizmo_metrics(win_w, win_h, &cx, &cy_top, &radius, &arm, &dot_r);
  float cy_gl = (float)win_h - cy_top;
  int dark = (backgrndcol == 0);
  int tier = get_ui_font_tier();
  float cap_half = (tier == 0) ? 4.5f : (tier == 1 ? 6.5f : 10.5f);

  CGXGizmoHandle handles[6];
  build_gizmo_handles(win_w, win_h, handles);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  /* Subtle circular backdrop */
  if (dark) glColor4f(0.06f, 0.09f, 0.14f, 0.65f);
  else      glColor4f(0.92f, 0.94f, 0.97f, 0.78f);
  draw_filled_circle(cx, cy_gl, radius, 28);

  if (dark) glColor4f(0.22f, 0.32f, 0.44f, 0.70f);
  else      glColor4f(0.68f, 0.74f, 0.82f, 0.85f);
  glLineWidth(1.0f);
  draw_circle_outline(cx, cy_gl, radius, 28);

  /* Draw axis stems back-to-front */
  for (int i = 0; i < 6; i++)
  {
    float alpha = handles[i].is_pos ? 0.92f : 0.40f;
    glLineWidth(handles[i].is_pos ? 2.2f : 1.2f);
    glColor4f(handles[i].r, handles[i].g, handles[i].b, alpha);
    glBegin(GL_LINES);
      glVertex2f(cx, cy_gl);
      glVertex2f(handles[i].px_gl, handles[i].py_gl);
    glEnd();
  }

  /* Center Iso handle */
  int iso_hov = (g_gizmo_hovered_idx == 6);
  if (iso_hov) glColor4f(0.25f, 0.78f, 1.0f, 0.95f);
  else if (dark) glColor4f(0.35f, 0.45f, 0.58f, 0.85f);
  else           glColor4f(0.55f, 0.62f, 0.72f, 0.90f);
  draw_filled_circle(cx, cy_gl, dot_r * (iso_hov ? 0.65f : 0.48f), 16);

  /* Draw axis handles back-to-front */
  for (int i = 0; i < 6; i++)
  {
    int hov = (g_gizmo_hovered_idx == handles[i].id);
    float r_circle = handles[i].is_pos ? dot_r : (dot_r * 0.76f);
    if (hov) r_circle *= 1.16f;

    float depth_alpha = 0.55f + 0.45f * ((handles[i].sz + 1.0f) * 0.5f);
    if (!handles[i].is_pos && !hov) depth_alpha *= 0.65f;
    if (hov) depth_alpha = 1.0f;

    glColor4f(handles[i].r, handles[i].g, handles[i].b, depth_alpha);
    draw_filled_circle(handles[i].px_gl, handles[i].py_gl, r_circle, 20);

    if (hov)
    {
      glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
      glLineWidth(1.5f);
      draw_circle_outline(handles[i].px_gl, handles[i].py_gl, r_circle, 20);
    }

    if (handles[i].is_pos || hov)
    {
      float tw = get_text_width_dynamic(handles[i].label);
      draw_ui_text_dynamic(handles[i].px_top - tw * 0.5f,
                           handles[i].py_top + cap_half,
                           handles[i].label,
                           1.0f, 1.0f, 1.0f, win_h);
    }
  }
}

/* --------------------------------------------------------------------  */
/* GLFW Callback Handlers & Event Translation                            */
/* --------------------------------------------------------------------  */

static void glfw_key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  (void)window; (void)scancode; (void)mods;
  if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

  if (action == GLFW_PRESS && mods == 0)
  {
    if (key == GLFW_KEY_F2)
    {
      cgx_glfw_toggle_hud_toolbar();
      return;
    }
    if (key == GLFW_KEY_F3)
    {
      cgx_glfw_toggle_gizmo();
      return;
    }
    if (key == GLFW_KEY_F4)
    {
      cgx_tree_toggle();
      return;
    }
  }

  if (g_cascade_depth > 0 && key == GLFW_KEY_ESCAPE)
  {
    g_cascade_depth = 0;
    g_need_redisplay = 1;
    return;
  }

  /* Universal Clipboard Paste: Cmd+V / Ctrl+V / Shift+Insert from anywhere in the window */
  if ((key == GLFW_KEY_V && (mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER))) ||
      (key == GLFW_KEY_INSERT && (mods & GLFW_MOD_SHIFT)))
  {
    g_cmd_bar_visible = 1;
    g_cmd_focused = 1;
    handle_command_bar_paste();
    return;
  }

  if (g_cmd_bar_visible && g_cmd_focused && g_cascade_depth <= 0)
  {
    /* Clipboard Copy: Cmd+C (macOS) or Ctrl+C (Linux/Windows) */
    if (key == GLFW_KEY_C && (mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER)))
    {
      if (g_cmd_len > 0)
      {
        glfwSetClipboardString(g_glfw_window, g_cmd_buf);
        cgx_set_gui_status("Copied command to clipboard");
      }
      return;
    }
    /* Clipboard Cut: Cmd+X (macOS) or Ctrl+X (Linux/Windows) */
    else if (key == GLFW_KEY_X && (mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER)))
    {
      if (g_cmd_len > 0)
      {
        glfwSetClipboardString(g_glfw_window, g_cmd_buf);
        g_cmd_buf[0] = '\0';
        g_cmd_len = 0;
        cgx_set_gui_status("Cut command to clipboard");
        g_need_redisplay = 1;
      }
      return;
    }
    /* Clear Line: Ctrl+U or Cmd+Backspace */
    else if ((key == GLFW_KEY_U && (mods & GLFW_MOD_CONTROL)) || (key == GLFW_KEY_BACKSPACE && (mods & GLFW_MOD_SUPER)))
    {
      g_cmd_buf[0] = '\0';
      g_cmd_len = 0;
      g_need_redisplay = 1;
      return;
    }
    else if (key == GLFW_KEY_ENTER)
    {
      submit_command_bar();
      return;
    }
    else if (key == GLFW_KEY_BACKSPACE)
    {
      if (g_cmd_len > 0)
      {
        g_cmd_buf[--g_cmd_len] = '\0';
        g_need_redisplay = 1;
      }
      return;
    }
    else if (key == GLFW_KEY_ESCAPE)
    {
      g_cmd_buf[0] = '\0';
      g_cmd_len = 0;
      g_need_redisplay = 1;
      return;
    }
    else if (key == GLFW_KEY_UP)
    {
      if (g_cmd_hist_count > 0 && g_cmd_hist_pos > 0)
      {
        g_cmd_hist_pos--;
        strncpy(g_cmd_buf, g_cmd_history[g_cmd_hist_pos], sizeof(g_cmd_buf) - 1);
        g_cmd_len = (int)strlen(g_cmd_buf);
        g_need_redisplay = 1;
      }
      return;
    }
    else if (key == GLFW_KEY_DOWN)
    {
      if (g_cmd_hist_pos < g_cmd_hist_count - 1)
      {
        g_cmd_hist_pos++;
        strncpy(g_cmd_buf, g_cmd_history[g_cmd_hist_pos], sizeof(g_cmd_buf) - 1);
        g_cmd_len = (int)strlen(g_cmd_buf);
      }
      else
      {
        g_cmd_hist_pos = g_cmd_hist_count;
        g_cmd_buf[0] = '\0';
        g_cmd_len = 0;
      }
      g_need_redisplay = 1;
      return;
    }
  }

  g_current_mods = mods;
  CGXWindow *win = get_window(g_current_window_id);
  if (!win) win = get_window(1);
  if (!win) return;

  int special = 0;
  switch (key)
  {
    case GLFW_KEY_F1: special = GLUT_KEY_F1; break;
    case GLFW_KEY_F2: special = GLUT_KEY_F2; break;
    case GLFW_KEY_F3: special = GLUT_KEY_F3; break;
    case GLFW_KEY_F4: special = GLUT_KEY_F4; break;
    case GLFW_KEY_F5: special = GLUT_KEY_F5; break;
    case GLFW_KEY_F6: special = GLUT_KEY_F6; break;
    case GLFW_KEY_F7: special = GLUT_KEY_F7; break;
    case GLFW_KEY_F8: special = GLUT_KEY_F8; break;
    case GLFW_KEY_F9: special = GLUT_KEY_F9; break;
    case GLFW_KEY_F10: special = GLUT_KEY_F10; break;
    case GLFW_KEY_F11: special = GLUT_KEY_F11; break;
    case GLFW_KEY_F12: special = GLUT_KEY_F12; break;
    case GLFW_KEY_LEFT: special = GLUT_KEY_LEFT; break;
    case GLFW_KEY_UP: special = GLUT_KEY_UP; break;
    case GLFW_KEY_RIGHT: special = GLUT_KEY_RIGHT; break;
    case GLFW_KEY_DOWN: special = GLUT_KEY_DOWN; break;
    case GLFW_KEY_PAGE_UP: special = GLUT_KEY_PAGE_UP; break;
    case GLFW_KEY_PAGE_DOWN: special = GLUT_KEY_PAGE_DOWN; break;
    case GLFW_KEY_HOME: special = GLUT_KEY_HOME; break;
    case GLFW_KEY_END: special = GLUT_KEY_END; break;
    case GLFW_KEY_INSERT: special = GLUT_KEY_INSERT; break;
    default: break;
  }

  int sx, sy, sw, sh;
  get_window_screen_rect(win, &sx, &sy, &sw, &sh);
  int mx = (int)g_last_mouse_x - sx;
  int my = (int)g_last_mouse_y - sy;

  if (special && win->special_func)
  {
    win->special_func(special, mx, my);
    g_need_redisplay = 1;
  }
}

static void glfw_char_callback(GLFWwindow *window, unsigned int codepoint)
{
  (void)window;
  if (g_glfw_window)
  {
    if (glfwGetKey(g_glfw_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(g_glfw_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(g_glfw_window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS ||
        glfwGetKey(g_glfw_window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
    {
      return;
    }
  }
  if (codepoint >= 32 && codepoint < 127)
  {
    if (g_cmd_bar_visible && g_cmd_focused && g_cascade_depth <= 0)
    {
      g_gui_status_msg[0] = '\0';
      if (g_cmd_len < (int)sizeof(g_cmd_buf) - 2)
      {
        g_cmd_buf[g_cmd_len++] = (char)codepoint;
        g_cmd_buf[g_cmd_len] = '\0';
        g_need_redisplay = 1;
      }
      return;
    }

    CGXWindow *win = get_window(g_current_window_id);
    if (!win) win = get_window(1);
    if (win && win->keyboard_func)
    {
      int sx, sy, sw, sh;
      get_window_screen_rect(win, &sx, &sy, &sw, &sh);
      int mx = (int)g_last_mouse_x - sx;
      int my = (int)g_last_mouse_y - sy;
      win->keyboard_func((unsigned char)codepoint, mx, my);
      g_need_redisplay = 1;
    }
  }
}

static void glfw_cursor_pos_callback(GLFWwindow *window, double xpos, double ypos)
{
  (void)window;
  g_last_mouse_x = xpos;
  g_last_mouse_y = ypos;

  int win_w, win_h;
  glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
  int bar_h = get_cmd_bar_height();

  /* Send button hover check */
  if (g_cmd_bar_visible && ypos >= win_h - bar_h && xpos >= win_w - 96 && xpos <= win_w - 10)
  {
    g_send_hovered = 1;
    g_need_redisplay = 1;
  }
  else if (g_send_hovered)
  {
    g_send_hovered = 0;
    g_need_redisplay = 1;
  }

  /* Multi-level menu hover update */
  if (g_cascade_depth > 0)
  {
    g_hud_hovered_btn = -1;
    g_hud_tab_hovered = 0;
    g_gizmo_hovered_idx = -1;
    update_cascade_hover((int)xpos, (int)ypos, win_w, win_h);
    return;
  }

  /* Lightweight fixed top-right Tools tab + optional HUD Toolbar hover checks */
  {
    int tab_h = hit_test_hud_pull_tab((int)xpos, (int)ypos, win_w, win_h);
    if (tab_h != g_hud_tab_hovered)
    {
      g_hud_tab_hovered = tab_h;
      g_need_redisplay = 1;
    }
    if (tab_h)
    {
      if (g_hud_hovered_btn != -1)
      {
        g_hud_hovered_btn = -1;
        g_need_redisplay = 1;
      }
      if (g_mouse_buttons[0] != GLUT_DOWN &&
          g_mouse_buttons[1] != GLUT_DOWN &&
          g_mouse_buttons[2] != GLUT_DOWN &&
          !g_right_dragging)
      {
        return;
      }
    }
  }

  if (g_hud_toolbar_visible)
  {
    int h = hit_test_hud_toolbar((int)xpos, (int)ypos, win_w, win_h);
    int btn_h = (h >= 0) ? h : -1;
    if (btn_h != g_hud_hovered_btn)
    {
      g_hud_hovered_btn = btn_h;
      g_need_redisplay = 1;
    }
    if (h != -1 &&
        g_mouse_buttons[0] != GLUT_DOWN &&
        g_mouse_buttons[1] != GLUT_DOWN &&
        g_mouse_buttons[2] != GLUT_DOWN &&
        !g_right_dragging)
    {
      return;
    }
  }
  else
  {
    g_hud_hovered_btn = -1;
  }

  if (g_gizmo_visible)
  {
    int gh = hit_test_orientation_gizmo((int)xpos, (int)ypos, win_w, win_h);
    int handle_h = (gh >= 0) ? gh : -1;
    if (handle_h != g_gizmo_hovered_idx)
    {
      g_gizmo_hovered_idx = handle_h;
      g_need_redisplay = 1;
    }
    if (gh != -1 &&
        g_mouse_buttons[0] != GLUT_DOWN &&
        g_mouse_buttons[1] != GLUT_DOWN &&
        g_mouse_buttons[2] != GLUT_DOWN &&
        !g_right_dragging)
    {
      return;
    }
  }
  else
  {
    g_gizmo_hovered_idx = -1;
  }

  if (cgx_tree_on_mouse_move((int)xpos, (int)ypos, win_w, win_h) &&
      g_mouse_buttons[0] != GLUT_DOWN &&
      g_mouse_buttons[1] != GLUT_DOWN &&
      g_mouse_buttons[2] != GLUT_DOWN &&
      !g_right_dragging)
  {
    return;
  }

  if (g_cmd_bar_visible && ypos >= win_h - bar_h) return;

  /* Check right-button drag threshold */
  if (g_right_down && !g_right_dragging)
  {
    double rdx = xpos - g_right_down_x;
    double rdy = ypos - g_right_down_y;
    double thresh = 4.0 * (double)cgx_get_fb_scale();
    if ((rdx * rdx + rdy * rdy) > (thresh * thresh))
    {
      g_right_dragging = 1;
      g_mouse_buttons[GLUT_RIGHT_BUTTON] = GLUT_DOWN;

      CGXWindow *start_win = find_window_at((int)g_right_down_x, (int)g_right_down_y);
      if (start_win && start_win->mouse_func)
      {
        int sx, sy, sw, sh;
        get_window_screen_rect(start_win, &sx, &sy, &sw, &sh);
        int local_sx = (int)g_right_down_x - sx;
        int local_sy = (int)g_right_down_y - sy;
        int prev_win_id = g_current_window_id;
        g_current_window_id = start_win->id;
        start_win->mouse_func(GLUT_RIGHT_BUTTON, GLUT_DOWN, local_sx, local_sy);
        g_current_window_id = prev_win_id;
      }
    }
  }

  CGXWindow *win = find_window_at((int)xpos, (int)ypos);
  if (!win) return;

  int sx, sy, sw, sh;
  get_window_screen_rect(win, &sx, &sy, &sw, &sh);
  int local_x = (int)xpos - sx;
  int local_y = (int)ypos - sy;

  int prev_win_id = g_current_window_id;
  g_current_window_id = win->id;

  int any_btn_down = (g_mouse_buttons[0] == GLUT_DOWN ||
                      g_mouse_buttons[1] == GLUT_DOWN ||
                      g_mouse_buttons[2] == GLUT_DOWN ||
                      g_right_dragging);

  if (any_btn_down && win->motion_func)
  {
    win->motion_func(local_x, local_y);
    g_need_redisplay = 1;
  }
  else if (!any_btn_down && win->passive_motion_func)
  {
    win->passive_motion_func(local_x, local_y);
    g_need_redisplay = 1;
  }

  g_current_window_id = prev_win_id;
}

static void glfw_mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
  (void)window;
  g_current_mods = mods;

  int win_w, win_h;
  glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
  int bar_h = get_cmd_bar_height();

  /* Handle Cascade Popup Menu Click */
  if (g_cascade_depth > 0 && action == GLFW_PRESS)
  {
    if (handle_cascade_click((int)g_last_mouse_x, (int)g_last_mouse_y))
    {
      return;
    }
  }

  /* Handle Command Bar Click */
  if (g_cmd_bar_visible && action == GLFW_PRESS && g_last_mouse_y >= win_h - bar_h)
  {
    if (g_last_mouse_x >= win_w - 96 && g_last_mouse_x <= win_w - 10)
    {
      submit_command_bar();
      return;
    }
    g_cmd_focused = 1;
    g_cascade_depth = 0;
    g_need_redisplay = 1;
    return;
  }

  /* Consume paired press/release on HUD Toolbar or Orientation Gizmo without stopping auto-rotate */
  static int g_hud_click_captured = 0;
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && g_hud_click_captured)
  {
    g_hud_click_captured = 0;
    return;
  }
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
  {
    if (hit_test_hud_pull_tab((int)g_last_mouse_x, (int)g_last_mouse_y, win_w, win_h))
    {
      g_hud_click_captured = 1;
      cgx_glfw_toggle_hud_toolbar();
      return;
    }
    if (g_hud_toolbar_visible)
    {
      int h = hit_test_hud_toolbar((int)g_last_mouse_x, (int)g_last_mouse_y, win_w, win_h);
      if (h != -1)
      {
        g_hud_click_captured = 1;
        if (h >= 0) dispatch_hud_button(h);
        g_need_redisplay = 1;
        return;
      }
    }
    if (g_gizmo_visible)
    {
      int gh = hit_test_orientation_gizmo((int)g_last_mouse_x, (int)g_last_mouse_y, win_w, win_h);
      if (gh != -1)
      {
        g_hud_click_captured = 1;
        if (gh >= 0) dispatch_gizmo_click(gh);
        g_need_redisplay = 1;
        return;
      }
    }
  }

  if (cgx_tree_on_mouse_button(button, action, mods, (int)g_last_mouse_x, (int)g_last_mouse_y, win_w, win_h))
  {
    g_need_redisplay = 1;
    return;
  }

  CGXWindow *win = find_window_at((int)g_last_mouse_x, (int)g_last_mouse_y);
  if (!win) return;

  /* Any direct click on the 3D viewport stops interactive auto-rotate */
  if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT && g_auto_rotate_active)
  {
    g_auto_rotate_active = 0;
    cgx_set_gui_status("Auto-rotate stopped");
  }

  int sx, sy, sw, sh;
  get_window_screen_rect(win, &sx, &sy, &sw, &sh);
  int local_x = (int)g_last_mouse_x - sx;
  int local_y = (int)g_last_mouse_y - sy;

  /* Right-Click Handling: Drag vs Menu */
  if (button == GLFW_MOUSE_BUTTON_RIGHT)
  {
    if (action == GLFW_PRESS)
    {
      g_right_down = 1;
      g_right_down_x = g_last_mouse_x;
      g_right_down_y = g_last_mouse_y;
      g_right_dragging = 0;
      return;
    }
    else if (action == GLFW_RELEASE)
    {
      g_right_down = 0;
      if (g_right_dragging)
      {
        g_right_dragging = 0;
        g_mouse_buttons[GLUT_RIGHT_BUTTON] = GLUT_UP;
        int prev_win_id = g_current_window_id;
        g_current_window_id = win->id;
        if (win->mouse_func) win->mouse_func(GLUT_RIGHT_BUTTON, GLUT_UP, local_x, local_y);
        g_current_window_id = prev_win_id;
        g_need_redisplay = 1;
      }
      else
      {
        int menu_to_open = (mainmenu > 0) ? mainmenu : win->attached_menu[GLUT_RIGHT_BUTTON];
        if (menu_to_open <= 0 && g_num_menus > 0) menu_to_open = g_menus[g_num_menus - 1].id;
        if (menu_to_open > 0)
        {
          open_cascade_root(menu_to_open, (int)g_right_down_x, (int)g_right_down_y, win_w, win_h);
        }
      }
      return;
    }
  }

  int glut_btn = GLUT_LEFT_BUTTON;
  if (button == GLFW_MOUSE_BUTTON_MIDDLE)
  {
    glut_btn = GLUT_MIDDLE_BUTTON;
  }
  else if (button == GLFW_MOUSE_BUTTON_LEFT)
  {
    if (action == GLFW_PRESS)
    {
      if ((mods & GLFW_MOD_ALT) ||
          ((mods & GLFW_MOD_SHIFT) && (mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER))))
      {
        g_left_mapped_btn = GLUT_MIDDLE_BUTTON; /* Zoom */
      }
      else if (mods & (GLFW_MOD_CONTROL | GLFW_MOD_SUPER | GLFW_MOD_SHIFT))
      {
        g_left_mapped_btn = GLUT_RIGHT_BUTTON; /* Pan */
      }
      else
      {
        g_left_mapped_btn = GLUT_LEFT_BUTTON; /* Rotate */
      }
    }
    glut_btn = g_left_mapped_btn;
  }

  int glut_state = (action == GLFW_PRESS) ? GLUT_DOWN : GLUT_UP;
  g_mouse_buttons[glut_btn] = glut_state;

  int prev_win_id = g_current_window_id;
  g_current_window_id = win->id;

  if (win->mouse_func)
  {
    win->mouse_func(glut_btn, glut_state, local_x, local_y);
    g_need_redisplay = 1;
  }

  g_current_window_id = prev_win_id;
}

static void glfw_scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
  (void)window; (void)xoffset;
  int win_w = 800, win_h = 600;
  if (g_glfw_window) glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
  if (cgx_tree_on_scroll(yoffset, (int)g_last_mouse_x, (int)g_last_mouse_y, win_w, win_h))
  {
    g_need_redisplay = 1;
    return;
  }

  CGXWindow *win = find_window_at((int)g_last_mouse_x, (int)g_last_mouse_y);
  if (!win) win = get_window(2);
  if (!win || !win->mouse_func) return;

  int btn = (yoffset > 0) ? GLUT_WEEL_UP : GLUT_WEEL_DOWN;
  int sx, sy, sw, sh;
  get_window_screen_rect(win, &sx, &sy, &sw, &sh);
  int local_x = (int)g_last_mouse_x - sx;
  int local_y = (int)g_last_mouse_y - sy;

  int prev_win_id = g_current_window_id;
  g_current_window_id = win->id;

  win->mouse_func(btn, GLUT_DOWN, local_x, local_y);
  win->mouse_func(btn, GLUT_UP, local_x, local_y);
  g_need_redisplay = 1;

  g_current_window_id = prev_win_id;
}

static void glfw_window_size_callback(GLFWwindow *window, int width, int height)
{
  (void)window;
  if (width < 10) width = 10;
  if (height < 10) height = 10;
  CGXWindow *root = get_window(1);
  if (root)
  {
    root->width = width;
    root->height = height;
    if (root->reshape_func) root->reshape_func(width, height);
  }
  g_need_redisplay = 1;
}

static void glfw_framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
  (void)window; (void)width; (void)height;
  int win_w = 1, win_h = 1;
  if (g_glfw_window) glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
  if (win_w < 10) win_w = 10;
  if (win_h < 10) win_h = 10;
  CGXWindow *root = get_window(1);
  if (root)
  {
    root->width = win_w;
    root->height = win_h;
    if (root->reshape_func) root->reshape_func(win_w, win_h);
  }
  g_need_redisplay = 1;
}

/* --------------------------------------------------------------------  */
/* Main Render Loop                                                      */
/* --------------------------------------------------------------------  */

void glutMainLoop(void)
{
  if (!g_glfw_window) return;

  glfwSetWindowSizeCallback(g_glfw_window, glfw_window_size_callback);
  glfwSetFramebufferSizeCallback(g_glfw_window, glfw_framebuffer_size_callback);
  glfwSetKeyCallback(g_glfw_window, glfw_key_callback);
  glfwSetCharCallback(g_glfw_window, glfw_char_callback);
  glfwSetCursorPosCallback(g_glfw_window, glfw_cursor_pos_callback);
  glfwSetMouseButtonCallback(g_glfw_window, glfw_mouse_button_callback);
  glfwSetScrollCallback(g_glfw_window, glfw_scroll_callback);

  int win_w, win_h, fb_w, fb_h;

  while (!glfwWindowShouldClose(g_glfw_window))
  {
    /* During startup (idle func active), auto-rotate, turntable recording, or pending redisplay, poll events.
       When truly idle, block with a timeout to save CPU/GPU cycles. */
    if (g_idle_func || g_need_redisplay || g_auto_rotate_active || g_turntable_rec_remaining > 0)
    {
      glfwPollEvents();
    }
    else
    {
      glfwWaitEventsTimeout(0.05);
    }
    /* ready gate: hold stdin commands until the startup idle function (model load) is done */
    if (!g_idle_func) process_queued_commands();

    if (g_idle_func)
    {
      g_idle_func();
      g_need_redisplay = 1;
    }
    else if (g_turntable_rec_remaining > 0)
    {
      g_need_redisplay = 1;
    }
    else if (g_auto_rotate_active)
    {
      double now = glfwGetTime();
      double dt = now - g_auto_rotate_last_time;
      g_auto_rotate_last_time = now;
      if (dt < 0.0) dt = 0.0;
      if (dt > 0.1) dt = 0.1;
      if (dt > 0.0)
      {
        rot_r(g_auto_rotate_speed * dt);
      }
      g_need_redisplay = 1;
    }

    if (g_need_redisplay)
    {
      g_need_redisplay = 0;

      glfwGetWindowSize(g_glfw_window, &win_w, &win_h);
      glfwGetFramebufferSize(g_glfw_window, &fb_w, &fb_h);
      double scale_x = (win_w > 0) ? (double)fb_w / (double)win_w : 1.0;
      double scale_y = (win_h > 0) ? (double)fb_h / (double)win_h : 1.0;

      /* Render root window (w0) with GL state isolation */
      CGXWindow *root = get_window(1);
      if (root && root->display_func)
      {
        g_current_window_id = 1;
        glViewport(0, 0, fb_w, fb_h);
        glDisable(GL_SCISSOR_TEST);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        root->display_func();
        glPopAttrib();
      }

      /* Render subwindows (w1 3D model, w2 axes triad, w3, etc.) */
      for (int i = 0; i < g_num_windows; i++)
      {
        CGXWindow *win = &g_windows[i];
        if (win->id == 1 || !win->visible || !win->display_func) continue;

        g_current_window_id = win->id;

        int sx, sy, sw, sh;
        get_window_screen_rect(win, &sx, &sy, &sw, &sh);

        int vp_x = (int)(sx * scale_x);
        int vp_y = (int)((win_h - (sy + sh)) * scale_y);
        int vp_w = (int)(sw * scale_x);
        int vp_h = (int)(sh * scale_y);

        glViewport(vp_x, vp_y, vp_w, vp_h);
        glEnable(GL_SCISSOR_TEST);
        glScissor(vp_x, vp_y, vp_w, vp_h);

        /* Only isolate GL state for non-w1 subwindows (e.g. w2 DrawAxes, w3 cmdline, etc.)
           w1 (the 3D viewport) inherits and maintains the active polygon mode (LINES/DOTS/FILL) */
        int is_w1 = (win->id == w1 || win->id == 2);
        if (!is_w1)
        {
          glPushAttrib(GL_ALL_ATTRIB_BITS);
          glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        win->display_func();

        glDisable(GL_SCISSOR_TEST);
        if (!is_w1)
        {
          glPopAttrib();
        }
      }

      if (g_hud_toolbar_visible)
      {
        GLint pm[2] = { GL_FILL, GL_FILL };
        glGetIntegerv(GL_POLYGON_MODE, pm);
        g_is_wireframe = (pm[0] == GL_LINE);
      }

      /* Capture screenshots & turntable frames before 2D HUD overlays so exported images stay clean */
      if (g_hcpy_pending)
      {
        char name[512];
        strcpy(name, g_hcpy_name);
        g_hcpy_pending = 0;
        createHardcopy(5, name[0] ? name : NULL);
      }

      if (g_turntable_rec_remaining > 0)
      {
        if (!cgx_movie_is_recording())
        {
          g_turntable_rec_remaining = 0;
        }
        else
        {
          cgx_movie_add_frame();
          g_turntable_rec_remaining--;
          if (g_turntable_rec_remaining > 0 && cgx_movie_is_recording())
          {
            rot_r(g_turntable_step_deg);
            g_need_redisplay = 1;
          }
          else
          {
            rot_r(g_turntable_step_deg);
            cgx_set_gui_status("Turntable recording complete");
          }
        }
      }

      /* 2D UI Overlay Pass: Command Bar, Optional HUD Widgets & Cascade Popup Menu */
      glViewport(0, 0, fb_w, fb_h);
      glDisable(GL_SCISSOR_TEST);

      glPushAttrib(GL_ALL_ATTRIB_BITS);
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();
      gluOrtho2D(0, win_w, 0, win_h);

      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();

      glDisable(GL_DEPTH_TEST);
      glDisable(GL_LIGHTING);
      glDisable(GL_CULL_FACE);
      glDisable(GL_TEXTURE_2D);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      /* Render expanded HUD Toolbar to the left of the fixed top-right Tools tab */
      if (g_hud_toolbar_visible)
      {
        render_hud_toolbar(win_w, win_h);
      }
      render_hud_pull_tab(win_w, win_h);

      /* Render optional top-right Interactive Axes if visible */
      if (g_gizmo_visible)
      {
        render_orientation_gizmo(win_w, win_h);
      }

      /* Render fixed top-left Tree tab and optional Model/Results drawer */
      cgx_tree_render(win_w, win_h, fb_w, fb_h);

      /* Render bottom Command Bar if visible */
      if (g_cmd_bar_visible)
      {
        render_command_bar(win_w, win_h);
      }

      /* Render all active Cascade Menu Levels */
      for (int lvl = 0; lvl < g_cascade_depth; lvl++)
      {
        draw_cascade_menu(&g_cascade[lvl], win_h);
      }

      glPopMatrix();
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glPopAttrib();

      glfwSwapBuffers(g_glfw_window);
    }
  }

  glfwDestroyWindow(g_glfw_window);
  glfwTerminate();
}
