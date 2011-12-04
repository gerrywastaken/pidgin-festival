/*
 * Pidgin Festival input plugin
 *
 * Copyright (C) 2000-2003, Tigrux <tigrux@ximian.com> (original code)
 * Copyright (C) 2003, Rishi Sharmac (rewritten code/update for changed API)
 * Copyright (C) 2004, Nathan Fredrickson
 * Copyright (C) 2007, Varun Hiremath (rewritten code/update for
 * changed API in Pidgin 2.0)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef FESTIVAL_VOICES_PATH
//#define FESTIVAL_VOICES_PATH "/usr/lib/festival/voices" //for gentoo
#define FESTIVAL_VOICES_PATH "/usr/share/festival/voices" //for other distros
#endif
#define PURPLE_PLUGINS
#define FESTIVAL_PLUGIN_ID "gtk-festival-speaker"

#include <dirent.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "pidgin.h"
#include "debug.h"
#include "prefs.h"
#include "signals.h"
#include "conversation.h"
#include "blist.h"
#include "connection.h"
#include "util.h"
#include "gtkutils.h"
#include "version.h"
#include "gtkplugin.h"

#include "../fg_config.h"

#include <libintl.h>
#include <locale.h>
#define _(String) dgettext (GETTEXT_PACKAGE, String)
#define N_(String) (String)

static FILE *festival_pf = NULL;
static time_t connect_time;
static PurpleConversation *silent_joins = NULL;
static time_t silent_joins_time;
static char prev_alias[80];

/*---------Function Headers-------------*/
static void set_stretch_duration(float duration);
static void set_preferred_voice(const char *voice);

static void load_conf();

static void on_stretch_duration_spinbutton_changed(GtkWidget *widget);
static void on_maxlength_spinbutton_changed(GtkWidget *widget);
static void on_radio_clicked(GtkWidget *widget,GString *data);
static void on_radio_destroy(GtkWidget *widget,GString *data);
static void on_prepend_who_checkbutton_clicked(GtkWidget *widget, void *data);
static void on_replace_url_checkbutton_clicked(GtkWidget *widget, void *data);
static void on_announce_events_checkbutton_clicked(GtkWidget *widget, void *data);

static char * snd(char * sndType);
G_GNUC_CONST static gint badchar(char c);
static char *unlinkify_text(const char *text);

/*---------Functions---------------------*/

/** This function needs to be better written **/
static char * snd(char * sndType) {

  char *daemon = "";
  if (strcmp (sndType, "arts") == 0 ||
      strcmp (sndType, "esd")  == 0 ||
      strcmp (sndType, "alsa") == 0 ||
      strcmp (sndType, "automatic") == 0) {

    FILE *which_pf;
    char sndserver[1024];
    if (strcmp (sndType, "arts") == 0) 
      which_pf= popen("which artsdsp 2>/dev/null","r");
    else if (strcmp (sndType, "esd") == 0) 
      which_pf= popen("which esddsp 2>/dev/null","r");
    else if (strcmp (sndType, "alsa") == 0) 
      which_pf= popen("which aoss 2>/dev/null","r");
    else if (strcmp (sndType, "automatic") == 0) 
      which_pf= popen("which artsdsp 2>/dev/null","r");
    fscanf(which_pf,"%1023s",sndserver);
    pclose(which_pf);
    daemon=sndserver;
  }
  else{
    purple_debug(PURPLE_DEBUG_INFO, "pidgin festival sound method ", sndType);
  }
  return daemon;
}

G_GNUC_CONST static gint badchar(char c)
{
  switch (c) {
  case ' ':
  case ',':
  case '(':
  case ')':
  case '\0':
  case '\n':
  case '<':
  case '>':
  case '"':
  case '\'':
    return 1;
  default:
    return 0;
  }
}

