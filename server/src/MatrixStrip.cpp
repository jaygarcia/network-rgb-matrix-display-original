#include "MatrixStrip.h"



MatrixStrip::MatrixStrip(RGBMatrix *m) : ThreadedCanvasManipulator(m), mMatrix(m)  {
#ifdef __linux__
  pthread_mutex_destroy(&mMutex);
  pthread_mutex_init(&mMutex, nullptr);
#endif


  mShouldRun = true;

  mOffScreenCanvas = m->CreateFrameCanvas();
  mCanvasWidth = mOffScreenCanvas->width();
  mCanvasHeight = mOffScreenCanvas->height();
  mFrameCount = 0;

}

void MatrixStrip::Run() {
  mReceiverBuffer = (uint16_t *)std::malloc(sizeof(uint16_t) * mTotalPixels);

  mSegmentBuffer1 = (uint16_t *)std::malloc(sizeof(uint16_t) * mTotalPixels);
  memset(mSegmentBuffer1, 0, mTotalPixels);
  mInputBuffer = mSegmentBuffer1;

  mSegmentBuffer2 = (uint16_t *)std::malloc(sizeof(uint16_t) * mTotalPixels);
  memset(mSegmentBuffer2, 0, mTotalPixels);
  mOutputBuffer = mSegmentBuffer2;

  Describe();
  fflush(stdout);


  volatile uint32_t currentFrameCount = 0;
  while (running() && mShouldRun) {
    if(mFrameCount != currentFrameCount) {
      SwapBuffers();
    }

#ifdef __MATRIX_STRIP_BOTTOM_UP__
   int ptrIndex = mTotalPixels - 1;
#else
    int ptrIndex = 0;
#endif
    uint16_t *outputBuff = mOutputBuffer;

    int col = 0;
    int row = mCanvasWidth;
    for (; row > 0 ; row--) {

      col = 0;
      for (; col < mCanvasHeight; col++) {

#ifdef __MATRIX_STRIP_BOTTOM_UP__
        uint16_t pixel = outputBuff[ptrIndex--];
#else
        uint16_t pixel = outputBuff[ptrIndex++];
#endif
        // todo: consider moving this to the other thread (Network Server)
        // Color separation based off : https://stackoverflow.com/questions/38557734/how-to-convert-16-bit-hex-color-to-rgb888-values-in-c
        uint8_t r = (pixel & 0xF800) >> 8;       // rrrrr... ........ -> rrrrr000
        uint8_t g = (pixel & 0x07E0) >> 3;       // .....ggg ggg..... -> gggggg00
        uint8_t b = (pixel & 0x1F) << 3;         // ............bbbbb -> bbbbb000

        mOffScreenCanvas->SetPixel(row, col, r, g, b);
      }

    }

//    UnlockMutex();

    mOffScreenCanvas = mMatrix->SwapOnVSync(mOffScreenCanvas);
  }

  printf("MatrixStrip::Run() ended!!\n");
}

void MatrixStrip::Describe() {
  printf("MatrixStrip %p\n", this);

  printf("\tmTotalPixels = %lu\n", mTotalPixels);
  printf("\tmCanvasWidth = %i\n", mCanvasWidth);
  printf("\tmCanvasHeight = %i\n", mCanvasHeight);

}