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
#include <stdio.h>
#include <ctype.h>

#define THREAD_STACK_SIZE 16*1024
#define THREAD_PRIORITY 1000

/* Allow 100 save games */
#define SAVE_LIST_MAX_DIRECTORIES 100
/* Max 3 files : icon, screenshot, save data */
#define SAVE_LIST_MAX_FILES 3

#define BUFFER_SETTINGS_BUFSIZE (SAVE_LIST_MAX_DIRECTORIES * \
      sizeof(sysSaveDirectoryList))
#define MEMORY_CONTAINER_SIZE (5*1024*1024)

#define SAVE_DATA_FILENAME "GAME"


static int read_game_data_from_buffer (void *ctx, void *buf, int len);

void
saveload_game_list_cb (sysSaveCallbackResult *result,
    sysSaveListIn *in, sysSaveListOut *out)
{
  frontend *fe = result->user_data;
  int i;

  printf ("saveload_game_list_cb called\n");

  printf ("Found %d directories. Listing %d\n", in->maxDirectories,
      in->numDirectories);
  for (i = 0; i < in->numDirectories; i++)
    printf ("   Directory : %s [%s]\n",
        in->directoryList[i].directoryName,
        in->directoryList[i].listParameter);

  memset (out, 0, sizeof(sysSaveListOut));
  out->focus = SYS_SAVE_FOCUS_POSITION_LIST_HEAD;
  out->numDirectories = in->numDirectories;
  out->directoryList = in->directoryList;

  if (fe->save_data.saving && out->numDirectories < SAVE_LIST_MAX_DIRECTORIES) {
    sysSaveNewSaveGame *new_save = &fe->save_data.new_save;
    sysSaveNewSaveGameIcon *new_save_icon = &fe->save_data.new_save_icon;
    char *dir = malloc (SYS_SAVE_MAX_DIRECTORY_NAME + 1);
    int idx = -1;
    int j;

    for (i = 0; idx == -1 && i <= 99; i++) {
      snprintf (dir, SYS_SAVE_MAX_DIRECTORY_NAME, "%s%d",
          fe->save_data.prefix, i);
      idx = i;
      for (j = 0; j < in->numDirectories; j++) {
        if (strcmp (dir, in->directoryList[j].directoryName) == 0) {
          idx = -1;
          break;
        }
      }
    }

    if (idx != -1) {
      printf ("New directory : %s\n", dir);
      memset (new_save, 0, sizeof(sysSaveNewSaveGame));
      memset (new_save_icon, 0, sizeof(sysSaveNewSaveGameIcon));
      new_save->position = SYS_SAVE_NEW_SAVE_POSITION_TOP;
      new_save->directoryName = dir;
      if (fe->save_data.screenshot_size > 0) {
        new_save->icon = new_save_icon;
        new_save_icon->iconBufferSize = fe->save_data.icon_size;
        new_save_icon->iconBuffer = fe->save_data.icon_data;
      }
      out->newSaveGame = new_save;
    } else {
      free (dir);
    }
  }

  result->result = SYS_SAVE_CALLBACK_RESULT_CONTINUE;
  return;
}

void
saveload_game_status_cb (sysSaveCallbackResult *result,
    sysSaveStatusIn *in, sysSaveStatusOut *out)
{
  frontend *fe = result->user_data;
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
        in->fileList[i].fileType,
        in->fileList[i].fileSize,
        in->fileList[i].filename);

  result->result = SYS_SAVE_CALLBACK_RESULT_CONTINUE;
  out->setParam = &in->getParam;

  if (fe->save_data.loading) {
    out->recreateMode = SYS_SAVE_RECREATE_MODE_OVERWRITE_NOT_CORRUPTED;
    fe->save_data.mode = PS3_SAVE_MODE_DATA;
    fe->save_data.save_size = 0;
    for (i = 0; i < in->numFiles; i++) {
      switch (in->fileList[i].fileType) {
        case SYS_SAVE_FILETYPE_STANDARD_FILE:
          fe->save_data.save_size = in->fileList[i].fileSize;
          break;
        case SYS_SAVE_FILETYPE_CONTENT_ICON0:
          fe->save_data.icon_size = in->fileList[i].fileSize;
          break;
        case SYS_SAVE_FILETYPE_CONTENT_PIC1:
          fe->save_data.screenshot_size = in->fileList[i].fileSize;
          break;
        default:
          break;
      }
    }
    if (fe->save_data.save_size == 0) {
      printf ("Couldn't find the save data.. !\n");
      result->result = SYS_SAVE_CALLBACK_RESULT_CORRUPTED;
      return;
    } else {
      printf ("Found save game data of size : %lu\n", fe->save_data.save_size);
      fe->save_data.save_data = malloc (fe->save_data.save_size);
    }
  } else {
    out->recreateMode = SYS_SAVE_RECREATE_MODE_DELETE;
    fe->save_data.mode = PS3_SAVE_MODE_ICON;

    /* TODO: Check for availability of free space */

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

}

