#include "StreamSource.h"


/// Demo 60 FPS (approx.) capture
u_int8_t* Grab60FPS()
{
    std::cout << "START 1" << std::endl;
    static const int WAIT_BASE = 17;
    StreamSource::DemoApplication Demo;
    HRESULT hr = S_OK;
    LARGE_INTEGER start = { 0 };
    LARGE_INTEGER end = { 0 };
    LARGE_INTEGER interval = { 0 };
    LARGE_INTEGER freq = { 0 };
    int wait = WAIT_BASE;
    std::cout << "START 2" << std::endl;

    QueryPerformanceFrequency(&freq);
    std::cout << "START 3" << std::endl;

    /// Reset waiting time for the next screen capture attempt
#define RESET_WAIT_TIME(start, end, interval, freq)         \
    QueryPerformanceCounter(&end);                          \
    interval.QuadPart = end.QuadPart - start.QuadPart;      \
    MICROSEC_TIME(interval, freq);                          \
    wait = (int)(WAIT_BASE - (interval.QuadPart * 1000));

    std::cout << "START 4" << std::endl;
    /// Initialize Demo app
    hr = Demo.Init();
    std::cout << "START 4.1" << std::endl;
    if (FAILED(hr))
    {
        printf("Initialization failed with error 0x%08x\n", hr);
        return nullptr;
    }
    std::cout << "START 5" << std::endl;
	/// get start timestamp.
	/// use this to adjust the waiting period in each capture attempt to approximately attempt 60 captures in a second
	QueryPerformanceCounter(&start);
	/// Get a frame from DDA
	hr = Demo.Capture(wait);
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
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
		RESET_WAIT_TIME(start, end, interval, freq);
		/// Preprocess for encoding
		hr = Demo.Preproc();
		if (FAILED(hr))
		{
			printf("Preproc failed with error 0x%08x\n", hr);
			return nullptr;
		}
		return Demo.Encode();
	}
    std::cout << "START 6" << std::endl;
	return nullptr;
}

StreamSource* StreamSource::createNew(UsageEnvironment& env) {
  return new StreamSource(env);
}

StreamSource::StreamSource(UsageEnvironment& env): FramedSource(env) {
}

StreamSource::~StreamSource() {
}

void StreamSource::doGetNextFrame() {
	u_int8_t* stream = Grab60FPS();
	fTo = stream;
	//fFrameSize = TODO: trouver une taille qui correspond, une constante à priori
	FramedSource::afterGetting(this);
}
