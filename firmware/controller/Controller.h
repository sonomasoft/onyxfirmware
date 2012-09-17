#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "Geiger.h"
#include "GUI.h"
#include "utils.h"
#include "display.h"
#include "realtime.h"
#include "flashstorage.h"
#include "rtc.h"
#include "serialinterface.h"
#include "power.h"
#include <stdio.h>
#include "accel.h"
#include "log.h"

class Controller {

public:

  Controller(Geiger &g) : m_geiger(g) {
    m_sleeping=false;
    m_powerup=false;
    m_log_period_seconds = 120;
    rtc_clear_alarmed();
    rtc_set_alarm(RTC,rtc_get_time(RTC)+m_log_period_seconds);
    rtc_enable_alarm(RTC);
    m_alarm_log = false;
  }

  void set_gui(GUI &g) {
    m_gui = &g;
  }

  void update_calibration() {
    int c1 = m_gui->get_item_state_uint8("CAL1");
    int c2 = m_gui->get_item_state_uint8("CAL2");
    int c3 = m_gui->get_item_state_uint8("CAL3");
    int c4 = m_gui->get_item_state_uint8("CAL4");
    float calibration_scaling = ((float)c1) + (((float)c2)/10) + (((float)c3)/100) + (((float)c4)/1000);

    char text_sieverts[50];
    float_to_char(m_calibration_base*calibration_scaling,text_sieverts,5);
    text_sieverts[5] = ' ';
    text_sieverts[6] = 'u';
    text_sieverts[7] = 'S';
    text_sieverts[8] = 'v';
    text_sieverts[9] = 0;
    m_gui->receive_update("FIXEDSV",text_sieverts);
  }
 
  void save_calibration() {
    int c1 = m_gui->get_item_state_uint8("CAL1");
    int c2 = m_gui->get_item_state_uint8("CAL2");
    int c3 = m_gui->get_item_state_uint8("CAL3");
    int c4 = m_gui->get_item_state_uint8("CAL4");
    float calibration_scaling = ((float)c1) + (((float)c2)/10) + (((float)c3)/100) + (((float)c4)/1000);
    float base_sieverts = m_geiger.get_microsieverts();    

    char text_sieverts[50];
    float_to_char(base_sieverts*calibration_scaling,text_sieverts,5);
    text_sieverts[5] = ' ';
    text_sieverts[6] = 'u';
    text_sieverts[7] = 'S';
    text_sieverts[8] = 'v';
    text_sieverts[9] = 0;

    m_gui->receive_update("Sieverts",text_sieverts);
    m_geiger.set_calibration(calibration_scaling);
    m_gui->jump_to_screen(0);
  }

  void initialise_calibration() {
    m_calibration_base = m_geiger.get_microsieverts();
    char text_sieverts[50];
    float_to_char(m_calibration_base,text_sieverts,5);
    text_sieverts[5] = ' ';
    text_sieverts[6] = 'u';
    text_sieverts[7] = 'S';
    text_sieverts[8] = 'v';
    text_sieverts[9] = 0;
    m_gui->receive_update("FIXEDSV",text_sieverts);
    uint8_t val=1;
    m_gui->receive_update("CAL1",&val);
  }

  void save_time() {
    int h1 = m_gui->get_item_state_uint8("TIMEHOUR1");
    int h2 = m_gui->get_item_state_uint8("TIMEHOUR2");
    int m1 = m_gui->get_item_state_uint8("TIMEMIN1");
    int m2 = m_gui->get_item_state_uint8("TIMEMIN2");
    int s1 = m_gui->get_item_state_uint8("TIMESEC1");
    int s2 = m_gui->get_item_state_uint8("TIMESEC2");

    int new_hours = h2 + (h1*10);
    int new_min   = m2 + (m1*10);
    int new_sec   = s2 + (s1*10);

    uint8_t hours,min,sec,day,month;
    uint16_t year;
    realtime_getdate(hours,min,sec,day,month,year);
    hours = new_hours;
    min   = new_min;
    sec   = new_sec;
    realtime_setdate(hours,min,sec,day,month,year);

    m_gui->jump_to_screen(0);
  }

  void save_date() {
    int d1 = m_gui->get_item_state_uint8("DATEDAY1");
    int d2 = m_gui->get_item_state_uint8("DATEDAY2");
    int m1 = m_gui->get_item_state_uint8("DATEMON1");
    int m2 = m_gui->get_item_state_uint8("DATEMON2");
    int y1 = m_gui->get_item_state_uint8("DATEYEAR1");
    int y2 = m_gui->get_item_state_uint8("DATEYEAR2");

    int new_day  = d2 + (d1*10);
    int new_mon  = m2 + (m1*10);
    int new_year = y2 + (y1*10);

    uint8_t hours,min,sec,day,month;
    uint16_t year;
    realtime_getdate(hours,min,sec,day,month,year);
    day   = new_day;
    month = new_mon-1;
    year  = (2000+new_year)-1900;
    realtime_setdate(hours,min,sec,day,month,year);

    m_gui->jump_to_screen(0);
  }

  void receive_gui_event(char *event,char *value) {

    if(strcmp(event,"Sleep")) {
      if(m_sleeping == false) {
        display_powerdown();
        m_sleeping=true;
        m_gui->set_key_trigger();
        m_gui->set_sleeping(true);
        power_standby();
      }
    } else
    if(strcmp(event,"KEYPRESS")) {
      m_powerup=true;
    } else
    if(strcmp(event,"Save")) {
      save_calibration();
    } else
    if(strcmp(event,"SaveTime")) {
      save_time();
    } else
    if(strcmp(event,"SaveDate")) {
      save_date();
    } else
    if(strcmp(event,"CALIBRATE")) {
      initialise_calibration();
    } else
    if(strcmp(event,"varnumchange")) {
      if(strcmpl("CAL",value,3)) {
        update_calibration();
      }
    } else
    if(strcmp(event,"Data Transfer")) {
      display_draw_text(0,64,"Sending Log",0);
      serial_sendlog();
      display_draw_text(0,64,"Xfer Complete",0);
    }
  }

