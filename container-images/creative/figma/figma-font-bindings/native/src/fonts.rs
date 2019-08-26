extern crate libfonthelper;

use neon::prelude::*;
use libfonthelper::{get_fonts, types::FontMap};

pub struct Fonts {
    fonts: FontMap,
}

impl Fonts {
    pub fn new(directories: Vec<String>) -> Self {
        Fonts {
            fonts: get_fonts(&directories).expect("Cannot get fonts"),
        }
    }

    pub fn fonts_to_js(self, mut cx: FunctionContext) -> JsResult<JsObject> {
        let JsFonts = JsObject::new(&mut cx);

        for (path, font) in &self.fonts {
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


