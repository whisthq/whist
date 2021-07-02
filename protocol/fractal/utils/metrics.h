#define MAX_NUM_TRACKED_PACKETS 1024

void init_tracking();
void destroy_tracking();
int track_new_packet();
void finish_tracking_packet(int id);
int *flush_tracked_packets();
void track_packet(int id);
