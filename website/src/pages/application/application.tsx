import React from "react"
import { connect } from "react-redux"

import "styles/application.css"

function Application(props: any) {
    return <div>Application</div>
}

function mapStateToProps() {
    return {}
}

export default connect(mapStateToProps)(Application)
