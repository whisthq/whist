const css = (element: any, style: object) => {
  for (const property in style) element.style[property] = style[property]
}

const element = (html: string) => {
  let template = document.createElement("template")
  html = html.trim()
  template.innerHTML = html
  return template.content.firstChild as HTMLElement
}

export { css, element }
