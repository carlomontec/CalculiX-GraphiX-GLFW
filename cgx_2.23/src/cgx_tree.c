/* --------------------------------------------------------------------  */
/* CalculiX GraphiX (GLFW Edition)                                       */
/* File: cgx_tree.c - Lightweight Collapsible Model & Results Tree       */
/*                                                                       */
/* Created by Carlo Monjaraz-Tec (2026) with AI pair-programming         */
/* assistance based on the original work of Klaus Wittig & contributors. */
/*                                                                       */
/* Licensed under GNU General Public License v2 (GPL-2.0 or later).      */
/* --------------------------------------------------------------------  */

#include "cgx_tree.h"
#include "cgx_glut_glfw.h"
#include "extUtil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* External references to CGX core state */
extern Summen    anz[1];
extern SumGeo    anzGeo[1];
extern Sets     *set;
extern Psets    *pset;
extern Datasets *lcase;
extern Materials *material;
extern int       cur_lc;
extern int       pre_lc;
extern int       cur_entity;
extern int       backgrndcol;
extern int       inpformat;
extern char      drawMode;
extern char      surfFlag;

extern void selectDataset(int selection);
extern void selectEntity(int selection);
extern void selectView(int selection);
extern void ConfigureAndShowWindow_Load(void);
extern void plot(char *record);
extern void plus(char *record);
extern void minus(char *record);

#define MAX_TREE_ROWS 1024

typedef enum {
  SEC_MAT = 0,
  SEC_ELEM,
  SEC_NODE,
  SEC_GEO,
  SEC_COUNT
} TreeSectionId;

typedef enum {
  ROW_SECTION = 0,
  ROW_SET,
  ROW_DATASET,
  ROW_COMPONENT,
  ROW_INFO
} TreeRowKind;

typedef struct {
  TreeRowKind kind;
  int id;       /* section id, set index, or dataset index */
  int sub_id;   /* category for ROW_SET, or component index for ROW_COMPONENT */
  int count;    /* item/entity count for badges */
} TreeRow;

static int g_tree_visible = 0;
static int g_tree_tab = 0; /* 0: Model, 1: Results */
static int g_sec_collapsed[SEC_COUNT] = { 0, 0, 0, 0 };
static int g_expanded_lc = -2; /* -2 = uninitialized sentinel */

static int g_scroll_model = 0;
static int g_scroll_results = 0;

static int g_tab_hovered = 0;
static int g_header_hovered = -1; /* 0: Model tab, 1: Results tab, 2: All button */
static int g_hovered_row = -1;
static int g_hovered_solo = 0;
static int g_tree_click_captured = 0;
static int g_last_values_mode = 0;

static TreeRow g_rows[MAX_TREE_ROWS];
static int g_row_count = 0;

static void trim_trailing_spaces(const char *src, char *dst, size_t dst_sz)
{
  if (!src || dst_sz == 0) return;
  strncpy(dst, src, dst_sz - 1);
  dst[dst_sz - 1] = '\0';
  size_t len = strlen(dst);
  while (len > 0 && (dst[len - 1] == ' ' || dst[len - 1] == '\t' || dst[len - 1] == '\r' || dst[len - 1] == '\n'))
  {
    dst[--len] = '\0';
  }
}

static void draw_chevron_lr(float cx, float cy_gl, float half_w, float half_h, int point_right)
{
  float dx = point_right ? half_w : -half_w;
  glBegin(GL_LINE_STRIP);
    glVertex2f(cx - dx, cy_gl - half_h);
    glVertex2f(cx + dx, cy_gl);
    glVertex2f(cx - dx, cy_gl + half_h);
  glEnd();
}

static void draw_chevron_down(float cx, float cy_gl, float half_w, float half_h)
{
  glBegin(GL_LINE_STRIP);
    glVertex2f(cx - half_w, cy_gl + half_h);
    glVertex2f(cx,          cy_gl - half_h);
    glVertex2f(cx + half_w, cy_gl + half_h);
  glEnd();
}

