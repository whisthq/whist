import { getTabId } from "./session"
import { state } from "../shared/state"

let whistParameters: string | null = null

const initializeWhistTagHandler = () => {
  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (parsed.type === "MANDELBOX_INFO") {
      // Keep the whist element in-focus
      whistParameters = JSON.stringify(parsed.value)
    }
  })
}

const initializeRequestMandelbox = () => {
  const whistTag: any = document.querySelector("whist")
  let mandelboxRequested = false

  setInterval(() => {
    if (
      !whistTag.isWhistConnected() &&
      !mandelboxRequested &&
      state.wasConnected
    ) {
      mandelboxRequested = true
      ;(chrome as any).whist.broadcastWhistMessage(
        JSON.stringify({
          type: "MANDELBOX_NEEDED",
        })
      )
    }

    if (!whistTag.isWhistConnected()) {
      state.wasConnected = false
    }

    if (whistTag.isWhistConnected()) {
      state.wasConnected = true
      mandelboxRequested = false
    }
  }, 100)
}

// const initializeWhistFreezeHandler = () => {
//   const whistTag: any = document.querySelector("whist")

//   ;(chrome as any).whist.onMessage.addListener((message: string) => {
//     const parsed = JSON.parse(message)

//     if (
//       parsed.type === "FREEZE_TAB" && 
//       parsed.value.exclude_id !== getTabId()
//     ) {
//       console.log("freeze ", getTabId())
//       whistTag.freeze()
//     }
//   })

//   // // Freeze all other cloud tabs when another cloud tab has been activated
//   // chrome.tabs.onActivated.addListener((activeInfo: { tabId: number }) => {
//   //   if (activeInfo.tabId !== sessionStorage.getNumber("tabId")) {
//   //     console.log("free ", activeInfo.tabId)
//   //     whistTag.freeze()
//   //   }
//   // })
// }

// const initializeWhistThawHandler = () => {
//   const whistTag: any = document.querySelector("whist")

//   ;(chrome as any).whist.onMessage.addListener((message: string) => {
//     const parsed = JSON.parse(message)

//     if (
//       (parsed.type === "ACTIVATE_TAB" || parsed.type === "UPDATE_TAB") &&
//       parsed.value.id === getTabId()
//     ) {
//       if (whistParameters !== null && !whistTag.isWhistConnected()) {
//         whistTag.focus()
//         whistTag.whistConnect(whistParameters)
//       }

//       ;(chrome as any).whist.broadcastWhistMessage(
//         JSON.stringify({
//           type: "FREEZE_TAB",
//           value: {
//             exclude_id: sessionStorage.getNumber("tabId"),
//           },
//         })
//       )

//       // We delay by 100ms because this is the amount of time we expect the protocol to lag behind
//       // the socket.io message, since the protocol has extra video encode/decode latency.
//       // Lower values were tested and they led to flashes of other tabs being seen.
//       setTimeout(() => {
//         whistTag.thaw()
//       }, 100)
//     }
//   })
// }

const initializeWhistFreezeAllHandler = () => {
  const whistTag: any = document.querySelector("whist")

  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (parsed.type = "CHANGE_FOCUSED_TAB" && parsed.value.id === getTabId()) {
      if (whistParameters !== null && !whistTag.isWhistConnected()) {
        whistTag.focus()
        whistTag.whistConnect(whistParameters)
      }

      var spotlightId = whistTag.freezeAll()

      ;(chrome as any).whist.broadcastWhistMessage(
        JSON.stringify({
          type: "WEB_UIS_FROZEN",
          value: {
            new_active_tab_id: getTabId(),
            spotlight_id: spotlightId
          },
        })
      )
    }
  })
}

const initializeWhistSpotlightHandler = () => {
  const whistTag: any = document.querySelector("whist")

  ;(chrome as any).whist.onMessage.addListener((message: string) => {
    const parsed = JSON.parse(message)

    if (
      (parsed.type === "ACTIVATE_TAB" || parsed.type === "UPDATE_TAB") &&
      parsed.value.id === getTabId()
    ) {
      if (whistParameters !== null && !whistTag.isWhistConnected()) {
        whistTag.focus()
        whistTag.whistConnect(whistParameters)
      }

      var spotlightId = parsed.value.spotlightId;

      if (spotlightId != undefined) {
        // We delay by 100ms because this is the amount of time we expect the protocol to lag behind
        // the socket.io message, since the protocol has extra video encode/decode latency.
        // Lower values were tested and they led to flashes of other tabs being seen.
        setTimeout(() => {
          whistTag.requestSpotlight(spotlightId)
        }, 100)
      }
    }
  })
}

export {
  initializeWhistTagHandler,
  initializeRequestMandelbox,
  // initializeWhistFreezeHandler,
  // initializeWhistThawHandler,
  initializeWhistFreezeAllHandler,
  initializeWhistSpotlightHandler,
}
