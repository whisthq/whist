import React from "react"
import { connect } from "react-redux"

import NameForm from "pages/dashboard/settings/profile/components/nameForm"
import PasswordForm from "pages/dashboard/settings/profile/components/passwordForm"

import styles from "styles/profile.module.css"

const Profile = (props: any) => {
    const { user } = props

    return (
        <div className="w-full mt-12 md:m-auto md:max-w-screen-sm md:mt-16">
            <div className={styles.sectionTitle}>Email</div>
            <div className={`${styles.sectionInfo} ${styles.dashedBox}`}>
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
