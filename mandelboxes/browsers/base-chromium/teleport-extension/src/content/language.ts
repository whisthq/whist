import {
  ContentScriptMessage,
  ContentScriptMessageType,
} from "@app/constants/ipc"
import { injectResourceIntoDOM } from "@app/utils/dom"

const initLanguageSpoofer = () => {
    injectResourceIntoDOM(document, "js/language.js")

    // NOTE: there is a potential race condition here. Since chrome.storage.session.get
    //     is asynchronous, this meta tag may not be created before the navigator.language/languages
    //     is called for the first time.
    chrome.storage.session.get(["language", "languages"], (addedItems: any) => {
        const language = addedItems.language
        const languages = addedItems.languages

        const metaLanguage = document.createElement("meta")
        metaLanguage.name = `browser-language`
        metaLanguage.content = JSON.stringify({
          language: language,
          languages: languages,
        })
        document.documentElement.appendChild(metaLanguage)

        // Listen for LANGUAGE_UPDATE message and update corresponding meta tag
        // NOTE: while this could be implemented with a listener on
        //     chrome.storage.onChanged, the implementation becomes
        //     more complicated and so we use the ContentScriptyMessage instead.
        chrome.runtime.onMessage.addListener(async (msg: ContentScriptMessage) => {
            if (msg.type !== ContentScriptMessageType.LANGUAGE_UPDATE) return

            metaLanguage.content = JSON.stringify(msg.value)
        })
    })
}

export { initLanguageSpoofer }