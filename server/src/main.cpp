/*
This gist demonstrates how to setup threaded & blocking (probably could have used the async transfer instead)
Boost ASIO server to receive a number of bytes from a client. It was developed as a proof of concept and has many areas that 
could be improved for speed.  I ran this on a Raspberry PI 3b+ using the active RGB Matrix board from electrodragon.
Here's a demonstration this code executing on the PI 3b+ pumping 49,152 bytes for the video data ((6 x (64x64 rgb matrices) * 2). 
https://www.youtube.com/watch?v=ODnbhvbLX9E&feature=youtu.be 


This works spawning two threads (Network & Display)

We first spawn a thread that will constantly listen on the specified port & IP.  We expect a certain number of bytes.
After the TCIP/IP transfer is complete, the networked buffer data is then copied to the networkScreenBuffer array.

Next, we kick off the RGB Matrix thread, and upon every execution of Run(), the contents of networkScreenBuffer is
split into rgb values. We use those rgb values to paint the off screen canvas via it's SetPixel method. 

Sender example:
https://gist.github.com/jaygarcia/5d77e6687e742dcfb19f58728c997b76

Resources:
LibBoost 1.7.0 (installed from scratch -- not hard!): https://dl.bintray.com/boostorg/release/1.70.0/source/boost_1_70_0.tar.gz 
RGB Matrix library: https://github.com/hzeller/rpi-rgb-led-matrix/
Issue on the RGB Matrix library project: https://github.com/hzeller/rpi-rgb-led-matrix/issues/796
*/

#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>
#include <boost/asio.hpp>
#include <cstring>
#include <pthread.h>
#include <time.h>

#include "led-matrix.h"
#include "pixel-mapper.h"
#include "graphics.h"

#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>

#include "NetworkServer.h"
#include "MatrixStrip.h"

using std::min;
using std::max;
using boost::asio::ip::tcp;
using namespace rgb_matrix;


pthread_mutex_t bufferMutex;

// Change the following constants to your liking. The number of bytes that are sent
// will need to match readBufferSize!
const uint16_t singlePanelWidth = 64;
const uint16_t singlePanelHeight = 64;
const uint16_t totalSinglePanelSize = (singlePanelHeight * singlePanelWidth);
const uint16_t numMatricesWide = 3;
const uint16_t numMatricesTall = 1; // Should be 4!!
const uint16_t totalPixels = totalSinglePanelSize * numMatricesWide * numMatricesTall;
const size_t readBufferSize = totalPixels * sizeof(uint16_t);



/***********************************************************************/

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

NetworkServer *server;
double priorAverage = 0;

int retries = 0;

bool shouldQuit = false;
MatrixStrip *matrixStrip = NULL;


void interrupterThread() {

  while(1) {
    if (interrupt_received) {
      server->StopThread();
      exit(1);
    }

//    server->LockMutex();
    printf("avg %f      \r", server->mAverage);

    if (server->mAverage == priorAverage) {
      retries ++;
      if (retries > 50) {
        server->mAverage = 0;
        retries = 0;
        matrixStrip->ClearBuffers();
//        printf("**CLEAR BUFFERS!! retries=%i\n",retries);
      }

    }

    priorAverage = server->mAverage;
//    server->UnlockMutex();
    usleep(50000);
  }
}

/***********************************************************************/


void start_matrix() {

  RGBMatrix::Options mMatrixOptions;
  rgb_matrix::RuntimeOptions runtime_opt;

  // I hard coded options here. You'll need to change this per your own specs!
  mMatrixOptions.chain_length = numMatricesWide;
  mMatrixOptions.cols = singlePanelWidth;
  mMatrixOptions.rows = singlePanelHeight;
  mMatrixOptions.parallel = 1;


  RGBMatrix *matrix = CreateMatrixFromOptions(mMatrixOptions, runtime_opt);
  if (matrix == NULL) {
    printf("ERROR! Could not create RGBMatrix instance!!!!\n");
    exit(1);
  }

  printf("Size: %dx%d. Hardware gpio mapping: %s\n",
         matrix->width(), matrix->height(), mMatrixOptions.hardware_mapping);

  matrixStrip = new MatrixStrip(matrix);
  matrixStrip->mTotalPixels = totalPixels;

  matrixStrip->Start();
}



/**** MAIN *****/
int main(int argc, char* argv[]) {
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  printf("Server starting & expecting (%lu bytes)...\n", readBufferSize); fflush(stdout);
  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);
  std::cout << "\x1B[2J\x1B[H";
  printf("%s\n", hostname);
  fflush(stdout);


  start_matrix();



  NetworkServerConfig serverConfig;
  serverConfig.incomingPort = 9890;
  serverConfig.numPanelsWide = numMatricesWide;
  serverConfig.numPanelsTall = numMatricesTall;
  serverConfig.singlePanelWidth = singlePanelWidth;
  serverConfig.singlePanelHeight = singlePanelHeight;
  serverConfig.segmentId = 1;
  serverConfig.matrixStripInstance = matrixStrip;

  usleep(1000);
  server = new  NetworkServer(serverConfig);
  server->StartThread();

  usleep(10);

  std::thread(interrupterThread).detach();


  while (!interrupt_received) {
//    printf("Main sleeping\n");
    sleep(1); // Time doesn't really matter. The syscall will be interrupted.
  }

  return 0;
}