static char *unlinkify_text(const char *text)
{
  const char *c, *t, *q = NULL;
  const char *URL;
  const int BUF_LEN = 1024;
  char *tmp;
  char url_buf[BUF_LEN * 4];

  URL=_("U R L");

  GString *ret = g_string_new("");

  /* Assumes you have a buffer able to cary at least BUF_LEN * 2 bytes */
  
  c = text;
  while (*c) {
    if(!q && (*c == '\"' || *c == '\'')) {
      q = c;
    } else if(q) {
      if(*c == *q)
	q = NULL;
    } else if (!g_ascii_strncasecmp(c, "<A", 2)) {
      while (1) {
	if (!g_ascii_strncasecmp(c, "/A>", 3)) {
	  break;
	}
	ret = g_string_append_c(ret, *c);
	c++;
	if (!(*c))
	  break;
      }
    } else if ((*c=='h') && (!g_ascii_strncasecmp(c, "http://", 7) ||
			     (!g_ascii_strncasecmp(c, "https://", 8)))) {
      t = c;
      while (1) {
	if (badchar(*t)) {
	  
	  if (*(t) == ',' && (*(t + 1) != ' ')) {
	    t++;
	    continue;
	  }
	  
	  if (*(t - 1) == '.')
	    t--;
	  g_string_append(ret, URL);
	  c = t;
	  break;
	}
	if (!t)
	  break;
	t++;
	
      }
    } else if (!g_ascii_strncasecmp(c, "www.", 4)) {
      if (c[4] != '.') {
	t = c;
	while (1) {
	  if (badchar(*t)) {
	    if (t - c == 4) {
	      break;
	    }
	    
	    if (*(t) == ',' && (*(t + 1) != ' ')) {
	      t++;
	      continue;
	    }
	    
	    if (*(t - 1) == '.')
	      t--;
	    g_string_append(ret, URL);
	    c = t;
	    break;
	  }
	  if (!t)
	    break;
	  t++;
	}
      }
    } else if (!g_ascii_strncasecmp(c, "ftp://", 6)) {
      t = c;
      while (1) {
	if (badchar(*t)) {
	  if (*(t - 1) == '.')
	    t--;
	  g_string_append(ret, URL);
	  c = t;
	  break;
	}
	if (!t)
	  break;
	t++;
	
      }
    } else if (!g_ascii_strncasecmp(c, "ftp.", 4)) {
      if (c[4] != '.') {
	t = c;
	while (1) {
	  if (badchar(*t)) {
	    if (t - c == 4) {
	      break;
	    }
	    if (*(t - 1) == '.')
	      t--;
	    g_string_append(ret, URL);
	    c = t;
	    break;
	  }
	  if (!t)
	    break;
	  t++;
	}
      }
    } else if (!g_ascii_strncasecmp(c, "mailto:", 7)) {
      t = c;
      while (1) {
	if (badchar(*t)) {
	  if (*(t - 1) == '.')
	    t--;
	  g_string_append(ret, URL);
	  c = t;
	  break;
	}
	if (!t)
	  break;
	t++;
	
      }
    } else if (c != text && (*c == '@')) {
      int flag;
      int len = 0;
      const char illegal_chars[] = "!@#$%^&*()[]{}/|\\<>\":;\r\n \0";
      url_buf[0] = 0;
      
      if (strchr(illegal_chars,*(c - 1)) || strchr(illegal_chars, *(c + 1)))
	flag = 0;
      else
	flag = 1;
      
      t = c;
      while (flag) {
	if (badchar(*t)) {
	  ret = g_string_truncate(ret, ret->len - (len - 1));
	  break;
	} else {
	  len++;
	  t--;
	  if (t < text) {
	    ret = g_string_assign(ret, "");
	    break;
	  }
	}
      }
      
      t = c + 1;
      
      while (flag) {
	if (badchar(*t)) {
	  g_string_append(ret, URL);
	  c = t;
	  
	  break;
	} else {
	  len++;
	}
	
	t++;
      }
    }
    
    if (*c == 0)
      break;
    
    ret = g_string_append_c(ret, *c);
    c++;
    
  }
  tmp = ret->str;
  g_string_free(ret, FALSE);
  return tmp;
}

