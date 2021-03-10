import React from "react"
import { connect } from "react-redux"
import classNames from "classnames"

import NameForm from "pages/dashboard/settings/profile/components/nameForm"
import PasswordForm from "pages/dashboard/settings/profile/components/passwordForm"

import styles from "styles/profile.module.css"

const Profile = (props: any) => {
    const { user } = props

    // Clear redux values on page load that will be set when editing password
    // useEffect(() => {
    //     dispatch(
    //         PureAuthAction.updateAuthFlow({
    //             resetDone: false,
    //             passwordVerified: null,
    //         })
    //     )
    // }, [dispatch])

    return (
        <div className="font-body w-full mt-12 md:m-auto md:max-w-screen-sm md:mt-16">
            <div className={styles.sectionTitle}>Email</div>
            <div className={classNames("font-body", `${styles.sectionInfo} ${styles.dashedBox}`)}>
                {user.userID}
            </div>
            <NameForm />
            <PasswordForm />
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { user: any }
    DashboardReducer: { stripeInfo: any }
}) => {
    return {
        user: state.AuthReducer.user,
        stripeInfo: state.DashboardReducer.stripeInfo,
    }
}

export default connect(mapStateToProps)(Profile)
