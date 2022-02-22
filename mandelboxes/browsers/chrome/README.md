# Whist Chrome Mandelbox

These are the files/resources used for the chrome mandelbox image.

## Teleport

Teleport is used primarily for various forms of file synchronization. The `teleport-extension` is installed into chrome and used for detecting file downloads along with various other things (like enforcing single-window, etc). `teleport-extension-host.py` is installed as a chrome native host application. It communicates with the extension as an liason to the server. The `teleport-kde-proxy.py` script is substitued with the kde kdialog binary to intercept file open/save requests from chrome.
