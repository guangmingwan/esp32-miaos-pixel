/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
/*
 * Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
 * Copyright (c) 2011-2026, SDLPAL development team.
 * All rights reserved.
 *
 * This file is part of SDLPAL.
 *
 * SDLPAL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"
#ifndef __LAVA__
#include <setjmp.h>
#endif

#ifndef __LAVA__
#if SDL_VERSION_ATLEAST(3,0,0)
#if __EMSCRIPTEN__
#define SDL_MAIN_HANDLED
#endif
    #include <SDL3/SDL_main.h>
#endif
#endif

#ifndef __LAVA__
#if defined(PAL_HAS_GIT_REVISION)
# undef PAL_GIT_REVISION
# include "generated.h"
#endif
#endif

#ifndef __LAVA__
static jmp_buf g_exit_jmp_buf;
#endif

int g_exit_code = 0;

char gExecutablePath[PAL_MAX_PATH];

#ifdef __LAVA__
#define BITMAPNUM_SPLASH_UP         (gConfig.fIsWIN95 ? 0x03 : 0x26)
#define BITMAPNUM_SPLASH_DOWN       (gConfig.fIsWIN95 ? 0x04 : 0x27)
#else
#define BITMAPNUM_SPLASH_UP         (gConfig.fIsWIN95 ? 0x03 : 0x26)
#define BITMAPNUM_SPLASH_DOWN       (gConfig.fIsWIN95 ? 0x04 : 0x27)
#endif
#define SPRITENUM_SPLASH_TITLE      0x47
#define SPRITENUM_SPLASH_CRANE      0x49
#define NUM_RIX_TITLE               0x05

VOID
PAL_Init(
   VOID
)
{
   int           e;
#if PAL_HAS_GIT_REVISION
   UTIL_LogOutput(LOGLEVEL_DEBUG, "SDLPal build revision: %s\n", PAL_GIT_REVISION);
#endif

   e = PAL_InitGlobals();
   if (e != 0)
   {
#ifdef __LAVA__
	   TerminateOnError("Could not initialize global data.\n");
#else
	   TerminateOnError("Could not initialize global data: %d.\n", e);
#endif
   }

   e = VIDEO_Startup();
   if (e != 0)
   {
      TerminateOnError("Could not initialize Video: %d.\n", e);
   }

   VIDEO_SetWindowTitle("Loading...");

   e = PAL_InitUI();
   if (e != 0)
   {
      TerminateOnError("Could not initialize UI subsystem: %d.\n", e);
   }

   e = PAL_InitText();
   if (e != 0)
   {
      TerminateOnError("Could not initialize text subsystem: %d.\n", e);
   }

   e = PAL_InitFont(&gConfig);
   if (e != 0)
   {
      TerminateOnError("Could not load fonts: %d.\n", e);
   }

   PAL_InitInput();
   PAL_InitResources();
   AUDIO_OpenDevice();
   PAL_AVIInit();

#ifdef __LAVA__
   VIDEO_SetWindowTitle("Pal");
#else
   VIDEO_SetWindowTitle((char *)UTIL_va(UTIL_GlobalBuffer(0), PAL_GLOBAL_BUFFER_SIZE,
	   "Pal %s%s%s%s",
	   gConfig.fIsWIN95 ? "Win95" : "DOS",
#if defined(_DEBUG) || defined(DEBUG)
	   " (Debug) ",
#else
	   "",
#endif
#if defined(PAL_HAS_GIT_REVISION) && defined(PAL_GIT_REVISION)
	   " ["  PAL_GIT_REVISION "] "
#else
	   ""
#endif
       ,(gConfig.fEnableGLSL && gConfig.pszShader ? (char *)gConfig.pszShader : "")
   ));
#endif
}

VOID
PAL_Shutdown(
   int exit_code
)
{
   AUDIO_CloseDevice();
   PAL_AVIShutdown();
   PAL_FreeFont();
   PAL_FreeResources();
   PAL_FreeUI();
   PAL_FreeText();
   PAL_ShutdownInput();
   VIDEO_Shutdown();

   PAL_FreeGlobals();

   g_exit_code = exit_code;
#if defined(__LAVA__) && defined(LAVA_NATIVE_COMPILED)
   g_lava_shutdown_requested = 1;
   g_user_thread_done = 1;
   return;
#elif defined(__LAVA__)
   exit(exit_code);
#else
#if !__EMSCRIPTEN__ && SDL_MAJOR_VERSION < 3
   longjmp(g_exit_jmp_buf, 1);
#else
   SDL_Quit();
   UTIL_Platform_Quit();
#if !__WINRT__ && !__IOS__
   exit(0);
#endif
   return;
#endif
#endif
}

VOID
PAL_TrademarkScreen(
   VOID
)
{
#ifdef __LAVA__
   if (g_lava_autotest_search || g_lava_autotest_walk || g_lava_autotest_exits || g_lava_autotest_scene5 || g_lava_autotest_scene6 || g_lava_autotest_scene13 || g_lava_autotest_scene9 || g_lava_autotest_hooks || g_lava_autotest_door || g_lava_autotest_hall || g_lava_autotest_kitchen || g_lava_autotest_load || g_lava_autotest_status || g_lava_autotest_input || g_lava_autotest_intro || g_lava_autotest_battle || g_lava_autotest_fengshen || g_lava_autotest_bsilence || g_lava_autotest_bsleep || g_lava_autotest_op48 || g_lava_autotest_xianling || g_lava_autotest_obj204)
   {
      return;
   }
#endif
   if (PAL_PlayAVI("1.avi")) return;

   PAL_SetPalette(3, FALSE);
   g_lava_dbg_trademark_rng_ok = PAL_RNGPlay(6, 0, -1, 25) ? 1 : 0;
   printf("[LAVA][TRADEMARK] rng=%d\n", g_lava_dbg_trademark_rng_ok);
   UTIL_Delay(1000);
   PAL_FadeOut(1);
}

VOID
PAL_SplashScreen(
   VOID
)
{
#ifdef __LAVA__
      if (g_lava_autotest_search || g_lava_autotest_walk || g_lava_autotest_exits || g_lava_autotest_scene5 || g_lava_autotest_scene6 || g_lava_autotest_scene13 || g_lava_autotest_scene9 || g_lava_autotest_hooks || g_lava_autotest_door || g_lava_autotest_hall || g_lava_autotest_kitchen || g_lava_autotest_load || g_lava_autotest_status || g_lava_autotest_input || g_lava_autotest_intro || g_lava_autotest_battle || g_lava_autotest_fengshen || g_lava_autotest_bsilence || g_lava_autotest_bsleep || g_lava_autotest_op48 || g_lava_autotest_xianling || g_lava_autotest_obj204)
     {
        return;
     }
      {
        long ret_up;
        long ret_down;
        int ret_title_read;
        long ret_title_dec;
        int ret_crane_read;
        long ret_crane_dec;
        FILE *fp_fbp;
        FILE *fp_mgo;
        addr src;
        addr title;
        addr crane;
       LPCBITMAPRLE title_frame;
       int title_height;
       int crane_pos[9][3];
       int i;
       int img_pos;
       int crane_frame;
       int frame;

    if (PAL_PlayAVI("2.avi")) return;
    PAL_SetPalette(1, FALSE);
    ret_up = -999;
    ret_down = -999;
    ret_title_read = -999;
    ret_title_dec = -999;
    ret_crane_read = -999;
    ret_crane_dec = -999;

    fp_fbp = UTIL_OpenFile("FBP.MKF");
    if (fp_fbp != 0)
    {
       ret_up = PAL_MKFDecompressChunk((addr)g_lava_fbp_buf, 64000, BITMAPNUM_SPLASH_UP, fp_fbp);
       if (PAL_LavaDecompressOK(ret_up, 64000))
       {
           memcpy((addr)g_lava_map_tiles_buf, (addr)g_lava_fbp_buf, 64000);
       }
       ret_down = PAL_MKFDecompressChunk((addr)g_lava_fbp_buf, 64000, BITMAPNUM_SPLASH_DOWN, fp_fbp);
       fclose(fp_fbp);
    }
    printf("[LAVA][SPLASH] up=%d down=%d\n",
       PAL_LavaDecompressOK(ret_up, 64000) ? 1 : 0,
       PAL_LavaDecompressOK(ret_down, 64000) ? 1 : 0);

    fp_mgo = UTIL_OpenRequiredFile("MGO.MKF");
    if (fp_mgo != 0)
    {
       src = g_lava_mkf_buf;
       title = (addr)g_lava_sprite_buf;
       crane = (addr)(g_lava_sprite_buf + 32000);
       ret_title_read = PAL_MKFReadChunk(src, 32000, SPRITENUM_SPLASH_TITLE, fp_mgo);
       ret_title_dec = ret_title_read > 0 ? Decompress(src, title, 32000) : -999;
       ret_crane_read = PAL_MKFReadChunk(src, 32000, SPRITENUM_SPLASH_CRANE, fp_mgo);
       ret_crane_dec = ret_crane_read > 0 ? Decompress(src, crane, 32000) : -999;
       fclose(fp_mgo);
    }
    printf("[LAVA][SPLASH] title=%d crane=%d\n", ret_title_dec > 0 ? 1 : 0, ret_crane_dec > 0 ? 1 : 0);

    if (PAL_LavaDecompressOK(ret_down, 64000))
    {
       g_lava_dbg_splash_fbp_ok = 1;
    }
    if (PAL_LavaDecompressOK(ret_title_dec, 32000) && PAL_LavaDecompressOK(ret_crane_dec, 32000))
    {
       g_lava_dbg_splash_mgo_ok = 1;
    }

    if (PAL_LavaDecompressOK(ret_up, 64000) &&
       PAL_LavaDecompressOK(ret_down, 64000) &&
       PAL_LavaDecompressOK(ret_title_dec, 32000) &&
       PAL_LavaDecompressOK(ret_crane_dec, 32000))
    {
       title_frame = PAL_SpriteGetFrame(title, 0);
       title_height = PAL_RLEGetHeight(title_frame);
       title_frame[2] = 0;
       title_frame[3] = 0;
       for (i = 0; i < 9; i++)
       {
          crane_pos[i][0] = RandomLong(300, 600);
          crane_pos[i][1] = RandomLong(0, 80);
          crane_pos[i][2] = RandomLong(0, 8);
       }

       img_pos = 200;
       crane_frame = 0;
       for (frame = 0; frame < 260; frame++)
       {
          int j;
          int h;
          int w;

          if (img_pos > 1)
          {
             img_pos--;
          }

          h = 200 - img_pos;
          if (h > 0)
          {
              PAL_FBPBlitRectToSurface((addr)g_lava_map_tiles_buf, gpScreen, img_pos, 0, h);
          }
          if (img_pos > 0)
          {
             PAL_FBPBlitRectToSurface((addr)g_lava_fbp_buf, gpScreen, 0, 200 - img_pos, img_pos);
          }

          for (j = 0; j < 9; j++)
          {
             crane_pos[j][2] = (crane_pos[j][2] + (crane_frame & 1)) % PAL_SpriteGetNumFrames(crane);
             PAL_RLEBlitToSurface((LPCBITMAPRLE)PAL_SpriteGetFrame(crane, crane_pos[j][2]), gpScreen,
                PAL_XY(crane_pos[j][0], crane_pos[j][1]));
             crane_pos[j][0]--;
             if (img_pos > 1 && (img_pos & 1))
             {
                crane_pos[j][1]++;
             }
          }
          crane_frame++;

          w = title_frame[2] | (title_frame[3] << 8);
          if (w < title_height)
          {
             w++;
             title_frame[2] = (w & 0xFF);
             title_frame[3] = (w >> 8);
          }
          PAL_RLEBlitToSurface(title_frame, gpScreen, PAL_XY(255, 10));
          VIDEO_UpdateScreen(0);
          if (PAL_LavaDelaySkippable(35))
          {
             break;
          }
        }
        PAL_LavaDelaySkippable(800);
     }
     else if (PAL_LavaDecompressOK(ret_down, 64000))
     {
        PAL_FBPBlitToSurface((addr)g_lava_fbp_buf, gpScreen);
        VIDEO_UpdateScreen(0);
        PAL_LavaDelaySkippable(1200);
     }

    PAL_FadeOut(1);
    return;
    }
#else
   SDL_Color     *palette = (SDL_Color *)PAL_GetPalette(1, FALSE);
   SDL_Color      rgCurrentPalette[256];
   SDL_Surface   *lpBitmapDown, *lpBitmapUp;
   SDL_Rect       srcrect, dstrect;
   LPSPRITE       lpSpriteCrane;
   LPBITMAPRLE    lpBitmapTitle;
   LPBYTE         buf;
   LPBYTE         buf2;
   int            cranepos[9][3], i, iImgPos = 200, iCraneFrame = 0, iTitleHeight;
   DWORD          dwTime, dwBeginTime;
   BOOL           fUseCD = TRUE;

   memset(rgCurrentPalette, 0xff, sizeof(SDL_Color) * 256);

   if (PAL_PlayAVI("2.avi")) return;

   if (palette == NULL)
   {
      fprintf(stderr, "ERROR: PAL_SplashScreen(): palette == NULL\n");
      return;
   }

   buf = (LPBYTE)UTIL_calloc(1, 320 * 200 * 2);
   buf2 = (LPBYTE)(buf + 320 * 200);
   lpSpriteCrane = (LPSPRITE)buf2 + 32000;

   lpBitmapDown = (SDL_Surface *)VIDEO_CreateCompatibleSurface(gpScreen);
   lpBitmapUp = (SDL_Surface *)VIDEO_CreateCompatibleSurface(gpScreen);

   PAL_MKFReadChunk(buf, 320 * 200, BITMAPNUM_SPLASH_UP, gpGlobals->f.fpFBP);
   Decompress(buf, buf2, 320 * 200);
   PAL_FBPBlitToSurface(buf2, lpBitmapUp);
   PAL_MKFReadChunk(buf, 320 * 200, BITMAPNUM_SPLASH_DOWN, gpGlobals->f.fpFBP);
   Decompress(buf, buf2, 320 * 200);
   PAL_FBPBlitToSurface(buf2, lpBitmapDown);
   PAL_MKFReadChunk(buf, 32000, SPRITENUM_SPLASH_TITLE, gpGlobals->f.fpMGO);
   Decompress(buf, buf2, 32000);
   lpBitmapTitle = (LPBITMAPRLE)PAL_SpriteGetFrame(buf2, 0);
   PAL_MKFReadChunk(buf, 32000, SPRITENUM_SPLASH_CRANE, gpGlobals->f.fpMGO);
   Decompress(buf, lpSpriteCrane, 32000);

   iTitleHeight = PAL_RLEGetHeight(lpBitmapTitle);
   lpBitmapTitle[2] = 0;
   lpBitmapTitle[3] = 0;

   for (i = 0; i < 9; i++)
   {
      cranepos[i][0] = RandomLong(300, 600);
      cranepos[i][1] = RandomLong(0, 80);
      cranepos[i][2] = RandomLong(0, 8);
   }

   if (!AUDIO_PlayCDTrack(7))
   {
      fUseCD = FALSE;
      AUDIO_PlayMusic(NUM_RIX_TITLE, TRUE, 2);
   }

   PAL_ProcessEvent();
   PAL_ClearKeyState();

   dwBeginTime = SDL_GetTicks();

   srcrect.x = 0;
   srcrect.w = 320;
   dstrect.x = 0;
   dstrect.w = 320;

   while (TRUE)
   {
      PAL_ProcessEvent();
      dwTime = SDL_GetTicks() - dwBeginTime;

      if (dwTime < 15000)
      {
         for (i = 0; i < 256; i++)
         {
            rgCurrentPalette[i].r = (BYTE)(palette[i].r * ((float)dwTime / 15000));
            rgCurrentPalette[i].g = (BYTE)(palette[i].g * ((float)dwTime / 15000));
            rgCurrentPalette[i].b = (BYTE)(palette[i].b * ((float)dwTime / 15000));
         }
      }

      VIDEO_SetPalette(rgCurrentPalette);
      VIDEO_UpdateSurfacePalette(lpBitmapDown);
      VIDEO_UpdateSurfacePalette(lpBitmapUp);

      if (iImgPos > 1)
      {
         iImgPos--;
      }

      srcrect.y = iImgPos;
      srcrect.h = 200 - iImgPos;

      dstrect.y = 0;
      dstrect.h = srcrect.h;

	  VIDEO_CopySurface(lpBitmapUp, &srcrect, gpScreen, &dstrect);

      srcrect.y = 0;
      srcrect.h = iImgPos;

      dstrect.y = 200 - iImgPos;
      dstrect.h = srcrect.h;

	  VIDEO_CopySurface(lpBitmapDown, &srcrect, gpScreen, &dstrect);

      for (i = 0; i < 9; i++)
      {
         LPCBITMAPRLE lpFrame = (LPCBITMAPRLE)PAL_SpriteGetFrame(lpSpriteCrane,
            cranepos[i][2] = (cranepos[i][2] + (iCraneFrame & 1)) % 8);
         cranepos[i][1] += ((iImgPos > 1) && (iImgPos & 1)) ? 1 : 0;
         PAL_RLEBlitToSurface(lpFrame, gpScreen,
            PAL_XY(cranepos[i][0], cranepos[i][1]));
         cranepos[i][0]--;
      }
      iCraneFrame++;

      if (PAL_RLEGetHeight(lpBitmapTitle) < iTitleHeight)
      {
         WORD w = lpBitmapTitle[2] | (lpBitmapTitle[3] << 8);
         w++;
         lpBitmapTitle[2] = (w & 0xFF);
         lpBitmapTitle[3] = (w >> 8);
      }

      PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));
      VIDEO_UpdateScreen(NULL);

      if (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch))
      {
         lpBitmapTitle[2] = iTitleHeight & 0xFF;
         lpBitmapTitle[3] = iTitleHeight >> 8;

         PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));

         VIDEO_UpdateScreen(NULL);

         if (dwTime < 15000)
         {
            while (dwTime < 15000)
            {
               for (i = 0; i < 256; i++)
               {
                  rgCurrentPalette[i].r = (BYTE)(palette[i].r * ((float)dwTime / 15000));
                  rgCurrentPalette[i].g = (BYTE)(palette[i].g * ((float)dwTime / 15000));
                  rgCurrentPalette[i].b = (BYTE)(palette[i].b * ((float)dwTime / 15000));
               }
               VIDEO_SetPalette(rgCurrentPalette);
               VIDEO_UpdateSurfacePalette(lpBitmapDown);
               VIDEO_UpdateSurfacePalette(lpBitmapUp);
               UTIL_Delay(8);
               dwTime += 250;
            }
            UTIL_Delay(500);
         }

         break;
      }

      PAL_ProcessEvent();
      while (SDL_GetTicks() - dwBeginTime < dwTime + 85)
      {
         SDL_Delay(1);
         PAL_ProcessEvent();
      }
   }

   VIDEO_FreeSurface(lpBitmapDown);
   VIDEO_FreeSurface(lpBitmapUp);
   free(buf);

   if (!fUseCD)
   {
      AUDIO_PlayMusic(0, FALSE, 1);
   }

   PAL_FadeOut(1);
#endif
}

#ifndef __LAVA__
static int
PAL_MainEntry(
   int      argc,
   char    *argv[]
)
{
#if !defined( __EMSCRIPTEN__ ) && !defined(__WINRT__) && !defined(__N3DS__)
   memset(gExecutablePath,0,PAL_MAX_PATH);
   if (argv[0] != NULL)
   {
      strncpy(gExecutablePath, argv[0], PAL_MAX_PATH - 1);
      gExecutablePath[PAL_MAX_PATH - 1] = '\0';
   }
#endif

#if PAL_HAS_PLATFORM_STARTUP
   UTIL_Platform_Startup(argc,argv);
#endif

#if !__EMSCRIPTEN__ && SDL_MAJOR_VERSION < 3
   if (setjmp(g_exit_jmp_buf) != 0)
   {
	   SDL_Quit();
	   UTIL_Platform_Quit();
	   return g_exit_code;
   }
#endif

#if !defined(UNIT_TEST) || defined(UNIT_TEST_GAME_INIT)
   if (SDL_Init(PAL_SDL_INIT_FLAGS) == SDL_FAIL)
   {
	   TerminateOnError("Could not initialize SDL: %s.\n", SDL_GetError());
   }

   PAL_LoadConfig(TRUE);

   if (UTIL_Platform_Init(argc, argv) != 0)
	   return -1;

   if (PAL_HAS_CONFIG_PAGE && gConfig.fLaunchSetting)
	   return 0;

   if (gConfig.pszLogFile)
	   UTIL_LogAddOutputCallback(UTIL_LogToFile, gConfig.iLogLevel);

   PAL_Init();
#endif

#if !defined(UNIT_TEST)
   PAL_TrademarkScreen();
   PAL_SplashScreen();

   PAL_GameMain();

#ifndef __LAVA__
   assert(FALSE);
   return 255;
#else
   return g_exit_code;
#endif
#else
   extern int testmain(int argc, char *argv[]);
   return testmain(argc, argv);
#endif
}
#endif

#ifdef __LAVA__
void
#else
int
#endif
main(
#ifdef __LAVA__
   VOID
#else
   int      argc,
   char    *argv[]
#endif
)
{
#ifdef __LAVA__
   PAL_Init();
   PAL_TrademarkScreen();
   PAL_SplashScreen();
   PAL_GameMain();
#else
   return PAL_MainEntry(argc, argv);
#endif
}
