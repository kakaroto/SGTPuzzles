/*
 * ps3save.c : PS3 Puzzles Loading/Saving mechanism
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 */

#include "ps3save.h"
#include "ps3.h"

#include <sys/thread.h>
#include <sys/memory.h>
#include <sysutil/save.h>
#include <string.h>
#include <ctype.h>

#define THREAD_STACK_SIZE 16*1024
#define THREAD_PRIORITY 1000
#define SAVE_LIST_MAX_DIRECTORIES 100
#define SAVE_LIST_MAX_FILES 5
#define BUFFER_SETTINGS_BUFSIZE (SAVE_LIST_MAX_DIRECTORIES * \
      sizeof(sysSaveDirectoryList))
#define MEMORY_CONTAINER_SIZE 5*1024*1024

void
saveload_game_list_cb (sysSaveCallbackResult *result,
    sysSaveListIn *in, sysSaveListOut *out)
{
  frontend *fe = (frontend *) ((u64) result->user_data);
  int i;

  printf ("saveload_game_list_cb called\n");

  printf ("Found %d directories. Listing %d\n", in->maxDirectories,
      in->numDirectories);
  for (i = 0; i < in->numDirectories; i++)
    printf ("   Directory : %s [%s]\n",
        SYS_SAVE_DIRECTORY_LIST_PTR (in->directoryList)[i].directoryName,
        SYS_SAVE_DIRECTORY_LIST_PTR (in->directoryList)[i].listParameter);

  memset (out, 0, sizeof(sysSaveListOut));
  out->focus = SYS_SAVE_FOCUS_POSITION_LIST_HEAD;
  out->numDirectories = in->numDirectories;
  out->directoryList = in->directoryList;

  if (fe->save_data.saving && out->numDirectories < SAVE_LIST_MAX_DIRECTORIES) {
    sysSaveNewSaveGame *new_save = &fe->save_data.new_save;
    char *dir = malloc (SYS_SAVE_MAX_DIRECTORY_NAME + 1);
    int idx = -1;
    int j;

    for (i = 0; idx == -1 && i <= 99; i++) {
      snprintf (dir, SYS_SAVE_MAX_DIRECTORY_NAME, "%s%d",
          fe->save_data.prefix, i);
      idx = i;
      for (j = 0; j < in->numDirectories; j++) {
        if (strcmp (dir, SYS_SAVE_DIRECTORY_LIST_PTR (in->directoryList)[j]. \
                directoryName) == 0) {
          idx = -1;
          break;
        }
      }
    }

    if (idx != -1) {
      printf ("New directory : %s\n", dir);
      memset (new_save, 0, sizeof(sysSaveNewSaveGame));
      new_save->position = SYS_SAVE_NEW_SAVE_POSITION_TOP;
      new_save->directoryName = PTR (dir);
      /* TODO: Show icon depending on the puzzle */
      out->newSaveGame = PTR (new_save);
    }
  }

  result->result = SYS_SAVE_CALLBACK_RESULT_CONTINUE;
  return;
}

void
saveload_game_status_cb (sysSaveCallbackResult *result,
    sysSaveStatusIn *in, sysSaveStatusOut *out)
{
  frontend *fe = (frontend *) ((u64) result->user_data);
  int i;
  int preset;

  printf ("saveload_game_status_cb called\n");

  printf ("Free space : %d\n"
      "New save game? : %d\n"
      " -dirname : %s\n"
      " -title : %s\n"
      "  subtitle : %s\n"
      "  detail : %s\n"
      "  copy protected? : %d\n"
      "  parental level : %d\n"
      "  listParameter : %s\n"
      "binding information : %d\n"
      "size of save data : %d\n"
      "size of system file : %d\n"
      "total files : %d\n"
      "number of files : %d\n",
      in->freeSpaceKB,
      in->isNew,
      in->directoryStatus.directoryName,
      in->getParam.title,
      in->getParam.subtitle,
      in->getParam.detail,
      in->getParam.copyProtected,
      in->getParam.parentalLevel,
      in->getParam.listParameter,
      in->bindingInformation,
      in->sizeKB,
      in->systemSizeKB,
      in->totalFiles,
      in->numFiles);

  for (i = 0; i < in->numFiles; i++)
    printf ("  -File type : %d\n"
        "   File size : %lu\n"
        "   filename : %s\n",
        SYS_SAVE_FILE_STATUS_PTR (in->fileList)[i].fileType,
        SYS_SAVE_FILE_STATUS_PTR (in->fileList)[i].fileSize,
        SYS_SAVE_FILE_STATUS_PTR (in->fileList)[i].filename);

  /* TODO: Check for availability of free space */

  out->recreateMode = SYS_SAVE_RECREATE_MODE_DELETE;
  out->setParam = PTR (&in->getParam);

  if (fe->save_data.saving) {
    strncpy (in->getParam.title, "SGT Puzzles", SYS_SAVE_MAX_TITLE);
    strncpy (in->getParam.subtitle, fe->thegame->name, SYS_SAVE_MAX_SUBTITLE);

    /* Force recreation of npresets so it can find the right preset.. */
    midend_num_presets (fe->me);
    preset = midend_which_preset (fe->me);
    if (preset >= 0 && preset < midend_num_presets (fe->me)) {
      char* name;
      game_params *params;

      midend_fetch_preset(fe->me, preset, &name, &params);
      strncpy (in->getParam.detail, name, SYS_SAVE_MAX_DETAIL);
    } else {
      strncpy (in->getParam.detail, "Custom", SYS_SAVE_MAX_DETAIL);
    }
  }

  result->result = SYS_SAVE_CALLBACK_RESULT_CONTINUE;
}

