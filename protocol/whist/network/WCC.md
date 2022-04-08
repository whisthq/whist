# Whist Congestion Control(WCC)

In this document we will describe the Whist Congestion Control algorithm. The primary requirement of this algorithm is to estimate the bandwidth available for AV streaming between the server and client as accurately as possible. This algorithm is heavily based on [Google Congestion Control](#References) with some simplifications and modifications. This algorithm is based on the fact that there is a queue of unknown size between the sender and receiver, somewhere in the network. It tries to detect if the queues are filling-up, draining or empty and estimates the bandwidth accordingly. In addition to the queue length, it also takes into account the packet losses in the network to estimate the available bandwidth.

# Terminology

- Group of packets - A set of RTP packets transmitted from the sender uniquely identified by the group departure and group arrival time. These could be video packets, audio packets, or a mix of audio and video packets.

# Pacer (Network Throttler)

Pacing is used to actuate the target bitrate computed by the congestion control algorithm. (This is called as Network Throttler inside Whist code).
When media encoder produces data, this is fed into a Pacer queue. The Pacer sends a group of packets to the network every burst_time interval. RECOMMENDED value for burst_time is 5 ms. The size of a group of packets is computed as the product between the target bitrate and the burst_time. The group of packets that are sent in a burst_time interval, should be assigned a group_id for easier reference on the client side. group_id could start with 1 and is incremented for each burst.

# Algorithm

This algorithm is run only for video packets. Audio packets are ignored as they are insignificant in size and doesn't align well video frames.

## Arrival time model

We define the inter-arrival time, t(i) - t(i-1), as the difference in arrival time of two groups of packets. Correspondingly, the inter-departure time, T(i) - T(i-1), is defined as the difference in departure-time of two groups of packets. Finally, the inter-group delay variation, d(i), is defined as the difference between the inter-arrival time and the inter-departure time. Or interpreted differently, as the difference between the delay of group i and group i-1.

    d(i) = t(i) - t(i-1) - (T(i) - T(i-1))

An inter-departure time is computed between consecutive groups as T(i) - T(i-1), where T(i) is the departure timestamp of the last packet in the current packet group being processed. Any packets received out of order are ignored by the arrival-time model.

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

The inter-group delay variation estimate m(i), obtained as the output of the arrival-time filter, is compared with a threshold del_var_th. An estimate above the threshold is considered as an indication of over-use. Such an indication is not enough for the detector to signal over-use to the rate control subsystem. If the packet loss over last 1 second is observed to be higher than 10% then also over-use is signaled. A definitive over-use will be signaled only if over-use has been detected for at least overuse_time_th milliseconds. Similarly, the opposite state, under-use, is detected when m(i) < -del_var_th. If neither over-use nor under-use is detected, the detector will be in the normal state.

The RECOMMENDED value for del_var_th is 10 milliseconds.
The RECOMMENDED value for overuse_time_th is 10 milliseconds.

## Rate control

The rate control is designed to increase the estimate of the available bandwidth as long as there is no detected congestion and to ensure that we will eventually match the available bandwidth of the channel and detect an over-use.

As soon as over-use has been detected, the available bandwidth estimated by the controller is decreased.

The rate control subsystem has 3 states: Increase, Decrease and Hold.

- "Increase" is the state when no congestion is detected
- "Decrease" is the state where congestion is detected
- "Hold" is a state that waits until built-up queues have drained before going to "increase" state.

The state transitions (with blank fields meaning "remain in state") are:

| Signal \ State | Hold     | Increase | Decrease |
| -------------- | -------- | -------- | -------- |
| Over-use       | Decrease | Decrease |          |
| Normal         | Increase |          | Hold     |
| Under-use      |          | Hold     | Hold     |

New bitrate message is sent once every update_interval duration, except when switching from Increase state to Decrease state where a new bitrate message will be sent immediately. The RECOMMENDED value of update_interval is 1 second

Herein we define incoming_bitrate R(i) as follows. It is the incoming bitrate measured over the last 1 second.

The new bitrate A(i) will be calculated as follows

### Increase bitrate

The bitrate is increased to a new value only if the following conditions are met. - System is currently in "Increase state" - update_interval duration has passed since we sent the last new bitrate message - R(i) is greater than BANDWITH_USED_THRESHOLD times of A(i-1). The RECOMMENDED value of BANDWITH_USED_THRESHOLD is 0.95

The bitrate A(i) is updated as per the below equation

    A(i) = A(i-1) * ((100 + increase_percent(i)) / 100)

The value of increase_percent(i), is a value that starts with a high initial value when current A(i) appears to be far from convergence. increase_percent(i) gets reduced gradually as we approach convergence. The exact method of calculating increase_percent(i) will specified later in this document.

### Decrease bitrate

The bitrate is decreased to a new value only if the following conditions are met. - System is currently in "Decrease state" - update_interval duration has passed since we sent the last "decrease" bitrate message

The bitrate A(i) is updated as per the below equation

    A(i) = R(i) * DECREASE_RATIO

The RECOMMENDED value of DECREASE_RATIO is 0.95

### Saturate Bandwidth, Increase percentage and Maximum Bandwidth Available

As one can observe from the above equations, the bitrate will be increased to a new value only if the incoming bitrate is above a particular threshold. But the video encoder will not generate enough bits to utilize the entire video bitrate configured when the client is idle and simple video content is being encoded. During such cases the bitrate cannot be increased to the available bandwidth till the client becomes active. In order to converge to the available bandwidth quickly, the algo will send a "saturate_bandwidth" signal to the server, so that server can send filler packets and/or FEC packets to send data as per the requested bitrate A(i). Once bitrate convergence is reached, "saturate_bandwidth" will be set to false so that we don't waste bandwidth unnecessarily.

Also the value increase_percent(i) should be kept higher at MAX_INCREASE_PERCENTAGE when we are looking to reach/find the maximum bandwidth available. When the birate search seems to approaching covergence then increase_percent(i) is reduced gradually.

Maximum bandwidth available in this connection is estimated iteratively as it used to decide on our convergence point. Whenever a bitrate exceeding the maximum bandwidth, is found to have worked without any congestion then this new bitrate is our maximum bandwidth available. Similarly if we find a bitrate lesser than maximum bandwidth, is found to cause congestion, then maximum bandwidth available is reduced as a EWMA of current maximum bandwidth and the last time a bitrate was used without congestion. This effectively means, that we will increase the maximum bandwidth available immediately/faster, but we will decrease the same in a slow EWMA-based decay. This is done to ensure that congestion that lasts for only a few seconds doesn't skew the maximum bandwidth available unreasonably. We want to err on the positive side of bandwidth available, so that we can use the maximum bitrate possible to ensure video quality.

The value saturate_bandwidth and increase_percent(i) is updated as per below

    if (A(i) > A(i-1)) {
        // Looks like the network has found a new max bitrate. Let find the new max bandwidth.
        last_successful_bitrate = A(i - 1)
        if (A(i - 1) > max_bandwidth_available) {
            max_bandwidth_available = A(i - 1);
            saturate_bandwidth = true;
            increase_percent(i) = MAX_INCREASE_PERCENTAGE;
        }
        if (network_settings->saturate_bandwidth == true) {
            increase_percentage = MAX_INCREASE_PERCENTAGE;
        }
    } else if (A(i) < A(i-1) && A(i) > max_bandwidth_available * CONVERGENCE_THRESHOLD_LOW) {
        saturate_bandwidth = false;
        increase_percent(i) = max(increase_percent(i - 1) / 2.0, MIN_INCREASE_PERCENTAGE);
        if (last_successful_bitrate < max_bitrate_available) {
            max_bitrate_available = max_bitrate_available * (1.0 - MAX_BITRATE_EWMA_FACTOR) +
                                    last_successful_bitrate * MAX_BITRATE_EWMA_FACTOR;
        }
    }

where,

The RECOMMENDED value of CONVERGENCE_THRESHOLD_LOW is 0.75
The RECOMMENDED value of MAX_INCREASE_PERCENTAGE is 16
The RECOMMENDED value of MIN_INCREASE_PERCENTAGE is 0.5
The RECOMMENDED value of MAX_BITRATE_EWMA_FACTOR is 0.1

#### Little tweaks

- When saturate bandwidth is off, bitflow will be low during idle frames and spiky in non-idle frames. To ensure averages are not skewed due to 1 or 2 spiky frames, we should wait for a longer duration to confirm overuse. It is RECOMMENDED to increase `overuse_time_th` is 100 milliseconds when saturate bandwidth is off.
- When saturate bandwidth is off, then incoming_bitrate is not reliable. So bitrate reduction during "Decrease state" cannot be made based on current bitrate. So we estimate a new bitrate based on the previous bitrate and turn on saturate bandwidth so that the actual bandwidth available can be found accurately. In such situations, a new bitrate in decrease state is calculated as below,

    A(i) = A(i - 1) * min(DECREASE_RATIO, 1.0 - (0.5 * packet_loss_ratio));

- When saturate bandwidth is on, we should not be wasting bandwidth unnecessarily if the bitrate is stuck in the same value. This can happen due to suddenly changing network conditions, flaws in our algorithm, etc., In such cases we don't want to be in an infinite quest to reach the maximum bitrate found earlier. If we cannot update the bitrate for more than 5 seconds, then we will switch off saturate bandwidth flag to avoid unnecessary wastage of network bandwidth.

### Burst Bitrate

When the available bandwidth is greater than the max_bitrate then the server can send bits at a greater rate which can reduce the latency significantly. When the available bandwidth A(i) exceeds a threshold bitrate, then we use a burst bitrate greater than available bandwidth that can potentially reduce latency. The burst bitrate B(i) is calculated as per the below equation

    if (A(i) > STARTING_BITRATE && saturate_bandwidth == false) {
        burst_mode = true;
    } else {
        burst_mode = false;
    }
    if (burst_mode)
        B(i) = A(i) * BURST_BITRATE_RATIO;
    else
        B(i) = A(i)

The RECOMMENDED value of BURST_BITRATE_RATIO is 4

The value of STARTING_BITRATE should be decided based on the application. It should be bitrate at which the target application can work smoothly with acceptable quality and performance. It is beyond the scope of this document to recommend any values for the same.

## RECOMMENDED values and reasons

| Parameter                    | RECOMMENDED value | Reason                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| ---------------------------- | ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| burst_time                   | 5ms               | From [Google Congestion control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-4)                                                                                                                                                                                                                                                                                                                                                                            |
| EWMA_FACTOR                  | 0.3               | Based on heuristics. Not sure if it will work for all cases. We might need to replace this with a kalman filter, where this factor is changed adaptively. TODO for future.                                                                                                                                                                                                                                                                                                           |
| del_var_th                   | 10ms              | Based on heuristics. Google Congestion control recommends an adaptive threshold of [6ms to 600ms](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-5.4), based on current network conditions. They mention that a static value will lead to starvation if there is a concurrent TCP flow. TODO for future.                                                                                                                                                      |
| overuse_time_th              | 10ms              | From [Google Congestion control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-5.4)                                                                                                                                                                                                                                                                                                                                                                          |
| update_interval              | 1 second          | From [Google Congestion control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-3)                                                                                                                                                                                                                                                                                                                                                                            |
| MAX_INCREASE_PERCENTAGE      | 16                | Based on heuristics. A 16% increase every 1 second, will mean that we move from MINIMUM bitrate to MAXIMUM bitrate in 12 seconds. This is quick enough to react to frequent variations in bandwidth. Google Congestion control uses a different algorithm for increasing bitrate that didn't work for me very well. Maybe a TODO for future, if ppl find any limitations with this |
| MIN_INCREASE_PERCENTAGE      | 0.5               | Based on heuristics. If we choose a very low value, then we might get stuck in a lower bitrate forever. If we choose a very high value, then we will not reach accurate convergence |
| MAX_BITRATE_EWMA_FACTOR      | 0.1               | Based on heuristics. We want the maximum bandwidth to reduce slowly in the event of intermittent congestion, but not too slow in the event of sustained congestion. |
| DECREASE_RATIO               | 0.95              | Based on heuristics or faster convergence. [Google Congestion control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02#section-5.5) suggests a value between 0.8 to 0.95                                                                                                                                                                                                                                                                                               |
| CONVERGENCE_THRESHOLD_LOW    | 0.75              | Based on heuristics or faster convergence. Google Congestion control does not have concepts such as conditional saturate bandwidth which uses this value. These are our improvisations                                                                                                                                                                                                                                                                                                   |
| BANDWITH_USED_THRESHOLD      | 0.95              | Based on heuristics. Google Congestion control have a such concept of conditional saturate bandwidth. It always assumes bandwidth is fully used. These are our improvisations                                                                                                                                                                                                                                                                                                        |
| BURST_BITRATE_RATIO          | 4                 | The current value being used in the dev branch, as of writing this document                                                                                                                                                                                                                                                                                                                                                                                                          |

# References

1. [Google Congestion Control](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-gcc-02)