static void draw_clipped_label(float x, float baseline_y, const char *str, float max_w,
                               float r, float g, float b, int win_h)
{
  if (!str || !*str || max_w <= 8.0f) return;
  char buf[128];
  strncpy(buf, str, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  if (cgx_ui_text_width(buf) <= max_w)
  {
    cgx_ui_draw_text(x, baseline_y, buf, r, g, b, win_h);
    return;
  }

  size_t len = strlen(buf);
  while (len > 1)
  {
    len--;
    if (len + 3 < sizeof(buf))
    {
      buf[len] = '.';
      buf[len + 1] = '.';
      buf[len + 2] = '.';
      buf[len + 3] = '\0';
      if (cgx_ui_text_width(buf) <= max_w) break;
    }
    buf[len] = '\0';
  }
  cgx_ui_draw_text(x, baseline_y, buf, r, g, b, win_h);
}

/* --------------------------------------------------------------------  */
/* Geometry & Layout Helpers                                             */
/* --------------------------------------------------------------------  */

static void get_pull_tab_rect(int win_w, int win_h, int *x1, int *y1, int *x2, int *y2)
{
  int vx, vy, vw, vh;
  cgx_ui_get_viewport_rect(win_w, win_h, &vx, &vy, &vw, &vh);
  (void)vw; (void)vh;
  int tier = cgx_ui_font_tier();
  int pad_x = (tier == 0) ? 9 : (tier == 1 ? 11 : 15);
  int gap_chev = (tier == 0) ? 6 : (tier == 1 ? 7 : 10);
  int chev_w = (tier == 0) ? 6 : (tier == 1 ? 8 : 11);
  int h = cgx_ui_top_tab_height();
  float tw = cgx_ui_text_width("Tree");
  int w = (int)ceil(tw) + gap_chev + chev_w + pad_x * 2;
  *x1 = vx;
  *x2 = *x1 + w;
  *y1 = vy;
  *y2 = vy + h;
}

static int get_row_height(void)
{
  int tier = cgx_ui_font_tier();
  if (tier == 0) return 22;
  if (tier == 1) return 26;
  return 36;
}

static void get_drawer_rect(int win_w, int win_h,
                            int *dx1, int *dy1, int *dx2, int *dy2,
                            int *list_y1, int *list_y2)
{
  int vx, vy, vw, vh;
  cgx_ui_get_viewport_rect(win_w, win_h, &vx, &vy, &vw, &vh);

  int tx1, ty1, tx2, ty2;
  get_pull_tab_rect(win_w, win_h, &tx1, &ty1, &tx2, &ty2);
  (void)tx1; (void)ty1; (void)tx2;

  int tier = cgx_ui_font_tier();
  int w = (tier == 0) ? 236 : (tier == 1 ? 276 : 348);
  int max_w = (int)(vw * 0.42f);
  if (w > max_w) w = max_w;
  if (w < 170) w = 170;

  int cmd_h = cgx_glfw_is_command_bar_visible() ? cgx_ui_cmd_bar_height() : 0;
  int hdr_h = (tier == 0) ? 28 : (tier == 1 ? 34 : 46);
  int bottom_lim = win_h - cmd_h - 8;
  if (vy + vh - 4 < bottom_lim) bottom_lim = vy + vh - 4;

  *dx1 = vx;
  *dx2 = vx + w;
  *dy1 = ty2;
  *dy2 = bottom_lim;
  if (*dy2 < *dy1 + hdr_h + 40) *dy2 = *dy1 + hdr_h + 40;

  *list_y1 = *dy1 + hdr_h;
  *list_y2 = *dy2 - 4;
}

static int hit_test_pull_tab(int mx, int my, int win_w, int win_h)
{
  if (win_w < 200 || win_h < 120) return 0;
  int x1, y1, x2, y2;
  get_pull_tab_rect(win_w, win_h, &x1, &y1, &x2, &y2);
  return (mx >= x1 && mx <= x2 && my >= y1 && my <= y2) ? 1 : 0;
}

static int is_point_in_drawer(int mx, int my, int win_w, int win_h)
{
  if (!g_tree_visible || win_w < 240 || win_h < 180) return 0;
  int dx1, dy1, dx2, dy2, ly1, ly2;
  get_drawer_rect(win_w, win_h, &dx1, &dy1, &dx2, &dy2, &ly1, &ly2);
  return (mx >= dx1 && mx <= dx2 && my >= dy1 && my <= dy2) ? 1 : 0;
}

static void get_header_button_rects(int dx1, int dy1, int dx2, int ly1,
                                    int out_x1[3], int out_x2[3],
                                    int *out_y1, int *out_y2)
{
  int tier = cgx_ui_font_tier();
  int pad = 6;
  int gap = 4;
  int btn_y1 = dy1 + 4;
  int btn_y2 = ly1 - 4;

  float all_tw = cgx_ui_text_width("All");
  int all_w = (g_tree_tab == 0) ? ((int)ceil(all_tw) + ((tier == 0) ? 14 : (tier == 1 ? 18 : 24))) : 0;
  int avail = (dx2 - dx1) - pad * 2 - gap - ((all_w > 0) ? (all_w + gap) : 0);
  int tab_w = avail / 2;

  out_x1[0] = dx1 + pad;
  out_x2[0] = out_x1[0] + tab_w;

  out_x1[1] = out_x2[0] + gap;
  out_x2[1] = out_x1[1] + tab_w;

  if (all_w > 0)
  {
    out_x1[2] = out_x2[1] + gap;
    out_x2[2] = dx2 - pad;
  }
  else
  {
    out_x1[2] = out_x2[2] = -1;
  }

  *out_y1 = btn_y1;
  *out_y2 = btn_y2;
}

/* --------------------------------------------------------------------  */
/* CGX Model & Results Inspection Helpers                                */
/* --------------------------------------------------------------------  */

static void sync_values_mode_tracker(void)
{
  if (drawMode == 1)
  {
    g_last_values_mode = 1;
    return;
  }
  if (drawMode == 2)
  {
    g_last_values_mode = 0;
    return;
  }
  for (int j = 0; j < anzGeo->psets; j++)
  {
    if (pset[j].type[0] != ' ' && pset[j].type[0] != '\0' && pset[j].type[1] == 'v')
    {
      g_last_values_mode = 1;
      return;
    }
  }
}

static int find_active_pset_index(int set_idx, char *out_typ0)
{
  for (int j = 0; j < anzGeo->psets; j++)
  {
    if (pset[j].nr == set_idx && pset[j].type[0] != ' ' && pset[j].type[0] != '\0')
    {
      if (out_typ0) *out_typ0 = pset[j].type[0];
      return j;
    }
  }
  return -1;
}

static int is_material_set(int set_idx)
{
  if (set[set_idx].material >= 0) return 1;
  if (anz->mats > 0 && material != NULL && set[set_idx].name != NULL)
  {
    for (int m = 0; m < anz->mats; m++)
    {
      if (material[m].name[0] != '\0' && strcmp(material[m].name, set[set_idx].name) == 0)
      {
        return 1;
      }
    }
  }
  return 0;
}

static int classify_set_category(int set_idx)
{
  if (set_idx < 0 || set_idx >= anz->sets) return -1;
  if (set[set_idx].name == NULL || set[set_idx].type != 0) return -1;
  if (set[set_idx].name[0] == '-') return -1; /* skip internal CGX sets */

  if (is_material_set(set_idx) && (set[set_idx].anz_e > 0 || set[set_idx].anz_f > 0))
  {
    return SEC_MAT;
  }
  if (set[set_idx].anz_e > 0 || set[set_idx].anz_f > 0)
  {
    return SEC_ELEM;
  }
  if (set[set_idx].anz_n > 0)
  {
    return SEC_NODE;
  }
  if (set[set_idx].anz_p + set[set_idx].anz_l + set[set_idx].anz_c +
      set[set_idx].anz_s + set[set_idx].anz_b + set[set_idx].anz_nurl +
      set[set_idx].anz_nurs > 0)
  {
    return SEC_GEO;
  }
  return -1;
}

static int get_set_entity_count(int set_idx, int cat)
{
  switch (cat)
  {
    case SEC_MAT:
    case SEC_ELEM:
      return (set[set_idx].anz_e > 0) ? set[set_idx].anz_e : set[set_idx].anz_f;
    case SEC_NODE:
      return set[set_idx].anz_n;
    case SEC_GEO:
      return set[set_idx].anz_p + set[set_idx].anz_l + set[set_idx].anz_s +
             set[set_idx].anz_b + set[set_idx].anz_nurl + set[set_idx].anz_nurs;
    default:
      return 0;
  }
}

static void get_set_plot_type(int set_idx, int cat, int is_solo, char out_typ[8])
{
  sync_values_mode_tracker();
  int show_val = (g_last_values_mode && anz->l > 0) ? 1 : 0;

  if (cat == SEC_MAT || cat == SEC_ELEM)
  {
    if (surfFlag && set[set_idx].anz_f > 0)
      strcpy(out_typ, show_val ? "fv" : "f");
    else if (set[set_idx].anz_e > 0)
      strcpy(out_typ, show_val ? "ev" : "e");
    else
      strcpy(out_typ, show_val ? "fv" : "f");
    return;
  }
  if (cat == SEC_NODE)
  {
    strcpy(out_typ, (is_solo && show_val) ? "nv" : "n");
    return;
  }
  if (set[set_idx].anz_s > 0) strcpy(out_typ, "s");
  else if (set[set_idx].anz_b > 0) strcpy(out_typ, "b");
  else if (set[set_idx].anz_l > 0) strcpy(out_typ, "l");
  else if (set[set_idx].anz_p > 0) strcpy(out_typ, "p");
  else if (set[set_idx].anz_nurs > 0) strcpy(out_typ, "S");
  else if (set[set_idx].anz_nurl > 0) strcpy(out_typ, "L");
  else strcpy(out_typ, "s");
}

typedef struct {
  const char *prefix;
  char code;
  float r;
  float g;
  float b;
} NodeSetColorRule;

static int starts_with_ci(const char *s, const char *prefix)
{
  if (!s || !prefix) return 0;
  while (*prefix)
  {
    if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return 0;
    s++;
    prefix++;
  }
  return 1;
}

/* Map auto-generated ('+') vs user/default node sets to CGX entitycol letters and RGB swatches. */
static char get_node_set_color_code(const char *name, float *out_r, float *out_g, float *out_b)
{
  static const NodeSetColorRule rules[] = {
    { "+bou",      'r', 0.95f, 0.25f, 0.25f },
    { "+spcf",     'r', 0.95f, 0.25f, 0.25f },
    { "+clo",      'o', 1.00f, 0.55f, 0.10f },
    { "+dlo",      'o', 1.00f, 0.55f, 0.10f },
    { "+cflx",     'o', 1.00f, 0.55f, 0.10f },
    { "+dflx",     'o', 1.00f, 0.55f, 0.10f },
    { "+rad",      'o', 1.00f, 0.55f, 0.10f },
    { "+flm",      'o', 1.00f, 0.55f, 0.10f },
    { "+mflw",     'o', 1.00f, 0.55f, 0.10f },
    { "+mpc",      'm', 0.92f, 0.30f, 0.92f },
    { "+dep",      'm', 0.92f, 0.30f, 0.92f },
    { "+ind",      'm', 0.92f, 0.30f, 0.92f },
    { "+tiecyc2",  'y', 0.95f, 0.85f, 0.15f },
    { "+tiemult2", 'y', 0.95f, 0.85f, 0.15f },
    { "+tie2",     'y', 0.95f, 0.85f, 0.15f },
    { "+cont2",    'y', 0.95f, 0.85f, 0.15f },
    { "+tie",      't', 0.10f, 0.85f, 0.90f },
    { "+cont",     't', 0.10f, 0.85f, 0.90f },
  };
  char code = 'g';
  float r = 0.15f, g = 0.82f, b = 0.35f;

  if (name != NULL && name[0] == '+')
  {
    code = 'r';
    r = 0.95f; g = 0.25f; b = 0.25f;
    int n_rules = (int)(sizeof(rules) / sizeof(rules[0]));
    for (int i = 0; i < n_rules; i++)
    {
      if (starts_with_ci(name, rules[i].prefix))
      {
        code = rules[i].code;
        r = rules[i].r;
        g = rules[i].g;
        b = rules[i].b;
        break;
      }
    }
  }

  if (out_r) *out_r = r;
  if (out_g) *out_g = g;
  if (out_b) *out_b = b;
  return code;
}

static void append_row(TreeRowKind kind, int id, int sub_id, int count)
{
  if (g_row_count >= MAX_TREE_ROWS) return;
  g_rows[g_row_count].kind = kind;
  g_rows[g_row_count].id = id;
  g_rows[g_row_count].sub_id = sub_id;
  g_rows[g_row_count].count = count;
  g_row_count++;
}

static void rebuild_visible_rows(void)
{
  g_row_count = 0;

  if (g_tree_tab == 0)
  {
    if (!inpformat || set == NULL || anz->sets <= 0)
    {
      append_row(ROW_INFO, 0, 0, 0);
      return;
    }

    int sec_counts[SEC_COUNT] = { 0, 0, 0, 0 };
    for (int i = 0; i < anz->sets; i++)
    {
      int cat = classify_set_category(i);
      if (cat >= 0 && cat < SEC_COUNT) sec_counts[cat]++;
    }

    int any_sec = 0;
    for (int s = 0; s < SEC_COUNT; s++)
    {
      if (sec_counts[s] == 0) continue;
      any_sec = 1;
      append_row(ROW_SECTION, s, 0, sec_counts[s]);
      if (g_sec_collapsed[s]) continue;

      for (int i = 0; i < anz->sets; i++)
      {
        if (classify_set_category(i) == s)
        {
          append_row(ROW_SET, i, s, get_set_entity_count(i, s));
        }
      }
    }

    if (!any_sec)
    {
      append_row(ROW_INFO, 0, 0, 0);
    }
  }
  else
  {
    if (!inpformat || lcase == NULL || anz->l <= 0)
    {
      append_row(ROW_INFO, 1, 0, 0);
      return;
    }

    if (g_expanded_lc == -2 && cur_lc >= 0 && cur_lc < anz->l)
    {
      g_expanded_lc = cur_lc;
    }

    for (int lc = 0; lc < anz->l; lc++)
    {
      append_row(ROW_DATASET, lc, 0, lcase[lc].ncomps);
      if (lc == g_expanded_lc && lcase[lc].ncomps > 0)
      {
        for (int c = 0; c < lcase[lc].ncomps; c++)
        {
          if (lcase[lc].menu && !lcase[lc].menu[c]) continue;
          append_row(ROW_COMPONENT, lc, c, 0);
        }
      }
    }
  }
}

static int get_current_scroll(void)
{
  return (g_tree_tab == 0) ? g_scroll_model : g_scroll_results;
}

static void set_current_scroll(int val, int list_h)
{
  int row_h = get_row_height();
  int total_h = g_row_count * row_h;
  int max_scroll = total_h - list_h;
  if (max_scroll < 0) max_scroll = 0;
  if (val < 0) val = 0;
  if (val > max_scroll) val = max_scroll;

  if (g_tree_tab == 0) g_scroll_model = val;
  else g_scroll_results = val;
}

/* --------------------------------------------------------------------  */
/* Action Handlers                                                       */
/* --------------------------------------------------------------------  */

static void action_show_all(void)
{
  if (!inpformat) return;
  sync_values_mode_tracker();
  if (g_last_values_mode && anz->l > 0)
  {
    char cmd[64];
    strcpy(cmd, surfFlag ? "fv all" : "ev all");
    plot(cmd);
    ConfigureAndShowWindow_Load();
    cgx_set_gui_status("Showing all (results)");
  }
  else
  {
    selectView(1);
    cgx_set_gui_status("Showing all elements");
  }
}

static void build_set_plot_cmd(int set_idx, int cat, int is_solo, char *cmd, size_t cmd_sz)
{
  char typ[8];
  get_set_plot_type(set_idx, cat, is_solo, typ);
  if (cat == SEC_NODE && strcmp(typ, "n") == 0)
  {
    char col = get_node_set_color_code(set[set_idx].name, NULL, NULL, NULL);
    snprintf(cmd, cmd_sz, "%s %s %c", typ, set[set_idx].name, col);
  }
  else
  {
    snprintf(cmd, cmd_sz, "%s %s", typ, set[set_idx].name);
  }
}

static void action_toggle_set(int set_idx, int cat)
{
  if (!inpformat || set_idx < 0 || set_idx >= anz->sets || set[set_idx].name == NULL) return;
  sync_values_mode_tracker();

  char active_typ0 = 0;
  int p_idx = find_active_pset_index(set_idx, &active_typ0);
  char cmd[MAX_LINE_LENGTH + 16];

  if (p_idx >= 0)
  {
    snprintf(cmd, sizeof(cmd), "%c %s", active_typ0, set[set_idx].name);
    minus(cmd);
    cgx_set_gui_status("Hid set");
  }
  else
  {
    build_set_plot_cmd(set_idx, cat, 0, cmd, sizeof(cmd));
    plus(cmd);
    cgx_set_gui_status("Added set");
  }
}

static void action_solo_set(int set_idx, int cat)
{
  if (!inpformat || set_idx < 0 || set_idx >= anz->sets || set[set_idx].name == NULL) return;
  sync_values_mode_tracker();

  char cmd[MAX_LINE_LENGTH + 16];
  build_set_plot_cmd(set_idx, cat, 1, cmd, sizeof(cmd));
  plot(cmd);

  char status[128];
  snprintf(status, sizeof(status), "Isolated: %s", set[set_idx].name);
  cgx_set_gui_status(status);
}

static void action_select_dataset(int lc)
{
  if (!inpformat || lc < 0 || lc >= anz->l) return;
  if (g_expanded_lc == lc) g_expanded_lc = -1;
  else g_expanded_lc = lc;

  selectDataset(lc);
  if (lcase[lc].ncomps == 1)
  {
    selectEntity(1);
    g_last_values_mode = 1;
  }
}

static void action_select_component(int lc, int comp)
{
  if (!inpformat || lc < 0 || lc >= anz->l) return;
  if (comp < 0 || comp >= lcase[lc].ncomps) return;
  if (pre_lc != lc || !lcase[lc].loaded)
  {
    selectDataset(lc);
  }
  selectEntity(comp + 1);
  g_last_values_mode = 1;
}

/* --------------------------------------------------------------------  */
/* Public API                                                            */
/* --------------------------------------------------------------------  */

void cgx_tree_set_visible(int visible)
{
  g_tree_visible = visible ? 1 : 0;
  g_hovered_row = -1;
  g_hovered_solo = 0;
  g_header_hovered = -1;
  printf("\n Model Tree %s (F4).\n\n", g_tree_visible ? "shown" : "hidden");
  cgx_set_gui_status(g_tree_visible ? "Model Tree ON (F4)" : "Model Tree OFF (F4)");
  glutPostRedisplay();
}

void cgx_tree_toggle(void)
{
  cgx_tree_set_visible(!g_tree_visible);
}

int cgx_tree_is_visible(void)
{
  return g_tree_visible;
}

int cgx_tree_on_mouse_move(int mx, int my, int win_w, int win_h)
{
  int tab_h = hit_test_pull_tab(mx, my, win_w, win_h);
  if (tab_h != g_tab_hovered)
  {
    g_tab_hovered = tab_h;
    glutPostRedisplay();
  }
  if (tab_h)
  {
    if (g_header_hovered != -1 || g_hovered_row != -1)
    {
      g_header_hovered = -1;
      g_hovered_row = -1;
      g_hovered_solo = 0;
      glutPostRedisplay();
    }
    return 1;
  }

  if (!g_tree_visible)
  {
    g_header_hovered = -1;
    g_hovered_row = -1;
    g_hovered_solo = 0;
    return 0;
  }

  if (!is_point_in_drawer(mx, my, win_w, win_h))
  {
    if (g_header_hovered != -1 || g_hovered_row != -1 || g_hovered_solo != 0)
    {
      g_header_hovered = -1;
      g_hovered_row = -1;
      g_hovered_solo = 0;
      glutPostRedisplay();
    }
    return 0;
  }

  int dx1, dy1, dx2, dy2, ly1, ly2;
  get_drawer_rect(win_w, win_h, &dx1, &dy1, &dx2, &dy2, &ly1, &ly2);

  int hx1[3], hx2[3], hy1, hy2;
  get_header_button_rects(dx1, dy1, dx2, ly1, hx1, hx2, &hy1, &hy2);

  int new_hdr = -1;
  if (my >= hy1 && my <= hy2)
  {
    for (int i = 0; i < 3; i++)
    {
      if (hx1[i] >= 0 && mx >= hx1[i] && mx <= hx2[i])
      {
        new_hdr = i;
        break;
      }
    }
  }

  int new_row = -1;
  int new_solo = 0;
  if (my >= ly1 && my <= ly2)
  {
    rebuild_visible_rows();
    int row_h = get_row_height();
    int scroll = get_current_scroll();
    int idx = (my - ly1 + scroll) / row_h;
    if (idx >= 0 && idx < g_row_count)
    {
      new_row = idx;
      if (g_rows[idx].kind == ROW_SET)
      {
        int solo_w = row_h - 4;
        int solo_x2 = dx2 - 12;
        int solo_x1 = solo_x2 - solo_w;
        if (mx >= solo_x1 && mx <= solo_x2) new_solo = 1;
      }
    }
  }

  if (new_hdr != g_header_hovered || new_row != g_hovered_row || new_solo != g_hovered_solo)
  {
    g_header_hovered = new_hdr;
    g_hovered_row = new_row;
    g_hovered_solo = new_solo;
    glutPostRedisplay();
  }
  return 1;
}

int cgx_tree_on_mouse_button(int button, int action, int mods, int mx, int my, int win_w, int win_h)
{
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && g_tree_click_captured)
  {
    g_tree_click_captured = 0;
    return 1;
  }

  if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS) return 0;

  if (hit_test_pull_tab(mx, my, win_w, win_h))
  {
    g_tree_click_captured = 1;
    cgx_tree_toggle();
    return 1;
  }

  if (!is_point_in_drawer(mx, my, win_w, win_h)) return 0;

  g_tree_click_captured = 1;
  int dx1, dy1, dx2, dy2, ly1, ly2;
  get_drawer_rect(win_w, win_h, &dx1, &dy1, &dx2, &dy2, &ly1, &ly2);

  int hx1[3], hx2[3], hy1, hy2;
  get_header_button_rects(dx1, dy1, dx2, ly1, hx1, hx2, &hy1, &hy2);

  if (my >= hy1 && my <= hy2)
  {
    if (mx >= hx1[0] && mx <= hx2[0])
    {
      g_tree_tab = 0;
      g_hovered_row = -1;
      glutPostRedisplay();
      return 1;
    }
    if (mx >= hx1[1] && mx <= hx2[1])
    {
      g_tree_tab = 1;
      g_hovered_row = -1;
      glutPostRedisplay();
      return 1;
    }
    if (hx1[2] >= 0 && mx >= hx1[2] && mx <= hx2[2])
    {
      action_show_all();
      glutPostRedisplay();
      return 1;
    }
    return 1;
  }

  if (my >= ly1 && my <= ly2)
  {
    rebuild_visible_rows();
    int row_h = get_row_height();
    int scroll = get_current_scroll();
    int idx = (my - ly1 + scroll) / row_h;
    if (idx >= 0 && idx < g_row_count)
    {
      TreeRow row = g_rows[idx];
      if (row.kind == ROW_SECTION && row.id >= 0 && row.id < SEC_COUNT)
      {
        g_sec_collapsed[row.id] = !g_sec_collapsed[row.id];
        set_current_scroll(get_current_scroll(), ly2 - ly1);
      }
      else if (row.kind == ROW_SET)
      {
        int solo_w = row_h - 4;
        int solo_x2 = dx2 - 12;
        int solo_x1 = solo_x2 - solo_w;
        int is_solo = ((mods & GLFW_MOD_SHIFT) || (mx >= solo_x1 && mx <= solo_x2)) ? 1 : 0;
        if (is_solo) action_solo_set(row.id, row.sub_id);
        else action_toggle_set(row.id, row.sub_id);
      }
      else if (row.kind == ROW_DATASET)
      {
        action_select_dataset(row.id);
      }
      else if (row.kind == ROW_COMPONENT)
      {
        action_select_component(row.id, row.sub_id);
      }
      glutPostRedisplay();
    }
  }
  return 1;
}

