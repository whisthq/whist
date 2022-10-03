import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

const initLanguageHandler = (socket: Socket) => {
	socket.on("language-changed", ([language, languages]) => {
		// When language has changed, tell all tabs about the
		//     updated languages.
		chrome.tabs.query({}, (tabs) => {
			tabs.forEach((tab) => {
				chrome.tabs.sendMessage(
					tab.id,
					{
						type: ContentScriptMessageType.LANGUAGE_UPDATE,
						value: {
							language, languages
						}
					} as ContentScriptMessage,
				)
			})
		})
	})
}