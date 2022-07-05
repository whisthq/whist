# Whist Spicy Cursors

The core challenge of cursors when streaming Linux is that X11 can successfully grab the cursor _image_ and _metadata_, but not the cursor _type_. So when we capture cursor type on the server, we have to jump through inordinate hoops to get a cursor type to send to the client.

Instead, we took the cursor bitmap data, compressed to PNG, and sent that to the client for SDL to render as a custom cursor image. To make things feel more native, we then picked a cursor theme for Linux designed to look like the macOS native cursors.

This caused two issues. First, what about Windows users? And second, the cursors had to be scaled up client side according to DPI scaling factor, which caused blurriness and pixel artifacts. Users would often notice and complain.

Earlier attempts to solve this problem involved pre-computing hashes for all the native cursor types. Then, we could hash incoming cursor images and associate them to the appropriate system cursor. This failed for a number of reasons, but the biggest one was that there were just too many cursors! Every frame of animation had to be handled, as well as multiple different cursor sizes which occurred on different DPIs.

Finally, we found the correct solution! ("Correct", maybe.) Create our own cursor theme, with only one size. No more pesky DPI resizing. Next, we needed a way to map captured cursor data to the GTK cursor type. The initial idea was to continue to use hashing, or else to otherwise encode cursor type in the pixel data. But a simpler solution worked instead!

To indicate that a cursor is a GTK system cursor and not some custom cursor, we set the image to a 32 by 32 square with distinctive colors in each quadrant. The northwest is `#42f58a`, the northeast is `#e742f5`, the southeast is `#ff7e20`, and the southwest is `#2f7aeb`.

Next, we encode the cursor type in the _metadata_! Namely, in the hot x and y coordinate of the cursor. Hence the name "spicy", or "hot" cursors!

How do we do the encoding? We first alphabetize the GTK cursor names, and zero-index them to get an index `i`. We map each index to the `(x, y)` coordinate pair `((i % 7) * 3, (i / 7) * 3)`, where division is integer divison.

To reconstruct the index from a coordinate pair, we just compute `(x / 3) + 7 * (y / 3)`. Obviously, much more efficient packings are possible. This choice was intended to allow us to salvage this approach even if cursors were scaled according to DPI, since they would never be scaled by more than a factor of 3 from a 32 by 32 image. Fortunately, it seems that we won't need to use this robustness.

Lastly, GTK cursor theme types needed to be mapped to a new `WhistCursorType` enum, and subsequently to SDL, Chromium (CSS), and/or macOS and Windows cursor types. This work can be seen [here](https://docs.google.com/spreadsheets/d/1rfaAAePjQrz_19CTkpN4zmnNJ_FOSrjOfWbyGLgE4zE/edit?usp=sharing).
