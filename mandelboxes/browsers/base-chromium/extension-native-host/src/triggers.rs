use inotify::{Inotify, WatchMask};
use std::io::Write;
use std::path::PathBuf;

#[derive(Debug, Clone, Copy)]
pub enum Trigger {
    FileDownload,
    PointerLock,
    DidUpdateExtension,
    KeyboardRepeatRate,
}

pub fn trigger_path(trigger: Trigger) -> PathBuf {
    PathBuf::from("/home/whist/.teleport").join(match trigger {
        Trigger::FileDownload => "downloaded-file",
        Trigger::PointerLock => "pointer-lock-update",
        Trigger::DidUpdateExtension => "did-update-extension",
        Trigger::KeyboardRepeatRate => "keyboard-repeat-rate-update",
    })
}

// Directly write a trigger to the filesystem.
pub fn write_trigger(trigger: Trigger, data: &str) -> Result<(), String> {
    let path = trigger_path(trigger);
    let mut file =
        std::fs::File::create(&path).map_err(|e| format!("trigger file create failed: {}", e))?;
    file.write_all(data.as_bytes())
        .map_err(|e| format!("trigger write failed: {}", e))?;
    Ok(())
}

// Wait for the protocol to handle and delete the trigger file
// (if it exists) before writing the new one.
pub fn write_trigger_sequential(trigger: Trigger, data: &str) -> Result<(), String> {
    let path = trigger_path(trigger);

    let mut inotify = Inotify::init().map_err(|e| format!("{}", e))?;
    match inotify.add_watch(path, WatchMask::DELETE) {
        Ok(_) => {}
        Err(e) => match e.kind() {
            std::io::ErrorKind::NotFound => {}
            _ => return Err(format!("inotify watch add failed: {}", e)),
        },
    };

    if trigger_path(trigger).exists() {
        let mut inotify_buffer = [0; 1024];
        inotify
            .read_events_blocking(&mut inotify_buffer)
            .map_err(|e| format!("inotify read failed: {}", e))?;
    }

    // At this point we have a guarantee that the trigger file doesn't exist.
    write_trigger(trigger, data)
}
