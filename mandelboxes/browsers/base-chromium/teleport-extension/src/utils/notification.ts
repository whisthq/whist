const createNotification = (document: any, text: string) => {
  let div = document.createElement("div")

  div.style.width = "400px"
  div.style.background = "rgba(0, 0, 0, 0.8)"
  div.style.color = "white"
  div.style.position = "fixed"
  div.style.top = "10px"
  div.style.right = "10px"
  div.style.borderRadius = "8px"
  div.style.padding = "14px 20px"
  div.style.zIndex = "99999999"
  div.style.fontSize = "12px"
  div.innerHTML = `<div style="color:rgb(209 213 219);">${text}</div>`

  document.body.appendChild(div)
}

export { createNotification }
