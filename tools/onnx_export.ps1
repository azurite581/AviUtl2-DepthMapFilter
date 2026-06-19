# Usage:
#   pwsh -ExecutionPolicy Bypass -File .\onnx_export.ps1

Set-Location ../python

# Setup venv
uv sync

# Clone repository
mkdir external/Depth-Anything-V2
git clone https://github.com/DepthAnything/Depth-Anything-V2.git external/Depth-Anything-V2

# Clone checkpoints
mkdir -p external/checkpoints
Invoke-WebRequest https://huggingface.co/depth-anything/Depth-Anything-V2-Small/resolve/main/depth_anything_v2_vits.pth -OutFile external/checkpoints/depth_anything_v2_vits.pth

# Export
mkdir -p model
$env:PYTHONPATH = "external/Depth-Anything-V2"
uv run python export.py
