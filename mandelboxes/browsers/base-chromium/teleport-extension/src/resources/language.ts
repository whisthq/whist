// Need to override anything to do with language (events fired on langauge change, etc.)
// * navigator.language
// * navigator.languages
// * navigator.languagechange

// Use Object.defineProperty(navigator, 'language', {value: newlang})

console.log(navigator)
Object.defineProperty(navigator, 'language', {value: "boi"})
console.log("updated ", navigator)