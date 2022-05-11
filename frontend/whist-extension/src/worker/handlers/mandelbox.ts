import { ipcMessage } from "@app/worker/utils/messaging"

import { ContentScriptMessageType } from "@app/constants/messaging"

const initMandelboxAssign = () => {
  ipcMessage(ContentScriptMessageType.AUTH_SUCCESS).subscribe(() => {})
}

export { initMandelboxAssign }
