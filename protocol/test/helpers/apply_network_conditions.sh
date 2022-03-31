#!/bin/bash

set -Eeuo pipefail

## Sample usage:

# ./apply_network_conditions.sh -d ens55 -b 10Mbit,20Mbit -p 5,10 -q 50,100 -i 100,1000

# The command above will change the network conditions each 100 to 1000 ms and apply the following settings to device ens55:
# Bandwidth limited to a random value in the 10-20 Mbit/s range
# 5% - 10% of packets, chosen at random, are dropped
# Queue length is fixed at 50ms to 100ms


# Input arguments: -d [device name] -b [bandwidth range] -p [packet drop range] -q [queue length range] -i [changing interval range]
while getopts d:b:p:q:i: flag
do
  case "${flag}" in
    d) device=${OPTARG} ;;
    b) bandwidth_range=${OPTARG} ;;
    p) packet_drop_range=${OPTARG} ;;
    q) queue_length_range=${OPTARG} ;;
    i) interval_range=${OPTARG} ;;
  esac
done

IFS=","
read -a bandwidth_range <<< "$bandwidth_range"
read -a packet_drop_range <<< "$packet_drop_range"
read -a queue_length_range <<< "$queue_length_range"
read -a interval_range <<< "$interval_range"

if [ ${#bandwidth_range[*]} -eq 2 ]; then
  max_bandwidth="${bandwidth_range[1]}"
elif [ ${#bandwidth_range[*]} -eq 1 ]; then
  max_bandwidth="${bandwidth_range[0]}"
else
  echo "Bandwidth must be specified either as a single value or as a range (comma-separated min-max values)"
  exit 1
fi
min_bandwidth="${bandwidth_range[0]}"

if [ ${#packet_drop_range[*]} -eq 2 ]; then
  max_packet_drop=${packet_drop_range[1]}
elif [ ${#packet_drop_range[*]} -eq 1 ]; then
  max_packet_drop=${packet_drop_range[0]}
else
  echo "Packet drop must be specified either as a single value or as a range (comma-separated min-max values)."
  exit 1
fi
min_packet_drop=${packet_drop_range[0]}

if [ ${#queue_length_range[*]} -eq 2 ]; then
  max_queue_length=${queue_length_range[1]}
elif [ ${#queue_length_range[*]} -eq 1 ]; then
  max_queue_length=${queue_length_range[0]}
else
  echo "Queue length must be specified either as a single value or as a range (comma-separated min-max values)"
  exit 1
fi
min_queue_length=${queue_length_range[0]}

if [ ${#interval_range[*]} -eq 2 ]; then
  max_interval=${interval_range[1]}
elif [ ${#interval_range[*]} -eq 1 ]; then
  max_interval=${interval_range[0]}
else
  echo "Interval duration must be specified either as a single value or as a range (comma-separated min-max values)"
  exit 1
fi
min_interval=${interval_range[0]}

# This script only supports bandwidth rates expressed with the format: <integer>{G,M,k}bit.
# The max and min values in the range should also be expressed using the same units (Gbit, Mbit, kbit)
if [[ ! ( "${min_bandwidth%????}" =~ ^[0-9]+$ ) || ! ( "${max_bandwidth%????}" =~ ^[0-9]+$ ) || "${min_bandwidth%????}" -lt 0 || "${min_bandwidth%????}" -gt "${max_bandwidth%????}" || ${min_bandwidth:(-4)} != ${max_bandwidth:(-4)} ]]; then
  echo "Bandwidth min and max must be positive integer values and they must be expressed using the same units (Gbit, Mbit, kbit). Min value should be <= max value."
  exit 1
fi

# This script only supports packet drop percentages expressed using integers between 0 and 100.
if [[ ! ( "${min_packet_drop}" =~ ^[0-9]+$ ) || ! ( "${max_packet_drop}" =~ ^[0-9]+$ ) || "${min_packet_drop}" -lt 0 || "${max_packet_drop}" -gt 100 || "${min_packet_drop}" -gt "${max_packet_drop}" ]]; then
  echo "Packet drop rates should be integer percentages between 0 and 100. Min value should be <= max value."
  exit 1
fi

# This script only supports queue lengths expressed in milliseconds and using integers.
if [[ ! ( "${min_queue_length}" =~ ^[0-9]+$ ) || ! ( "${max_queue_length}" =~ ^[0-9]+$ ) || "${min_queue_length}" -lt 0 || "${min_queue_length}" -gt "${max_queue_length}" ]]; then
  echo "Queue lengths should be positive integer values expressed in milliseconds. Min value should be <= max value."
  exit 1
fi
# This script only supports intervals expressed in milliseconds using integers.
if [[ ! ( "${min_interval}" =~ ^[0-9]+$ ) || ! ( "${max_interval}" =~ ^[0-9]+$ ) || "${min_interval}" -lt 0 || "${min_interval}" -gt "${max_interval}" ]]; then
  echo "Intervals should be positive integer values expressed in milliseconds. Min value should be <= max value."
  exit 1
fi

echo "Network device: $device"
echo "Bandwidth: [ $min_bandwidth to $max_bandwidth ] "
echo "Min packet drop: [ $min_packet_drop to $max_packet_drop ]"
echo "Min queue length: [ $min_queue_length to $max_queue_length ]"
echo "Min interval duration: [ $min_interval to $max_interval ]"

# Initial Setup
sudo modprobe ifb
sudo ip link set dev ifb0 up
sudo tc qdisc add dev "$device" ingress
sudo tc filter add dev "$device" parent ffff: protocol ip u32 match u32 0 0 flowid 1:1 action mirred egress redirect dev ifb0
sudo tc qdisc add dev "$device" root netem delay "$max_queue_length"ms loss "$max_packet_drop"% rate "$min_bandwidth"
sudo tc qdisc add dev ifb0 root netem delay "$max_queue_length"ms loss "$max_packet_drop"% rate "$min_bandwidth"

echo "Setting network conditions on device $device to max bandwidth: $min_bandwidth, packet drop rate: $max_packet_drop%, queue length: $max_queue_length ms"

if [[ ( $min_bandwidth != $max_bandwidth ) || ( $min_packet_drop != $max_packet_drop ) || ( $min_queue_length != $max_queue_length ) ]]; then
    # Seed the random number generator to always get the same results
    RANDOM=34587
    
    # Recurrent net conditions variation
    while true
    do
        bandwidth=$(( $RANDOM % (${max_bandwidth%????} - ${min_bandwidth%????} + 1) + ${min_bandwidth%????} ))
        bandwidth_unit=${min_bandwidth:(-4)}
        packet_drop=$(( $RANDOM % (${max_packet_drop} - ${min_packet_drop} + 1) + ${min_packet_drop} )) 
        delay=$(( $RANDOM % (${max_queue_length} - ${min_queue_length} + 1) + ${min_queue_length} ))
        interval=$(( $RANDOM % (${max_interval} - ${min_interval} + 1) + ${min_interval} ))

        sudo tc qdisc change dev "$device" root netem delay "$delay"ms loss "$packet_drop"% rate "$bandwidth""$bandwidth_unit"
        sudo tc qdisc change dev ifb0 root netem delay "$delay"ms loss "$packet_drop"% rate "$bandwidth""$bandwidth_unit"

        interval_seconds=$(($interval / 1000))
        interval_milliseconds=$(($interval - $interval_seconds))

        echo "Setting network conditions on device $device to max bandwidth: ${bandwidth}${bandwidth_unit}, packet drop rate: $packet_drop%, queue length: $delay ms"
        echo "Sleeping for ${interval_seconds}.${interval_milliseconds} seconds"

        # Sleep takes seconds as the smallest value, so we need to convert the randomly-generated interval number from ms to s
        sleep "${interval_seconds}.${interval_milliseconds}"
    done
fi


