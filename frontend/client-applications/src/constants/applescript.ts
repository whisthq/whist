const getAllWindows = (browser: string) => `
    tell application "${browser}"
        
        --Variables
        set winlist to every window
        set totaltabcount to 0
        
        -- Loop through each window to count the number of open tabs
        repeat with win in winlist
            try
                set tabcount to number of tabs in win
                set totaltabcount to totaltabcount + tabcount
                -- log "tab count: " & tabcount & " totaltabcount: " & totaltabcount
            on error errmsg
                -- Often getting error message like this: 
                -- "Safari got an error: AppleEvent handler failed."
                -- log "error message: " & errmsg
            end try
        end repeat
        
        log "There are " & totaltabcount & " Safari tabs open."
        
    end tell
`
