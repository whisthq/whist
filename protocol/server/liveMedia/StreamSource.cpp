#include "StreamSource.h"

StreamSource::DemoApplication2 Demo;
/// Demo 60 FPS (approx.) capture
u_int8_t* Grab60FPS2()
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

StreamSource* StreamSource::createNew(UsageEnvironment& env) {
  return new StreamSource(env);
}

StreamSource::StreamSource(UsageEnvironment& env): FramedSource(env) {
    Demo.Init();
}

StreamSource::~StreamSource() {
    Demo.Cleanup();
}

void StreamSource::doGetNextFrame() {
    //u_int8_t* stream =
    Grab60FPS2();



	if(!Demo.vPacket.empty()) {
        fFrameSize = Demo.vPacket[0].size() - 4;
        fDurationInMicroseconds = 100;
        memmove(fTo, Demo.vPacket[0].data() + 4, fFrameSize);
    }
	else {
	    fFrameSize = 0;
        handleClosure();
        return;
	}
	FramedSource::afterGetting(this);
}
