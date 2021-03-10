import React, { useState, CSSProperties, Dispatch } from "react"
import { connect } from "react-redux"
import { FaApple, FaWindows } from "react-icons/fa"
import { config } from "shared/constants/config"
import { User, AuthFlow } from "shared/types/reducers"

import { DASHBOARD_DOWNLOAD_BOX_IDS } from "testing/utils/testIDs"

// import WindowsBin from "downloads/Fractal.exe"

const DOWNLOAD_TIMEOUT = 3000

const winBin = config.client_download_urls["Windows"]
const macBin = config.client_download_urls["macOS"]
const winBinName = "Fractal.exe"
const macBinName = "bin/Fractal.dmg"

// type necessary for icons not to yell
// later let's make a css class
const iconStyle: CSSProperties = {
    color: "white",
    position: "relative",
    top: 5,
    marginRight: 15,
}
const macIcon = <FaApple style={iconStyle} />
const windowsIcon = <FaWindows style={iconStyle} />

export const DownloadBox = (props: {
    dispatch: Dispatch<any>
    user: User
    osName: string
}) => {
    /*
        Component for downloading the Electron app
 
        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
            osName (string): User's local operating system, read by react-device-detect
    */

    const { user, osName } = props
    // need length from some js is dumb stuff
    const [canBig, setCanBig] = useState(
        user && user.userID && user.userID.length > 0
    )
    const [canSmall, setCanSmall] = useState(
        user && user.userID && user.userID.length > 0
    )

    // helpful for having to avoid a ton of ternaries
    const withOS = (obWin: any, obMac: any) =>
        osName === "Windows" ? obWin : obMac
    const againstOS = (obWin: any, obMac: any) =>
        osName === "Windows" ? obMac : obWin

    const largeIcon = withOS(windowsIcon, macIcon)
    const smallIcon = againstOS(windowsIcon, macIcon)
    const supportedOS = ["Windows", "macOS"]
    
    if (!supportedOS.includes(osName)) {
        return (
            <div data-testid={DASHBOARD_DOWNLOAD_BOX_IDS.OS_WARN}>
                <div
                    style={{
                        marginTop: 40,
                        padding: 25,
                        background: "#ffebeb",
                        fontSize: 14,
                    }}
                >
                    Warning: Fractal currently does not support {osName} and is
                    only available on Windows and Mac. We expect to support
                    other operating systems in the coming months.
                </div>
            </div>
        )
    }
    return (
        <div
            style={{
                marginTop: 40,
            }}
        >
            <div data-testid={DASHBOARD_DOWNLOAD_BOX_IDS.DOWNLOAD_OS}>
                <a
                    style={{
                        textDecoration: "None",
                    }}
                    href={withOS(winBin, macBin)}
                    download={withOS(winBinName, macBinName)}
                >
                    <button
                        style={{
                            width: "100%",
                            opacity: canBig ? 1 : 0.5,
                        }}
                        onClick={() => {
                            if (canBig) {
                                setCanBig(false)
                                setTimeout(
                                    () => setCanBig(true),
                                    DOWNLOAD_TIMEOUT
                                )
                            }
                        }}
                        disabled={!canBig}
                        className="rounded bg-blue text-white border-none px-4 md:px-20 py-3 mt-8 flex justify-center"
                    >
                        {largeIcon}
                        <div className="font-body transform translate-y-0.5">
                            {canBig
                                ? "Download Fractal for " + osName
                                : "Downloading..."}
                        </div>
                    </button>
                </a>
            </div>
            <div data-testid={DASHBOARD_DOWNLOAD_BOX_IDS.DOWNLOAD_AGAINST_OS}>
                <a
                    style={{
                        textDecoration: "None",
                    }}
                    href={againstOS(winBin, macBin)}
                    download={againstOS(winBinName, macBinName)}
                >
                    <button
                        style={{
                            width: "100%",
                            backgroundColor: "#3930b8",
                            opacity: canSmall ? 0.7 : 0.2,
                        }}
                        onClick={() => {
                            if (canSmall) {
                                setCanSmall(false)
                                setTimeout(
                                    () => setCanSmall(true),
                                    DOWNLOAD_TIMEOUT
                                )
                            }
                        }}
                        disabled={!canSmall}
                        className="rounded bg-blue text-white border-none px-4 md:px-20 py-3 mt-3 flex justify-center"
                    >
                        {smallIcon}
                        <div className="font-body transform translate-y-0.5">
                            {canSmall
                                ? "Download Fractal for " +
                                  againstOS("Windows", "macOS")
                                : "Downloading..."}
                        </div>
                    </button>
                </a>
            </div>
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { user: User; authFlow: AuthFlow }
}) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(DownloadBox)
