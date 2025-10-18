import torch
from depth_anything_v2.dpt import DepthAnythingV2

DEVICE = 'cpu'

model_configs = {
    'vits': {'encoder': 'vits', 'features': 64, 'out_channels': [48, 96, 192, 384]},
    'vitb': {'encoder': 'vitb', 'features': 128, 'out_channels': [96, 192, 384, 768]},
    'vitl': {'encoder': 'vitl', 'features': 256, 'out_channels': [256, 512, 1024, 1024]},
    'vitg': {'encoder': 'vitg', 'features': 384, 'out_channels': [1536, 1536, 1536, 1536]}
}

encoder = 'vits'

model = DepthAnythingV2(**model_configs[encoder])
model.load_state_dict(torch.load(f'./checkpoints/depth_anything_v2_{encoder}.pth', map_location='cpu'))
model = model.to(DEVICE).eval()

torch.onnx.export(
    model,
    torch.randn(1, 3, 518, 518).to(DEVICE),
    './model/depth_anything_v2_vits.onnx',
    input_names=["image"],
    output_names=["depth"],
    opset_version=17,
)
