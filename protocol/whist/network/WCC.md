# Whist Congestion Control(WCC)

In this document we will describe the Whist Congestion Control algorithm. The primary requirement of this algorithm is to estimate the bandwidth available for AV streaming between the server and client as accurately as possible. This algorithm is heavily based on [Google Congestion Control](#References) with some simplifications and modifications. This algorithm is based on the fact that there is a queue of unknown size between the sender and receiver, somewhere in the network. It tries to detect if the queues are filling-up, draining or empty and estimates the bandwidth accordingly. In addition to the queue length, it also takes into account the packet losses in the network to estimate the available bandwidth.

# Terminology

- Group of packets - A set of RTP packets transmitted from the sender uniquely identified by the group departure and group arrival time. These could be video packets, audio packets, or a mix of audio and video packets.

# Pacer (Network Throttler)

Pacing is used to actuate the target bitrate computed by the congestion control algorithm. (This is called as Network Throttler inside Whist code).
When media encoder produces data, this is fed into a Pacer queue. The Pacer sends a group of packets to the network every burst_time interval. RECOMMENDED value for burst_time is 5 ms.  The size of a group of packets is computed as the product between the target bitrate and the burst_time. The group of packets that are sent in a burst_time interval, should be assigned a group_id for easier reference on the client side. group_id could start with 1 and is incremented for each burst.

# Algorithm

This algorithm is run only for video packets. Audio packets are ignored as they are insignificant in size and doesn't align well video frames.

## Arrival time model

We define the inter-arrival time, t(i) - t(i-1), as the difference in arrival time of two groups of packets.  Correspondingly, the inter-departure time, T(i) - T(i-1), is defined as the difference in departure-time of two groups of packets.  Finally, the inter-group delay variation, d(i), is defined as the difference between the inter-arrival time and the inter-departure time.  Or interpreted differently, as the difference between the delay of group i and group i-1.

    d(i) = t(i) - t(i-1) - (T(i) - T(i-1))

An inter-departure time is computed between consecutive groups as T(i) - T(i-1), where T(i) is the departure timestamp of the last packet in the current packet group being processed.  Any packets received out of order are ignored by the arrival-time model.

Each group is assigned a receive time t(i), which corresponds to the time at which the last packet of the group was received.

## Inter-group delay variation filter

The parameter d(i) is readily available for each group of packets, i > 1. We will use a EWMA filter to remove any noise in d(i), to get a inter-group delay variation estimate m(i).

For i > 1,
    `m(i) = m(i - 1) * (1 - EWMA_FACTOR) + d(i) * EWMA_FACTOR`

The RECOMMENDED value for EWMA_FACTOR is 0.3.

As a the special case to handle initialization,
For i == 1
    `m(i) = d(i)`

## Over-use detector

The inter-group delay variation estimate m(i), obtained as the output of the arrival-time filter, is compared with a threshold del_var_th. An estimate above the threshold is considered as an indication of over-use. Such an indication is not enough for the detector to signal over-use to the rate control subsystem. A definitive over-use will be signaled only if over-use has been detected for at least overuse_time_th milliseconds. Similarly, the opposite state, under-use, is detected when m(i) < -del_var_th. If neither over-use nor under-use is detected, the detector will be in the normal state.

The RECOMMENDED value for del_var_th is 10 milliseconds.
The RECOMMENDED value for overuse_time_th is 10 milliseconds.

Also if the packet loss over last 250ms is observed to be higher than 10% then the delay-variation based signal is ignored and over-use is signaled.

## Rate control

The rate control is designed to increase the estimate of the available bandwidth as long as there is no detected congestion and to ensure that we will eventually match the available bandwidth of the channel and detect an over-use.

As soon as over-use has been detected, the available bandwidth estimated by the controller is decreased.

The rate control subsystem has 3 states: Increase, Decrease and Hold.
   - "Increase" is the state when no congestion is detected
   - "Decrease" is the state where congestion is detected
   - "Hold" is a state that waits until built-up queues have drained before going to "increase" state.

The state transitions (with blank fields meaning "remain in state") are:

|Signal \ State| Hold      | Increase   |Decrease|
| -----------  | ----------| ---------- | ------ |
|  Over-use    | Decrease  |  Decrease  |        |
|  Normal      | Increase  |            |  Hold  |
|  Under-use   |           |   Hold     |  Hold  |

New bitrate message is sent once every update_interval duration, except when switching from Increase state to Decrease state where a new bitrate message will be sent immediately. The RECOMMENDED value of update_interval is 1 second

Herein we define incoming_bitrate R(i) as follows. It is the incoming bitrate measured over the last 250ms.

The new bitrate A(i) will be calculated as follows
### Increase bitrate

The bitrate is increased to a new value only if the following conditions are met.
    - System is currently in "Increase state"
    - update_interval duration has passed since we sent the last new bitrate message
    - R(i) is greater than BANDWITH_USED_THRESHOLD times of A(i-1). The RECOMMENDED value of BANDWITH_USED_THRESHOLD is 0.9

The bitrate A(i) is updated as per the below equation

    A(i) = A(i-1) * ((100 + increase_percent(i)) / 100)

The value of increase_percent(i), is a value that starts with a high initial value when current A(i) appears to be far from convergence. increase_percent(i) gets reduced gradually as we approach convergence. The exact method of calculating increase_percent(i) will specified later in this document.

### Decrease bitrate

The bitrate is decreased to a new value only if the following conditions are met.
    - System is currently in "Decrease state"
    - update_interval duration has passed since we sent the last "decrease" bitrate message

The bitrate A(i) is updated as per the below equation

    A(i) = R(i) * DECREASE_RATIO

The RECOMMENDED value of DECREASE_RATIO is 0.95

### Saturate Bandwidth

As one can observe from the above equations, the bitrate will be increased to a new value only if the incoming bitrate is above a particular threshold. But the video encoder will not generate enough bits to utilize the entire video bitrate configured when the client is idle and simple video content is being encoded. During such cases the bitrate cannot be increased to the available bandwidth till the client becomes active. In order to converge to the available bandwidth quickly, the algo will send a "saturate_bandwidth" signal to the server, so that server can send filler packets and/or FEC packets to send data as per the requested bitrate A(i). Once bitrate convergence is reached, "saturate_bandwidth" will be set to false so that we don't waste bandwidth unnecessarily.

The value saturate_bandwidth is updated as per below

    if (A(i) < A_inc(i-1) * CONVERGENCE_THRESHOLD_LOW || A(i) > A_fail(i-1) * CONVERGENCE_THRESHOLD_HIGH || i = 0)) {
        saturate_bandwidth = true;
    } else if (A(i) < A(i-1)) {
        saturate_bandwidth = false;
    } else if (A(i) > max_bitrate) {
        saturate_bandwidth = false;
    }

