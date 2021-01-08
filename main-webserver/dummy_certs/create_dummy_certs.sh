
echo "*** When the command line prompts, just keep hitting 'enter' ***"
echo "Making keys..."
echo ""
echo ""

openssl req  -nodes -new -x509  -keyout key.pem -out cert.pem
