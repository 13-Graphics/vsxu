/**
* Project: VSXu: Realtime visual programming language, music/audio visualizer, animation tool and much much more.
*
* @author Jonatan Wallmander, Robert Wenzel, Vovoid Media Technologies Copyright (C) 2003-2011
* @see The GNU Public License (GPL)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef VSX_NO_CLIENT
#include "vsx_gl_global.h"
#include <map>
#include <list>
#include <vector>
#include <math.h>
#include "vsx_math_3d.h"
#include "vsx_texture_info.h"
#include "vsx_texture.h"
#include "vsx_command.h"
#include "vsx_font.h"
#include "vsxfst.h"
// local includes
#include "lib/vsx_widget_lib.h"
#include "vsx_widget_anchor.h"
#include "vsx_widget_desktop.h"
#include "vsx_widget_comp.h"
#include <vsx_bezier_calc.h>

#include "vsx_widget_connector_bezier.h"

bool vsx_widget_connector_bezier::receiving_focus;
float vsx_widget_connector_bezier::dim_alpha = 1.0f;

void vsx_widget_connector_bezier::vsx_command_process_b(vsx_command_s *t) {
  if (t->cmd == "disconnect") {
    // syntax:
    //   param_unalias [-1/1] [component] [param_name]
    if (alias_conn) 
    {
      vsx_string bb;
      if (((vsx_widget_anchor*)parent)->io == -1) bb = "-1"; else bb = "1";
      if (((vsx_widget_anchor*)parent)->io == -1)
      command_q_b.add_raw("param_unalias "+bb+" "+((vsx_widget_component*)((vsx_widget_anchor*)destination)->component)->name+" "+destination->name);
      else
      command_q_b.add_raw("param_unalias "+bb+" "+((vsx_widget_component*)((vsx_widget_anchor*)parent)->component)->name+" "+parent->name);
    } else
    {
      command_q_b.add_raw(
        "param_disconnect "+
        ((vsx_widget_anchor*)parent)->component->name+" "+
        parent->name+" "+
        ((vsx_widget_anchor*)destination)->component->name+" "+
        ((vsx_widget_anchor*)destination)->name
      );
    }
    parent->vsx_command_queue_b(this);
  }
}

int vsx_widget_connector_bezier::inside_xy_l(vsx_vector &test, vsx_vector &global)
{
  vsx_vector world = test-global;
  if (!visible) return false;
  if (((vsx_widget_component*)((vsx_widget_anchor*)parent)->component)->ethereal_all) return false;
  if (!receiving_focus) return false;
  if (destination)
  if (!((vsx_widget_anchor*)parent)->component->visible || !((vsx_widget_anchor*)destination)->component->visible) return false;
  if (open && order != -1) {
    float s = parent->size.x * 0.2f;
    pv.y = world.y - (parent->size.y/2 - ((float)order)*s*2.2);
    pv.x = world.x + parent->size.y*0.7;
    if ( fabs(pv.x) < s && fabs(pv.y) < s)
    {
      return coord_type;
    }
  }
  #define sensitivity 0.005f
  #define VSX_DEBUG 1
  if (alt || !ctrl) return 0;
  if (alt && ctrl) return 0;
  // NOTE! sx/sy is lower right!
  if (test.x > sx || test.x < ex)
  {
    #if VSX_DEBUG
      printf("outside sx/ex\n");
    #endif
    return 0;
  }
  float tsy = sy;
  float tey = ey;
  if (ey < sy)
  {
    tey = sy;
    tsy = ey;
  }
    
  
  if (test.y < tsy-sensitivity || test.y > tey+sensitivity) 
  {
    #if VSX_DEBUG
      printf("outside sy/ey\n");
    #endif
    return 0;
  }
  // calculate t position from y

  vsx_bezier_calc bez_calc;
  bez_calc.x0 = sx;
  bez_calc.y0 = sy;

  bez_calc.x1 = sx+(ex-sx)*0.5f;
  bez_calc.y1 = sy;

  bez_calc.x2 = ex-(ex-sx)*0.5f;
  bez_calc.y2 = ey;

  bez_calc.x3 = ex;
  bez_calc.y3 = ey;
  bez_calc.init();
  float t = bez_calc.t_from_x(test.x);
  printf("global: %f,%f    t: %f\n", test.x, test.y, t); 
  float y = bez_calc.y_from_t(t);
  if ( fabs(test.y - y) < 0.005f ) 
  {
    #if VSX_DEBUG
    printf("hit on bezier!\n");
    #endif
    return coord_type;
  }
  return 0;
}
void vsx_widget_connector_bezier::event_mouse_down(vsx_widget_distance distance,vsx_widget_coords coords,int button) {
  if (button == 0)
  {
    mouse_down_l = 1;
    m_focus = this;
    a_focus = this;
  }
}

void vsx_widget_connector_bezier::event_mouse_move(vsx_widget_distance distance,vsx_widget_coords coords) {
  move_d(vsx_vector(0.0f,(distance.center.y-mouse_down_pos.center.y)));
}

void vsx_widget_connector_bezier::event_mouse_up(vsx_widget_distance distance,vsx_widget_coords coords,int button)
{
  if (open) {
    command_q_b.add_raw("connections_order_int 1");
    parent->vsx_command_queue_b(this);
    set_pos(vsx_vector(0));
  }
  vsx_widget::event_mouse_up(distance,coords,button);
}

void vsx_widget_connector_bezier::draw() 
{
  float alpha = 1.0f;
  if (marked_for_deletion) return;

  if ( ((vsx_widget_component*) ((vsx_widget_anchor*)parent)->component)->deleting )
  {
    return;
  }
  if ( destination && ( (vsx_widget_component*) ((vsx_widget_anchor*)destination)->component)->deleting)
  {
    return;
  }


  if (destination) {
    if (!((vsx_widget_anchor*)destination)->component->visible) return;
      if (
        ((vsx_widget_component*)((vsx_widget_anchor*)parent)->component)->open
        && ((vsx_widget_anchor*)parent)->io == -1
        ) {
        alpha = 0.1f;
      }
  } //else return;
  pv = parent->get_pos_p();
  if (!parent->visible)
  {
    open = false;
    ((vsx_widget_anchor*)parent)->conn_open = false;
  }

  if (((vsx_widget_anchor*)parent)->io == -1)
  if (open && order != -1) {
    float s = parent->size.x * 0.2;

    pv.x = pos.x + pv.x - parent->size.y*0.7;
    pv.y = pos.y + pv.y + parent->size.y/2 - ((float)order)*s*2.2;
    pv.z = pos.z;
    if (a_focus == this)
    glColor4f(1,0.5,0.5,alpha);
      else
    glColor4f(0.8,0.8,0.8,alpha);
  }

  if (!destination) {
    //printf("a1:%s:%d\n",name.c_str(),destination);
    ex = pv.x+size.x;
    ey = pv.y+size.y;
  } else {
    vsx_vector dv = destination->get_pos_p();
    ex = dv.x;
    ey = dv.y;
  }

  glEnable(GL_LINE_SMOOTH);


  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  mtex_blob.bind();
    draw_box_texf(pv.x,pv.y,0,0.004f,0.004f);
    draw_box_texf(ex,ey,0,0.004f,0.004f);
  mtex_blob._bind();

//   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
// 
//   glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
//   glLineWidth(3.0f);
//   glBegin(GL_LINES);
     sx = pv.x;
     sy = pv.y;
//     glVertex3f(sx,sy,pos.z);
//     glVertex3f(ex,ey,pos.z);
//   glEnd();
//   glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  if (m_focus == this || a_focus == this || k_focus == this)
  {
    glLineWidth(1.0f+fmod(time*20.0f,5.0f));
  }
  else
  {
    glLineWidth(1.0f);
  }

  vsx_bezier_calc bez_calc;
  bez_calc.x0 = sx;
  bez_calc.y0 = sy;

  bez_calc.x1 = sx+(ex-sx)*0.5f;
  bez_calc.y1 = sy;

  bez_calc.x2 = ex-(ex-sx)*0.5f;
  bez_calc.y2 = ey;

  bez_calc.x3 = ex;
  bez_calc.y3 = ey;
  bez_calc.init();
  
  glLineWidth(2.0f);
  if (k_focus == this)
  {
    glColor4f(1,0.5,0.5,alpha);
  }
  else
  {
    if (parent == a_focus || destination == a_focus)
    {
      glColor4f(1,1,1,alpha);
    }
    else
    {
      glColor4f(1,1,1,alpha*(dim_alpha+dim_my_alpha));
    }
  }

  glBegin(GL_LINE_STRIP);
    for (float t = 0.0f; t <= 1.05f; t += 0.05f)
    {
      glVertex3f(bez_calc.x_from_t(t),bez_calc.y_from_t(t),pos.z);
    }
  glEnd();

//  glBegin(GL_LINES);
//    glVertex3f(sx,sy,pos.z);
//    glVertex3f(ex,ey,pos.z);
//  glEnd();
  glDisable(GL_LINE_SMOOTH);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  real_pos = pv;
  draw_children();
}

void vsx_widget_connector_bezier::before_delete() 
{
  marked_for_deletion = true;
  ((vsx_widget_anchor*)parent)->connections.remove(this);
  if (destination) {
    ((vsx_widget_anchor*)destination)->connectors.remove(this);
    // see if the destination is an alias that needs to go as well
    vsx_widget_anchor* dest = (vsx_widget_anchor*)destination;
    if (dest)
    if (dest->io != 1)
    if (dest->alias && dest->io == ((vsx_widget_anchor*)parent)->io) {
      dest->_delete();
      ((vsx_widget_component*)dest->component)->macro_fix_anchors();
    }
    destination = 0;
  }
}

void vsx_widget_connector_bezier::on_delete() 
{
  if (!global_delete)
  ((vsx_widget_anchor*)parent)->update_connection_order();
}

void vsx_widget_connector_bezier::init() 
{
  order = 0;
  dim_my_alpha = 0.0f;
  open = false;
  if (init_run) return;
  widget_type = VSX_WIDGET_TYPE_CONNECTOR;
  title = "connector";
  coord_type = VSX_WIDGET_COORD_CORNER;
  transparent = false;
  menu = add(new vsx_widget_popup_menu,".connector_menu");
  menu->commands.adds(VSX_COMMAND_MENU, "disconnect", "disconnect","");
  menu->size.x = 0.3;
  menu->size.y = 0.5;
  init_children();
  mtex_blob.load_png(skin_path+"interface_extras/connection_blob.png");

  constrained_x = false;
  constrained_y = false;
  constrained_z = false;

  if (((vsx_widget_component*)((vsx_widget_anchor*)parent)->component)->component_type == "macro") macro_child = true;
  init_run = true;
  receiving_focus = true;
}





// constructor ----->
vsx_widget_connector_bezier::vsx_widget_connector_bezier()
{
  alias_conn = false;
  visible = 1;
  destination = 0;
  receiving_focus = true;
  support_interpolation = true;
}

bool vsx_widget_connector_bezier::event_key_down(signed long key, bool alt, bool ctrl, bool shift) 
{
  if (key == 127) {
    command_q_b.add_raw("disconnect");
    vsx_command_queue_b(this);
  }
  return true;
}
#endif
