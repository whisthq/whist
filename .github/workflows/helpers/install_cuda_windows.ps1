## -------------------
## Constants
## -------------------

# Dictionary of known cuda versions and their download URLS, which do not follow a consistent pattern :(
# We only hardcode the inconsistent ones, i.e. before 10.1.105
$CUDA_KNOWN_URLS = @{
    "8.0.44" = "http://developer.nvidia.com/compute/cuda/8.0/Prod/network_installers/cuda_8.0.44_win10_network-exe";
    "8.0.61" = "http://developer.nvidia.com/compute/cuda/8.0/Prod2/network_installers/cuda_8.0.61_win10_network-exe";
    "9.0.176" = "http://developer.nvidia.com/compute/cuda/9.0/Prod/network_installers/cuda_9.0.176_win10_network-exe";
    "9.1.85" = "http://developer.nvidia.com/compute/cuda/9.1/Prod/network_installers/cuda_9.1.85_win10_network";
    "9.2.148" = "http://developer.nvidia.com/compute/cuda/9.2/Prod2/network_installers2/cuda_9.2.148_win10_network";
    "10.0.130" = "http://developer.nvidia.com/compute/cuda/10.0/Prod/network_installers/cuda_10.0.130_win10_network";
}

# CUDA version <-> max/min msc versions supported
$CUDA_MAX_MSC_SUPPORT = @{
    "11.3" = "1929";
    "11.2" = "1929";
    "11.1" = "1929";
    "11.0" = "1929";
    "10.2" = "1929";
    "10.1" = "1929";
    "10.0" = "1916";
    "9.2" = "1913";
    "9.1" = "1910";
    "9.0" = "1910";
    "8.0" = "1900";
}

$CUDA_MIN_MSC_SUPPORT = @{
    "11.3" = "1910";
    "11.2" = "1910";
    "11.1" = "1910";
    "11.0" = "1700";
    "10.2" = "1700";
    "10.1" = "1700";
    "10.0" = "1700";
    "9.2" = "1700";
    "9.1" = "1700";
    "9.0" = "1700";
    "8.0" = "1700";
}

# cuda_runtime.h is in nvcc <= 10.2, but cudart >= 11.0
$CUDA_PACKAGES_IN = @(
    "nvcc";
    "visual_studio_integration";
    "curand_dev";
    "nvrtc_dev";
    "cudart";
)

## -------------------
## Select CUDA version
## -------------------

# Get the CUDA version from the environment as env:cuda.
$CUDA_VERSION_FULL = $env:cuda

# Make sure CUDA_VERSION_FULL is set and valid, otherwise error.
# Validate CUDA version, extracting components via regex
$cuda_ver_matched = $CUDA_VERSION_FULL -match "^(?<major>[1-9][0-9]*)\.(?<minor>[0-9]+)\.(?<patch>[0-9]+)$"
if (-not $cuda_ver_matched) {
    Write-Output "Invalid CUDA version specified, <major>.<minor>.<patch> required. '$CUDA_VERSION_FULL'."
    exit 1
}
$CUDA_MAJOR=$Matches.major
$CUDA_MINOR=$Matches.minor
$CUDA_PATCH=$Matches.patch
$CUDA_FIRSTPATCH = $CUDA_PATCH.SubString(0, 1)

## ---------------------------
## Visual Studio support check
## ---------------------------

# Get cl.exe location
$mvs_dir = "C:\Program Files (x86)\Microsoft Visual Studio"
$mvs_year = Get-ChildItem -Path $mvs_dir -Name | Select-Object -First 1
$mvs_edition = Get-ChildItem -Path "$($mvs_dir)\$($mvs_year)" -Name | Select-Object -Last 1
$msvc_dir = "$($mvs_dir)\$($mvs_year)\$($mvs_edition)\VC\Tools\MSVC"
$msvc_version = Get-ChildItem -Path $msvc_dir -Name | Select-Object -Last 1
$cl_location = "$($msvc_dir)\$($msvc_version)\bin\Hostx64\x64\cl.exe"

# Exit if VS compiler version isn't supported by CUDA version
# We get the compiler version by checking installed Visual C++ versions
"_MSC_VER" | Out-File -FilePath mscver.c
$MSC_VER = & $cl_location /nologo /EP mscver.c | Select-Object -Last 1
Write-Output "Found Microsoft C++ version $($MSC_VER)"
$CUDA_MAJOR_MINOR = $CUDA_MAJOR + "." + $CUDA_MINOR
if ($MSC_VER.length -ge 4) {
    $MIN_MSC_VER = $CUDA_MIN_MSC_SUPPORT[$CUDA_MAJOR_MINOR]
    $MAX_MSC_VER = $CUDA_MAX_MSC_SUPPORT[$CUDA_MAJOR_MINOR]
    if (([int]$MSC_VER -gt [int]$MAX_MSC_VER) -or ([int]$MSC_VER -lt [int]$MIN_MSC_VER)) {
	Write-Output "Error: CUDA $($CUDA_MAJOR_MINOR) requires Microsoft C++ $($MIN_MSC_VER)-$($MAX_MSC_VER)"
	exit 1
    }
} else {
    Write-Output "Warning: Unknown Microsoft C++ Version. CUDA version may not be compatible."
}

