import React, { useState, useEffect } from "react";
import { connect } from "react-redux";
import { Row, Col, Button } from "react-bootstrap";

import { db } from "utils/firebase";

import "styles/application.css";

function Application(props: any) {
  return (
    <div>
      Application
    </div>
  );
}

function mapStateToProps(state) {
  return {
  };
}

export default connect(mapStateToProps)(Application);
