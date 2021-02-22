/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react"
import { cleanup, screen } from "@testing-library/react"
import DownloadBox from "pages/dashboard/home/components/downloadBox"
import { validUserCanLogin } from "testing/utils/testState"
import { singleAssign } from "testing/utils/testHelpers"
import { customRender } from "testing/utils/customRender"

import { DASHBOARD_DOWNLOAD_BOX_IDS } from "testing/utils/testIDs"
import { removeNondeterminism } from "testing/utils/utils"

describe("<DownloadBox />", () => {
    describe("renders correctly", () => {
        const getDownload = () =>
            screen.getByTestId(DASHBOARD_DOWNLOAD_BOX_IDS.DOWNLOAD_OS)
        const getDownloadAgainst = () =>
            screen.getByTestId(DASHBOARD_DOWNLOAD_BOX_IDS.DOWNLOAD_AGAINST_OS)
        const getWarning = () =>
            screen.getByTestId(DASHBOARD_DOWNLOAD_BOX_IDS.OS_WARN)

        const render = (node: any) =>
            customRender(
                node,
                singleAssign({
                    AuthReducer: {
                        user: validUserCanLogin,
                    },
                })
                // no mocked responses, no GraphQL, etc...
            )

        const supportedOS = ["Windows", "macOS"]

        test("when the OS is supported)", async () => {
            for (const index in supportedOS) {
                const OS = supportedOS[index]
                const tree = removeNondeterminism(
                    render(<DownloadBox osName={OS} />)
                )

                getDownload()
                getDownloadAgainst()
                expect(getWarning).toThrow()

                // both should show up
                const gotWindows = screen.getByText("Windows", { exact: false })
                const gotMac = screen.getByText("macOS", { exact: false }) //
                expect(gotWindows).not.toEqual(gotMac)

                // expect(tree).toMatchSnapshot()

                cleanup() // necessary since there is a for loop
            }
        })

        test("when the OS is not supported", async () => {
            const badOS = "Android" // hehe
            const tree = removeNondeterminism(
                render(<DownloadBox osName={badOS} />)
            )

            expect(getDownload).toThrow()
            expect(getDownloadAgainst).toThrow()
            getWarning()

            // expect(tree).toMatchSnapshot()
        })
    })
})
