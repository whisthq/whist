curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/shared-libs.tar.gz" | tar xzf -
if not exist server\build32 mkdir server\build32
if not exist server\build64 mkdir server\build64
if not exist desktop\build32 mkdir desktop\build32
if not exist desktop\build64 mkdir desktop\build64

copy share\32\Windows\* desktop\build32\Windows\
copy share\32\Windows\* server\build32\Windows\
copy share\64\Windows\* desktop\build64\Windows\
copy share\64\Windows\* server\build64\Windows\
