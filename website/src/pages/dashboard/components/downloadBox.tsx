import React, { useState, CSSProperties } from "react"
import { connect } from "react-redux"
import { osName } from "react-device-detect"
import { FaApple, FaWindows } from "react-icons/fa"
import { config } from "shared/constants/config"
import "styles/auth.css"

const DOWNLOAD_TIMEOUT = 3000

const winBin = config.client_download_urls["Windows"]
const macBin = config.client_download_urls["MacOS"]
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

// helpful for having to avoid a ton of ternaries
const withOS = (obWin: any, obMac: any) =>
    osName === "Windows" ? obWin : obMac
const againstOS = (obWin: any, obMac: any) =>
    osName === "Windows" ? obMac : obWin

const DownloadBox = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const { user } = props
    // need length from some js is dumb stuff
    const [canBig, setCanBig] = useState(
        user && user.user_id && user.user_id.length > 0
    )
    const [canSmall, setCanSmall] = useState(
        user && user.user_id && user.user_id.length > 0
    )

    const largeIcon = withOS(windowsIcon, macIcon)
    const smallIcon = againstOS(windowsIcon, macIcon)

    return (
        <div
            style={{
                marginTop: 40,
            }}
        >
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
                            setTimeout(() => setCanBig(true), DOWNLOAD_TIMEOUT)
                        }
                    }}
                    disabled={!canBig}
                    className="google-button" // reuse this for now
                >
                    {largeIcon}
                    <div>
                        {canBig
                            ? "Download Fractal for " + osName
                            : "Downloading..."}
                    </div>
                </button>
            </a>
            <a
                style={{
                    textDecoration: "None",
                }}
                href={againstOS(winBin, macBin)}
                download={againstOS(winBinName, macBinName)}
            >
                <button
                    style={{
                        marginTop: 20,
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
                    className="google-button" // reuse this for now
                >
                    {smallIcon}
                    <div>
                        {canSmall
                            ? "Or Download for " + againstOS("Windows", "MacOS")
                            : "Downloading..."}
                    </div>
                </button>
            </a>
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(DownloadBox)
