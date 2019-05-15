#ifndef MATRIX_NETWORKSERVER_H
#define MATRIX_NETWORKSERVER_H

#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>
#include <boost/asio.hpp>
#include <cstring>
#include <pthread.h>
#include <time.h>

#include <thread>
#include "MatrixStrip.h"

using boost::asio::ip::tcp;


struct NetworkServerConfig {
  uint16_t singlePanelWidth;
  uint16_t singlePanelHeight;
  uint8_t numPanelsWide;
  uint8_t numPanelsTall;
  uint8_t segmentId;
  uint16_t incomingPort;
  MatrixStrip *matrixStripInstance;
};

class NetworkServer {
public:
  uint8_t  mSegmentId;
  uint16_t mSinglePanelWidth;
  uint16_t mSinglePanelHeight;
  uint16_t mPixelsPerPanel;

  uint16_t mPanelsWide;
  uint16_t mPanelsTall;

  uint16_t mSegmentWidth;
  uint16_t mSegmentHeight;

  uint16_t mTotalPixels;

  size_t mTotalBytes;


  uint16_t mIncomingPort;

  volatile uint32_t mNumberSamples;
  volatile double mTotalDelta;
  volatile double mAverage;
  volatile double mPriorAverage;

public:
  explicit NetworkServer(struct NetworkServerConfig config);

  ~NetworkServer();

  void ReceiveDataThread(tcp::socket sock);
  void ServerStartingThread();

  void LockMutex();
  void UnlockMutex();

  void ClearBuffers() {
    printf("%s\n", __FUNCTION__); fflush(stdout);
    LockMutex();
    memset(mSegmentBuffer1, 0, mTotalPixels);
    memset(mSegmentBuffer2, 0, mTotalPixels);
    mFrameCount++;
    UnlockMutex();
  }

  void WritePixel(uint16_t index, uint16_t color) {
    LockMutex();
    mInputBuffer[index] = color;
    UnlockMutex();
  }

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

  uint16_t *GetInputBuffer() {
    return mInputBuffer;
  }
  uint16_t *GetOutputBuffer() {
    return mOutputBuffer;
  }

  bool GetThreadRunning() {
    return mThreadRunning;
  }

  void StartThread();
  void StopThread();

  void Describe();

  uint16_t GetFrameCount() {
    return mFrameCount;
  }

  void IncrementFrameCount() {
    mFrameCount++;
  }
private:
  volatile uint16_t mFrameCount;
  volatile bool mThreadRunning;

  uint16_t *mSegmentBuffer1;
  uint16_t *mSegmentBuffer2;

  uint16_t *mInputBuffer;
  uint16_t *mOutputBuffer;

  pthread_mutex_t mMutex;
  std::thread mThread;

  MatrixStrip *mMatrixStrip;
};




#endif //MATRIX_NETWORKSERVER_H
