## Fractal Windows/Linux Ubuntu Servers

This folder builds a server to stream via Fractal. It supports Windows and Linux Ubuntu. See below for basic build instructions, and the folder's README for build and development details.

### Development

You can run `server.bat` (Windows) or `server.sh` to restart your dev environment.

To compile, you should first run `cmake .` (MacOS/Linux) or `cmake -G "NMake Makefiles"` (Windows) from the root directory, `/protocol/`. You can then cd into this folder and run `make` (MacOS/Linux) or `nmake` (Windows).

If you're having trouble compiling, make sure that you followed the instructions in the root-level README. If still having issue, you should delete the CmakeCache or start from a fresh repository.

### Running

From this directory, `/server`, you can simply run:

```
Windows:
FractalServer

MacOS/Linux:
./FractalServer
```

### Updating Fractal Virtual Machines

The Fractal virtual machines run this server executable. We run 3 different types of executables: 

`master`, for the latest release branch | User-facing

`staging`, for the current beta-version release branch | Beta User-facing

`dev`, for the current internal development branch | NOT User-facing

#### Windows

After compiling the server under the appropriate branch by running `nmake`, you can then run one of the following, from a Windows computer, to update the machines running a specific branch:

`update master` to update the Windows VMs running the master branch

`update staging` to update the Windows VMs running the staging branch

`update dev` to update the Windows VMs running the dev branch

You should only do this if you are sure of what you are doing and the team has agreed it is time to push an update.

#### Linux Ubuntu

After compiling the server under the appropriate branch by running `make`, you can then run one of the following, from a Windows computer, to update the machines running a specific branch:

`./update master` to update the Linux Ubuntu VMs running the master branch

`./update staging` to update the Linux Ubuntu VMs running the staging branch

`./update dev` to update the Linux UbuntuVMs running the dev branch

You should only do this if you are sure of what you are doing and the team has agreed it is time to push an update.