where,

    A_inc(i-1) = previous bitrate sent while in increase state.

    A_fail(i-1) = previous bitrate used when there was a state transition from increase to decrease and a lower bitrate was sent.

    max_bitrate = maximum bitrate that the system can allot for this AV streaming. The method of computing max_bitrate can be decided independently by the system, based on the application, resolution etc., and is beyond the scope of this document.

The RECOMMENDED value of CONVERGENCE_THRESHOLD_LOW is 0.80
The RECOMMENDED value of CONVERGENCE_THRESHOLD_HIGH is 1.1


### Increase percentage

The value increase_percent(i) is updated as per below. When the current bitrate appears to be far from convergence then INITIAL_INCREASE_PERCENTAGE is used. When we seem to approaching covergence then increase_percent(i) is reduced.

    if (A(i) < A_inc(i-1) * CONVERGENCE_THRESHOLD_LOW || A(i) > A_fail(i-1) * CONVERGENCE_THRESHOLD_HIGH || i == 0) {
        increase_percent(i) = INITIAL_INCREASE_PERCENTAGE;
    } else if (A(i) < A(i-1)) {
        increase_percent(i) = increase_percent(i - 1) / 2;
    }

The RECOMMENDED value of INITIAL_INCREASE_PERCENTAGE is 1.

