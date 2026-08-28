#ifndef WDR_BGM_DATA_H
#define WDR_BGM_DATA_H

#include <stddef.h>
#include <stdint.h>

typedef struct WdrBgmSong {
  const uint8_t *data;
  size_t size;
} WdrBgmSong;

#ifdef WDR_PRIVATE_BGM
#include "bgmpriv.h"

// ndpalbum.py sorts the selected sources by their original file names.
static const WdrBgmSong wdr_bgm_songs[] = {
  {ndp_album_song_2, sizeof(ndp_album_song_2)},  // NDP12-1 TITLE / DEMO
  {ndp_album_song_4, sizeof(ndp_album_song_4)},  // NDP13-2 COURSE / HOW TO
  {ndp_album_song_1, sizeof(ndp_album_song_1)},  // NDP11-4 GAME
  {ndp_album_song_0, sizeof(ndp_album_song_0)},  // NDP10-1 FINAL LAP
  {ndp_album_song_3, sizeof(ndp_album_song_3)},  // NDP12-2 RESULT
};

#define WDR_BGM_AVAILABLE 1
#else
static const WdrBgmSong wdr_bgm_songs[] = {{0, 0}};
#define WDR_BGM_AVAILABLE 0
#endif

#define WDR_BGM_SONG_COUNT 5

#endif