void
saveload_game_file_cb (sysSaveCallbackResult *result,
    sysSaveFileIn *in, sysSaveFileOut *out)
{
  frontend *fe = result->user_data;

  printf ("saveload_game_file_cb called\n");

  printf ("Last option %s %d bytes\n", fe->save_data.saving? "wrote" : "read",
      in->previousOperationResultSize);

  memset (out, 0, sizeof(sysSaveFileOut));
  switch (fe->save_data.mode) {
    case PS3_SAVE_MODE_ICON:
      {
        printf ("Saving icon\n");

        /* The screenshot doesn't appear in the load screen, so we need to save
           it as the icon too... */
        out->fileOperation = SYS_SAVE_FILE_OPERATION_WRITE;
        out->fileType = SYS_SAVE_FILETYPE_CONTENT_ICON0;
        out->size = fe->save_data.screenshot_size;
        out->bufferSize = fe->save_data.screenshot_size;
        out->buffer = fe->save_data.screenshot_data;

        result->result = SYS_SAVE_CALLBACK_RESULT_CONTINUE;
        result->incrementProgress = 30;
        fe->save_data.mode = PS3_SAVE_MODE_SCREENSHOT;
        break;
      }
    case PS3_SAVE_MODE_SCREENSHOT:
      {
        printf ("Saving screenshot\n");

        out->fileOperation = SYS_SAVE_FILE_OPERATION_WRITE;
        out->fileType = SYS_SAVE_FILETYPE_CONTENT_PIC1;
        out->size = fe->save_data.screenshot_size;
        out->bufferSize = fe->save_data.screenshot_size;
        out->buffer = fe->save_data.screenshot_data;

        result->result = SYS_SAVE_CALLBACK_RESULT_CONTINUE;
        result->incrementProgress = 30;
        fe->save_data.mode = PS3_SAVE_MODE_DATA;
        break;
      }
    case PS3_SAVE_MODE_DATA:
      {
        if (fe->save_data.saving) {
          printf ("Writing game data\n");
          out->fileOperation = SYS_SAVE_FILE_OPERATION_WRITE;
        } else {
          printf ("Reading game data\n");
          out->fileOperation = SYS_SAVE_FILE_OPERATION_READ;
        }

        out->filename = SAVE_DATA_FILENAME;
        out->fileType = SYS_SAVE_FILETYPE_STANDARD_FILE;
        out->size = fe->save_data.save_size;
        out->bufferSize = fe->save_data.save_size;
        out->buffer = fe->save_data.save_data;

        result->result = SYS_SAVE_CALLBACK_RESULT_CONTINUE;
        result->incrementProgress = 100;
        fe->save_data.mode = PS3_SAVE_MODE_DONE;
        break;
      }
    case PS3_SAVE_MODE_DONE:
    default:
      result->result = SYS_SAVE_CALLBACK_RESULT_DONE;
      if (fe->save_data.loading) {
        if (in->previousOperationResultSize != fe->save_data.save_size) {
          result->result = SYS_SAVE_CALLBACK_RESULT_CORRUPTED;
        } else {
          char *error = NULL;

          error = midend_deserialise(fe->me, read_game_data_from_buffer, fe);
          if (error != NULL) {
            result->result = SYS_SAVE_CALLBACK_RESULT_ERROR_CUSTOM;
            result->customErrorMessage = error;
          }
        }
      }
      break;
  }

  printf ("saveload_game_file_cb exit\n");
}

static int
load_file (const char *filename, u8 **data, u64 *size)
{
  FILE *fd = fopen (filename, "rb");

  if (fd == NULL)
    return FALSE;

  fseek (fd, 0, SEEK_END);
  *size = ftell (fd);
  fseek (fd, 0, SEEK_SET);

  *data = malloc (*size);
  fread (*data, *size, 1, fd);
  fclose (fd);

  return TRUE;
}

