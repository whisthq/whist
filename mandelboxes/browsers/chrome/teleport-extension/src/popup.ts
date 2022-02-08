const initUploadHandler = () => {
  let uploadFileButton = document.getElementById("uploadFileButton");
  const hostPort = chrome.runtime.connectNative("whist_teleport_extension_host")
  hostPort.onMessage.addListener(console.log)

  if(uploadFileButton == null) {
    return;
  }

  uploadFileButton.addEventListener("click", () => {
    console.log("Triggering file upload");
    hostPort.postMessage({ fileUploadTrigger: true})
  });

}

initUploadHandler();
