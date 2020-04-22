cd build64

$bytes = 64

[System.Security.Cryptography.RNGCryptoServiceProvider] $rng = New-Object System.Security.Cryptography.RNGCryptoServiceProvider
$rndbytes = New-Object byte[] $bytes
$rng.GetBytes($rndbytes)
$EncodedText = ($rndbytes|ForEach-Object ToString X2) -join ''
Set-Content version "$EncodedText"

cmd.exe /c aws s3 cp FractalServer.exe s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/FractalServer.exe
cmd.exe /c aws s3 cp version s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/version

type version
