const initUploadHandler = () => {
  let uploadFileButton = document.getElementById("uploadFileButton");

  if(uploadFileButton == null) {
    console.warn("Cannot find file uploader button!");
    return;
  }

  uploadFileButton.addEventListener("click", () => {
    chrome.runtime.sendMessage('file-upload-trigger', (response) => {});
  });

}

initUploadHandler();
