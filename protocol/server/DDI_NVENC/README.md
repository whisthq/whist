# DDI NVENC
This application reads frames from Windows Desktop Duplication API continously
and encodes them using Nvidia NVENC encoder sdk to H.264. Each frame is left
held in memory and then flushed when a new frame comes in. This application is
part 1 and 2 of the Fractal Protocol from the Server to Client side of things,
and it needs to be connected with a RTSP server to stream the encoded frame held
in memory before flushing it.
