import React from "react";

const Loading = () => {
  return (
    <div
      style={{
        width: "100%",
        minHeight: "100vh",
        display: "flex",
        justifyContent: "center",
        alignItems: "center",
      }}
    >
      <div>
        {" "}
        <h2>Loading...</h2>
        <p>You will be taken back shortly</p>
      </div>
    </div>
  );
};

export default Loading;
