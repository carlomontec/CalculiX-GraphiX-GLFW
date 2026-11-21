#ifndef CGX_TREE_H
#define CGX_TREE_H

/* --------------------------------------------------------------------  */
/* CalculiX GraphiX (GLFW Edition)                                       */
/* File: cgx_tree.h - Lightweight Collapsible Model & Results Tree       */
/*                                                                       */
/* Created by Carlo Monjaraz-Tec (2026) with AI pair-programming         */
/* assistance based on the original work of Klaus Wittig & contributors. */
/*                                                                       */
/* Licensed under GNU General Public License v2 (GPL-2.0 or later).      */
/* --------------------------------------------------------------------  */

#ifdef __cplusplus
extern "C" {
#endif

void cgx_tree_set_visible(int visible);
void cgx_tree_toggle(void);
int  cgx_tree_is_visible(void);
void cgx_tree_render(int win_w, int win_h, int fb_w, int fb_h);
int  cgx_tree_on_mouse_move(int mx, int my, int win_w, int win_h);
int  cgx_tree_on_mouse_button(int button, int action, int mods, int mx, int my, int win_w, int win_h);
int  cgx_tree_on_scroll(double yoffset, int mx, int my, int win_w, int win_h);

#ifdef __cplusplus
}
#endif

#endif /* CGX_TREE_H */
