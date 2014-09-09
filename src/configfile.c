#include "configfile.h"
#include "opts.h"
#include <glib.h>
#include <sys/stat.h>

#define GRP_PERMISSIONS "Permissions"
#define GRP_DEFAULTS "Defaults"
#define KEY_ALLOW_NEW_PRIVS_PRERMITTED "PermitAllowNewPrivs"
#define KEY_PRIVATE_NETWORK "PrivateNetwork"
#define KEY_RUN_MODE "Run"
#define KEY_RUN_MEDIA "RunMedia"

static bool check_permissions() {
  struct stat st;

  /* Check if the configuration file exists, is owned by root and only writable by root */
  if(stat(APPJAIL_CONFIGFILE, &st) != 0) {
    fprintf(stderr, "Configuration file " APPJAIL_CONFIGFILE " does not exist.\n");
    return false;
  }
  if(st.st_uid != 0) {
    fprintf(stderr, "Configuration file " APPJAIL_CONFIGFILE " is not owned by root.\n");
    return false;
  }
  if(st.st_mode & (S_IWGRP | S_IWOTH)) {
    fprintf(stderr, "Configuration file " APPJAIL_CONFIGFILE " must only be writable by root.\n");
    return false;
  }

  return true;
}

static bool get_boolean(GKeyFile *cfgfile, const char *group, const char *key, bool *result, bool def) {
  bool res, ret;
  GError *err = NULL;
  ret = false;

  res = g_key_file_get_boolean(cfgfile, group,key, &err);
  if(err == NULL) {
    /* use value from configuration file */
    ret = true;
    *result = res;
  }
  else if(err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND || err->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
    /* default value */
    ret = true;
    *result = def;
  }
  g_clear_error(&err);
  return ret;
}

static bool get_run_mode(GKeyFile *cfgfile, const char *group, const char *key, run_mode_t *result, run_mode_t def) {
  bool ret = false;
  GError *err = NULL;
  char *s;

  s = g_key_file_get_string(cfgfile, group, key, &err);
  if(err == NULL)
    /* use value from configuration file */
    ret = string_to_run_mode(result, s);
  else if(err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND || err->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
    /* default value */
    ret = true;
    *result = def;
  }
  g_clear_error(&err);
  return ret;
}

appjail_config *parse_config() {
  appjail_config *config;
  GKeyFile *cfgfile;

  if(!check_permissions())
    return NULL;

  if((config = malloc(sizeof(appjail_config))) == NULL)
    errExit("malloc");

  /* Parse configuration file */
  cfgfile = g_key_file_new();
  if (!g_key_file_load_from_file(cfgfile, APPJAIL_CONFIGFILE, G_KEY_FILE_NONE, NULL))
    goto parse_error;

  if(!get_boolean(cfgfile, GRP_PERMISSIONS, KEY_ALLOW_NEW_PRIVS_PRERMITTED, &(config->allow_new_privs_permitted), false))
    goto parse_error;
  if(!get_boolean(cfgfile, GRP_DEFAULTS, KEY_PRIVATE_NETWORK, &(config->default_private_network), false))
    goto parse_error;
  if(!get_run_mode(cfgfile, GRP_DEFAULTS, KEY_RUN_MODE, &(config->default_run_mode), RUN_PRIVATE))
    goto parse_error;
  if(!get_boolean(cfgfile, GRP_DEFAULTS, KEY_RUN_MEDIA, &(config->default_bind_run_media), false))
    goto parse_error;

  g_key_file_free(cfgfile);
  return config;
parse_error:
  fprintf(stderr, "Failed to parse configuration file.\n");
  free_config(config);
  g_key_file_free(cfgfile);
  return NULL;
}

void free_config(appjail_config *config) {
  free(config);
}
