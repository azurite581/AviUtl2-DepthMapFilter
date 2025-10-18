#include <dxgi.h>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winml/onnxruntime_cxx_api.h>
#include <winrt/Microsoft.Windows.AI.MachineLearning.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/base.h>
#include <wrl/client.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <opencv2/opencv.hpp>
#include <string>
#include <string_view>
#include <utility>

#include "../aviutl2_sdk/filter2.h"

using namespace Microsoft::WRL;

namespace {
constexpr const wchar_t* MODEL_FILENAME = L"depth_anything_v2_vits.onnx";
constexpr uint32_t INPUT_W              = 518;
constexpr uint32_t INPUT_H              = 518;
int32_t g_gpu_id                        = -1;
bool g_exist_model                      = false;
std::filesystem::path g_model_path;

template <typename... Args>
void showMessage(std::wstring_view fmt, Args... args)
{
    std::wstring msg = std::vformat(fmt, std::make_wformat_args(args...));
    MessageBoxW(nullptr, msg.c_str(), L"DepthMapFilter", MB_OK | MB_ICONHAND);
}

std::wstring multiByteToWideChar(const std::string& str, uint32_t code_page = CP_UTF8)
{
    int32_t wchar_len = MultiByteToWideChar(code_page, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstring_utf16_str(wchar_len, 0);
    MultiByteToWideChar(code_page, 0, str.c_str(), -1, &wstring_utf16_str[0], wchar_len);
    return wstring_utf16_str;
}

std::filesystem::path getModuleDirectoryPath(HMODULE hmod)
{
    wchar_t path[_MAX_PATH];
    GetModuleFileNameW(hmod, path, _MAX_PATH);
    std::filesystem::path current_path{path};
    return current_path.remove_filename();
}

std::tuple<cv::Mat, cv::Mat> splitBGRandAlpha(cv::Mat& img)
{
    cv::Mat bgr(img.size(), CV_8UC3);
    cv::Mat alpha(img.size(), CV_8U);

    cv::Mat dst[]       = {bgr, alpha};
    int channel_index[] = {0, 0, 1, 1, 2, 2, 3, 3};

    cv::mixChannels(&img, 1, dst, 2, channel_index, 4);
    return {bgr, alpha};
}

cv::Mat mergeBGRandAlpha(cv::Mat& bgr, cv::Mat& alpha)
{
    cv::Mat out;
    if (bgr.channels() == 3) {
        cv::Mat bgr_channels[3];
        cv::split(bgr, bgr_channels);
        std::vector<cv::Mat> channels;
        channels.push_back(bgr_channels[0]);
        channels.push_back(bgr_channels[1]);
        channels.push_back(bgr_channels[2]);
        channels.push_back(alpha);
        cv::merge(channels, out);
    } else {
        cv::Mat channels[] = {bgr, bgr, bgr, alpha};
        cv::merge(channels, 4, out);
    }
    return {out};
}

template <typename T>
std::vector<T> mat2vector(cv::Mat& mat)
{
    return static_cast<std::vector<T>>(mat.isContinuous() ? mat.reshape(1, 1) : mat.reshape(1, 1).clone());
}
}  // namespace

FILTER_ITEM_SELECT::ITEM colormap_list[] = {
    {L"GRAY", -1},
    {L"AUTUMN", cv::COLORMAP_AUTUMN},
    {L"BONE", cv::COLORMAP_BONE},
    {L"JET", cv::COLORMAP_JET},
    {L"WINTER", cv::COLORMAP_WINTER},
    {L"RAINBOW", cv::COLORMAP_RAINBOW},
    {L"OCEAN", cv::COLORMAP_OCEAN},
    {L"SUMMER", cv::COLORMAP_SUMMER},
    {L"SPRING", cv::COLORMAP_SPRING},
    {L"COOL", cv::COLORMAP_COOL},
    {L"HSV", cv::COLORMAP_HSV},
    {L"PINK", cv::COLORMAP_PINK},
    {L"HOT", cv::COLORMAP_HOT},
    {L"PARULA", cv::COLORMAP_PARULA},
    {L"MAGMA", cv::COLORMAP_MAGMA},
    {L"INFERNO", cv::COLORMAP_INFERNO},
    {L"PLASMA", cv::COLORMAP_PLASMA},
    {L"VIRIDIS", cv::COLORMAP_VIRIDIS},
    {L"CIVIDIS", cv::COLORMAP_CIVIDIS},
    {L"TWILIGHT", cv::COLORMAP_TWILIGHT},
    {L"TWILIGHT_SHIFTED", cv::COLORMAP_TWILIGHT_SHIFTED},
    {L"TURBO", cv::COLORMAP_TURBO},
    {L"DEEPGREEN", cv::COLORMAP_DEEPGREEN},
    {nullptr}};

bool func_proc_video(FILTER_PROC_VIDEO* video);

//---------------------------------------------------------------------
//	フィルタ設定項目定義
//---------------------------------------------------------------------
auto colormap = FILTER_ITEM_SELECT(L"カラーマップ", -1, colormap_list);
void* items[] = {&colormap, nullptr};

//---------------------------------------------------------------------
//	フィルタプラグイン構造体定義
//---------------------------------------------------------------------
FILTER_PLUGIN_TABLE filter_plugin_table = {
    FILTER_PLUGIN_TABLE::FLAG_VIDEO,
    L"DepthMapFilter",
    L"加工",
    L"DepthMapFilter v1.0.0 by azurite",
    items,
    func_proc_video,
};

//---------------------------------------------------------------------
//	プラグインDLL初期化関数 (未定義なら呼ばれません)
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version)
{
    // モデルファイルのパスを取得
    g_model_path = getModuleDirectoryPath(GetModuleHandleW(L"DepthMapFilter.auf2")) / L"DepthMapFilter_model" / MODEL_FILENAME;
    if (std::filesystem::exists(g_model_path)) {
        g_exist_model = true;
    } else {
        showMessage(L"The model file was not found.");
        g_exist_model = false;
    }

    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        showMessage(L"CreateDXGIFactory1() failed.");
    }

    SIZE_T vram = 0;
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, adapter.ReleaseAndGetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // ソフトウェアアダプターは除外
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            continue;
        }

        // GPUが複数ある場合はメモリサイズが最も大きいものを使うようにする
        if (desc.DedicatedVideoMemory > vram) {
            vram     = desc.DedicatedVideoMemory;
            g_gpu_id = i;
        }
    }
    return true;
}

