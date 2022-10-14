TELEPORT_DIRECTORY=/home/whist/.teleport
KEYBOARD_REPEAT_RATE_UPDATE_TRIGGER_FILE=$TELEPORT_DIRECTORY/keyboard-repeat-rate-update

while true; do
	# If file doesn't exist, keep polling until it exists.
	if [ -f $KEYBOARD_REPEAT_RATE_UPDATE_TRIGGER_FILE ]; then
		xset r rate $(head -n 1 filename)
	else
		sleep 20
	fi
	inotifywait -e modify $KEYBOARD_REPEAT_RATE_UPDATE_TRIGGER_FILE
done