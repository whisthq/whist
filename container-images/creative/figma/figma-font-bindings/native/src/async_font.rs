extern crate libfonthelper;

use neon::{prelude::*, task::Task};
use libfonthelper::{get_fonts, types::FontMap};
use super::fonts::Fonts;

struct Worker {
    dirs: Vec<String>,
}

impl Task for Worker {
    type Output = FontMap;
    type Error = String;
    type JsEvent = JsObject;

    fn perform(&self) -> Result<FontMap, String> {
        match get_fonts(&self.dirs) {
            Err(err) => return Err(String::from(err.to_string())),
            Ok(fonts) => return Ok(fonts),
        };
    }

    fn complete(self, mut cx: TaskContext, result: Result<FontMap, String>) -> JsResult<JsObject> {
        let JsFonts = JsObject::new(&mut cx);

        for (path, font) in result.unwrap() {
            let JsFontEnties = JsArray::new(&mut cx, font.len() as u32);

            for (index, entry) in font.iter().enumerate() {
                let JsFontEntry = JsObject::new(&mut cx);

                let id = cx.string(&entry.id);
                let postscript = cx.string(&entry.postscript);
                let family = cx.string(&entry.family);
                let style = cx.string(&entry.style);
                let weight = cx.number(i32::from(entry.weight));
                let stretch = cx.number(i32::from(entry.stretch));
                let italic = cx.boolean(entry.italic);

                JsFontEntry.set(&mut cx, "id", id);
                JsFontEntry.set(&mut cx, "postscript", postscript);
                JsFontEntry.set(&mut cx, "family", family);
                JsFontEntry.set(&mut cx, "style", style);
                JsFontEntry.set(&mut cx, "weight", weight);
                JsFontEntry.set(&mut cx, "stretch", stretch);
                JsFontEntry.set(&mut cx, "italic", italic);

                JsFontEnties.set(&mut cx, index.to_string().as_str(), JsFontEntry);
            }

            JsFonts.set(&mut cx, path.as_str(), JsFontEnties);
        }

        Ok(JsFonts)
    }
}


pub fn fonts_async(mut cx: FunctionContext) -> JsResult<JsUndefined> {
    let dirs: Handle<JsArray> = cx.argument::<JsArray>(0)?;
    let callback = cx.argument::<JsFunction>(1)?;

    let arr: Vec<String> = dirs
        .to_vec(&mut cx)
        .unwrap()
        .iter()
        .map(|js_value| {
            js_value
                .downcast::<JsString>()
                .unwrap()
                .value()
        })
        .collect();

    let worker = Worker { dirs: arr };
    worker.schedule(callback);

    Ok(cx.undefined())
}