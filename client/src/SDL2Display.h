#ifdef __USE_SDL2_VIDEO__

#ifndef NETWORK_DISPLAY_CLIENT_SDL2DISPLAY_H
#define NETWORK_DISPLAY_CLIENT_SDL2DISPLAY_H

#include <SDL2/SDL.h>


class SDL2Display {
public :
  SDL2Display(uint16_t screenHeight, uint16_t screenWidth) {
    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

    uint16_t flags =  SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_RESIZABLE| SDL_WINDOW_SHOWN;

    // Create an application window with the following settings:
    mScreen = SDL_CreateWindow(
      "Network Display",         // window title
      SDL_WINDOWPOS_UNDEFINED,   // initial resources position
      SDL_WINDOWPOS_UNDEFINED,   // initial y position
      screenWidth * 2,          // Width
      screenHeight * 2,         // Height
      flags                      // flags - see below
    );

    SDL_SetWindowMinimumSize(mScreen, screenWidth * 2, screenHeight * 2);

    // Check that the window was successfully created
    if (mScreen == nullptr) {
      // In the case that the window could not be made...
      printf("Could not create window: %s\n", SDL_GetError());
      exit(1);
    }

    mRenderer = SDL_CreateRenderer(mScreen, -1, 0);
    if (! mRenderer) {
      printf("Cannot create mRenderer %s\n", SDL_GetError());
      exit(1);
    }

    SDL_RenderSetLogicalSize(mRenderer, screenWidth, screenHeight);
    SDL_RenderSetIntegerScale(mRenderer, SDL_TRUE);

    mTexture = SDL_CreateTexture(
      mRenderer, ///
      SDL_PIXELFORMAT_ARGB8888,
      SDL_TEXTUREACCESS_STREAMING,
      screenWidth,
      screenHeight
    );


    if (! mTexture) {
      printf("Cannot create mTexture %s\n", SDL_GetError());
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
    SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 0);
    SDL_RenderClear(mRenderer);
    SDL_GL_SetSwapInterval(1);

    SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 255);
    SDL_RenderClear(mRenderer);
    SDL_RenderPresent(mRenderer);

    // try to move window, to fix SDL2 bug on MacOS (Mojave)
    int x, y;
    SDL_GetWindowPosition(mScreen, &x, &y);
    SDL_SetWindowPosition(mScreen, x+1, y+1);
  }
  ~SDL2Display() {
    // Close and destroy the window
    SDL_DestroyTexture(mTexture);
    SDL_DestroyRenderer(mRenderer);
    SDL_DestroyWindow(mScreen);

    // Clean up
    SDL_Quit();
  }

  void Update(uint16_t *pixels, uint16_t totalBytes) {
    // try to move window, to fix SDL2 bug on MacOS (Mojave)
    if (!hackInitialized){
      int x, y;
      SDL_GetWindowPosition(mScreen, &x, &y);
      SDL_SetWindowPosition(mScreen, x+1, y+1);
      SDL_SetWindowPosition(mScreen, x, y);
      hackInitialized = true;
    }

    void *screenBuf;
    uint16_t pitch;

    if (0 == SDL_LockTexture(mTexture, nullptr, &screenBuf, &pitch)) {
      memcpy(screenBuf, pixels, totalBytes);
      SDL_UnlockTexture(mTexture);
    }
    else {
      printf("Can't lock mTexture (%s)\n", SDL_GetError());
    }

    SDL_SetRenderDrawColor(mRenderer, 0, 0, 0, 0);
    SDL_RenderClear(mRenderer);
    SDL_RenderCopy(mRenderer, mTexture, nullptr, nullptr); // Render mTexture to entire window
    SDL_RenderPresent(mRenderer);              // Do update
  }

private:
  SDL_Window   *mScreen   = nullptr;
  SDL_Renderer *mRenderer = nullptr;
  SDL_Texture  *mTexture  = nullptr;
  bool hackInitialized = false;
};


#endif //NETWORK_DISPLAY_CLIENT_SDL2DISPLAY_H


#endif