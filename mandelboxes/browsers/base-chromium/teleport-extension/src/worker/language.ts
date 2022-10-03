import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"

import { Socket } from "socket.io-client"

const initLanguageHandler = (socket: Socket) => {
	socket.on("language-changed", ([language, languages]: [string, [string]]) => {
		// When language has changed, tell all tabs about the
		//     updated languages.
		chrome.tabs.query({}, (tabs) => {
			tabs.forEach((tab) => {
				if (tab.id) {
					chrome.tabs.sendMessage(
						tab.id,
						{
							type: ContentScriptMessageType.LANGUAGE_UPDATE,
							value: {
								language, languages
							}
						} as ContentScriptMessage,
					)
				}
			})
		})
	})
}