### Burst Bitrate

When the available bandwidth is greater than the max_bitrate then the server can send bits at a greater rate which can reduce the latency significantly. When the available bandwidth A(i) exceeded max_bitrate succesively multiple times, then we use a burst bitrate greater than available bandwidth that can potentially reduce latency. The burst bitrate B(i) is calculated as per the below equation

    if (A(i) > max_bitrate) {
        if (max_bitrate_count >= pre_burst_mode_count) {
            burst_mode = true;
        }
        A(i) = max_bitrate;
        max_bitrate_count++;
    } else {
        burst_mode = false;
        max_bitrate_count = 0;
    }
    if (burst_mode)
        B(i) = A(i) * BURST_BITRATE_RATIO;
    else
        B(i) = A(i)

The RECOMMENDED value of BURST_BITRATE_RATIO is 4

pre_burst_mode_count is increased if the burst mode causes the system to go into decrease state. But if the congestion is not due to burst mode then the pre_burst_mode_count is re-initialized to its starting value. pre_burst_mode_count is calculated as per the below equation

    if (burst_mode && A(i) < A(i-1) && max_bitrate_count < 2 * pre_burst_mode_count) {
        pre_burst_mode_count *= 2;
    } else if (A(i-1) < max_bitrate || i == 0) {
        pre_burst_mode_count = INITIAL_PRE_BURST_MODE_COUNT;
    }

The RECOMMENDED value of INITIAL_PRE_BURST_MODE_COUNT is 5

## RECOMMENDED values and reasons

| Parameter                     | RECOMMENDED value | Reason     |
| ----------------------------- | ----------------- | ---------- |
|  burst_time                   | 5ms               | From [Google Congestion control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-4)  |
|  EWMA_FACTOR                  | 0.3               | Based on heuristics. Not sure if it will work for all cases. We might need to replace this with a kalman filter, where this factor is changed adaptively. TODO for future. |
|  del_var_th                   | 10ms              | Based on heuristics. Google Congestion control recommends an adaptive threshold of [6ms to 600ms](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-5.4), based on current network conditions. They mention that a static value will lead to starvation if there is a concurrent TCP flow. TODO for future. |
|  overuse_time_th              | 10ms              | From [Google Congestion control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-5.4) |
|  update_interval              | 1 second          | From [Google Congestion control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-3) |
|  INITIAL_INCREASE_PERCENTAGE  | 1                 | Based on heuristics. A 1% increase every 1 second, will mean that we can double the bitrate in 70 seconds. This is quick enough to reach to available bandwidth. Also this is small enough to make sure that we don't lose a lot of packets when there is a congestion triggered due to this increase. Google Congestion control uses a different algorithm for increasing bitrate that didn't work for me very well. Maybe a TODO for future, if ppl find any limitations with this |
|  DECREASE_RATIO               | 0.95              | Based on heuristics or faster convergence. [Google Congestion control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-5.5) suggests a value between 0.8 to 0.95  |
|  CONVERGENCE_THRESHOLD_LOW    | 0.8               | Based on heuristics or faster convergence. Google Congestion control have such concepts such as conditional saturate bandwidth which uses this value. These are our improvisations |
|  CONVERGENCE_THRESHOLD_HIGH   | 1.1               | Based on heuristics or faster convergence. Google Congestion control have such concepts such as conditional saturate bandwidth which uses this value. These are our improvisations |
|  BANDWITH_USED_THRESHOLD      | 0.9               | Based on heuristics. Google Congestion control have a such concept of conditional saturate bandwidth. It always assumes bandwidth is fully used. These are our improvisations |
|  BURST_BITRATE_RATIO          | 4                 | The current value being used in the dev branch, as of writing this document |
| INITIAL_PRE_BURST_MODE_COUNT  | 5                 | Based on heuristics. Google Congestion control doesn't have a concept of burst bitrate |

# References

1. [Google Congestion Control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02)
