`test.sh` does the following:

- Downloads the test video
- Launches two containers with Chrome playing a looped video
- Gets server.log and extracts the end-to-end time between frames
- Prints the output (in ms, so lower is better)
- Kills the containers

This should only be run on the dev machine
