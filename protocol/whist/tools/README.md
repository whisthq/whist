# A Glimpse of Debug Console + Protocol Analyzer

Take a look at the screenshots inside this [PR](https://github.com/whisthq/whist/pull/5352).

# Debug Console

## Overview

When enabled, the `Debug Console` provides a telnet style console on port 9090 at client side, allows you to send commands to the client while it is running. You can change parameters of the client interactively/dynamically (e.g. change `bitrate` `audio_fec_ratio` `video_fec_ratio`), or let the program run some commands for you (e.g. get report from `protocol_analyzer`).

## How to enable

Debug Console can only be enabled with a `DEBUG` build.

To enable the debug console, add `--debug-console PORT` to the options of client, for example:

If without debug console, the command is:

```
./wclient 54.219.45.195 -p32262:30420.32263:35760.32273:4473 -k 1ef9aec50197a20aa35b68b85fb7ace0
```

Then the following command will run the client with debug console enable on port 9090:

```
./wclient 54.219.45.195 -p32262:30420.32263:35760.32273:4473 -k 1ef9aec50197a20aa35b68b85fb7ace0 --debug-console 9090
```

## How to access

the easiest way to access debug console is to use the tool `netcat`:

```
nc 127.0.0.1 9090 -u
```

Then you get a telnet style console. You can run commands (inside the console) and get outputs, in a way similar to a shell (e.g. `bash`).

the `rlwrap` tool can make your use of `netcat` easier, e.g. you can use the arrows to move cursor and recall previous command, when used with rlwrap, the full commands is:

```
rlwrap nc 127.0.0.1 9090 -u
```

(you can get both `netcat` and `rlwrap` easily from `apt` or `brew`)

## Support Commands

### `report_XXX`

use `report_video` or `report_audio` to get a report of the last `report_num` (default:2000) of records of video or audio.

`report_video_moreformat` or `report_audio_moreformat` are similar, increase the use of format string at the cost of a longer output.

examples:

```
report_video  video1.txt                    #get a report of video, save to video1.txt
report_audio  /home/yancey/audio1.txt       #get a report of audio, save to /home/yancey/audio1.txt
report_video_moreformat  video2.txt         #get a report of video, with more format attempt
report_audio_moreformat  audio2.txt.cpp     #get a report of audio, save the file as cpp, so that you can cheat your editor to highlight it
```

### `set`

Set allows you to change parameters inside the client dynamically.
Support parameters:

- `video_fec_ratio`, change the video fec ratio
- `audio_fec_ratio`, change the audio fec ratio
- `bitrate`, force the bitrate outputted by bitrate algorithm
- `burst_bitrate`, force the burst bitrate
- `no_minimize`, disable the MESSAGE_STOP_STREAMING sent by client when there is a window minimize, which might be annoying for debugging (e.g. when switching to other window and look at logs)
- `verbose_log`, toggle the verbose_log flag, enable more logs inside the client.
- `report_num`, change the number of records/frames included inside a report, default: 2000
- `skip_last`, skip the last n frames recorded by the analyzer, default:60. (the last few records might not have not arrived yet, including them will make metrics inaccurate)

Examples:

```
set video_fec_ratio 0.15             #set video fec ratio to 0.15
set verbose_log 1                    #enable verbose log
set bitrate 2000000                  #force bitrate to 2Mbps
set report_num 5000                  #include 5000 records in following reports
```

### `info`

Take a peek of the values of parameters supported by set

### `insert_atexit_handler` or `insert`

Insert a atexit handler that turns exit() to abort, so that we can get the stack calling exit(). It's helpful for debugging problems, when you don't know where in the code called exit(), especially the exit() is called in some lib.

# Protocol Analyzer

## Overview

This is a protocol analyzer in the side-way style, that you can talk to interactively via the debug console. Allows you to analyze the protocol dynamically, get advanced metrics.

Features/Advantages:

- It only gives output if you ask to, instead of spamming the log. As a result you can put as many infos as you like.
- It tracks the life of frames/segments, organize info in a compact structured way. Reduce eye parsing and brain processing.
- Centralized way to put meta infos, allows advanced metric to be implemented easily.
- It hooks into the protocol to track the protocol's running, instead of inserting attributes into the data structures of protocol. Almost doesn't increase the complexity of code of protocol

Disadvantages:

- when you make serious changes to the protocol, you need to update the protocol analyzer to track it correctly, increases maintain burden of codebase. But (1) this usually won't happen unless something essential is changed in protocol (2) when it happens it should be easy to update the analyzer

## How to run

When debug console is enabled, the protocol analyzer is automatically enabled. You don't need to explicitly start it, all you need to do is talk to it via `debug_console`. To get report from it, or change some parameters (e.g `report_num`, `skip_last`)

## Report format

The report of protocol analyzer consists two sections: (1) high level information/statistics (2) a breakdown of all frames inside the report

### high level information/statistic

This section is a list of information/statistic, consist of:

- `type`, whether the report is for audio or video. Possible values: `VIDEO` or `AUDIO`
- `frame_count`, number of frames includes in the report
- `frame_seen_count`, number of frames that are seen, i.e, we received at least one segment of those frames.
- `first_seen_to_ready_time`, the average time that frames become ready to render from first seen. Unit: ms
- `first_seen_to_decode_time`, the average time that frames are fed to decode from first seen. Unit: ms
- `recover_by_nack`, the percentage of frames recovered by nack. "Recovered by nack" means, when the frame becomes ready, at least one retransmission of packet arrives
- `recover_by_fec`, the percentage of frames recovered by fec.
- `recover_by_fec_after_nack`, the percentage of frames that is recovered by fec followed after nack. Or you think those frames are recovered by both fec and nack.
- `false_drop`, the percentage of frames that becomes ready but never feed into decoder
- `recoverable_drop`, the percentage of frames that only lost a small proportion (<10%), should be recovered easily but actually never become ready.
- `not_seen`, the percentage of frames that we never saw a single segment for.
- `not_ready`, the percentage of frames that never become ready.
- `not_decode`, the percentage of frames that was never feed into decoder
- `frame_skip_times`, the times of frames skips happened
- `num_frames_skiped`, the number of total frame skipped
- `ringbuffer_reset_times`, the times of ringbuffer reset happened
- `estimated_network_loss`, the estimated network packet loss. (only exist in `VIDEO` report)

All time are the relative time since client start. This also applies to the sections below, in the whole report.

### break down of frames

This section is a list of frame infos, each one consist of:

Main Attributes(always showing):

- `id`, id of the frame
- `type`, type of frame. Possible values: `VIDEO` or `AUDIO`
- `num`, the number of all packets inside the frame, including fec packets.
- `fec`, the number of fec packets inside the frame.
- `is_iframe`, whether the frame is iframe or not. Possible values: `1`, `0`. (Only for `Video`)
- `first_seen`, the time we first seen the frame, i.e. the first segment of the frame arrives
- `ready_time`, the time when the frame becomes ready to render, correspond to the `is_ready_to_render()` insider `ringbuffer.c`
- `pending_time`, the time when the frame becomes pending, i.e. in a state that is waiting to be taken by the decoder thread
- `decode_time`, the time when the frame is fed into the decoder.
- `segment`, a break down of segments info

(In addition to the possible values, the value can be `-1` or `null`, indicate there is no value available)

Other attributes(onlys shows when there are something interesting happens):

- `nack_used`, when this shows, it means nack is used for recovering this frame
- `fec_used`, when this shows, it means fec is used fro recovering this frame
- `skip_to`, when this shows, it means there is a frame skip from the current frame to the frame `skip_to` points to
- `reset_ringbuffer_to`, when this shows, it means there is a ringbuffer reset from the current frame to the frame `reset_ringbuffer_to` points to
- `reset_ringbuffer_from`, when this shows, it means there is a ringbuffer reset from the frame `reset_ringbuffer_from` points to, to the current frame. (kind of redundant with above)
- `overwrite`, when this shows, it means when the current frame becomes "current rendering" inside ringbuffer, it overwrites a frame that never becomes pending
- `queue full`, when this shows, it means when the current frame tries to become pending, it triggers an "Audio queue full". (Only for audio)

#### breakdown of segments

The breakdown of segments is a list of items. Each items correspond of a segment of the frame, contains:

- `idx`, the index of segment. this `idx` label is omitted by default output
- a list contains 3 sections:
  - `arrival`, a list of all times of packets which arrive without an `is_a_nack` flag, i.e. the non-retransmit packets. The label is omitted by default output.
  - `retrans_arrival`, a list all times of packets which arrive with an `is_a_nack` flag, i.e. the retransmitted packets. The label is shown as `retr` by default output, for short.
  - `nack_sent` , a list of all times of an NACK is sent for this segment. The label is shown as `nack` by default output, for short.

Note about the terminology: A `segment` is a segment of `frame`, a `segment` is sent via a `packet`. There might be multiple `packets` contains the same `segment`.

An example that helps understand, with default format:

```
segments=[
{0[4542.4,4543.0]},
{1[4545.0]},
{2[4545.1; retr:4630.0; nack:4545.0]},
{3[retr:4640.3; nack:4560.0]},
{4[retr:4680.0; nack:4561.0, 4610.0]},
{5[4545.2]}]
```

or with the `moreformat` format:

```
segments=[
{idx=0,[arrival:4542.4,4543.0]},
{idx=1,[arrival:4545.0]},
{idx=2,[arrival:4546.1;retrans_arrival:4630.0; nack_sent:4545.0]},
{idx=3,[retrans_arrival:4640.3; nack_sent:4560.0]},
{idx=4,[retrans_arrival:4680.0; nack_send:4561.0, 4610.0]},
{idx=5,[arrivale:4545.2]}]
```

(this is an artificial example for help understanding, it should be rare to see such segments in real use )

(the outputs are in one line in the real output, instead of multiple lines, to reduce page space. an editor plug-in such as `rainbow_parentheses`, significantly increase the readability, it's very suggested to install one)

In the example, the frame consist of 6 segments:

1. The 0th segment arrives twice without nack, there might be a network duplication.
2. the 1st segment arrives as normal
3. for the 2nd segment, the non-retransmit packet successfully arrives, but there is still an NACK sent, and we got the retransmitted segment
4. for the 3rd segment, the non-retransmit packet got lost, and NACK was sent, we got the retransmitted segment.
5. for the 4th segment, the non-retransmit packet got lost, and NACK was sent, the retransmit segment got lost, we sent and NACK again. And finally got the segment.
6. the 5th segment arrives as normal.

## Implementation

This section talks about the implementation of protocol analyzer, hopefully you can get an idea on how to extend it.

The protocol analyzer mainly consist of:

1. data structures, that store the recorded info
2. hooks, that hook into the protocol to record info
3. statistics generator, for the high-level info/stat in report
4. pretty printer for the frame breakdown in report

### Data structures

Below are the data structures used for protocol analyzer.

```
//info at segment level
Struct SegmentLevelInfo
{
... //attributes
}

//info at frame level
struct FrameLevelInfo {
map<int, SegmentLevelInfo> segments;
... //attributes
}

//a map from id to frame
typedef map<int, FrameLevelInfo> FrameMap;

// type level info.  By "type" I mean VIDEO or AUDIO
struct TypeLevelInfo {
    FrameMap frames;
    ... //attributes
};

//the protocol analyzer class
struct ProtocolAnalyzer {
    map<int, TypeLevelInfo> type_level_infos;   //a map from type to type level info.
}

```

If you want to record new info, you will need to put it as an attribute of the data structures.

### Hooks

Hooks are the public functions with name `whist_analyzer_record_XXXXX()`, they hook into the protocol and save info into the data structure, for example:

```
// record info related to the arrival of a segment
void whist_analyzer_record_segment(WhistSegment *packet)
{
  FUNC_WRAPPER(record_segment, packet);
}
```

It's a wrapper of the corresponding method of ProtocolAnalyzer class:

```
void ProtocolAnalyzer::record_segment()
{
//real implementation is here
}

```

If you want to record new info, you might need to implement new hooks.

### Statistics generator

The Statistics generator is just the function:

```
string ProtocolAnalyzer::get_stat(）
{
...
}
```

If you want to implement new metric/stat, add code to this function

### Pretty printer for frame breakdown

The pretty printer for frame breakdown is just the function:

```
string FrameLevelInfo::to_string()
{
...
}
```

If you want new attributes of frames to be printed out, add code to this function.
