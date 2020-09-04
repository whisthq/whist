import React, { useState, useEffect } from "react";
import { connect } from "react-redux";

import { db } from "utils/firebase";

import "styles/landing.css";
import WaitlistForm from "pages/landing/components/waitlistForm";
import CountdownTimer from "pages/landing/components/countdown";

function Landing(props: any) {
  const [state, setState] = useState(() => {
    return ({
      "waitlist": []
    })
  })

  useEffect(() => {
    getWaitlist().then(function (waitlist) {
      setState(prevState => { return { ...prevState, "waitlist": waitlist } })
    })
  }, []);

  async function getWaitlist() {
    const waitlist = await db.collection("waitlist").get()
    return (waitlist.docs.map(doc => doc.data()))
  }

  return (
    <div style={{
      fontWeight: "bold",
      textAlign: "center",
      marginTop: 200,
    }}>
      <WaitlistForm />
      <CountdownTimer />
    </div>
  );
}

function mapStateToProps(state) {
  return {
    user: state.AuthReducer.user
  }
}

export default connect(mapStateToProps)(Landing);
