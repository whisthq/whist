#[macro_use]
extern crate neon;

use neon::prelude::*;

fn get_fonts(mut cx: FunctionContext) -> JsResult<JsObject> {
    let s1 = cx.string("hello");
    let s2 = cx.string("node");

    let array: Handle<JsObject> = cx.empty_array();

    array.set(&mut cx, 0, s1)?;
    array.set(&mut cx, 1, s2)?;

    Ok(array)
}

register_module!(mut cx, {
    cx.export_function("getFonts", get_fonts)
});
