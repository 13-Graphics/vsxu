/**
* Project: VSXu Engine: Realtime modular visual programming engine.
*
* This file is part of Vovoid VSXu Engine.
*
* @author Jonatan Wallmander, Robert Wenzel, Vovoid Media Technologies AB Copyright (C) 2003-2013
* @see The GNU Lesser General Public License (LGPL)
*
* VSXu Engine is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*
* You should have received a copy of the GNU Lesser General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#ifndef VSX_MODULE_LIST_STATIC_H
#define VSX_MODULE_LIST_STATIC_H

#include <map>
#include <vector>
#include <vsx_string.h>
#include <vsx_param.h>
#include <vsx_module.h>

#include "vsx_dlopen.h"
#include "vsx_module_list_abs.h"
#include "vsx_module_plugin_info.h"


#include "vsx_dlopen.h"
// Implementation of Module List Class for Linux

// See vsx_module_list_abs.h for reference documentation for this class


class vsxm_sf_info
{
public:
  vsx_string name;
  void* cm; //create module func
  void* dm; //delete module func
  void* nm; //number of modules
  vsxm_sf_info(vsx_string n_name, void *n_cm, void* n_dm, void* n_nm)
  {
    name = n_name;
    cm = n_cm;
    dm = n_dm;
    nm = n_nm;
  }
};

class vsx_module_list : public vsx_module_list_abs
{
private:
  vsx_avector<vsxm_sf_info*> modules;
public:
  void init()
  {
    // woops, looks like we already built the list
    if (module_list.size())
      return;

    // statistics counter - how many modules are loaded in total?
    //unsigned long total_num_modules = 0;

    // set up engine environment for later use (directories in which modules can look for config)
    vsx_engine_environment engine_environment;
    engine_environment.engine_parameter[0] = PLATFORM_SHARED_FILES+"plugin-config/";

    #include <plugins/static_factory_funcmapping.h>
    #define VSXM_SF_ADD_MODULE(name,m_cm,m_dm,m_nm) \
      modules.push_back(new vsxm_sf_info(#name,(void*)m_cm,(void*)m_dm,(void*)m_nm))
    // build the definitions (auto-generated by cmake)
    #include <plugins/static_factory_funcmapping.cpp>




    //-------------------------------------------------------------------------
    // Iterate through all the filenames, treat them as plugins with dlopen
    // and probe them to see if they are vsxu modules.
    for (size_t i = 0; i < modules.size(); i++)
    {
      // initialize constructor (factory) method
      vsx_module*(*create_new_module)(unsigned long, void*) =
          (vsx_module*(*)(unsigned long, void*))
          modules[i]->cm
          ;
      //-------------------------------------------------------------------------



      //-------------------------------------------------------------------------
      // init destructor method
      void(*destroy_module)(vsx_module*,unsigned long) =
          (void(*)(vsx_module*,unsigned long))
          modules[i]->dm
          ;
      //-------------------------------------------------------------------------



      //-------------------------------------------------------------------------
      // init get_num_modules method
      unsigned long(*get_num_modules)(vsx_engine_environment*) =
          (unsigned long(*)(vsx_engine_environment*))
          modules[i]->nm
          ;
      //-------------------------------------------------------------------------



      //-------------------------------------------------------------------------
      // get the number of modules in this plugin
      unsigned long num_modules_in_this_plugin = get_num_modules(&engine_environment);

      // iterate through modules in this plugin
      for (
           size_t module_index_iterator = 0;
           module_index_iterator < num_modules_in_this_plugin;
           module_index_iterator++
      )
      {
        // ask the constructor / factory to create a module instance for us
        vsx_module* module_object =
            create_new_module(module_index_iterator, (void*)vsx_argvector::get_instance() );
        // check for error
        if (0x0 == module_object)
        {
          printf(
                "vsx_module_list init: Error: trying to load statically linked module \"%s\"\n"
                "                      Cause: create_new_module returned 0x0 for module_index_iterator %lx\n"
                "                      Hint: If you are developing, check to see that get_num_modules returns\n"
                "                            the correct module count!\n"
                ,
                modules[i]->name.c_str(),
                module_index_iterator
                );
          continue; // try to load the next module
        }

        // ask the module to provide its module info
        vsx_module_info* module_info = new vsx_module_info;
        module_object->module_info( module_info );

        // check to see if this module can run on this system
        bool can_run = module_object->can_run();

        destroy_module( module_object, module_index_iterator );

        if (!can_run)
          continue; // try to load the next module

        module_info->location = "external";

        // create module_plugin_info template
        vsx_module_plugin_info module_plugin_info_template;

        module_plugin_info_template.create_new_module = create_new_module;
        module_plugin_info_template.destroy_module = destroy_module;

        module_plugin_info_template.module_id = module_index_iterator;

        // split the module identifier string into its individual names
        // a module can have multiple names (and locations in the gui tree)
        // some of these are hidden, thus the name begins with an exclamation mark - !
        // example module identifier string:
        //   examples;my_modules;my_module||!old_path;old_category;old_name
        // Only the first will show up in the gui. The second identifier is still usable in
        // old state files.
        vsx_string deli = "||";
        vsx_avector<vsx_string> parts;
        explode(module_info->identifier, deli, parts);
        vsx_module_plugin_info* applied_plugin_info = 0;

        // iterate through the individual names for this module
        for (unsigned long i = 0; i < parts.size(); ++i)
        {
          // create a copy of the template
          applied_plugin_info = new vsx_module_plugin_info;
          *applied_plugin_info = module_plugin_info_template;
          vsx_module_info* applied_module_info = new vsx_module_info;
          *applied_module_info = *module_info;
          applied_plugin_info->module_info = applied_module_info;


          vsx_string module_identifier;
          if (parts[i][0] == '!')
          {
            // hidden from gui
            applied_plugin_info->hidden_from_gui = true;
            module_identifier = parts[i].substr(1);
          } else
          {
            // normal
            applied_plugin_info->hidden_from_gui = false;
            module_identifier = parts[i];
          }
          // set module info identifier
          applied_module_info->identifier = module_identifier;
          // add the applied_plugin_info to module_plugin_list
          module_plugin_list[module_identifier] = applied_plugin_info;

          // add the module info to the module list
          module_list[module_identifier] = module_info;
        } // iterate through the individual names for this module
        module_infos.push_back(module_info);
      } // iterate through modules in this plugin
    } // Iterate through all the filenames, treat them as plugins
  }

  void destroy()
  {
  }



  std::vector< vsx_module_info* >* get_module_list( bool include_hidden = false)
  {
    std::vector< vsx_module_info* >* result = new std::vector< vsx_module_info* >;
    for (std::map< vsx_string, void* >::const_iterator it = module_plugin_list.begin(); it != module_plugin_list.end(); it++)
    {
      vsx_module_plugin_info* plugin_info = (vsx_module_plugin_info*)((*it).second);
      if
      (
          (include_hidden && plugin_info->hidden_from_gui)
          ||
          !plugin_info->hidden_from_gui
      )
      {
        result->push_back( plugin_info->module_info );
      }
    }
    return result;
  }


  vsx_module* load_module_by_name(vsx_string name)
  {
    if ( module_plugin_list.find(name) == module_plugin_list.end() )
    {
      return 0x0;
    }

    // call constrcuction factory
    vsx_module* module =
      ((vsx_module_plugin_info*)module_plugin_list[ name ])
      ->
      create_new_module
      (
        ((vsx_module_plugin_info*)module_plugin_list[ name ])->module_id,
        (void*)vsx_argvector::get_instance()
      )
    ;
    module->module_id = ((vsx_module_plugin_info*)module_plugin_list[ name ])->module_id;
    module->module_identifier = ((vsx_module_plugin_info*)module_plugin_list[ name ])->module_info->identifier;
    return module;
  }

  void unload_module( vsx_module* module_pointer )
  {
    // call destrcuction factory
    ((vsx_module_plugin_info*)module_plugin_list[ module_pointer->module_identifier ])
    ->
    destroy_module
    (
      module_pointer,
      module_pointer->module_id
    );
  }

  bool find( const vsx_string &module_name_to_look_for)
  {
    if (!(module_list.find(module_name_to_look_for) != module_list.end()))
    {
      return false;
    }
    return true;
  }


  void print_help()
  {
    if (!module_list.size())
      return;

    // TODO
//    for (size_t i = 0; i < plugin_handles.size(); i++)
//    {
//      if ( vsx_dlopen::sym(plugin_handles[i], "print_help") )
//      {
//        void(*print_help)() =
//            (void(*)())
//            vsx_dlopen::sym(
//              plugin_handles[i],
//              "print_help"
//            );
//        print_help();
//        printf("\n-----------------------------------------\n\n");
//      }
//    }
  }
};

#endif