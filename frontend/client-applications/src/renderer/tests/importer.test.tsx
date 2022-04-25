import React from "react"
import { render, waitFor } from "@testing-library/react"
import { screen } from "@testing-library/dom"

import Importer from "../pages/importer"

test("Handles no available browsers case", async () => {
  render(
    <Importer
      browsers={[]}
      onSubmit={() => {}}
      allowSkip={false}
      mode="importing"
    />
  )

  await waitFor(() => {
    expect(
      screen.getByText("No supported browsers found :(")
    ).toBeInTheDocument()
  })
})
