#[macro_use]
extern crate neon;

mod async_font;

register_module!(mut cx, {
    cx.export_function("getFonts", async_font::fonts_async)
});