  void update() {

    if(rtc_alarmed()) {
      m_alarm_log = true;
      m_last_alarm_time = rtc_get_time(RTC);
      #ifndef DISABLE_ACCEL
      int8 res = accel_read_state(&m_accel_x_stored,&m_accel_y_stored,&m_accel_z_stored);
      #endif

      // set new alarm for log_period_seconds from now.
      rtc_clear_alarmed();
    }

    if(m_alarm_log == true) {
      if(m_geiger.is_cpm_valid()) {

        log_data_t data;
        #ifndef DISABLE_ACCEL
        int8 res = accel_read_state(&data.accel_x_end,&data.accel_y_end,&data.accel_z_end);
        #endif

        data.time  = rtc_get_time(RTC);
        data.cpm   = m_geiger.get_cpm();
        data.accel_x_start = m_accel_x_stored;
        data.accel_y_start = m_accel_y_stored;
        data.accel_z_start = m_accel_z_stored;
 
        flashstorage_log_pushback((uint8_t *) &data,sizeof(log_data_t));
        m_alarm_log = false;

        rtc_set_alarm(RTC,m_last_alarm_time+m_log_period_seconds);
        rtc_enable_alarm(RTC);
      }
    }

    //TODO: I should change this so it only sends the messages the GUI currently needs.

    if(m_powerup == true) {
      display_powerup();
      m_gui->set_sleeping(false);
      m_gui->redraw();
      m_sleeping=false;
      m_powerup =false;
    }


    if(m_sleeping) {
      if(!rtc_alarmed()) {} // go back to sleep.
      return;
    }

    char text_cpm[50];
    char text_cpmd[50];
    char text_sieverts[50];

    text_cpm[0]     =0;
    text_sieverts[0]=0;
    int_to_char(m_geiger.get_cpm(),text_cpm+5,4);
    float_to_char(m_geiger.get_microsieverts(),text_sieverts+5,5);
    float_to_char(m_geiger.get_cpm_deadtime_compensated(),text_cpmd+5,5);
    
    text_cpmd[0]      = 'C';
    text_cpmd[1]      = 'P';
    text_cpmd[2]      = 'M';
    text_cpmd[3]      = 'd';
    text_cpmd[4]      = ':';

    text_cpm[0]      = 'C';
    text_cpm[1]      = 'P';
    text_cpm[2]      = 'M';
    text_cpm[3]      = ' ';
    text_cpm[4]      = ':';

    text_sieverts[0]  = 'S';
    text_sieverts[1]  = 'v';
    text_sieverts[2]  = 't';
    text_sieverts[3]  = ' ';
    text_sieverts[4]  = ':';

    text_sieverts[10] = ' ';
    text_sieverts[11] = 'u';
    text_sieverts[12] = 'S';
    text_sieverts[13] = 'v';
    text_sieverts[14] = 0;

    float *graph_data;
    graph_data = m_geiger.get_cpm_last_min();

    uint8_t hours,min,sec,day,month;
    uint16_t year;
    realtime_getdate(hours,min,sec,day,month,year);

    char text_time[50];
    int_to_char(hours,text_time,3);
    text_time[3]=':';
    int_to_char(min  ,text_time+4,3);
    text_time[6]=':';
    int_to_char(sec  ,text_time+7,3);
    text_time[10] = 0;

    char text_date[50];
    int_to_char(day,text_date,3);
    text_date[3]='/';
    int_to_char(month+1,text_date+4,3);
    text_date[6]='/';
    int_to_char(year+1900,text_date+7,4);
    text_date[11] = 0;

    char text_bat[50];
    uint16 bat = power_battery_level();
    sprintf(text_bat,"BatLevel: %u",bat);

    char text_acc[50];
    int16 x,y,z;
    int8 res=1;
    //#ifndef DISABLE_ACCEL
    //res = accel_read_state(&x,&y,&z);
    //#endif

    sprintf(text_acc,"%d %d %d    ",x,y,z);
    if(res) sprintf(text_acc,"ACCERR %d    ",res);

    //if(m_geiger.is_cpm_valid()) m_gui->receive_update("CPMVALID","true");
    //                       else m_gui->receive_update("CPMVALID","false");
    m_gui->receive_update("CPM",text_cpm);
    m_gui->receive_update("CPMDEAD",text_cpmd);
    m_gui->receive_update("SIEVERTS",text_sieverts);
    m_gui->receive_update("RECENTDATA",graph_data);
    m_gui->receive_update("DELAYA",NULL);
    m_gui->receive_update("DELAYB",NULL);
    m_gui->receive_update("TIME",text_time);
    m_gui->receive_update("DATE",text_date);
    m_gui->receive_update("BATLEVEL",text_bat);
    //m_gui->receive_update("ACCEL",text_acc);
  }

  GUI     *m_gui;
  Geiger  &m_geiger;
  bool     m_sleeping;
  bool     m_powerup;
  float    m_calibration_base;
  uint32_t m_log_period_seconds;
  bool     m_alarm_log;
  uint32_t m_last_alarm_time;
  int16    m_accel_x_stored;
  int16    m_accel_y_stored;
  int16    m_accel_z_stored;
};

#endif
