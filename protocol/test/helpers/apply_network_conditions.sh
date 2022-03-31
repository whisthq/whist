#!/bin/bash

set -Eeuo pipefail

## Sample usage:
# ./apply_network_conditions.sh -d ens55 -b 10Mbit,20Mbit -p 5,10 -q 50,100 -i 100,1000

# The command above will change the network conditions each 100 to 1000 ms and apply the following settings to device ens55:
# Bandwidth limited to a random value in the 10-20 Mbit/s range
# 5% - 10% of packets, chosen at random, are dropped
# Queue length is fixed at 50ms to 100ms

# SSH usage:
# nohup timeout <120s> ./apply_network_conditions.sh -d ens55 -b 10Mbit,20Mbit -p 5,10 -q 50,100 -i 100,1000 &


# Input arguments: -d [comma-separated device names] -b [bandwidth range] -p [packet drop range] -q [queue length range] -i [changing interval range]
while getopts d:b:p:q:i: flag
do
  case "${flag}" in
    d) devices=${OPTARG} ;;
    b) bandwidth_range=${OPTARG} ;;
    p) packet_drop_range=${OPTARG} ;;
    q) queue_length_range=${OPTARG} ;;
    i) interval_range=${OPTARG} ;;
  esac
done

IFS=","

# Check that at least one network device was specified
if [ -z "${devices:-}" ]; then
  echo "Cannot set network conditions -- no network device specified."
  exit 1
fi

# Read device names and save them in a array
read -a devices <<< "$devices"
echo "Network devices: ${devices[*]}"

# Check that at least one network condition was specified. If no network condition was specified, we don't have anything to do
if [ -z "${bandwidth_range:-}" ] && [ -z "${packet_drop_range:-}" ] && [ -z "${queue_length_range:-}" ] ; then
  echo "No artificial network condition passed."
  exit 0
fi

