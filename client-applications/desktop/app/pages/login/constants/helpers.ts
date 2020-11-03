export const checkActive = (app: any): boolean => {
    return app.active
}

export const urlToApp = (url: string, featuredAppData: any): any => {
    console.log("WE WERE PASSED THE URL " + url)
    for (var i = 0; i < featuredAppData.length; i++) {
        if (
            url
                .toLowerCase()
                .includes(featuredAppData[i].app_id.toLowerCase()) &&
            featuredAppData[i].app_id !== "Google Chrome"
        ) {
            console.log("COMPARING " + url + " to " + featuredAppData[i].app_id)
            return { app_id: featuredAppData[i].app_id, url: null }
        }
    }
    return { app_id: "Google Chrome", url: url }
}
