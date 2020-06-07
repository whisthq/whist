package org.fractal.app;

import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.util.Log;

import org.libsdl.app.SDLActivity;

public class Fractal extends SDLActivity {
    static Fractal __singleton;
    static boolean __changed;
    static String[] __types= {ClipDescription.MIMETYPE_TEXT_PLAIN, ClipDescription.MIMETYPE_TEXT_INTENT, ClipDescription.MIMETYPE_TEXT_URILIST};
    public Fractal() {
        __singleton = this;
    }

    public static Object log(String s) {
        Log.d("Native", s);
        return new Object();
    }

    public static void initClipboard() {
        __changed = true;
        ClipboardManager clipboard = (ClipboardManager) __singleton.getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard != null) {
            clipboard.addPrimaryClipChangedListener(new ClipboardManager.OnPrimaryClipChangedListener() {
                @Override
                public void onPrimaryClipChanged() {
                    __changed = true;
                }
            });
        }

        Log.d("Native", "CALL : initClipboard");
    }

    public static boolean hasClipboardUpdated() {
        return __changed;
    }

    public static int GetClipboardType() {
        ClipboardManager clipboard = (ClipboardManager) __singleton.getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard != null && clipboard.getPrimaryClip() != null) {
            if(clipboard.getPrimaryClip().getDescription().hasMimeType(ClipDescription.MIMETYPE_TEXT_INTENT))
                return 2;
            if(clipboard.getPrimaryClip().getDescription().hasMimeType(ClipDescription.MIMETYPE_TEXT_URILIST))
                return 3;
            if(clipboard.getPrimaryClip().getDescription().hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN))
                return 1;
        }
        return 0;
    }

    public static String GetClipboard() {
        ClipboardManager clipboard = (ClipboardManager) __singleton.getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard != null) {
            ClipData data = clipboard.getPrimaryClip();
            if (data != null) {
                Log.d("Native", "CALL : GetClipboard");
                Log.d("CLIP BOARD CONTENT : ", data.getItemAt(0).coerceToText(__singleton.getApplicationContext()).toString());
                return data.getItemAt(0).coerceToText(__singleton.getApplicationContext()).toString();
            }
        }
        return "";
    }

    public static int GetClipboardSize() {
        ClipboardManager clipboard = (ClipboardManager) __singleton.getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard != null) {
            ClipData data = clipboard.getPrimaryClip();
            if (data != null) {
                String str = data.getItemAt(0).coerceToText(__singleton.getApplicationContext()).toString();
                return str.length();
            }
        }
        return 0;
    }

    public static void SetClipboard(int size, int type, String text) {
        ClipboardManager clipboard = (ClipboardManager) __singleton.getSystemService(Context.CLIPBOARD_SERVICE);
        if(clipboard != null && type != 0) {
            ClipData clip = ClipData.newPlainText(__types[type - 1], text);
            clipboard.setPrimaryClip(clip);
        }
    }
}
