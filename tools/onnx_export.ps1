Set-Location python

# Setup venv
uv sync

# Clone repository
if (-not (Test-Path "external/Depth-Anything-V2")) {
    git clone https://github.com/DepthAnything/Depth-Anything-V2.git external/Depth-Anything-V2
}

# Clone checkpoints
New-Item -ItemType Directory -Force -Path "external/checkpoints" | Out-Null

if (-not (Test-Path "external/checkpoints/depth_anything_v2_vits.pth")) {
    Invoke-WebRequest `
        https://huggingface.co/depth-anything/Depth-Anything-V2-Small/resolve/main/depth_anything_v2_vits.pth `
        -OutFile external/checkpoints/depth_anything_v2_vits.pth
}

# Export
New-Item -ItemType Directory -Force -Path "model" | Out-Null

$env:PYTHONUTF8 = "1"
$env:PYTHONPATH = "external/Depth-Anything-V2"
uv run python export.py
