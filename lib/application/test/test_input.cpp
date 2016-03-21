/**
* Project: VSXu: Realtime modular visual programming language, music/audio visualizer.
*
* This file is part of Vovoid VSXu.
*
* @author Jonatan Wallmander, Vovoid Media Technologies AB Copyright (C) 2003-2016
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

#include <vsx_version.h>
#include <vsx_platform.h>
#include <vsx_gl_global.h>
#include <vsx_application_manager.h>
#include <vsx_application_run.h>

class input_test_application
    : public vsx_application
{
public:

  void draw(int id)
  {
    VSX_UNUSED(id);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  void input_event(const vsx_input_event& event)
  {
    if (event.type == vsx_input_event::type_mouse)
      vsx_printf(L" mouse event, type: %d    button id: %d   button state: %d   x: %d     y: %d\n ", event.mouse.type, event.mouse.button_id, event.mouse.button_state, event.mouse.x, event.mouse.y);

    if (event.type == vsx_input_event::type_keyboard)
      vsx_printf(L" keyboard event, pressed: %d     scancode: %d \n ", event.keyboard.pressed,  event.keyboard.scan_code);
  }

  void key_down_event(long key)
  {
    vsx_printf(L"application key down event, scan code: %d\n", key);
  }
};






int main(int argc, char* argv[])
{
  vsx_argvector::get_instance()->init_from_argc_argv(argc, argv);

  input_test_application application;
  vsx_application_manager::get_instance()->application_set(&application);

  char titlestr[ 200 ];
  sprintf( titlestr, "Vovoid VSXu Applicataion Test Input %s [%s %d-bit]", vsxu_ver, PLATFORM_NAME, PLATFORM_BITS);
  application.window_title_set(vsx_string<>(&titlestr[0]));

  vsx_application_run::run();

  return 0;
}
