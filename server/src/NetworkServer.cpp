
#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>
#include <boost/asio.hpp>
#include <cstring>
#include <pthread.h>
#include <time.h>

#include "NetworkServer.h"


using boost::asio::ip::tcp;
using std::min;
using std::max;

NetworkServer::NetworkServer(struct NetworkServerConfig config) {
#ifdef __linux__
  pthread_mutex_destroy(&mMutex);
  pthread_mutex_init(&mMutex, nullptr);
#endif

  mNumberSamples = 0;
  mTotalDelta = 0;
  mAverage = 0;
  mPriorAverage = 0;

  mFrameCount = 0;
  mSegmentId = config.segmentId;
  mIncomingPort = config.incomingPort;

  mSinglePanelWidth = config.singlePanelWidth;
  mSinglePanelHeight = config.singlePanelHeight;

  mPixelsPerPanel = mSinglePanelWidth * mSinglePanelWidth;

  mSegmentWidth = config.singlePanelWidth * config.numPanelsWide;
  mSegmentHeight = config.singlePanelHeight * config.numPanelsTall;

  mPanelsWide = config.numPanelsWide;
  mPanelsTall = config.numPanelsTall;

  mTotalPixels = mPixelsPerPanel * mPanelsWide * mPanelsTall;
  mTotalBytes = mTotalPixels * sizeof(uint16_t);

  mSegmentBuffer1 = (uint16_t *)malloc(mTotalBytes);
  mSegmentBuffer2 = (uint16_t *)malloc(mTotalBytes);

  mMatrixStrip = config.matrixStripInstance;

  mInputBuffer = mSegmentBuffer1;
  mOutputBuffer = mSegmentBuffer2;
  Describe();
}

uint32_t nColor = 0;
void NetworkServer::ReceiveDataThread(tcp::socket sock) {
  int numBytesReceived = 0;
  uint16_t sbIndex = 0;

  volatile clock_t start = 0;
  volatile clock_t end = 0;

  uint16_t data[mTotalBytes];
  const char *returnData = "K";

  while (GetThreadRunning()) {
    try {
      start = clock();

      boost::system::error_code error;
      size_t length = boost::asio::read(sock, boost::asio::buffer(&data, mTotalBytes), boost::asio::transfer_exactly(mTotalBytes), error);

      numBytesReceived += length;

      // Ended early! No bueno!
      if (error == boost::asio::error::eof){
//        printf("Eof\n"); fflush(stdout);
        break;
      }
      else if (error) {
        throw boost::system::system_error(error); // Some other error.
      }


      boost::asio::write(sock, boost::asio::buffer(returnData, 1));


      if (numBytesReceived == mTotalBytes) {
        end = clock();

        mNumberSamples++;
        double delta = (end - start);
        mTotalDelta += delta;
        mAverage = (mTotalDelta / mNumberSamples) ;
        /// CLOCKS_PER_SEC


#ifdef __MATRIX_STRIP_BOTTOM_UP__
        int ptrIndex = mTotalPixels - 1;
#else
        int ptrIndex = 0;
#endif
        uint16_t *outputBuff = data;

        int col = 0;
        int row = mMatrixStrip->mCanvasWidth;
        for (; row > 0 ; row--) {

          col = 0;
          for (; col < mMatrixStrip->mCanvasHeight; col++) {

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

            mMatrixStrip->GetRenderCanvas()->SetPixel(row, col, r, g, b);
          }

        }

        mMatrixStrip->mFrameCount++;
        break;
      }

    }
    catch (std::exception& e) {
      std::cerr <<  __FUNCTION__ << " Exception: " << e.what() << "\n";
    }
  }
//  while (GetThreadRunning()) {
//    try {
//      start = clock();
//
//      boost::system::error_code error;
//      size_t length = boost::asio::read(sock, boost::asio::buffer(&data, mTotalBytes), boost::asio::transfer_exactly(mTotalBytes), error);
//
//      numBytesReceived += length;
//
//      // Ended early! No bueno!
//      if (error == boost::asio::error::eof){
////        printf("Eof\n"); fflush(stdout);
//        break;
//      }
//      else if (error) {
//        throw boost::system::system_error(error); // Some other error.
//      }
//
//
//      uint16_t *sBuffPtr = GetInputBuffer();
//
//      memcpy(sBuffPtr, data, length);
////      SwapBuffers();
//
//
//      boost::asio::write(sock, boost::asio::buffer(returnData, 1));
//
//
//      if (numBytesReceived == mTotalBytes) {
//        end = clock();
//
//        mNumberSamples++;
//        double delta = (end - start);
//        mTotalDelta += delta;
//        mAverage = (mTotalDelta / mNumberSamples) ;
//        /// CLOCKS_PER_SEC
//
////        mMatrixStrip->LockMutex();
//        memcpy(mMatrixStrip->GetRenderCanvas(), GetInputBuffer(), mTotalBytes);
////        mMatrixStrip->UnlockMutex();
////        memset(mMatrixStrip->GetInputBuffer(), nColor++, mTotalBytes);
//
//        mMatrixStrip->mFrameCount++;
//        break;
//      }
//
//    }
//    catch (std::exception& e) {
//      std::cerr <<  __FUNCTION__ << " Exception: " << e.what() << "\n";
//    }
//  }

  mFrameCount++;
}


