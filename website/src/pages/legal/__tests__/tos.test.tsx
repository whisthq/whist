import React from "react"
// import {} from "jest"

import { customRender } from "testing/utils/customRender"
import { removeNondeterminism } from "testing/utils/utils"

import TermsOfService from "pages/legal/tos"

// since tos is a static body of text it's ok to just snapshot it and nothing else
describe("<TermsOfService />", () => {
    const render = (node: any) => removeNondeterminism(customRender(node))
    test("renders snapshot correctly", async () => {
        const tree = render(<TermsOfService />)
        expect(tree).toMatchSnapshot()
    })
})
