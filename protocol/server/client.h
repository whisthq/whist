#include "../fractal/core/fractal.h"
#include "../fractal/utils/clock.h"
#include "../fractal/utils/rwlock.h"

// the write is_active_rwlock takes precedence over
typedef struct Client {
	/* ACTIVE */
	bool is_active; // protected by global is_active_rwlock
	bool is_controlling; // protected by state lock
	bool is_host; // protected by state lock

	/* USER INFO */
	int username; // not lock protected

	/* NETWORK */
	SocketContext UDP_context; // protected by global is_active_rwlock
	SocketContext TCP_context; // protected by global is_active_rwlock
    int UDP_port; // protected by global is_active_rwlock
    int TCP_port; // protected by global is_active_rwlock

	/* MOUSE */
	struct {
		int x;
		int y;
	    struct {
			int r;
			int g;
			int b;
		} color;
		bool is_active;
	} mouse; // protected by state lock

	/* PING */
	clock last_ping; // not lock protected

} Client;

extern SDL_mutex *state_lock;
extern RWLock is_active_rwlock;

extern Client clients[MAX_NUM_CLIENTS];

extern int num_controlling_clients;
extern int num_active_clients;
extern int host_id;

int initClients(void);

int destroyClients(void);

int quitClient(int id);

int quitClients(void);

int existsTimedOutClient(int timeout, bool *exists);

int reapTimedOutClients(int timeout);

int tryFindClientIdByUsername(int username, bool *found, int *id);

int getAvailableClientID(int *id);

int fillPeerUpdateMessages(PeerUpdateMessage *msgs, size_t *num_msgs);