void NetworkServer::ServerStartingThread() {
  mThreadRunning = true;

  boost::asio::io_context io_context;
  unsigned short port = 9890;
  tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));

  printf("%s\n", __FUNCTION__);
  while (mThreadRunning) {
    mThread = std::thread(&NetworkServer::ReceiveDataThread, this, a.accept());
    mThread.detach();
  }
}

void NetworkServer::StartThread() {

  std::thread(&NetworkServer::ServerStartingThread, this).detach();
//
//  mThreadRunning = true;
//
//  boost::asio::io_context io_context;
//  unsigned short port = 9890;
//  tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));
//
//  printf("%s\n", __FUNCTION__);
//  while (mThreadRunning) {
//    mThread = std::thread(&NetworkServer::ReceiveDataThread, this, a.accept());
//    mThread.detach();
//  }
}


void NetworkServer::StopThread() {

  mThreadRunning = false;
  if (mThread.joinable()) {
    mThread.join();
  }
  usleep(100);

}

void NetworkServer::LockMutex() {
//  printf("%i NetworkServer::%s %p\n", mSegmentId, __FUNCTION__, &mMutex);
  pthread_mutex_lock(&mMutex);
}

void NetworkServer::UnlockMutex() {
//  printf("NetworkServer::%s\n", __FUNCTION__);fflush(stdout);

  pthread_mutex_unlock(&mMutex);
}

void NetworkServer::Describe() {
  printf("NetworkServer %p\n", this);
  printf("\tmSegmentId = %i\n", mSegmentId);
  printf("\tmSinglePanelWidth = %i\n", mSinglePanelWidth);
  printf("\tmSinglePanelHeight = %i\n", mSinglePanelHeight);
  printf("\tmPixelsPerPanel = %i\n", mPixelsPerPanel);
  printf("\tmSegmentWidth = %i\n", mSegmentWidth);
  printf("\tmSegmentHeight = %i\n", mSegmentHeight);
  printf("\tmMatricesWide = %i\n", mPanelsWide);
  printf("\tmMatricesTall = %i\n", mPanelsTall);
  printf("\tmTotalPixels = %i\n", mTotalPixels);
  printf("\tmTotalBytes = %lu\n", mTotalBytes);
  printf("\tmIncomingPort = %i\n", mIncomingPort);
  printf("\n");
  fflush(stdout);
}

NetworkServer::~NetworkServer() {
  StopThread();

  delete mSegmentBuffer1;
  delete mSegmentBuffer2;
}
