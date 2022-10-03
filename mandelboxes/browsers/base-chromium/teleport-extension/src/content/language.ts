import { injectResourceIntoDOM } from "@app/utils/dom"

const initLanguageSpoofer = () => {
	injectResourceIntoDOM(document, "js/language.js")

	console.log(navigator)
	Object.defineProperty(navigator, 'language', {value: "boi"})
	console.log("updated ", navigator)

	// // Listen for LANGUAGE_UPDATE message and update corresponding meta tag
	// chrome.runtime.onMessage.addListener(async (msg: ContentScriptMessage) => {
	// 	if (msg.type !== ContentScriptMessageType.LANGUAGE_UPDATE) return

	// 	const metaLanguage = document.documentElement.querySelector(
	// 		`meta[name="browser-language"]`
	// 	) as HTMLMetaElement

	// 	if (metaLanguage) {
	// 		metaLanguage.content = JSON.stringify(msg.value)
	// 	}
	// })
}

export { initLanguageSpoofer }