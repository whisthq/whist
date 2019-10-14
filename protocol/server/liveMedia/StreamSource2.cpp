/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// A template for a MediaSource encapsulating an audio/video input device
//
// NOTE: Sections of this code labeled "%%% TO BE WRITTEN %%%" are incomplete, and need to be written by the programmer
// (depending on the features of the particular device).
// Implementation

#include "StreamSource2.hh"
#include <GroupsockHelper.hh> // for "gettimeofday()"
#include <iostream>

StreamSource2::DemoApplication Demo;
/// Demo 60 FPS (approx.) capture
u_int8_t* Grab60FPS()
{
    std::cout << " ---- DO GET NEXT ---- 1.1" << std::endl;
    Demo.vPacket.clear();
    static const int WAIT_BASE = 20;
    HRESULT hr = S_OK;
    LARGE_INTEGER start = { 0 };
    LARGE_INTEGER end = { 0 };
    LARGE_INTEGER interval = { 0 };
    LARGE_INTEGER freq = { 0 };
    int wait = WAIT_BASE;

    QueryPerformanceFrequency(&freq);
    std::cout << " ---- DO GET NEXT ---- 1.2" << std::endl;

    /// Reset waiting time for the next screen capture attempt
#define RESET_WAIT_TIME(start, end, interval, freq)         \
    QueryPerformanceCounter(&end);                          \
    interval.QuadPart = end.QuadPart - start.QuadPart;      \
    MICROSEC_TIME(interval, freq);                          \
    wait = (int)(WAIT_BASE - (interval.QuadPart * 1000));
    /// Initialize Demo app
    /*
    hr = Demo.Init();
    if (FAILED(hr))
    {
        printf("Initialization failed with error 0x%08x\n", hr);
        return nullptr;
    }*/
    /// get start timestamp.
    /// use this to adjust the waiting period in each capture attempt to approximately attempt 60 captures in a second
    QueryPerformanceCounter(&start);
    std::cout << " ---- DO GET NEXT ---- 1.3" << std::endl;
    /// Get a frame from DDA
    hr = Demo.Capture(wait);
    if (hr == DXGI_ERROR_WAIT_TIMEOUT)
    {
        std::cout << "Capture timeout" << std::endl;
        /// retry if there was no new update to the screen during our specific timeout interval
        /// reset our waiting time
        RESET_WAIT_TIME(start, end, interval, freq);

    }
    else
    {
        if (FAILED(hr))
        {
            /// Re-try with a new DDA object
            printf("Captrue failed with error 0x%08x. Re-create DDA and try again.\n", hr);
            Demo.Cleanup();
            hr = Demo.Init();
            if (FAILED(hr))
            {
                /// Could not initialize DDA, bail out/
                printf("Failed to Init DDDemo. return error 0x%08x\n", hr);
                return nullptr;
            }
            RESET_WAIT_TIME(start, end, interval, freq);
            QueryPerformanceCounter(&start);
            /// Get a frame from DDA
            Demo.Capture(wait);
        }
        std::cout << " ---- DO GET NEXT ---- 1.4" << std::endl;
        RESET_WAIT_TIME(start, end, interval, freq);
        std::cout << " ---- DO GET NEXT ---- 1.5" << std::endl;
        /// Preprocess for encoding
        hr = Demo.Preproc();
        std::cout << " ---- DO GET NEXT ---- 1.6" << std::endl;
        if (FAILED(hr))
        {
            printf("Preproc failed with error 0x%08x\n", hr);
            return nullptr;
        }
        std::cout << " ---- DO GET NEXT ---- 1.7" << std::endl;
        return Demo.Encode();
    }
    return nullptr;
}

StreamSource2*
StreamSource2::createNew(UsageEnvironment& env,
			DeviceParameters2 params) {
  return new StreamSource2(env, params);
}

StreamSource2*
StreamSource2::createNew(UsageEnvironment& env) {
    return new StreamSource2(env);
}

EventTriggerId StreamSource2::eventTriggerId = 0;

unsigned StreamSource2::referenceCount = 0;

StreamSource2::StreamSource2(UsageEnvironment& env,
			   DeviceParameters2 params)
  : FramedSource(env), fParams(params) {
  if (referenceCount == 0) {
    // Any global initialization of the device would be done here:
    //%%% TO BE WRITTEN %%%
  }
  ++referenceCount;

  // Any instance-specific initialization of the device would be done here:
  //%%% TO BE WRITTEN %%%

  // We arrange here for our "deliverFrame" member function to be called
  // whenever the next frame of data becomes available from the device.
  //
  // If the device can be accessed as a readable socket, then one easy way to do this is using a call to
  //     envir().taskScheduler().turnOnBackgroundReadHandling( ... )
  // (See examples of this call in the "liveMedia" directory.)
  //
  // If, however, the device *cannot* be accessed as a readable socket, then instead we can implement it using 'event triggers':
  // Create an 'event trigger' for this device (if it hasn't already been done):
  if (eventTriggerId == 0) {
    eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
  }
}

