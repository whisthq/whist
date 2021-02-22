import React from "react"
// import {} from "jest"

import { customRender } from "testing/utils/customRender"
import { removeNondeterminism } from "testing/utils/utils"

import Cookies from "pages/legal/cookies"

// since cookies is a static body of text it's ok to just snapshot it and nothing else
describe("<Cookies />", () => {
    const render = (node: any) => removeNondeterminism(customRender(node))
    test("renders snapshot correctly", async () => {
        const tree = render(<Cookies />)
        expect(tree).toMatchSnapshot()
    })
})
