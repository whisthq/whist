extern crate libfonthelper;

use libfonthelper::{font::Font, Fonts};
use neon::{prelude::*, task::Task};

struct Worker {
    dirs: Vec<String>,
}

impl Task for Worker {
    type Output = Vec<Font>;
    type Error = String;
    type JsEvent = JsObject;

    fn perform(&self) -> Result<Vec<Font>, String> {
        match Fonts::new(&self.dirs) {
            Err(err) => return Err(String::from(err.to_string())),
            Ok(fonts) => {
                let mut map: Vec<Font> = Vec::new();
                for font in fonts {
                    map.push(font);
                }

                return Ok(map);
            }
        };
    }

    fn complete(
        self,
        mut cx: TaskContext,
        result: Result<Vec<Font>, String>,
    ) -> JsResult<JsObject> {
        let js_fonts = JsObject::new(&mut cx);

        match result {
            Err(err) => println!("Cannot get fonts, error: {}", err),
            Ok(fonts) => {
                for font in fonts {
                    if (font.path.eq("")) {
                        continue;
                    }

                    let js_font_entries = JsArray::new(&mut cx, font.entries.len() as u32);

                    for (index, entry) in font.entries.iter().enumerate() {
                        let js_font_entry = JsObject::new(&mut cx);

                        let id = cx.string(&entry.id);
                        let postscript = cx.string(&entry.postscript);
                        let family = cx.string(&entry.family);
                        let style = cx.string(&entry.style);
                        let weight = cx.number(i32::from(entry.weight));
                        let stretch = cx.number(i32::from(entry.stretch));
                        let italic = cx.boolean(entry.italic);

                        js_font_entry.set(&mut cx, "id", id)?;
                        js_font_entry.set(&mut cx, "postscript", postscript)?;
                        js_font_entry.set(&mut cx, "family", family)?;
                        js_font_entry.set(&mut cx, "style", style)?;
                        js_font_entry.set(&mut cx, "weight", weight)?;
                        js_font_entry.set(&mut cx, "stretch", stretch)?;
                        js_font_entry.set(&mut cx, "italic", italic)?;

                        js_font_entries.set(&mut cx, index.to_string().as_str(), js_font_entry)?;
                    }

                    js_fonts.set(&mut cx, font.path.as_str(), js_font_entries)?;
                }
            }
        }

        Ok(js_fonts)
    }
}

pub fn fonts_async(mut cx: FunctionContext) -> JsResult<JsUndefined> {
    let dirs: Handle<JsArray> = cx.argument::<JsArray>(0)?;
    let callback = cx.argument::<JsFunction>(1)?;

    let arr: Vec<String> = dirs
        .to_vec(&mut cx)
        .unwrap()
        .iter()
        .map(|js_value| js_value.downcast::<JsString>().unwrap().value())
        .collect();

    let worker = Worker { dirs: arr };
    worker.schedule(callback);

    Ok(cx.undefined())
}
