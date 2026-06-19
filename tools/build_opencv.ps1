# Usage:
#   pwsh -ExecutionPolicy Bypass -File .\build_opencv.ps1

$ErrorActionPreference = "Stop"

Write-Host "==== Path settings ===="
$OPENCV_SRC = "third_party\opencv"
$OPENCV_BUILD = "opencv_build\opencv"

Write-Host "=== Configure OpenCV ==="
# Configuration options reference:
# https://docs.opencv.org/5.0/tutorials/introduction/config_reference/config_reference.html
$cmakeArgs = @(
    "-S", $OPENCV_SRC,
    "-B", $OPENCV_BUILD,
    "-G", "Visual Studio 18 2026",
    "-A", "x64",

    # Library Settings
    "-DBUILD_SHARED_LIBS=OFF",
    "-DBUILD_LIST=core,imgproc,imgcodecs,dnn",

    # Skip unnecessary builds
    "-DBUILD_TESTS=OFF",
    "-DBUILD_PERF_TESTS=OFF",
    "-DBUILD_EXAMPLES=OFF",
    "-DBUILD_opencv_apps=OFF",
    "-DBUILD_DOCS=OFF",
    "-DBUILD_opencv_world=OFF",

    # Optimization and security enhancements
    "-DENABLE_BUILD_HARDENING=ON",
    "-DENABLE_LTO=ON",
    "-DWITH_IPP=OFF",

    # Disable unnecessary backends
    "-DWITH_FFMPEG=OFF",
    "-DWITH_GSTREAMER=OFF",
    "-DWITH_GTK=OFF",
    "-DWITH_QT=OFF",
    "-DWITH_WIN32UI=OFF",
    "-DWITH_ANDROID_MEDIANDK=DFF",

    # DNN
    "-DWITH_ONNXRUNTIME=ON",
    "-DDOWNLOAD_ONNXRUNTIME_GPU=ON",

    # Codec
    "-DBUILD_ZLIB=ON",
    "-DBUILD_JPEG=ON",
    "-DBUILD_PNG=ON",
    "-DBUILD_TIFF=ON",
    "-DBUILD_WEBP=ON",
    "-DBUILD_OPENJPEG=ON",

    "-DINSTALL_CREATE_DISTRIB=ON"
)
cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "=== Build OpenCV ==="
cmake --build $OPENCV_BUILD --parallel --config Debug
cmake --build $OPENCV_BUILD --parallel --config Release
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "=== Install OpenCV ==="
cmake --install $OPENCV_BUILD --config Debug
cmake --install $OPENCV_BUILD --config Release
if ($LASTEXITCODE -ne 0) { exit 1 }

Write-Host "=== Done ==="
