import { injectResourceIntoDOM } from "@app/utils/dom"

const initLanguageSpoofer = () => {
	injectResourceIntoDOM(document "js/language.ts")
}

export { initLanguageSpoofer }