static cairo_status_t
write_png_to_buffer (void *closure, unsigned char *data, unsigned int length)
{
  frontend *fe = closure;

  printf ("Serializing screenshot of size : %d\n", length);
  fe->save_data.screenshot_data = realloc (fe->save_data.screenshot_data,
      fe->save_data.screenshot_size + length);
  memcpy (fe->save_data.screenshot_data + fe->save_data.screenshot_size,
      data, length);
  fe->save_data.screenshot_size += length;

  return CAIRO_STATUS_SUCCESS;
}

static void
write_game_data_to_buffer (void *ctx, void *buf, int len)
{
  frontend *fe = ctx;

  printf ("Serializing data of size : %d\n", len);
  fe->save_data.save_data = realloc (fe->save_data.save_data,
      fe->save_data.save_size + len);
  memcpy (fe->save_data.save_data + fe->save_data.save_size, buf, len);
  fe->save_data.save_size += len;

}

static int
read_game_data_from_buffer (void *ctx, void *buf, int len)
{
  frontend *fe = ctx;

  printf ("Deserializing data of size : %d\n", len);

  if (fe->save_data.deserialize_offset + len > fe->save_data.save_size)
    return FALSE;

  memcpy (buf, fe->save_data.save_data + fe->save_data.deserialize_offset, len);
  fe->save_data.deserialize_offset += len;

  return TRUE;
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
  char filename[256];
  s32 ret;

  printf ("saveload_thread started\n");

  snprintf (prefix, SYS_SAVE_MAX_DIRECTORY_NAME, "SGTPUZZLES-%s",
      gamelist_names[fe->game_idx]);

  /* Filename must be upper case, so let's uppercase it */
  while (*ptr != 0) {
    *ptr = islower ((int)*ptr) ? *ptr - 'a' + 'A' : *ptr;
    ptr++;
  }

  memset (&listSettings, 0, sizeof (sysSaveListSettings));
  listSettings.sortType = SYS_SAVE_SORT_TYPE_TIMESTAMP;
  listSettings.sortOrder = SYS_SAVE_SORT_ORDER_DESCENDING;
  listSettings.pathPrefix = fe->save_data.prefix;
  listSettings.reserved = NULL;

  memset (&bufferSettings, 0, sizeof (sysSaveBufferSettings));
  bufferSettings.maxDirectories = SAVE_LIST_MAX_DIRECTORIES;
  bufferSettings.maxFiles = SAVE_LIST_MAX_FILES;
  bufferSettings.bufferSize = BUFFER_SETTINGS_BUFSIZE;
  bufferSettings.buffer = malloc (bufferSettings.bufferSize);

  if (sysMemContainerCreate (&container, MEMORY_CONTAINER_SIZE) != 0 ) {
    printf ("Unable to create memory container\n");
    goto end;
  }

  if (fe->save_data.saving) {
    printf ("Load icon\n");
    snprintf (filename, 255, "%s/data/puzzles/%s.png", cwd,
        gamelist_names[fe->game_idx]);
    load_file (filename, &fe->save_data.icon_data, &fe->save_data.icon_size);

    printf ("Taking screenshot\n");
    cairo_surface_write_to_png_stream (fe->image,
        (cairo_write_func_t) write_png_to_buffer, fe);

    printf ("Serializing data\n");
    midend_serialise(fe->me, write_game_data_to_buffer, fe);

    ret = sysSaveListSave2 (SYS_SAVE_CURRENT_VERSION,
        &listSettings, &bufferSettings,
        saveload_game_list_cb, saveload_game_status_cb, saveload_game_file_cb,
        container, fe);
  } else {
    ret = sysSaveListLoad2 (SYS_SAVE_CURRENT_VERSION,
        &listSettings, &bufferSettings,
        saveload_game_list_cb, saveload_game_status_cb, saveload_game_file_cb,
        container, fe);
  }

  fe->save_data.result = ret;

  printf ("sysSaveListLoad2/Save2 returned : %d\n", ret);
  sysMemContainerDestroy (container);

end:
  if (bufferSettings.buffer)
    free (bufferSettings.buffer);
  if (fe->save_data.new_save.directoryName)
    free (fe->save_data.new_save.directoryName);
  if (fe->save_data.save_data)
    free (fe->save_data.save_data);
  if (fe->save_data.icon_data)
    free (fe->save_data.icon_data);
  if (fe->save_data.screenshot_data)
    free (fe->save_data.screenshot_data);

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

  memset (&fe->save_data, 0, sizeof(SaveData));
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
