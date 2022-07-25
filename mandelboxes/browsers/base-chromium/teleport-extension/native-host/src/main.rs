use inotify::{Inotify, WatchMask};
use serde::{Deserialize, Serialize};
use serde_json::{json, Value};
use std::io::{Read, Write};
use std::path::PathBuf;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::thread;

// TODO: Make this automatically in-sync with the typescript src/constants/ipc.ts.
// Until then, we must manually keep these in sync. Also, we could eventually force
// a schema for "value" based on "type".
#[derive(Serialize, Deserialize)]
enum NativeHostMessageType {
    #[serde(rename = "DOWNLOAD_COMPLETE")]
    DownloadComplete,
    #[serde(rename = "GEOLOCATION")]
    Geolocation,
    // Technically we abuse notation -- if we _send_ to the extension, it will
    // disconnect the native host. But we use this internally to signal various
    // _received_ messages from the extension which mean that we should exit from
    // the host.
    #[serde(rename = "NATIVE_HOST_EXIT")]
    NativeHostExit,
    #[serde(rename = "NATIVE_HOST_UPDATE")]
    NativeHostUpdate,
    #[serde(rename = "POINTER_LOCK")]
    PointerLock,
    #[serde(rename = "NATIVE_HOST_KEEPALIVE")]
    NativeHostKeepalive,
}

enum Trigger {
    FileDownload,
    PointerLock,
    DidUpdateExtension,
}

const TRIGGER_FOLDER: &str = "/home/whist/.teleport";
fn trigger_path(trigger: &Trigger) -> PathBuf {
    match trigger {
        Trigger::FileDownload => PathBuf::from(TRIGGER_FOLDER).join("downloaded-file"),
        Trigger::PointerLock => PathBuf::from(TRIGGER_FOLDER).join("pointer-lock-update"),
        Trigger::DidUpdateExtension => PathBuf::from(TRIGGER_FOLDER).join("did-update-extension"),
    }
}

// Directly write a trigger to the filesystem.
fn write_trigger(trigger: &Trigger, data: &str) -> Result<(), String> {
    let path = trigger_path(trigger);
    let mut file = std::fs::File::create(&path).map_err(|e| format!("{}", e))?;
    file.write_all(data.as_bytes())
        .map_err(|e| format!("{}", e))?;
    Ok(())
}

fn trigger_file_exists(trigger: &Trigger) -> bool {
    trigger_path(trigger).exists()
}

// Wait for the protocol to handle and delete the trigger file
// (if it exists) before writing the new one.
fn write_trigger_sequential(trigger: &Trigger, data: &str) -> Result<(), String> {
    let path = trigger_path(trigger);

    let mut inotify = Inotify::init().map_err(|e| format!("{}", e))?;
    inotify
        .add_watch(path, WatchMask::DELETE)
        .map_err(|e| format!("{}", e))?;

    if trigger_file_exists(trigger) {
        let mut inotify_buffer = [0; 1024];
        inotify
            .read_events_blocking(&mut inotify_buffer)
            .map_err(|e| format!("{}", e))?;
    }

    // At this point we have a guarantee that the trigger file doesn't exist.
    write_trigger(trigger, data)
}

#[derive(Serialize, Deserialize)]
struct NativeHostMessage {
    #[serde(rename = "type")]
    msg_type: NativeHostMessageType,
    value: Value,
}

fn send_message<W: Write>(mut output: W, msg: &NativeHostMessage) -> Result<(), String> {
    let msg_str = serde_json::to_string(msg).map_err(|e| e.to_string())?;
    let msg_len = msg_str.len();
    // Chromium limits size to 1 MB.
    if msg_len > 1024 * 1024 {
        return Err(format!("Message is too large: {} bytes", msg_len));
    }

    // [msg_len in native endian (4 bytes), msg_str in UTF-8 (msg_len bytes)]
    let mut msg_buf: Vec<u8> = Vec::new();
    msg_buf.extend_from_slice(&msg_len.to_ne_bytes());
    msg_buf.extend_from_slice(msg_str.as_bytes());

    // Note that by default, stdout should grab a lock, so writing will
    // be thread-safe.
    output.write_all(&msg_buf).map_err(|e| e.to_string())?;
    output.flush().map_err(|e| e.to_string())?;
    Ok(())
}

