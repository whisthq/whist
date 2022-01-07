// Unshare a thread like a boss.

// This code allows us to `unshare` a particular thread or process into
// a new user and mount namespace, appropriately mapping the new namespace
// UID/GID into the old namespace. This allows us to mount FUSE filesystems
// inside Docker containers without allowing for any privilege escalation.

#ifndef UNSHARE_H
#define UNSHARE_H

// Unshare the current thread, moving to new user and mount namespaces.
void unshare_thread();

#endif  // UNSHARE_H
