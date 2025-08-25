/* Copyright (C)
* 2020 - John Melton, G0ORX/N6LYT
* 2025 - Christoph van Wüllen, DL1YCF
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*
*/

#include <gtk/gtk.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "actions.h"
#include "appearance.h"
#include "client_server.h"
#include "ext.h"
#include "main.h"
#include "message.h"
#include "receiver.h"
#include "radio.h"
#include "sliders.h"
#include "vfo.h"
#include "zoompan.h"

static int width;
static int height;

static GtkWidget *zoompan;
static GtkWidget *zoom_label;
static GtkWidget *zoom_scale;
static gulong zoom_signal_id;
static GtkWidget *pan_label;
static GtkWidget *pan_scale;
static gulong pan_signal_id;

int sliders_zoom(gpointer data) {
  int id = GPOINTER_TO_INT(data);
  // This ONLY moves the zoom slider
  if (id != active_receiver->id || !display_zoompan) {
    return G_SOURCE_REMOVE;
  }

  g_signal_handler_block(G_OBJECT(zoom_scale), zoom_signal_id);
  gtk_range_set_value(GTK_RANGE(zoom_scale), active_receiver->zoom);
  g_signal_handler_unblock(G_OBJECT(zoom_scale), zoom_signal_id);
  return G_SOURCE_REMOVE;
}

int sliders_pan(gpointer data) {
  // This ONLY moves the pan sliders
  int id = GPOINTER_TO_INT(data);
  if (id != active_receiver->id || !display_zoompan) {
    return G_SOURCE_REMOVE;
  }

  g_signal_handler_block(G_OBJECT(pan_scale), pan_signal_id);
  gtk_range_set_value (GTK_RANGE(pan_scale), (double) active_receiver->pan);
  g_signal_handler_unblock(G_OBJECT(pan_scale), pan_signal_id);
  return G_SOURCE_REMOVE;
}

int zoompan_active_receiver_changed(void *data) {
  gpointer id = GINT_TO_POINTER(active_receiver->id);
  // no need to call these through the event queue
  sliders_zoom(id);
  sliders_pan(id);
  return G_SOURCE_REMOVE;
}

static void zoom_value_changed_cb(GtkWidget *widget, gpointer data) {
  active_receiver->zoom = (int)(gtk_range_get_value(GTK_RANGE(zoom_scale)) + 0.5);
  rx_update_zoom(active_receiver);
}

void set_zoom(int rx, double value) {
  if (rx >= receivers) { return; }

  int ival = (int) value;

  if (ival > MAX_ZOOM) { ival = MAX_ZOOM; }

  if (ival < 1       ) { ival = 1; }

  receiver[rx]->zoom = ival;
  rx_update_zoom(receiver[rx]);
}

static void pan_value_changed_cb(GtkWidget *widget, gpointer data) {
  active_receiver->pan = (int)(gtk_range_get_value(GTK_RANGE(pan_scale)) + 0.5);
  rx_update_pan(active_receiver);
}

void set_pan(int rx, double value) {
  if (rx >= receivers) { return; }

  int ival = (int) value;

  if (ival < 0) { ival = 0; }
  if (ival > 100) { ival = 100; }

  receiver[rx]->pan = ival;

  if (display_zoompan && rx == active_receiver->id) {
    gtk_range_set_value (GTK_RANGE(pan_scale), (double) receiver[rx]->pan);
  }
  rx_update_pan(receiver[rx]);
}

GtkWidget *zoompan_init(int my_width, int my_height) {
  width = my_width;
  height = my_height;
  //
  // the horizontal layout changes a little if the total width changes
  //
  int twidth, tpix, s1width, s2width;
  int t1pos, t2pos;
  int s1pos, s2pos;
  const char *csslabel;

  if (width < 1024) {
    tpix   =  width / 9;      // width of text label in pixels
    twidth =  1;              // width of text label in grid units
    s1width = 2;              // width of zoom slider in grid units
    s2width = 5;              // width of pan slider in grid units
  } else if (width < 1280) {
    tpix   =  width / 12;     // width of text label in pixels
    twidth =  1;              // width of text label in grid units
    s1width = 3;              // width of zoom slider in grid units
    s2width = 7;              // width of pan slider in grid units
  } else {
    tpix   =  width / 15;     // width of text label in pixels
    twidth =  1;              // width of text label in grid units
    s1width = 4;              // width of zoom slider in grid units
    s2width = 9;              // width of pan slider in grid units
  }

  t1pos = 0;
  s1pos = t1pos + twidth;
  t2pos = s1pos + s1width;
  s2pos = t2pos + twidth;

  if (tpix < 75 ) {
    csslabel = "slider1";
  } else if (tpix < 85) {
    csslabel = "slider2";
  } else {
    csslabel = "slider3";
  }

  zoompan = gtk_grid_new();
  gtk_widget_set_size_request (zoompan, width, height);
  gtk_grid_set_row_homogeneous(GTK_GRID(zoompan), FALSE);
  gtk_grid_set_column_homogeneous(GTK_GRID(zoompan), TRUE);
  zoom_label = gtk_label_new("Zoom");
  gtk_widget_set_name(zoom_label, csslabel);
  gtk_widget_set_halign(zoom_label, GTK_ALIGN_END);
  gtk_widget_show(zoom_label);
  gtk_grid_attach(GTK_GRID(zoompan), zoom_label, t1pos, 0, twidth, 1);
  zoom_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1.0, MAX_ZOOM, 1.00);
  gtk_widget_set_size_request(zoom_scale, 0, height);
  gtk_widget_set_valign(zoom_scale, GTK_ALIGN_CENTER);
  gtk_range_set_increments (GTK_RANGE(zoom_scale), 1.0, 1.0);
  gtk_range_set_value (GTK_RANGE(zoom_scale), active_receiver->zoom);
  gtk_widget_show(zoom_scale);
  gtk_grid_attach(GTK_GRID(zoompan), zoom_scale, s1pos, 0, s1width, 1);
  zoom_signal_id = g_signal_connect(G_OBJECT(zoom_scale), "value_changed", G_CALLBACK(zoom_value_changed_cb), NULL);
  pan_label = gtk_label_new("Pan");
  gtk_widget_set_name(pan_label, csslabel);
  gtk_widget_set_halign(pan_label, GTK_ALIGN_END);
  gtk_widget_show(pan_label);
  gtk_grid_attach(GTK_GRID(zoompan), pan_label, t2pos, 0, twidth, 1);
  pan_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
  gtk_widget_set_size_request(pan_scale, 0, height);
  gtk_widget_set_valign(pan_scale, GTK_ALIGN_CENTER);
  gtk_scale_set_draw_value (GTK_SCALE(pan_scale), FALSE);
  gtk_range_set_increments (GTK_RANGE(pan_scale), 1.0, 1.0);
  gtk_range_set_value (GTK_RANGE(pan_scale), (double) active_receiver->pan);
  gtk_widget_show(pan_scale);
  gtk_grid_attach(GTK_GRID(zoompan), pan_scale, s2pos, 0, s2width, 1);
  pan_signal_id = g_signal_connect(G_OBJECT(pan_scale), "value_changed", G_CALLBACK(pan_value_changed_cb), NULL);

  return zoompan;
}
