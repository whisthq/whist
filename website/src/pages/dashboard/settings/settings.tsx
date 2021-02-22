import React from "react"
import { Link, Switch, Route } from "react-router-dom"

import withTracker from "shared/utils/withTracker"
import Payment from "pages/dashboard/settings/payment/payment"
import Profile from "pages/dashboard/settings/profile/profile"

import { E2E_DASHBOARD_IDS } from "testing/utils/testIDs"

export const Settings = () => {
    /*
        Router for the entire Settings page in dashboard
 
        Arguments: none
    */

    return (
        <div>
            {window.location.pathname === "/dashboard/settings" ||
            window.location.pathname === "/dashboard/settings/payment" ? (
                <div className="flex text-left md:flex-none md:text-center md:justify-center">
                    <Link
                        to="/dashboard/settings"
                        id={E2E_DASHBOARD_IDS.SETTINGSPROFILE}
                    >
                        <div
                            className={
                                window.location.pathname ===
                                "/dashboard/settings"
                                    ? "bg-gray px-8 py-2 rounded inline-block mr-4 md:mx-5 text-white cursor-pointer text-md duration-500"
                                    : "bg-blue-100 px-8 py-2 rounded inline-block mr-4 md:mx-5 text-gray cursor-pointer text-md duration-500"
                            }
                        >
                            Profile
                        </div>
                    </Link>
                    <Link
                        to="/dashboard/settings/payment"
                        id={E2E_DASHBOARD_IDS.SETTINGSPAYMENT}
                    >
                        <div
                            className={
                                window.location.pathname.startsWith(
                                    "/dashboard/settings/payment"
                                )
                                    ? "bg-gray px-8 py-2 rounded inline-block mr-4 md:mx-5 text-white cursor-pointer text-md duration-500"
                                    : "bg-blue-100 px-8 py-2 rounded inline-block mr-4 md:mx-5 text-gray cursor-pointer text-md duration-500"
                            }
                        >
                            Billing
                        </div>
                    </Link>
                </div>
            ) : (
                <></>
            )}
            <Switch>
                <Route
                    exact
                    path="/dashboard/settings"
                    component={withTracker(Profile)}
                />
                <Route
                    path="/dashboard/settings/payment"
                    component={withTracker(Payment)}
                />
            </Switch>
        </div>
    )
}

export default Settings
