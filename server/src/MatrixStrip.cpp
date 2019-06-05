#include "MatrixStrip.h"



MatrixStrip::MatrixStrip(RGBMatrix *m) : ThreadedCanvasManipulator(m), mMatrix(m)  {
#ifdef __linux__
  pthread_mutex_destroy(&mMutex);
  pthread_mutex_init(&mMutex, nullptr);
#endif


  mShouldRun = true;

  mCanvas1 = m->CreateFrameCanvas();
  mRenderCanvas = mCanvas1;

  mCanvas2 = m->CreateFrameCanvas();
  mDisplayCanvas = mCanvas2;

  mCanvasWidth = mRenderCanvas->width();
  mCanvasHeight = mRenderCanvas->height();
  mFrameCount = 0;

  Describe();
  fflush(stdout);
}

void MatrixStrip::Run() {

  volatile uint32_t currentFrameCount = 0;

  while (running() && mShouldRun) {
    if(mFrameCount != currentFrameCount) {
      SwapBuffers();
    }

    mDisplayCanvas = mMatrix->SwapOnVSync(mDisplayCanvas);
  }

  printf("MatrixStrip::Run() ended!!\n");
}

void MatrixStrip::Describe() {
  printf("MatrixStrip %p\n", this);

  printf("\tmTotalPixels = %lu\n", mTotalPixels);
  printf("\tmCanvasWidth = %i\n", mCanvasWidth);
  printf("\tmCanvasHeight = %i\n", mCanvasHeight);
}