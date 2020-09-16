import React from "react"
import { connect } from "react-redux"

import "styles/application.css"

function Application(props: any) {
    return <div>Application Here</div>
}

function mapStateToProps() {
    return {}
}

export default connect(mapStateToProps)(Application)
