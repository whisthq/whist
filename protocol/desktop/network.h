#ifndef DESKTOP_NETWORK_H
#define DESKTOP_NETWORK_H

int establishConnections(void);

int closeConnections(void);

int establishSpectatorConnections(void);

int closeSpectatorConnections(void);

int sendServerQuitMessages(int num_messages);

int waitForServerInitMessage(int timeout);

#endif  // DESKTOP_NETWORK_H
