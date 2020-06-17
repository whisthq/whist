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
Set-Content version "${GitHash}-${EncodedText}"

cmd.exe /c aws s3 cp update.bat s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/${branch}/Windows/update.bat
cmd.exe /c aws s3 cp FractalServer.exe s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/${branch}/Windows/FractalServer.exe
cmd.exe /c aws s3 cp version s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/${branch}/Windows/version

type version
