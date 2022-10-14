KEYBOARD_REPEAT_RATE_UPDATE_TRIGGER_FILE="/home/whist/.teleport/keyboard-repeat-rate-update"

while true; do
	if -f $KEYBOARD_REPEAT_RATE_UPDATE_TRIGGER_FILE; then
		xset r rate $(head -n 1 filename)
	fi
	inotifywait -e modify -e create $KEYBOARD_REPEAT_RATE_UPDATE_TRIGGER_FILE
done