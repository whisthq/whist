param (
    [string]$Branch = ""
)

if ($branch.Length -eq 0) {
  exit
}

cd build64

$bytes = 64

[System.Security.Cryptography.RNGCryptoServiceProvider] $rng = New-Object System.Security.Cryptography.RNGCryptoServiceProvider
$rndbytes = New-Object byte[] $bytes
$rng.GetBytes($rndbytes)
$EncodedText = ($rndbytes|ForEach-Object ToString X2) -join ''
$EncodedText = $EncodedText.Substring(0, 8)
$GitHash = (git rev-parse --short HEAD)
$date = Get-Date -format "MM.dd.yyyy"
Set-Content version "${date}-${GitHash}-${EncodedText}"

cmd.exe /c aws s3 cp update.bat s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/${branch}/Windows/update.bat
cmd.exe /c aws s3 cp FractalServer.exe s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/${branch}/Windows/FractalServer.exe
cmd.exe /c aws s3 cp version s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/${branch}/Windows/version

# notify Slack
cmd.exe /c 'curl -X POST --data-urlencode "payload={\"channel\": \"#general\", \"username\": \"fractal-bot\", \"text\": \"Windows Server Auto-update Pushed to Production VMs\", \"icon_emoji\": \":fractal:\"}" https://hooks.slack.com/services/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'
echo ""
type version
