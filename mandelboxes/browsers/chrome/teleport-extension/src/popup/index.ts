// Set up pop upload button to trigger a whist file upload event
const initUploadHandler = () => {
  let uploadFileButton = document.getElementById("uploadFileButton");

  if(uploadFileButton == null) {
    console.warn("Cannot find file uploader button!");
    return;
  }

  // When clicked notify the service worker to trigger a host application message
  uploadFileButton.addEventListener("click", () => {
    chrome.runtime.sendMessage('file-upload-trigger', (response) => {});
  });

}

initUploadHandler();
