import React from "react"
// import {} from "jest"

import { customRender } from "testing/utils/customRender"

import Privacy from "pages/legal/privacy"
import { removeNondeterminism } from "testing/utils/utils"

// since privacy is a static body of text it's ok to just snapshot it and nothing else
describe("<Privacy />", () => {
    const render = (node: any) => removeNondeterminism(customRender(node))
    test("renders snapshot correctly", async () => {
        const tree = render(<Privacy />)
        expect(tree).toMatchSnapshot()
    })
})