static void speak(GString *text)
{
  if(festival_pf) {
    int i;
    GString *buffer= g_string_new("");

    // Remove non alphanumeric chars - Should this be done in unlinkify? -Rishi
    for(i=0;i<text->len;i++)
      if( strchr("¿¡\"!@#$%^&*()[]{}<>/|~`\r\n\0,.+-_:;'",text->str[i]))
	text->str[i] = ' ';

    g_string_printf(buffer,"(SayText \"%s\")", text->str);
    purple_debug(PURPLE_DEBUG_INFO, "pidgin festival", buffer->str);
    fputs(buffer->str,festival_pf);
    fflush(festival_pf);
    g_string_free(buffer,TRUE);
  }
}

static const char *get_best_name(PurpleAccount *account, char const * who)
{
  PurpleBuddy *buddy=purple_find_buddy(account, who);
  if (buddy == NULL) return who;
  return purple_buddy_get_alias(buddy);
}

static void
im_recv_im(PurpleAccount *account, char *who, char *what,
	    PurpleConversation * conv, PurpleMessageFlags flags)
{
  char *stripped;
  const char *alias = get_best_name(account, who);
  silent_joins = NULL;

  GString *buffer= g_string_new("");

/*  msg_utf8=purple_utf8_try_convert(what) */

  stripped = (char *)purple_markup_strip_html(/*msg_utf8?msg_utf8:*/what);
    
  if(purple_prefs_get_bool("/plugins/gtk/festival/speaker/replace_url"))
    stripped = unlinkify_text(stripped);

  if (strlen(stripped) <= purple_prefs_get_int("/plugins/gtk/festival/speaker/maxlength")) {

    if(purple_prefs_get_bool("/plugins/gtk/festival/speaker/prepend_who") &&
       strcmp(alias, prev_alias))
      g_string_printf(buffer,"%s %s %s", alias, _("says"), stripped);
    else
      g_string_printf(buffer,"%s", stripped);

    strcpy(prev_alias, alias);
    speak(buffer);
  } else {
    g_string_printf(buffer, "message overflow");
    speak(buffer);
  }

  g_free(stripped);
/*  if (msg_utf8) g_free(msg_utf8); */
  g_string_free(buffer,TRUE);
}

static void
_event_speak(const char *buddy, char *state)
{

  time_t now;
  time(&now);
  if(purple_prefs_get_bool("/plugins/gtk/festival/speaker/announce_events") &&
     now - connect_time > 10  ) {
    GString *buffer= g_string_new("");
    g_string_printf(buffer,"%s %s", buddy, state);
    speak(buffer);
    g_string_free(buffer,TRUE);
  }
}

static void
buddy_signed_on_cb(struct _PurpleBuddy *buddy, void *data)
{
  _event_speak((char *) purple_buddy_get_alias(buddy),_("signed on"));
}

static void
buddy_signed_off_cb(struct _PurpleBuddy *buddy, void *data)
{
  _event_speak((char *) purple_buddy_get_alias(buddy), _("signed off"));
}

static void
chat_buddy_joined_cb(PurpleConversation *conv, const char *user, void *data)
{
  if (silent_joins == conv)
  {
    time_t now;
    time(&now);

    if (now-silent_joins_time < 2) return;
    else silent_joins = NULL;
  }
  _event_speak(user, _("joined the conversation"));
}

static void 
chat_created(PurpleConversation *conv, void *data)
{
  silent_joins = conv;
  time(&silent_joins_time);
}

static void
chat_buddy_left_cb(PurpleConversation *conv, const char *user, const char *reason, void *data)
{
  silent_joins = NULL;
  _event_speak(user, _("left the conversation"));
}

static void
buddy_away_cb(struct _PurpleBuddy *buddy, void *data)
{
  _event_speak((char *) purple_buddy_get_alias(buddy), _("is now away"));
}

static void
buddy_back_cb(struct _PurpleBuddy *buddy, void *data)
{
  _event_speak((char *) purple_buddy_get_alias(buddy), _("is now back"));
}

static void
buddy_idle_cb(struct _PurpleBuddy *buddy, void *data)
{
  _event_speak((char *) purple_buddy_get_alias(buddy), _("is now idle"));
}

static void
buddy_unidle_cb(struct _PurpleBuddy *buddy, void *data)
{
  _event_speak((char *) purple_buddy_get_alias(buddy), _("is now un idle"));
}

static void
account_connecting_cb(PurpleAccount *account, void *data)
{
  time( &connect_time );
}