## ------------------------------------------------
## Select CUDA packages to install from environment
## ------------------------------------------------

$CUDA_PACKAGES = ""

# for CUDA >= 11 cudart is a required package.
# if([version]$CUDA_VERSION_FULL -ge [version]"11.0") {
#     if(-not $CUDA_PACKAGES_IN -contains "cudart") {
#         $CUDA_PACKAGES_IN += 'cudart'
#     }
# }

Foreach ($package in $CUDA_PACKAGES_IN) {
    # Make sure the correct package name is used for nvcc.
    if ($package -eq "nvcc" -and [version]$CUDA_VERSION_FULL -lt [version]"9.1") {
        $package="compiler"
    } elseif ($package -eq "compiler" -and [version]$CUDA_VERSION_FULL -ge [version]"9.1") {
        $package="nvcc"
    }
    $CUDA_PACKAGES += " $($package)_$($CUDA_MAJOR).$($CUDA_MINOR)"

}
echo "$($CUDA_PACKAGES)"

## -----------------
## Prepare download
## -----------------

# Select the download link if known, otherwise have a guess.
$CUDA_REPO_PKG_REMOTE=""
if ($CUDA_KNOWN_URLS.containsKey($CUDA_VERSION_FULL)) {
    # we hardcoded the URL
    $CUDA_REPO_PKG_REMOTE=$CUDA_KNOWN_URLS[$CUDA_VERSION_FULL]
    $CUDA_EXE = "cuda_$($CUDA_VERSION_FULL)_win10_network.exe"
} else {
    # "Guess" the URL
    $CUDA_URL_START = ""
    if ([version]$CUDA_VERSION_FULL -lt [version]"10.1.243") {
	$CUDA_URL_START = "https://developer.nvidia.com/compute/cuda"
    } else {
	$CUDA_URL_START = "https://developer.download.nvidia.com/compute/cuda"
    }
    $CUDA_URL_VERSION = ""
    $CUDA_PROD = ""
    if ([version]$CUDA_VERSION_FULL -lt [version]"11.0.0") {
	$CUDA_URL_VERSION = "$($CUDA_MAJOR).$($CUDA_MINOR)"
	$CUDA_PROD = "Prod/"
    } else {
	$CUDA_URL_VERSION = "$($CUDA_MAJOR).$($CUDA_MINOR).$($CUDA_FIRSTPATCH)"
    }
    if ([version]$CUDA_VERSION_FULL -lt [version]"11.0.0") {
	$CUDA_EXE = "cuda_$($CUDA_VERSION_FULL)_win10_network.exe"
    } else {
	$CUDA_EXE = "cuda_$($CUDA_URL_VERSION)_win10_network.exe"
    }
    $CUDA_REPO_PKG_REMOTE = "$($CUDA_URL_START)/$($CUDA_URL_VERSION)/$($CUDA_PROD)network_installers/$($CUDA_EXE)"
}
$CUDA_REPO_PKG_LOCAL=$CUDA_EXE

## ------------
## Install CUDA
## ------------

# Get CUDA network installer
Write-Output "Downloading CUDA Network Installer for $($CUDA_VERSION_FULL) from: $($CUDA_REPO_PKG_REMOTE)"
Invoke-WebRequest $CUDA_REPO_PKG_REMOTE -OutFile $CUDA_REPO_PKG_LOCAL | Out-Null
if (Test-Path -Path $CUDA_REPO_PKG_LOCAL) {
    Write-Output "Downloading Complete"
} else {
    Write-Output "Error: Failed to download $($CUDA_REPO_PKG_LOCAL) from $($CUDA_REPO_PKG_REMOTE)"
    exit 1
}

# Invoke silent install of CUDA (via network installer)
Write-Output "Installing CUDA $($CUDA_VERSION_FULL). Subpackages $($CUDA_PACKAGES)"
Start-Process -Wait -FilePath .\"$($CUDA_REPO_PKG_LOCAL)" -ArgumentList "-s $($CUDA_PACKAGES)"

# Check the return status of the CUDA installer
if (!$?) {
    Write-Output "Error: CUDA installer reported error. $($LASTEXITCODE)"
    exit 1
}

# Store the CUDA_PATH in the environment for the current session, to be forwarded in the action.
$CUDA_PATH = "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v$($CUDA_MAJOR).$($CUDA_MINOR)"
$CUDA_PATH_VX_Y = "CUDA_PATH_V$($CUDA_MAJOR)_$($CUDA_MINOR)"

# Set environmental variables in this session
$env:CUDA_PATH = "$($CUDA_PATH)"
$env:CUDA_PATH_VX_Y = "$($CUDA_PATH_VX_Y)"
Write-Output "CUDA_PATH $($CUDA_PATH)"
Write-Output "CUDA_PATH_VX_Y $($CUDA_PATH_VX_Y)"

# PATH needs updating elsewhere, anything in here won't persist.
# Append $CUDA_PATH/bin to path.
# Set CUDA_PATH as an environmental variable
