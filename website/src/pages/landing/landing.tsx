import React, { useState, useEffect } from "react";
import { connect } from "react-redux";

import { db } from "utils/firebase";

import "styles/landing.css";
import WaitlistForm from "pages/landing/components/waitlistForm"


function Landing(props: any) {
  const [state, setState] = useState(() => {
    return ({
      "waitlist": []
    })
  })

  useEffect(() => {
    getWaitlist().then(function (waitlist) {
      console.log("Use effect")
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
    </div>
  );
}

function mapStateToProps(state) {
  console.log(state)
  return {
    user: state.AuthReducer.user
  }
}

export default connect(mapStateToProps)(Landing);