// Set Functions
static void set_stretch_duration(float duration) {

  if(festival_pf) {
    char sd[8];
    sprintf(sd,"%2.1f",duration);
    purple_prefs_set_string("/plugins/gtk/festival/speaker/duration", sd);
    fprintf(festival_pf,"(Parameter.set 'Duration_Stretch %s)\n", sd);
    fflush(festival_pf);
  }
}

static void set_preferred_voice(const char *voice) {

  if (voice) {
    purple_prefs_set_string("/plugins/gtk/festival/speaker/voice", voice);
    if (festival_pf) {
      fprintf(festival_pf,"(voice_%s)\n", voice);
      fflush(festival_pf);
    }
  }
}

static void on_stretch_duration_spinbutton_changed(GtkWidget *widget) {
  set_stretch_duration( gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)) );
}

static void on_maxlength_spinbutton_changed(GtkWidget *widget) {
    purple_prefs_set_int("/plugins/gtk/festival/speaker/maxlength",gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget)));
}

static void on_radio_clicked(GtkWidget *widget,GString *voice) {
  if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ) {
    set_preferred_voice(voice->str);
  }
}

static void on_radio_destroy(GtkWidget *widget, GString *data) {
  g_string_free(data,TRUE);
}

static void on_prepend_who_checkbutton_clicked(GtkWidget *widget, void *data) {
  purple_prefs_set_bool("/plugins/gtk/festival/speaker/prepend_who", 
		      gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget)));
}

static void on_replace_url_checkbutton_clicked(GtkWidget *widget, void *data) {
  purple_prefs_set_bool("/plugins/gtk/festival/speaker/replace_url", 
		      gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget)));
}

static void on_announce_events_checkbutton_clicked(GtkWidget *widget, void *data) {
  purple_prefs_set_bool("/plugins/gtk/festival/speaker/announce_events", 
		      gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget)));
}

static void load_conf() {
  set_stretch_duration(atof(purple_prefs_get_string("/plugins/gtk/festival/speaker/duration")));
  set_preferred_voice(purple_prefs_get_string("/plugins/gtk/festival/speaker/voice"));
}


static gboolean 
plugin_load(PurplePlugin *plugin) {

  FILE *which_pf;
  char line[1024];
  which_pf= popen("which festival 2>/dev/null","r");
  fscanf(which_pf,"%1023s",line);
  pclose(which_pf);

  if( *line != '/')
    return FALSE;

  char proc[1024];
  sprintf(proc, "%s %s", snd((char *)purple_prefs_get_string("/pidgin/sound/method")), line);
  purple_debug(PURPLE_DEBUG_INFO, "pidgin festival", proc);

  int errno=0;
  festival_pf= popen(proc,"w");

  if(errno) {
    pclose(festival_pf);
    return FALSE;
  }

  load_conf();

  void *conv_handle = purple_conversations_get_handle();
  void *blist_handle = purple_blist_get_handle();
  void *accounts_handle = purple_accounts_get_handle();

  purple_signal_connect(conv_handle, "received-im-msg",
		      plugin, PURPLE_CALLBACK(im_recv_im), NULL);
  purple_signal_connect(conv_handle, "received-chat-msg",
  		      plugin, PURPLE_CALLBACK(im_recv_im), NULL);
  purple_signal_connect(conv_handle, "conversation-created",
  		      plugin, PURPLE_CALLBACK(chat_created), NULL);
  purple_signal_connect(blist_handle, "buddy-signed-on",
  		      plugin, PURPLE_CALLBACK(buddy_signed_on_cb), NULL);
  purple_signal_connect(blist_handle, "buddy-signed-off",
  		      plugin, PURPLE_CALLBACK(buddy_signed_off_cb), NULL);
  purple_signal_connect(conv_handle, "chat-buddy-joined",
  		      plugin, PURPLE_CALLBACK(chat_buddy_joined_cb), NULL);
  purple_signal_connect(conv_handle, "chat-buddy-left",
		      plugin, PURPLE_CALLBACK(chat_buddy_left_cb), NULL);

  /* Buddy List subsystem signals */
  purple_signal_connect(blist_handle, "buddy-away",
		      plugin, PURPLE_CALLBACK(buddy_away_cb), NULL);
  purple_signal_connect(blist_handle, "buddy-back",
		      plugin, PURPLE_CALLBACK(buddy_back_cb), NULL);
  purple_signal_connect(blist_handle, "buddy-idle",
		      plugin, PURPLE_CALLBACK(buddy_idle_cb), NULL);
  purple_signal_connect(blist_handle, "buddy-unidle",
		      plugin, PURPLE_CALLBACK(buddy_unidle_cb), NULL);

  purple_signal_connect(accounts_handle, "account-connecting",
		      plugin, PURPLE_CALLBACK(account_connecting_cb), NULL);

  return TRUE;
}