//---------------------------------------------------------------------
//	フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) FILTER_PLUGIN_TABLE* GetFilterPluginTable(void)
{
    return &filter_plugin_table;
}

//---------------------------------------------------------------------
//	画像フィルタ処理
//---------------------------------------------------------------------
bool func_proc_video(FILTER_PROC_VIDEO* video)
{
    if (!g_exist_model) return false;

    Ort::Env env = Ort::Env(ORT_LOGGING_LEVEL_ERROR);

    // DirectMLを使用する場合のセッションオプションを設定
    Ort::SessionOptions session_options{};
    if (g_gpu_id != -1) {
        std::vector<Ort::ConstEpDevice> ep_devices = env.GetEpDevices();
        Ort::ConstEpDevice ep_device;
        for (auto& device : ep_devices) {
            if (device.Device().Metadata().GetKeyValuePairs()["DxgiAdapterNumber"] == std::to_string(g_gpu_id)) {
                ep_device = device;
                break;
            }
        }
        session_options.DisableMemPattern();
        session_options.SetExecutionMode(ORT_SEQUENTIAL);
        Ort::KeyValuePairs ep_options{};
        session_options.AppendExecutionProvider_V2(env, {ep_device}, ep_options);
    }

    auto w      = video->object->width;
    auto h      = video->object->height;
    auto buffer = std::make_unique<PIXEL_RGBA[]>(w * h);
    video->get_image_data(buffer.get());

    cv::Mat mat = cv::Mat(h, w, CV_8UC4, buffer.get());
    cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGRA);
    auto [bgr_mat, alpha] = splitBGRandAlpha(mat);

    cv::Mat input_mat        = cv::dnn::blobFromImage(bgr_mat, 1.0 / 255.0, cv::Size(INPUT_W, INPUT_H), cv::Scalar(0, 0, 0), true, false);
    std::vector<float> input = mat2vector<float>(input_mat);

    try {
        Ort::Session session = Ort::Session(env, g_model_path.c_str(), session_options);

        // テンソルの形状を取得
        const auto input_type_info             = session.GetInputTypeInfo(0);
        const auto input_tensor_info           = input_type_info.GetTensorTypeAndShapeInfo();
        const std::vector<int64_t> input_shape = input_tensor_info.GetShape();

        // 入出力名を取得
        Ort::AllocatorWithDefaultOptions allocator;
        const auto input_name      = session.GetInputNameAllocated(0, allocator);
        const auto output_name     = session.GetOutputNameAllocated(0, allocator);
        const char* input_names[]  = {input_name.get()};
        const char* output_names[] = {output_name.get()};

        // テンソルを作成
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
        Ort::Value input_tensor     = Ort::Value::CreateTensor<float>(memory_info, input.data(), input.size(), input_shape.data(), input_shape.size());

        // 推論を実行
        std::vector<Ort::Value> output_tensor;
        try {
            output_tensor = session.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);
        } catch (const Ort::Exception& e) {
            showMessage(multiByteToWideChar(e.what()));
            return false;
        }

        // 出力データの取得
        float* depth = output_tensor.front().GetTensorMutableData<float>();
        cv::Mat depth_img(INPUT_H, INPUT_W, CV_32F, depth);

        double min_val{}, max_val{};
        cv::minMaxLoc(depth_img, &min_val, &max_val);
        depth_img.convertTo(depth_img, CV_32F);

        if (min_val != max_val) {
            depth_img = (depth_img - min_val) / (max_val - min_val);
        }
        depth_img *= 255.0;
        depth_img.convertTo(depth_img, CV_8UC1);

        // カラーマップを適用
        if (colormap.value != -1) cv::applyColorMap(depth_img, depth_img, colormap.value);

        cv::resize(depth_img, depth_img, cv::Size(w, h), cv::INTER_CUBIC);
        auto dst = mergeBGRandAlpha(depth_img, alpha);
        cv::cvtColor(dst, dst, cv::COLOR_BGRA2RGBA);

        video->set_image_data(reinterpret_cast<PIXEL_RGBA*>(dst.data), w, h);
        return true;

    } catch (const Ort::Exception& e) {
        showMessage(multiByteToWideChar(e.what()));
        return false;
    }
}