fn read_message<R: Read>(mut input: R) -> Result<NativeHostMessage, String> {
    let mut len_buf = [0; 4];
    match input
        .read_exact(&mut len_buf)
        .map(|()| i32::from_ne_bytes(len_buf))
    {
        Ok(msg_len) => {
            if msg_len == -1 {
                // (Undocumented Chromium behavior) The browser is exiting, so
                // tell the extension to gracefully disconnect this native host.
                return Ok(NativeHostMessage {
                    msg_type: NativeHostMessageType::NativeHostExit,
                    value: json!(null),
                });
            }
            let mut msg_buf = vec![0; msg_len as usize];
            input.read_exact(&mut msg_buf).map_err(|e| e.to_string())?;
            let value = serde_json::from_slice(&msg_buf).map_err(|e| e.to_string())?;
            Ok(value)
        }
        Err(e) => match e.kind() {
            std::io::ErrorKind::UnexpectedEof => Ok(NativeHostMessage {
                msg_type: NativeHostMessageType::NativeHostExit,
                value: json!(null),
            }),
            _ => Err(e.to_string()),
        },
    }
}

// If the extension hasn't tried updating yet, then tell the
// extension to update and use the trigger to keep track of this
// fact. Else, just tell the extension that we're ready to go.
fn check_for_updates() -> Result<(), String> {
    let needs_update = !trigger_file_exists(&Trigger::DidUpdateExtension);
    if needs_update {
        write_trigger(&Trigger::DidUpdateExtension, "")?;
    }
    send_message(
        std::io::stdout(),
        &NativeHostMessage {
            msg_type: NativeHostMessageType::NativeHostUpdate,
            value: json!(needs_update),
        },
    )
}

fn main() -> Result<(), String> {
    check_for_updates()?;

    let longitude = std::env::var("LONGITUDE").unwrap_or("".to_string());
    let latitude = std::env::var("LATITUDE").unwrap_or("".to_string());
    if !longitude.is_empty() && !latitude.is_empty() {
        send_message(
            std::io::stdout(),
            &NativeHostMessage {
                msg_type: NativeHostMessageType::Geolocation,
                value: json!({
                    "longitude": longitude,
                    "latitude": latitude,
                }),
            },
        )?;
    }

    let keep_running = Arc::new(AtomicBool::new(true));

    let keep_running_keepalive = keep_running.clone();
    let keepalive_thread = thread::spawn(move || -> Result<(), String> {
        let msg = NativeHostMessage {
            msg_type: NativeHostMessageType::NativeHostKeepalive,
            value: json!(null),
        };
        let stdout = std::io::stdout();
        while keep_running_keepalive.load(Ordering::Relaxed) {
            send_message(&stdout, &msg)?;
            thread::sleep(std::time::Duration::from_secs(15));
        }
        Ok(())
    });

    loop {
        match read_message(std::io::stdin()) {
            Ok(msg) => match msg.msg_type {
                NativeHostMessageType::NativeHostExit => break,
                NativeHostMessageType::DownloadComplete => {
                    let relative_filename = match msg.value.as_str() {
                        Some(filename) => match filename.split("/").last() {
                            Some(filename) => filename.to_string(),
                            None => {
                                eprintln!("Invalid filename: {}", filename);
                                continue;
                            }
                        },
                        None => {
                            eprintln!("DownloadComplete message did not contain a filename");
                            continue;
                        }
                    };
                    write_trigger_sequential(&Trigger::FileDownload, &relative_filename)?;
                }
                NativeHostMessageType::PointerLock => {
                    // These technically don't need to be sequential since we only care about
                    // latest state. But for now we do this to avoid any potential race conditions
                    // between the protocol reading and deleting the trigger file.
                    write_trigger_sequential(
                        &Trigger::PointerLock,
                        match msg.value.as_bool() {
                            Some(true) => "1",
                            Some(false) => "0",
                            None => {
                                eprintln!("PointerLock message did not contain a boolean");
                                continue;
                            }
                        },
                    )?;
                }
                _ => {}
            },
            Err(e) => eprintln!("{}", e),
        }
    }

    keep_running.store(false, Ordering::Relaxed);
    match keepalive_thread.join().unwrap() {
        Ok(_) => Ok(()),
        Err(e) => {
            eprintln!("Error in keepalive thread: {}", e);
            std::process::exit(1);
        }
    }
}
