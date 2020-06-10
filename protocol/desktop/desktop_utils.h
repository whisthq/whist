#ifndef DESKTOP_UTILS_H
#define DESKTOP_UTILS_H

int parseArgs(int argc, char* argv[]);

char *getLogDir(void);

int logConnectionID(int connection_id);

int initSocketLibrary(void);

int destroySocketLibrary(void);

int configureCache(void);

int configureSSHKeys(void);

#endif  // DESKTOP_UTILS_H