# Read the network conditions that are passed and parse the minimum and maximum value
if [ -n "${bandwidth_range:-}" ]; then
  read -a bandwidth_range <<< "$bandwidth_range";
  if [ ${#bandwidth_range[*]} -eq 2 ]; then
    max_bandwidth="${bandwidth_range[1]}"
  elif [ ${#bandwidth_range[*]} -eq 1 ]; then
    max_bandwidth="${bandwidth_range[0]}"
  else
    echo "Bandwidth must be specified either as a single value or as a range (comma-separated min-max values)"
    exit 1
  fi
  min_bandwidth="${bandwidth_range[0]}"

  # This script only supports bandwidth rates expressed with the format: <integer>{G,M,k}bit.
  # The max and min values in the range should also be expressed using the same units (Gbit, Mbit, kbit)
  if [[ ! ( "${min_bandwidth%????}" =~ ^[0-9]+$ ) || ! ( "${max_bandwidth%????}" =~ ^[0-9]+$ ) || "${min_bandwidth%????}" -lt 0 || "${min_bandwidth%????}" -gt "${max_bandwidth%????}" || ${min_bandwidth:(-4)} != ${max_bandwidth:(-4)} ]]; then
    echo "Bandwidth min and max must be positive integer values and they must be expressed using the same units (Gbit, Mbit, kbit). Min value should be <= max value."
    exit 1
  fi

  echo "Bandwidth: [ $min_bandwidth to $max_bandwidth ] "
fi

if [ -n "${packet_drop_range:-}" ]; then
  read -a packet_drop_range <<< "$packet_drop_range";
  if [ ${#packet_drop_range[*]} -eq 2 ]; then
    max_packet_drop=${packet_drop_range[1]}
  elif [ ${#packet_drop_range[*]} -eq 1 ]; then
    max_packet_drop=${packet_drop_range[0]}
  else
    echo "Packet drop must be specified either as a single value or as a range (comma-separated min-max values)."
    exit 1
  fi
  min_packet_drop=${packet_drop_range[0]}

  # This script only supports packet drop percentages expressed using integers between 0 and 100.
  if [[ ! ( "${min_packet_drop}" =~ ^[0-9]+$ ) || ! ( "${max_packet_drop}" =~ ^[0-9]+$ ) || "${min_packet_drop}" -lt 0 || "${max_packet_drop}" -gt 100 || "${min_packet_drop}" -gt "${max_packet_drop}" ]]; then
    echo "Packet drop rates should be integer percentages between 0 and 100. Min value should be <= max value."
    exit 1
  fi

  echo "Packet drop probability: [ $min_packet_drop to $max_packet_drop ]"
fi

if [ -n "${queue_length_range:-}" ]; then
  read -a queue_length_range <<< "$queue_length_range";
  if [ ${#queue_length_range[*]} -eq 2 ]; then
    max_queue_length=${queue_length_range[1]}
  elif [ ${#queue_length_range[*]} -eq 1 ]; then
    max_queue_length=${queue_length_range[0]}
  else
    echo "Queue length must be specified either as a single value or as a range (comma-separated min-max values)"
    exit 1
  fi
  min_queue_length=${queue_length_range[0]}

  # This script only supports queue lengths expressed in milliseconds and using integers.
  if [[ ! ( "${min_queue_length}" =~ ^[0-9]+$ ) || ! ( "${max_queue_length}" =~ ^[0-9]+$ ) || "${min_queue_length}" -lt 0 || "${min_queue_length}" -gt "${max_queue_length}" ]]; then
    echo "Queue lengths should be positive integer values expressed in milliseconds. Min value should be <= max value."
    exit 1
  fi

  echo "Packet transmission delay (queue length): [ $min_queue_length to $max_queue_length ]"
fi

if [ -n "${interval_range:-}" ]; then
  read -a interval_range <<< "$interval_range";
  if [ ${#interval_range[*]} -eq 2 ]; then
    max_interval=${interval_range[1]}
  elif [ ${#interval_range[*]} -eq 1 ]; then
    max_interval=${interval_range[0]}
  else
    echo "Interval duration must be specified either as a single value or as a range (comma-separated min-max values)"
    exit 1
  fi
  min_interval=${interval_range[0]}

  # This script only supports intervals expressed in milliseconds using integers.
  if [[ ! ( "${min_interval}" =~ ^[0-9]+$ ) || ! ( "${max_interval}" =~ ^[0-9]+$ ) || "${min_interval}" -lt 0 || "${min_interval}" -gt "${max_interval}" ]]; then
    echo "Intervals should be positive integer values expressed in milliseconds. Min value should be <= max value."
    exit 1
  fi

  echo "Interval duration: [ $min_interval to $max_interval ]"
else
  # If interval is not initialized, ensure that all passed network conditions are stable.
  if [[ ( "${min_bandwidth:-}" != "${max_bandwidth:-}" ) || ( "${min_packet_drop:-}" != "${max_packet_drop:-}" ) || ( "${min_queue_length:-}" != "${max_queue_length:-}" ) ]]; then
    echo "Interval must be specified unless all passed network conditions are stable."
    exit 1
  fi
fi


# Initial Setup for each device
#sudo modprobe ifb
for i in "${!devices[@]}"; do
  device="${devices[i]}"

  echo_string=""
  degradations_string=""
  if [ -n "${min_bandwidth:-}" ]; then
    echo_string="${echo_string} max bandwidth: ${min_bandwidth}"
    degradations_string="${degradations_string} rate ${min_bandwidth}"
  fi
  if [ -n "${max_packet_drop:-}" ]; then
    echo_string="${echo_string} packet drop rate: $max_packet_drop%"
    degradations_string="${degradations_string} loss ${max_packet_drop}%"
  fi
  if [ -n "${max_queue_length:-}" ]; then
    echo_string="${echo_string} queue length: ${max_queue_length}%"
    degradations_string="${degradations_string} delay ${max_queue_length}ms"
  fi
  echo "Setting network conditions on device $device to${echo_string}"

  sudo ip link set dev "ifb${i}" up
  sudo tc qdisc add dev "$device" ingress
  sudo tc filter add dev "$device" parent ffff: protocol ip u32 match u32 0 0 flowid 1:1 action mirred egress redirect dev "ifb${i}"
  sudo tc qdisc add dev "$device" root netem $degradations_string
  sudo tc qdisc add dev "ifb${i}" root netem $degradations_string
done

if [[ ( $min_bandwidth != $max_bandwidth ) || ( $min_packet_drop != $max_packet_drop ) || ( $min_queue_length != $max_queue_length ) ]]; then
  # Seed the random number generator to always get the same results
  RANDOM=34587

  # Recurrent net conditions variation
  while true
  do
    echo_string=""
    degradations_string=""
    if [ -n "${min_bandwidth:-}" ] && [ -n "${max_bandwidth:-}" ]; then
      bandwidth=$(( $RANDOM % (${max_bandwidth%????} - ${min_bandwidth%????} + 1) + ${min_bandwidth%????} ))
      bandwidth_unit=${min_bandwidth:(-4)}

      echo_string="${echo_string} max bandwidth: ${bandwidth}${bandwidth_unit}"
      degradations_string="${degradations_string} rate ${bandwidth}${bandwidth_unit}"
    fi
    if [ -n "${min_packet_drop:-}" ] && [ -n "${max_packet_drop:-}" ]; then
      packet_drop=$(( $RANDOM % (${max_packet_drop} - ${min_packet_drop} + 1) + ${min_packet_drop} ))
      echo_string="${echo_string} packet drop rate: $packet_drop%"
      degradations_string="${degradations_string} loss ${packet_drop}%"
    fi
    if [ -n "${min_queue_length:-}" ] && [ -n "${max_queue_length:-}" ]; then
      delay=$(( $RANDOM % (${max_queue_length} - ${min_queue_length} + 1) + ${min_queue_length} ))
      echo_string="${echo_string} queue length: ${delay} ms"
      degradations_string="${degradations_string} delay ${delay}ms"
    fi

    # Change the network conditions for each network device
    for i in "${!devices[@]}"; do
      device="${devices[i]}"
      echo "Setting network conditions on device $device to${echo_string}"
      sudo tc qdisc change dev "$device" root netem "$degradations_string"
      sudo tc qdisc change dev "ifb${i}" root netem "$degradations_string"
    done

    interval=$(( $RANDOM % (${max_interval} - ${min_interval} + 1) + ${min_interval} ))
    interval_seconds=$(($interval / 1000))
    interval_milliseconds=$(($interval - $interval_seconds))
    echo "Sleeping for ${interval_seconds}.${interval_milliseconds} seconds"
    # Sleep takes seconds as the smallest value, so we need to convert the randomly-generated interval number from ms to s
    sleep "${interval_seconds}.${interval_milliseconds}"
  done
fi