StreamSource2::~StreamSource2() {
  // Any instance-specific 'destruction' (i.e., resetting) of the device would be done here:
  //%%% TO BE WRITTEN %%%
    Demo.Cleanup();

  --referenceCount;
  if (referenceCount == 0) {
    // Any global 'destruction' (i.e., resetting) of the device would be done here:
    //%%% TO BE WRITTEN %%%

    // Reclaim our 'event trigger'
    envir().taskScheduler().deleteEventTrigger(eventTriggerId);
    eventTriggerId = 0;
  }
}

void StreamSource2::doGetNextFrame() {
  // This function is called (by our 'downstream' object) when it asks for new data.

  // Note: If, for some reason, the source device stops being readable (e.g., it gets closed), then you do the following:
  if (0 /* the source stops being readable */ /*%%% TO BE WRITTEN %%%*/) {
    handleClosure();
    return;
  }

  // If a new frame of data is immediately available to be delivered, then do this now:
  //if (0 /* a new frame of data is immediately available to be delivered*/ /*%%% TO BE WRITTEN %%%*/) {
    deliverFrame();
  //}

  // No new data is immediately available to be delivered.  We don't do anything more here.
  // Instead, our event trigger must be called (e.g., from a separate thread) when new data becomes available.
}

void StreamSource2::deliverFrame0(void* clientData) {
  ((StreamSource2*)clientData)->deliverFrame();
}

void StreamSource2::deliverFrame() {
  // This function is called when new frame data is available from the device.
  // We deliver this data by copying it to the 'downstream' object, using the following parameters (class members):
  // 'in' parameters (these should *not* be modified by this function):
  //     fTo: The frame data is copied to this address.
  //         (Note that the variable "fTo" is *not* modified.  Instead,
  //          the frame data is copied to the address pointed to by "fTo".)
  //     fMaxSize: This is the maximum number of bytes that can be copied
  //         (If the actual frame is larger than this, then it should
  //          be truncated, and "fNumTruncatedBytes" set accordingly.)
  // 'out' parameters (these are modified by this function):
  //     fFrameSize: Should be set to the delivered frame size (<= fMaxSize).
  //     fNumTruncatedBytes: Should be set iff the delivered frame would have been
  //         bigger than "fMaxSize", in which case it's set to the number of bytes
  //         that have been omitted.
  //     fPresentationTime: Should be set to the frame's presentation time
  //         (seconds, microseconds).  This time must be aligned with 'wall-clock time' - i.e., the time that you would get
  //         by calling "gettimeofday()".
  //     fDurationInMicroseconds: Should be set to the frame's duration, if known.
  //         If, however, the device is a 'live source' (e.g., encoded from a camera or microphone), then we probably don't need
  //         to set this variable, because - in this case - data will never arrive 'early'.
  // Note the code below.
    u_int8_t* newFrameDataStart = Grab60FPS();
    unsigned newFrameSize = Demo.vPacket[0].size() - 4; //%%% TO BE WRITTEN %%%
    std::cout << "Test 2" << std::endl;



    if(!Demo.vPacket.empty()) {
        //fFrameSize = Demo.vPacket[0].size() - 4;
        //fDurationInMicroseconds = 100;
        //memmove(fTo, Demo.vPacket[0].data() + 4, fFrameSize);
    }
    else { /*Demo.Cleanup(); Demo.Init();*/ handleClosure(); return;}
//  if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet

  //u_int8_t* newFrameDataStart = (u_int8_t*)0xDEADBEEF; //%%% TO BE WRITTEN %%%

  // Deliver the data here:
  if (newFrameSize > fMaxSize) {
    fFrameSize = fMaxSize;
    fNumTruncatedBytes = newFrameSize - fMaxSize;
  } else {
    fFrameSize = newFrameSize;
  }
  gettimeofday(&fPresentationTime, NULL); // If you have a more accurate time - e.g., from an encoder - then use that instead.
  // If the device is *not* a 'live source' (e.g., it comes instead from a file or buffer), then set "fDurationInMicroseconds" here.
  memmove(fTo, newFrameDataStart + 4, fFrameSize);

  // After delivering the data, inform the reader that it is now available:
  FramedSource::afterGetting(this);
}

StreamSource2::StreamSource2(UsageEnvironment &env) : FramedSource(env) {
    Demo.Init();
}


// The following code would be called to signal that a new frame of data has become available.
// This (unlike other "LIVE555 Streaming Media" library code) may be called from a separate thread.
// (Note, however, that "triggerEvent()" cannot be called with the same 'event trigger id' from different threads.
// Also, if you want to have multiple device threads, each one using a different 'event trigger id', then you will need
// to make "eventTriggerId" a non-static member variable of "DeviceSource".)
void stream_signalNewFrameData() {
  TaskScheduler* ourScheduler = NULL; //%%% TO BE WRITTEN %%%
  StreamSource2* ourDevice  = NULL; //%%% TO BE WRITTEN %%%

  if (ourScheduler != NULL) { // sanity check
    ourScheduler->triggerEvent(StreamSource2::eventTriggerId, ourDevice);
  }
}