int cgx_tree_on_scroll(double yoffset, int mx, int my, int win_w, int win_h)
{
  if (!is_point_in_drawer(mx, my, win_w, win_h)) return 0;

  int dx1, dy1, dx2, dy2, ly1, ly2;
  get_drawer_rect(win_w, win_h, &dx1, &dy1, &dx2, &dy2, &ly1, &ly2);
  rebuild_visible_rows();

  int row_h = get_row_height();
  int step = row_h * 2;
  int delta = (yoffset > 0.0) ? -step : step;
  set_current_scroll(get_current_scroll() + delta, ly2 - ly1);
  glutPostRedisplay();
  return 1;
}

/* --------------------------------------------------------------------  */
/* 2D Overlay Rendering                                                  */
/* --------------------------------------------------------------------  */

static void render_pull_tab(int win_w, int win_h)
{
  if (win_w < 200 || win_h < 120) return;

  int x1, y1, x2, y2;
  get_pull_tab_rect(win_w, win_h, &x1, &y1, &x2, &y2);
  int tier = cgx_ui_font_tier();
  int pad_x = (tier == 0) ? 9 : (tier == 1 ? 11 : 15);
  int gap_chev = (tier == 0) ? 6 : (tier == 1 ? 7 : 10);
  int chev_w = (tier == 0) ? 6 : (tier == 1 ? 8 : 11);
  int tab_h = y2 - y1;
  int dark = (backgrndcol == 0);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  if (g_tab_hovered)
  {
    glColor4f(0.18f, 0.48f, 0.82f, 0.96f);
  }
  else if (g_tree_visible)
  {
    if (dark) glColor4f(0.05f, 0.08f, 0.12f, 0.90f);
    else      glColor4f(0.94f, 0.96f, 0.98f, 0.94f);
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

  if (g_tab_hovered)
  {
    glColor4f(0.35f, 0.78f, 1.0f, 0.95f);
  }
  else if (g_tree_visible)
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
  if (g_tab_hovered)
  {
    tr = 1.0f; tg = 1.0f; tb = 1.0f;
  }
  else if (g_tree_visible)
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

  float tw = cgx_ui_text_width("Tree");
  int baseline_y = y1 + (int)(tab_h * 0.68f);
  cgx_ui_draw_text((float)x1 + (float)pad_x, (float)baseline_y, "Tree", tr, tg, tb, win_h);

  glColor4f(tr, tg, tb, ta);
  glLineWidth(1.5f);
  float chev_cx = (float)x1 + (float)pad_x + tw + (float)gap_chev + (float)chev_w * 0.5f;
  float chev_cy_gl = (float)win_h - ((float)y1 + (float)tab_h * 0.50f);
  draw_chevron_lr(chev_cx, chev_cy_gl, (float)chev_w * 0.40f, (float)tab_h * 0.16f, !g_tree_visible);
}

void cgx_tree_render(int win_w, int win_h, int fb_w, int fb_h)
{
  render_pull_tab(win_w, win_h);

  if (!g_tree_visible || win_w < 240 || win_h < 180) return;

  int dx1, dy1, dx2, dy2, ly1, ly2;
  get_drawer_rect(win_w, win_h, &dx1, &dy1, &dx2, &dy2, &ly1, &ly2);
  int dark = (backgrndcol == 0);

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  /* Drawer backdrop */
  if (dark) glColor4f(0.05f, 0.08f, 0.12f, 0.90f);
  else      glColor4f(0.95f, 0.97f, 0.99f, 0.94f);
  glRectf((float)dx1, (float)(win_h - dy2), (float)dx2, (float)(win_h - dy1));

  /* Drawer border */
  if (dark) glColor4f(0.20f, 0.30f, 0.42f, 0.95f);
  else      glColor4f(0.68f, 0.74f, 0.82f, 0.95f);
  glLineWidth(1.1f);
  glBegin(GL_LINE_LOOP);
    glVertex2f((float)dx1, (float)(win_h - dy1));
    glVertex2f((float)dx2, (float)(win_h - dy1));
    glVertex2f((float)dx2, (float)(win_h - dy2));
    glVertex2f((float)dx1, (float)(win_h - dy2));
  glEnd();

  /* Header tabs (Model | Results | All) */
  int hx1[3], hx2[3], hy1, hy2;
  get_header_button_rects(dx1, dy1, dx2, ly1, hx1, hx2, &hy1, &hy2);
  int hbtn_h = hy2 - hy1;
  int hbase = hy1 + (int)(hbtn_h * 0.70f);
  static const char *hdr_labels[3] = { "Model", "Results", "All" };

  for (int i = 0; i < 3; i++)
  {
    if (hx1[i] < 0) continue;
    int active = (i < 2 && g_tree_tab == i);
    int hovered = (g_header_hovered == i);

    if (hovered)
      glColor4f(0.18f, 0.48f, 0.82f, 0.96f);
    else if (active)
      glColor4f(dark ? 0.12f : 0.16f, dark ? 0.34f : 0.46f, dark ? 0.56f : 0.78f, 0.92f);
    else
      glColor4f(dark ? 0.10f : 0.86f, dark ? 0.14f : 0.89f, dark ? 0.20f : 0.94f, 0.90f);
    glRectf((float)hx1[i], (float)(win_h - hy2), (float)hx2[i], (float)(win_h - hy1));

    if (active)
    {
      glColor4f(0.25f, 0.80f, 1.0f, 1.0f);
      glRectf((float)hx1[i], (float)(win_h - hy2), (float)hx2[i], (float)(win_h - (hy2 - 2)));
    }

    float tr = 1.0f, tg = 1.0f, tb = 1.0f;
    if (!hovered && !active)
    {
      if (dark) { tr = 0.88f; tg = 0.92f; tb = 0.97f; }
      else      { tr = 0.12f; tg = 0.16f; tb = 0.22f; }
    }
    float tw = cgx_ui_text_width(hdr_labels[i]);
    float tx = (float)hx1[i] + ((float)(hx2[i] - hx1[i]) - tw) * 0.5f;
    cgx_ui_draw_text(tx, (float)hbase, hdr_labels[i], tr, tg, tb, win_h);
  }

  /* Separator line below header */
  if (dark) glColor4f(0.18f, 0.28f, 0.38f, 0.95f);
  else      glColor4f(0.72f, 0.78f, 0.85f, 0.95f);
  glBegin(GL_LINES);
    glVertex2f((float)dx1, (float)(win_h - ly1));
    glVertex2f((float)dx2, (float)(win_h - ly1));
  glEnd();

  /* Rebuild flat row list & clamp scroll */
  rebuild_visible_rows();
  int list_h = ly2 - ly1;
  if (list_h <= 4) return;
  set_current_scroll(get_current_scroll(), list_h);
  int scroll = get_current_scroll();
  int row_h = get_row_height();

  /* Scissor-clip virtualized rows inside the scrollable area */
  double scale_x = (win_w > 0) ? (double)fb_w / (double)win_w : 1.0;
  double scale_y = (win_h > 0) ? (double)fb_h / (double)win_h : 1.0;
  int sc_x = (int)floor((double)(dx1 + 1) * scale_x);
  int sc_y = (int)floor((double)(win_h - ly2) * scale_y);
  int sc_w = (int)ceil((double)(dx2 - dx1 - 2) * scale_x);
  int sc_h = (int)ceil((double)list_h * scale_y);
  if (sc_w <= 0 || sc_h <= 0) return;

  glEnable(GL_SCISSOR_TEST);
  glScissor(sc_x, sc_y, sc_w, sc_h);

  int first_idx = scroll / row_h;
  int last_idx = (scroll + list_h) / row_h + 1;
  if (first_idx < 0) first_idx = 0;
  if (last_idx > g_row_count) last_idx = g_row_count;

  static const char *sec_titles[SEC_COUNT] = {
    "Materials", "Element Sets", "Node Sets", "Geometry"
  };

  for (int i = first_idx; i < last_idx; i++)
  {
    int ry1 = ly1 + i * row_h - scroll;
    int ry2 = ry1 + row_h;
    int baseline = ry1 + (int)(row_h * 0.70f);
    TreeRow row = g_rows[i];
    int hov = (g_hovered_row == i);

    if (row.kind == ROW_SECTION)
    {
      if (hov) glColor4f(0.16f, 0.42f, 0.72f, 0.75f);
      else if (dark) glColor4f(0.09f, 0.13f, 0.19f, 0.85f);
      else           glColor4f(0.87f, 0.90f, 0.94f, 0.90f);
      glRectf((float)(dx1 + 2), (float)(win_h - (ry2 - 1)), (float)(dx2 - 10), (float)(win_h - (ry1 + 1)));

      float tr = dark ? 0.70f : 0.18f;
      float tg = dark ? 0.86f : 0.34f;
      float tb = dark ? 1.00f : 0.56f;
      if (hov) { tr = 1.0f; tg = 1.0f; tb = 1.0f; }

      glColor4f(tr, tg, tb, 0.95f);
      glLineWidth(1.4f);
      float chev_x = (float)dx1 + 12.0f;
      float chev_y_gl = (float)win_h - ((float)ry1 + (float)row_h * 0.5f);
      if (g_sec_collapsed[row.id])
        draw_chevron_lr(chev_x, chev_y_gl, 3.0f, 4.0f, 1);
      else
        draw_chevron_down(chev_x, chev_y_gl, 4.0f, 2.5f);

      char sec_label[64];
      snprintf(sec_label, sizeof(sec_label), "%s (%d)", sec_titles[row.id], row.count);
      draw_clipped_label((float)(dx1 + 22), (float)baseline, sec_label,
                         (float)(dx2 - dx1 - 36), tr, tg, tb, win_h);
    }
    else if (row.kind == ROW_SET)
    {
      int active = (find_active_pset_index(row.id, NULL) >= 0) ? 1 : 0;
      if (hov && !g_hovered_solo)
      {
        glColor4f(0.15f, 0.38f, 0.68f, 0.72f);
        glRectf((float)(dx1 + 4), (float)(win_h - ry2), (float)(dx2 - 10), (float)(win_h - ry1));
      }
      else if (active)
      {
        if (dark) glColor4f(0.10f, 0.24f, 0.38f, 0.45f);
        else      glColor4f(0.80f, 0.88f, 0.97f, 0.75f);
        glRectf((float)(dx1 + 4), (float)(win_h - ry2), (float)(dx2 - 10), (float)(win_h - ry1));
      }

      /* Checkbox */
      int box_sz = (row_h <= 22) ? 11 : ((row_h <= 26) ? 13 : 17);
      int bx1 = dx1 + 12;
      int bx2 = bx1 + box_sz;
      int by1 = ry1 + (row_h - box_sz) / 2;
      int by2 = by1 + box_sz;

      if (active)
      {
        glColor4f(0.20f, 0.72f, 0.96f, 0.95f);
        glRectf((float)bx1, (float)(win_h - by2), (float)bx2, (float)(win_h - by1));
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glLineWidth(1.6f);
        glBegin(GL_LINE_STRIP);
          glVertex2f((float)bx1 + box_sz * 0.20f, (float)win_h - ((float)by1 + box_sz * 0.52f));
          glVertex2f((float)bx1 + box_sz * 0.44f, (float)win_h - ((float)by1 + box_sz * 0.76f));
          glVertex2f((float)bx1 + box_sz * 0.82f, (float)win_h - ((float)by1 + box_sz * 0.26f));
        glEnd();
      }
      else
      {
        if (dark) glColor4f(0.40f, 0.50f, 0.62f, 0.90f);
        else      glColor4f(0.45f, 0.52f, 0.60f, 0.90f);
        glLineWidth(1.1f);
        glBegin(GL_LINE_LOOP);
          glVertex2f((float)bx1, (float)(win_h - by1));
          glVertex2f((float)bx2, (float)(win_h - by1));
          glVertex2f((float)bx2, (float)(win_h - by2));
          glVertex2f((float)bx1, (float)(win_h - by2));
        glEnd();
      }

      /* Solo 'S' pill */
      int solo_w = row_h - 4;
      int solo_x2 = dx2 - 12;
      int solo_x1 = solo_x2 - solo_w;
      int solo_y1 = ry1 + 2;
      int solo_y2 = ry2 - 2;
      int solo_hov = (hov && g_hovered_solo);
      if (solo_hov)
        glColor4f(0.22f, 0.58f, 0.94f, 0.96f);
      else if (dark)
        glColor4f(0.14f, 0.20f, 0.28f, 0.85f);
      else
        glColor4f(0.84f, 0.88f, 0.93f, 0.95f);
      glRectf((float)solo_x1, (float)(win_h - solo_y2), (float)solo_x2, (float)(win_h - solo_y1));

      float sw = cgx_ui_text_width("S");
      float sx = (float)solo_x1 + ((float)solo_w - sw) * 0.5f;
      cgx_ui_draw_text(sx, (float)baseline, "S",
                       solo_hov ? 1.0f : (dark ? 0.78f : 0.20f),
                       solo_hov ? 1.0f : (dark ? 0.86f : 0.24f),
                       solo_hov ? 1.0f : (dark ? 0.95f : 0.30f), win_h);

      /* Optional color swatch for Node Sets */
      int label_x = bx2 + 8;
      if (row.sub_id == SEC_NODE)
      {
        float sr = 0.0f, sg = 0.0f, sb = 0.0f;
        get_node_set_color_code(set[row.id].name, &sr, &sg, &sb);
        int sw_sz = 6;
        int sw_x1 = bx2 + 6;
        int sw_x2 = sw_x1 + sw_sz;
        int sw_y1 = ry1 + (row_h - sw_sz) / 2;
        int sw_y2 = sw_y1 + sw_sz;

        glColor4f(sr, sg, sb, 0.96f);
        glRectf((float)sw_x1, (float)(win_h - sw_y2), (float)sw_x2, (float)(win_h - sw_y1));
        if (!dark)
        {
          glColor4f(0.20f, 0.25f, 0.32f, 0.55f);
          glLineWidth(1.0f);
          glBegin(GL_LINE_LOOP);
            glVertex2f((float)sw_x1, (float)(win_h - sw_y1));
            glVertex2f((float)sw_x2, (float)(win_h - sw_y1));
            glVertex2f((float)sw_x2, (float)(win_h - sw_y2));
            glVertex2f((float)sw_x1, (float)(win_h - sw_y2));
          glEnd();
        }
        label_x = sw_x2 + 6;
      }

      /* Set name + count */
      char label[MAX_LINE_LENGTH + 32];
      snprintf(label, sizeof(label), "%s (%d)", set[row.id].name, row.count);
      float tr = dark ? 0.90f : 0.12f;
      float tg = dark ? 0.94f : 0.15f;
      float tb = dark ? 0.98f : 0.20f;
      if (hov && !g_hovered_solo) { tr = 1.0f; tg = 1.0f; tb = 1.0f; }
      float max_w = (float)(solo_x1 - label_x - 4);
      draw_clipped_label((float)label_x, (float)baseline, label, max_w, tr, tg, tb, win_h);
    }
    else if (row.kind == ROW_DATASET)
    {
      int lc = row.id;
      int is_cur = (lc == cur_lc);
      int is_exp = (lc == g_expanded_lc);

      if (hov)
        glColor4f(0.16f, 0.42f, 0.72f, 0.80f);
      else if (is_cur)
        glColor4f(dark ? 0.12f : 0.78f, dark ? 0.30f : 0.88f, dark ? 0.48f : 0.98f, 0.80f);
      else if (dark)
        glColor4f(0.08f, 0.12f, 0.18f, 0.75f);
      else
        glColor4f(0.90f, 0.92f, 0.95f, 0.85f);
      glRectf((float)(dx1 + 2), (float)(win_h - (ry2 - 1)), (float)(dx2 - 10), (float)(win_h - (ry1 + 1)));

      if (is_cur)
      {
        glColor4f(0.25f, 0.80f, 1.0f, 1.0f);
        glRectf((float)(dx1 + 2), (float)(win_h - (ry2 - 1)), (float)(dx1 + 5), (float)(win_h - (ry1 + 1)));
      }

      float tr = is_cur ? (dark ? 1.0f : 0.06f) : (dark ? 0.88f : 0.14f);
      float tg = is_cur ? (dark ? 1.0f : 0.18f) : (dark ? 0.92f : 0.18f);
      float tb = is_cur ? (dark ? 1.0f : 0.42f) : (dark ? 0.97f : 0.24f);
      if (hov) { tr = 1.0f; tg = 1.0f; tb = 1.0f; }

      glColor4f(tr, tg, tb, 0.95f);
      glLineWidth(1.4f);
      float chev_x = (float)dx1 + 12.0f;
      float chev_y_gl = (float)win_h - ((float)ry1 + (float)row_h * 0.5f);
      if (is_exp) draw_chevron_down(chev_x, chev_y_gl, 4.0f, 2.5f);
      else        draw_chevron_lr(chev_x, chev_y_gl, 3.0f, 4.0f, 1);

      char ds_name[128], label[180];
      trim_trailing_spaces(lcase[lc].name, ds_name, sizeof(ds_name));
      if (fabs(lcase[lc].value) > 1e-12)
        snprintf(label, sizeof(label), "%d %s (%.4g)", lc + 1, ds_name, lcase[lc].value);
      else
        snprintf(label, sizeof(label), "%d %s", lc + 1, ds_name);
      draw_clipped_label((float)(dx1 + 22), (float)baseline, label,
                         (float)(dx2 - dx1 - 36), tr, tg, tb, win_h);
    }
    else if (row.kind == ROW_COMPONENT)
    {
      int lc = row.id;
      int comp = row.sub_id;
      int is_active = (drawMode == 1 && lc == cur_lc && comp == cur_entity);

      if (hov)
      {
        glColor4f(0.15f, 0.40f, 0.72f, 0.78f);
        glRectf((float)(dx1 + 6), (float)(win_h - ry2), (float)(dx2 - 10), (float)(win_h - ry1));
      }
      else if (is_active)
      {
        if (dark) glColor4f(0.12f, 0.34f, 0.56f, 0.85f);
        else      glColor4f(0.18f, 0.48f, 0.80f, 0.88f);
        glRectf((float)(dx1 + 6), (float)(win_h - ry2), (float)(dx2 - 10), (float)(win_h - ry1));
      }

      float tr = (hov || is_active) ? 1.0f : (dark ? 0.84f : 0.18f);
      float tg = (hov || is_active) ? 1.0f : (dark ? 0.89f : 0.22f);
      float tb = (hov || is_active) ? 1.0f : (dark ? 0.95f : 0.28f);

      char comp_name[128] = "";
      if (lcase[lc].compName && lcase[lc].compName[comp])
        trim_trailing_spaces(lcase[lc].compName[comp], comp_name, sizeof(comp_name));
      else
        snprintf(comp_name, sizeof(comp_name), "Comp %d", comp + 1);

      draw_clipped_label((float)(dx1 + 28), (float)baseline, comp_name,
                         (float)(dx2 - dx1 - 42), tr, tg, tb, win_h);
    }
    else if (row.kind == ROW_INFO)
    {
      const char *msg = (row.id == 0) ? "No sets in model" : "No result datasets loaded";
      cgx_ui_draw_text((float)(dx1 + 14), (float)baseline, msg,
                       dark ? 0.55f : 0.45f,
                       dark ? 0.62f : 0.50f,
                       dark ? 0.70f : 0.58f, win_h);
    }
  }

  glDisable(GL_SCISSOR_TEST);

  /* Slim scrollbar indicator when content overflows */
  int total_h = g_row_count * row_h;
  if (total_h > list_h)
  {
    int track_x1 = dx2 - 6;
    int track_x2 = dx2 - 3;
    float frac_vis = (float)list_h / (float)total_h;
    float frac_top = (float)scroll / (float)total_h;
    int thumb_h = (int)(frac_vis * (float)list_h);
    if (thumb_h < 18) thumb_h = 18;
    int thumb_y1 = ly1 + (int)(frac_top * (float)list_h);
    if (thumb_y1 + thumb_h > ly2) thumb_y1 = ly2 - thumb_h;

    if (dark) glColor4f(0.32f, 0.44f, 0.58f, 0.75f);
    else      glColor4f(0.50f, 0.58f, 0.68f, 0.75f);
    glRectf((float)track_x1, (float)(win_h - (thumb_y1 + thumb_h)),
            (float)track_x2, (float)(win_h - thumb_y1));
  }
}
