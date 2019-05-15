#ifndef MATRIX_MATRIXSTRIP_H
#define MATRIX_MATRIXSTRIP_H

#include <thread>
#include <pthread.h>
#include <string.h>

#include "led-matrix.h"
#include "threaded-canvas-manipulator.h"
#include "pixel-mapper.h"
#include "graphics.h"

using namespace rgb_matrix;


class MatrixStrip : public ThreadedCanvasManipulator  {

public:
  MatrixStrip(RGBMatrix *m) ;

  ~MatrixStrip() {
    delete mSegmentBuffer1;
    delete mSegmentBuffer2;
  }

  void Run() override;

  size_t mTotalPixels;
  uint16_t mCanvasWidth;
  uint16_t mCanvasHeight;
  uint16_t *mReceiverBuffer;
  volatile uint16_t mFrameCount;
  uint16_t *mInputBuffer;
  uint16_t *mOutputBuffer;
  volatile bool mShouldRun;


  uint16_t *mSegmentBuffer1;
  uint16_t *mSegmentBuffer2;

public:
  void SwapBuffers() {
    LockMutex();

    if (mInputBuffer == mSegmentBuffer1) {
      mInputBuffer = mSegmentBuffer2;
      mOutputBuffer = mSegmentBuffer1;
    }
    else {
      mInputBuffer = mSegmentBuffer1;
      mOutputBuffer = mSegmentBuffer2;
    }

    UnlockMutex();
  }

  void LockMutex() {
//  printf("%i MatrixStrip::%s %p\n", mSegmentId, __FUNCTION__, &mMutex);
    pthread_mutex_lock(&mMutex);
  }

  void UnlockMutex() {
//  printf("MatrixStrip::%s\n", __FUNCTION__);fflush(stdout);
    pthread_mutex_unlock(&mMutex);
  }

  void ClearBuffers() {
//    printf("%s\n", __FUNCTION__); fflush(stdout);
    LockMutex();
    memset(mSegmentBuffer1, 0, mTotalPixels * sizeof(uint16_t));
    memset(mSegmentBuffer2, 0, mTotalPixels * sizeof(uint16_t));
    mFrameCount++;
    UnlockMutex();
  }

  void CopyNetworkData(uint16_t *aInData) {
    memcpy(mInputBuffer, aInData, mTotalPixels);
    SwapBuffers();
  }

  uint16_t *GetInputBuffer() {
    return mInputBuffer;
  }
  uint16_t *GetOutputBuffer() {
    return mOutputBuffer;
  }

  void Describe();

private:




  RGBMatrix *const mMatrix;
  FrameCanvas *mOffScreenCanvas;
  pthread_mutex_t mMutex;

};





#endif //MATRIX_MATRIXSTRIP_H
