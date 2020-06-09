#ifndef DESKTOP_UTILS_H
#define DESKTOP_UTILS_H

int parseArgs(int argc, char* argv[]);

char *getLogDir(void);

int initSocketLibrary(void);

int destroySocketLibrary(void);

int configureCache(void);

int configureSSHKeys(void);

#endif  // DESKTOP_UTILS_H