static gboolean
plugin_unload(PurplePlugin *plugin)
{
  if(festival_pf) {
    fprintf(festival_pf,"(quit)");
    pclose(festival_pf);
    festival_pf= 0;
  }
  return TRUE;
}

static GtkWidget *
get_config_frame(PurplePlugin *plugin)
{

  GString
    *base_dir_name= NULL,
    *lang_dir_name= NULL;
  GtkObject *stretch_duration_adjustment;
  GtkObject *maxlength_adjustment;
  GtkWidget
    *parent,
    *config_vbox, *voices_vbox, *stretch_duration_hbox, *maxlength_hbox,
    *frame,
    *radio_button,
    *message_label,
    *stretch_duration_spinbutton,
    *maxlength_spinbutton,
    *prepend_who_checkbutton,
    *replace_url_checkbutton,
    *announce_events_checkbutton;
  GSList *radio_group=NULL;
  
  DIR *dir, *lang_dir;
  struct dirent *next_lang, *next_voice;
  int count_lang, count_voices;
  int can_activate_voice= FALSE;

  parent = gtk_vbox_new(FALSE,5);
  gtk_container_set_border_width(GTK_CONTAINER(parent), 12);

  config_vbox = (GtkWidget *) pidgin_make_frame(parent, _("Festival"));
  gtk_container_set_border_width(GTK_CONTAINER(config_vbox), 5);

  /*--------------- Available Voices -----------*/
  
  base_dir_name= g_string_new(FESTIVAL_VOICES_PATH);
  lang_dir_name= g_string_new("");
  message_label= gtk_label_new(_("Availables voices:"));
  gtk_box_pack_start(GTK_BOX(config_vbox),message_label,FALSE,TRUE,3);

  //Examine directory for voices
  dir= opendir(base_dir_name->str);
  if(!dir) {
    GString *error_name;
    error_name= g_string_new("");
    g_string_printf(error_name, _("Error opening voices directory: %s"), base_dir_name->str);
    message_label= gtk_label_new(error_name->str);
    g_string_free(error_name,TRUE);
    gtk_box_pack_start(GTK_BOX(config_vbox),message_label,FALSE,TRUE,3);
  }
  else {
    count_lang=0;
    while( ( next_lang = readdir(dir) ) !=0  ) {
      /* hide hidden files */
      if( *next_lang->d_name == '.' )
	continue;

      frame= gtk_frame_new(next_lang->d_name);
      gtk_box_pack_start(GTK_BOX(config_vbox),frame,FALSE,TRUE,3);
      voices_vbox= gtk_vbox_new(FALSE,5);
      gtk_container_set_border_width(GTK_CONTAINER(voices_vbox), 5);
      gtk_container_add(GTK_CONTAINER(frame),voices_vbox);

      g_string_printf(lang_dir_name,"%s/%s",base_dir_name->str,next_lang->d_name);
		
      lang_dir= opendir(lang_dir_name->str);
      if(!lang_dir) {
	GString *error_name;
	error_name= g_string_new("");
	g_string_printf( error_name,  _("Error opening voice directory: %s"), lang_dir_name->str);
	message_label= gtk_label_new(error_name->str);
	g_string_free(error_name,TRUE);
	gtk_box_pack_start( GTK_BOX(voices_vbox), message_label, FALSE, TRUE, 3);
	break;
      }

      count_voices=0;
      while( ( next_voice = readdir(lang_dir) )!=0  ) {
	GString *voice_name;
	/* hide hidden files */
        if( *next_voice->d_name == '.' )
          continue;

	radio_button= gtk_radio_button_new_with_label(radio_group,next_voice->d_name);
				
	if( purple_prefs_get_string("/plugins/gtk/festival/speaker/voice") && 
	    strcmp(purple_prefs_get_string("/plugins/gtk/festival/speaker/voice"),next_voice->d_name ) == 0) {
	  gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(radio_button),TRUE );
	  can_activate_voice= TRUE;
	}
				
	voice_name= g_string_new(next_voice->d_name);
	g_signal_connect(GTK_OBJECT(radio_button),"clicked", G_CALLBACK(on_radio_clicked), voice_name);
	g_signal_connect(GTK_OBJECT(radio_button),"destroy", G_CALLBACK(on_radio_destroy), voice_name);
	gtk_box_pack_start(GTK_BOX(voices_vbox), radio_button, FALSE, TRUE, 3);
	radio_group= gtk_radio_button_get_group( GTK_RADIO_BUTTON(radio_button) );
				
      }
      closedir(lang_dir);
    }
    closedir(dir);
  }

  g_string_free(base_dir_name,TRUE);
  g_string_free(lang_dir_name,TRUE);

  /*--------------- Replace URL -----------*/  
  replace_url_checkbutton = gtk_check_button_new_with_label( _("Replace \"http://www.someurl.com\" with URL"));
  if(purple_prefs_get_bool("/plugins/gtk/festival/speaker/replace_url"))
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(replace_url_checkbutton), TRUE);
  g_signal_connect(G_OBJECT(replace_url_checkbutton), "clicked", 
		   G_CALLBACK(on_replace_url_checkbutton_clicked), NULL);
  gtk_box_pack_end(GTK_BOX(config_vbox),replace_url_checkbutton,FALSE,TRUE,3);

  /*--------------- Prepend Alias -----------*/  	
  prepend_who_checkbutton = gtk_check_button_new_with_label(_("Prepend Buddy Name (Alias) to message"));
  if(purple_prefs_get_bool("/plugins/gtk/festival/speaker/prepend_who"))
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(prepend_who_checkbutton), TRUE);
  g_signal_connect(G_OBJECT(prepend_who_checkbutton), "clicked", 
		   G_CALLBACK(on_prepend_who_checkbutton_clicked), NULL);
  gtk_box_pack_end(GTK_BOX(config_vbox),prepend_who_checkbutton,FALSE,TRUE,3);

  /*--------------- Announce Events -----------*/  	
  announce_events_checkbutton = gtk_check_button_new_with_label(_("Announce events"));
  if(purple_prefs_get_bool("/plugins/gtk/festival/speaker/announce_events"))
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(announce_events_checkbutton), TRUE);
  g_signal_connect(G_OBJECT(announce_events_checkbutton), "clicked", 
		   G_CALLBACK(on_announce_events_checkbutton_clicked), NULL);
  gtk_box_pack_end(GTK_BOX(config_vbox),announce_events_checkbutton,FALSE,TRUE,3);

  /*--------------- Duration -----------*/  	
  stretch_duration_hbox= gtk_hbox_new(FALSE,3);
  stretch_duration_adjustment= gtk_adjustment_new(1.0,0.3,10.0,0.1,1.0,1.0);
  stretch_duration_spinbutton= gtk_spin_button_new(GTK_ADJUSTMENT(stretch_duration_adjustment),0.1,1);
  g_signal_connect(G_OBJECT(stretch_duration_spinbutton), "value_changed", 
  		   G_CALLBACK(on_stretch_duration_spinbutton_changed), NULL);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(stretch_duration_spinbutton),GTK_UPDATE_IF_VALID);
  gtk_entry_set_editable(GTK_ENTRY(stretch_duration_spinbutton),FALSE);
  gtk_box_pack_start(GTK_BOX(stretch_duration_hbox),gtk_label_new(_("Pitch")),FALSE,FALSE,3);
  gtk_box_pack_start(GTK_BOX(stretch_duration_hbox),stretch_duration_spinbutton,FALSE,FALSE,3);
  gtk_box_pack_end(GTK_BOX(config_vbox),stretch_duration_hbox,FALSE,TRUE,3);
  gtk_spin_button_set_value( GTK_SPIN_BUTTON(stretch_duration_spinbutton),
  			     atof (purple_prefs_get_string("/plugins/gtk/festival/speaker/duration")));

  /*--------------- Max Length -----------*/  	
  maxlength_hbox= gtk_hbox_new(FALSE,3);
  maxlength_adjustment= gtk_adjustment_new(256,0,1000,1,256,256);
  maxlength_spinbutton= gtk_spin_button_new(GTK_ADJUSTMENT(maxlength_adjustment),1,0);
  g_signal_connect(G_OBJECT(maxlength_spinbutton), "value_changed", 
  		   G_CALLBACK(on_maxlength_spinbutton_changed), NULL);
  gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(maxlength_spinbutton),GTK_UPDATE_IF_VALID);
  gtk_entry_set_editable(GTK_ENTRY(maxlength_spinbutton),FALSE);
  gtk_box_pack_start(GTK_BOX(maxlength_hbox),gtk_label_new(_("Max Length")),FALSE,FALSE,3);
  gtk_box_pack_start(GTK_BOX(maxlength_hbox),maxlength_spinbutton,FALSE,FALSE,3);
  gtk_box_pack_end(GTK_BOX(config_vbox),maxlength_hbox,FALSE,TRUE,3);
  gtk_spin_button_set_value( GTK_SPIN_BUTTON(maxlength_spinbutton),
  			     purple_prefs_get_int("/plugins/gtk/festival/speaker/maxlength"));

  if(!can_activate_voice && radio_group) {
    GtkRadioButton *default_radio;
    default_radio= GTK_RADIO_BUTTON( g_slist_nth(radio_group,0)->data );
    gtk_button_clicked( GTK_BUTTON(default_radio) );
  }

  gtk_widget_show_all(parent);
  return parent;
}


