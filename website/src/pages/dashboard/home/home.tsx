import React, { Dispatch } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"
import { osName } from "react-device-detect"
import { Link } from "react-router-dom"
import moment from "moment"

import DownloadBox from "pages/dashboard/home/components/downloadBox"

import { DASHBOARD_IDS } from "testing/utils/testIDs"
import { User } from "shared/types/reducers"
import { StripeInfo } from "shared/types/reducers"
//import { CopyToClipboard } from "react-copy-to-clipboard"
// use copy to clipboard functionality when we add back in linux

const Home = (props: {
    dispatch: Dispatch<any>
    user: User
    stripeInfo: StripeInfo
}) => {
    /*
        Dashboard home page
 
        Arguments:
            dispatch (Dispatch<any>): Action dispatcher
            user (User): User from Redux state
    */

    const { user, stripeInfo } = props

    //const [copiedtoClip, setCopiedtoClip] = useState(false)
    //const linuxCommands = "sudo apt-get install libavcodec-dev libavdevice-dev libx11-dev libxtst-dev xclip x11-xserver-utils -y"
    const validUser = user.userID && user.userID !== ""
    const createdDate = stripeInfo.createdTimestamp
        ? new Date(stripeInfo.createdTimestamp * 1000).getTime() / 1000
        : 0
    const FREE_TRIAL_PERIOD = 60 * 60 * 24 * 7
    const trialEnd = createdDate + FREE_TRIAL_PERIOD
    const trialEndDate = moment.unix(trialEnd).format("MMMM Do, YYYY")

    if (!validUser) {
        return <Redirect to="/auth" />
    } else {
        if (
            stripeInfo.plan ||
            Date.now() / 1000 - createdDate < FREE_TRIAL_PERIOD
        ) {
            return (
                <div className="m-auto pt-24 max-w-md text-center">
                    <div className="text-4xl tracking-wide leading-snug">
                        The first cloud-powered browser.
                    </div>
                    <div data-testid={DASHBOARD_IDS.DOWNLOAD}>
                        <DownloadBox
                            osName={osName === "Mac OS" ? "macOS" : osName}
                        />
                    </div>
                </div>
            )
        } else {
            return (
                <div className="w-full m-auto pt-24 max-w-screen-sm text-center">
                    <div className="text-4xl tracking-wide leading-snug">
                        Upgrade your plan for unlimited access.
                    </div>
                    <div className="font-body mt-4">
                        Your free trial expired on {trialEndDate}.
                    </div>
                    <Link to="/dashboard/settings/payment/plan">
                        <button
                            type="button"
                            className="font-body text-white hover:text-black rounded bg-blue border-none px-20 py-3 mt-8 font-medium hover:bg-mint duration-500 tracking-wide"
                        >
                            UPGRADE PLAN
                        </button>
                    </Link>
                </div>
            )
        }
    }
}

const mapStateToProps = (state: {
    AuthReducer: { user: User }
    DashboardReducer: {
        stripeInfo: StripeInfo
    }
}) => {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(Home)
