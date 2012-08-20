#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Geiger.h"
#include "GUI.h"
#include "utils.h"
#include "display.h"

class Controller {

public:

  Controller(Geiger &g) : m_geiger(g) {
    m_sleeping=false;
    m_powerup=false;
  }

  void set_gui(GUI &g) {
    m_gui = &g;
  }

  void receive_gui_event(char *event,char *value) {

    if(strcmp(event,"Sleep")) {
      if(m_sleeping == false) {
        display_powerdown();
        m_sleeping=true;
        m_gui->set_key_trigger();
        m_gui->set_sleeping(true);
      }
    } else
    if(strcmp(event,"KEYPRESS")) {
      m_powerup=true;
    }

  }

  
  void update() {

    if(m_powerup == true) {
      display_powerup();
      m_gui->set_sleeping(false);
      m_gui->redraw();
      m_sleeping=false;
      m_powerup =false;
    }


    if(m_sleeping) return;

    char text_cpm[50];
    char text_cpmd[50];
    char text_seiverts[50];

    text_cpm[0]     =0;
    text_seiverts[0]=0;
    int_to_char(m_geiger.get_cpm(),text_cpm,4);
    float_to_char(m_geiger.get_microseiverts(),text_seiverts,6);
    float_to_char(m_geiger.get_cpm_deadtime_compenstated(),text_cpmd,6);

    text_seiverts[6] = ' ';
    text_seiverts[7] = 'u';
    text_seiverts[8] = 'S';
    text_seiverts[9] = 'v';
    text_seiverts[10] = 0;


    float *graph_data;
    graph_data = m_geiger.get_cpm_last_min();

    m_gui->receive_update("CPM",text_cpm);
    m_gui->receive_update("CPMDEAD",text_cpmd);
    m_gui->receive_update("SEIVERTS",text_seiverts);
    m_gui->receive_update("RECENTDATA",graph_data);
  }

  GUI    *m_gui;
  Geiger &m_geiger;
  bool    m_sleeping;
  bool    m_powerup;
};

#endif