static PidginPluginUiInfo ui_info =
{
  get_config_frame
};

static PurplePluginInfo info =
{
  PURPLE_PLUGIN_MAGIC,
  PURPLE_MAJOR_VERSION,
  PURPLE_MINOR_VERSION,
  PURPLE_PLUGIN_STANDARD,
  PIDGIN_PLUGIN_TYPE,
  0,
  NULL,
  PURPLE_PRIORITY_DEFAULT,
  FESTIVAL_PLUGIN_ID,
  N_("Festival"),
  FG_VERSION,
  N_("Text-to-Speech for Pidgin"),             /**  summary        */
  N_("Uses in the Festival Speech Synthesis System to read Pidgin conversations outloud."),   /**  description    */
  "Varun Hiremath <varunhiremath@gmail.com>\n\t\t\t(Maintainer/Author)\n\t\t\tRishi Sharma <rishsharma@yahoo.com>\n\t\t\t(Gaim Support)\n\t\t\tTigrux <tigrux@ximian.com>\n\t\t\t(Original Author)",
  "http://pidgin-festival.sourceforge.net/",          /**< homepage       */
  plugin_load,
  plugin_unload,
  NULL,
  &ui_info,
  NULL,
  NULL,
  NULL
};


static void
init_plugin(PurplePlugin *plugin)
{
#ifdef ENABLE_NLS
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
#endif

  purple_prefs_add_none("/plugins/gtk");
  purple_prefs_add_none("/plugins/gtk/festival");
  purple_prefs_add_none("/plugins/gtk/festival/speaker");

  purple_prefs_add_string("/plugins/gtk/festival/speaker/voice", "");
  purple_prefs_add_string("/plugins/gtk/festival/speaker/duration", "1.0");
  purple_prefs_add_int("/plugins/gtk/festival/speaker/maxlength", 256);
  purple_prefs_add_bool("/plugins/gtk/festival/speaker/prepend_who", FALSE);
  purple_prefs_add_bool("/plugins/gtk/festival/speaker/replace_url", FALSE);
  purple_prefs_add_bool("/plugins/gtk/festival/speaker/announce_events", FALSE);
}

PURPLE_INIT_PLUGIN(festival, init_plugin, info)
