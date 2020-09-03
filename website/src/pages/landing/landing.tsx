import React, { Component } from "react";

import { db } from "utils/firebase";

import "styles/landing.css";
import WaitlistForm from "pages/landing/components/waitlistForm"

type LandingProps = {};
type LandingState = { waitlist: Array<any> };

class Landing extends Component<LandingProps, LandingState> {
  componentDidMount() {
    const component = this;
    this.getWaitlist().then(function (waitlist) {
      component.setState({ waitlist: waitlist })
    })
  }

  getWaitlist = async () => {
    const waitlist = await db.collection("waitlist").get()
    return (waitlist.docs.map(doc => doc.data()))
  }

  render() {
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
}

export default Landing;