void
saveload_game_file_cb (sysSaveCallbackResult *result,
    sysSaveFileIn *in, sysSaveFileOut *out)
{
  printf ("saveload_game_file_cb called\n");
  result->result = SYS_SAVE_CALLBACK_RESULT_DONE;
}

void
saveload_game_thread(void *user_data)
{
  frontend *fe = user_data;
  sysSaveListSettings listSettings;
  sysSaveBufferSettings bufferSettings;
  mem_container_t container;
  char *prefix = fe->save_data.prefix;
  char *ptr = prefix;
  s32 ret;

  printf ("saveload_thread started\n");

  strncpy (prefix, "SGTPUZZLES-", SYS_SAVE_MAX_DIRECTORY_NAME);
  strncat (prefix, gamelist_names[fe->game_idx], SYS_SAVE_MAX_DIRECTORY_NAME);
  while (*ptr != 0) {
    *ptr = islower ((int)*ptr) ? *ptr - 'a' + 'A' : *ptr;
    ptr++;
  }

  memset (&listSettings, 0, sizeof (sysSaveListSettings));
  listSettings.sortType = SYS_SAVE_SORT_TYPE_TIMESTAMP;
  listSettings.sortOrder = SYS_SAVE_SORT_ORDER_DESCENDING;
  listSettings.pathPrefix = PTR (fe->save_data.prefix);
  listSettings.reserved = PTR (NULL);

  memset (&bufferSettings, 0, sizeof (sysSaveBufferSettings));
  bufferSettings.maxDirectories = SAVE_LIST_MAX_DIRECTORIES;
  bufferSettings.maxFiles = SAVE_LIST_MAX_FILES;
  bufferSettings.bufferSize = BUFFER_SETTINGS_BUFSIZE;
  bufferSettings.buffer = PTR (malloc (bufferSettings.bufferSize));

  if (sysMemContainerCreate (&container, MEMORY_CONTAINER_SIZE) != 0 ) {
    printf ("Unable to create memory container\n");
    goto end;
  }

  if (fe->save_data.saving)
    ret = sysSaveListSave2 (SYS_SAVE_CURRENT_VERSION,
        &listSettings, &bufferSettings,
        saveload_game_list_cb, saveload_game_status_cb, saveload_game_file_cb,
        container, fe);
  else
    ret = sysSaveListLoad2 (SYS_SAVE_CURRENT_VERSION,
        &listSettings, &bufferSettings,
        saveload_game_list_cb, saveload_game_status_cb, saveload_game_file_cb,
        container, fe);

  /* TODO: check ret */
  printf ("sysSaveListLoad2/Save2 returned : %d\n", ret);
  sysMemContainerDestroy (container);

end:
  if (bufferSettings.buffer)
    free (VOID_PTR (bufferSettings.buffer));

  printf ("saveload_thread exiting\n");
  fe->save_data.save_tid = 0;
  sysThreadExit (0);
}

int
_create_thread (frontend *fe, int saving, char *thread_name)
{
  s32 ret;

  if (fe->save_data.save_tid != 0) {
    printf ("Save/Load thread already running\n");
    return FALSE;
  }

  fe->save_data.saving = saving;
  fe->save_data.loading = !saving;
  ret = sysThreadCreate (&fe->save_data.save_tid, saveload_game_thread, fe,
      THREAD_PRIORITY, THREAD_STACK_SIZE, 0, thread_name);

  if (ret < 0) {
    printf ("Failed to create %s : %d\n", thread_name, ret);
    return FALSE;
  }

  return TRUE;
}

int
ps3_save_game (frontend *fe)
{
  return _create_thread (fe, TRUE, "save_thread");
}

int
ps3_load_game (frontend *fe)
{
  return _create_thread (fe, FALSE, "save_thread");
}
