import remove from "lodash.remove"

import { WhistTab } from "@app/constants/tabs"

let whistState = {
  openTabs: [] as WhistTab[],
}

const addTabToState = (tab: WhistTab) => {
  console.log("Adding tab to state", tab)
  whistState.openTabs.push(tab)
}

const removeTabFromState = (tab: WhistTab) => {
  remove(whistState.openTabs, (t) => t.tab.id === tab.tab.id)
}
export { whistState, addTabToState, removeTabFromState }
