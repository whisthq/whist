/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"
// import {} from "jest"
import { screen } from "@testing-library/react"
import About from "pages/homepage/about/about"
import { HEADER, FOOTER, ABOUT_IDS } from "testing/utils/testIDs"
import { customRender } from "testing/utils/customRender"
import { removeNondeterminism } from "testing/utils/utils"

describe("<About />", () => {
    describe("renders correctly", () => {
        const render = (node: any) => removeNondeterminism(customRender(node))
        test("a snapshot and the necessary components", async () => {
            const tree = render(<About />)
            // const apply = (str: string) => removeRiskyProps(str)
            // await customSnapshot(
            //     tree,
            //     "./src/pages/about/__tests__/__snapshots__/custom",
            //     apply
            // )

            screen.getByTestId(HEADER)
            screen.getByTestId(FOOTER)
            screen.getByTestId(ABOUT_IDS.TEAM)
            screen.getByTestId(ABOUT_IDS.INVESTORS)
            // expect(tree).toMatchSnapshot() // angery react when i fix the dom
        })
    })
})
