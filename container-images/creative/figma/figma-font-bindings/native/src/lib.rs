#[macro_use]
extern crate neon;

use neon::prelude::*;

mod fonts;
mod async_font;
mod types;

register_module!(mut cx, {
    cx.export_function("getFonts", async_font::fonts_async)
    // cx.export_function("getFontsSync", fonts::fonts)
});
