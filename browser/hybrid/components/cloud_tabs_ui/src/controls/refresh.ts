const initializePageRefreshHandler = () => {
  const refresh =
    (window.performance.navigation &&
      window.performance.navigation.type === 1) ||
    window.performance
      .getEntriesByType("navigation")
      .map((nav: any) => nav.type)
      .includes("reload")

  if (refresh) {
    ;(chrome as any).whist.broadcastWhistMessage(
      JSON.stringify({
        type: "REFRESH",
        value: {
          id: sessionStorage.getNumber("tabId"),
        },
      })
    )
  }
}

export { initializePageRefreshHandler }
