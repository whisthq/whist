import { bindActionCreators, Dispatch } from "redux";
import { connect } from "react-redux";
import Studios from "../pages/PageStudios/Studios";
import { counterStateType } from "../reducers/types";

function mapStateToProps(state: counterStateType) {}

function mapDispatchToProps(dispatch: Dispatch) {
    return bindActionCreators({}, dispatch);
}

export default connect(mapStateToProps, mapDispatchToProps)(Studios);
