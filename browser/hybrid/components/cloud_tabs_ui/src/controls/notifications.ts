const initializeNotificationHandler = () => {
    ;(chrome as any).whist.onMessage.addListener((message : string) => {
        const parsed = JSON.parse(message);
        
        if (parsed.type === "SERVER_NOTIFICATION") {
            // request permissions?
            if (Notification.permission === "granted") {
                // show notification
                server_notification = new Notification(parsed.value.title, parsed.value.opt);
                server_notification.onclick = function() {
                    console.log("clicked");
                }
            } else if (Notification.permission !== "denied") {
                Notification.requestPermission().then((permission) => {
                    if (permission === "granted") {
                        console.log("Permission granted");
                    }
                });
            }
        }
    })
}

