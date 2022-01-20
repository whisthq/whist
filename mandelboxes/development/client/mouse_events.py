#!/usr/bin/env python3

from pynput.mouse import Button, Controller
import time


def mouse_scroll(pos_x, pos_y, scroll, sleep_duration_ms, num_iterations):
    mouse = Controller()
    mouse.position = (pos_x, pos_y)
    for x in range(num_iterations):
        mouse.scroll(0, scroll)
        time.sleep(sleep_duration_ms / 1000)


if __name__ == "__main__":
    # Just ensure the mouse pointer is not in the address bar or side bar, so that scrolling works.
    # Assuming side bar is in the left
    # Chosen this coordinate, so that it will work for most resolutions/webpage.
    pos_x = 800
    pos_y = 400
    # Scroll length is chosen as 3, based on the number seen during manual scrolling.
    scroll_length = 3
    # Sleep for smaller duration to simulate fast scrolling.
    # Again this number was chosen based on data seen during manual scrolling.
    fast_scroll_sleep_ms = 20
    # Sleep for a little longer duration to simulate slower scrolling.
    slow_scroll_sleep_ms = 100
    # The below number of iterations are good enough to scroll a long webpage with lot of text.
    # For example : https://docs.nvidia.com/video-technologies/video-codec-sdk/nvenc-video-encoder-api-prog-guide/
    num_iterations = 75

    # Sleep for a sometime before switching between scrolling up and down.
    # Just to simulate manual reading before scrolling.
    sleep_inbetween_scroll = 1  # In seconds

    # Fast scroll down
    mouse_scroll(pos_x, pos_y, -scroll_length, fast_scroll_sleep_ms, num_iterations)
    time.sleep(sleep_inbetween_scroll)

    # Fast scroll up
    mouse_scroll(pos_x, pos_y, scroll_length, fast_scroll_sleep_ms, num_iterations)
    time.sleep(sleep_inbetween_scroll)

    # Slow scroll down
    mouse_scroll(pos_x, pos_y, -scroll_length, slow_scroll_sleep_ms, num_iterations)
    time.sleep(sleep_inbetween_scroll)

    # Slow scroll up
    mouse_scroll(pos_x, pos_y, scroll_length, slow_scroll_sleep_ms, num_iterations)
