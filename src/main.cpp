#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <memory>
#include <algorithm>
#include <vector>
#include <tuple>
#include <cstdint>
#include <filesystem>

#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>

#include "filter2.h"
#include "logger2.h"
#include "config2.h"
#include "cache2.h"

namespace {
LOG_HANDLE* g_log_handle;
CONFIG_HANDLE* g_config_handle;
CACHE_HANDLE* g_cache_handle;

std::filesystem::path g_app_data_path;
std::filesystem::path model_path;

int32_t g_colormap_value = -1;
bool g_initialized = false;

cv::dnn::Net g_net;
HMODULE g_ort = nullptr;

constexpr uint32_t INPUT_W = 518;
constexpr uint32_t INPUT_H = 518;
constexpr const wchar_t* PLUGIN_NAME = L"DepthMapFilter";
constexpr const wchar_t* PLUGIN_INFO = L"DepthMapFilter v1.0.0 azurite";
constexpr const wchar_t* LABEL = L"加工";

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
}

bool func_proc_video(FILTER_PROC_VIDEO* video);
bool func_proc_audio(FILTER_PROC_AUDIO* audio);

//---------------------------------------------------------------------
//	フィルタ設定項目定義
//---------------------------------------------------------------------
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
    {nullptr}
};
auto colormap = FILTER_ITEM_SELECT(L"カラーマップ", -1, colormap_list);

void* items[] = {
	&colormap,
	nullptr
};

//---------------------------------------------------------------------
//	フィルタプラグイン構造体定義
//---------------------------------------------------------------------
FILTER_PLUGIN_TABLE filter_plugin_table = {
	FILTER_PLUGIN_TABLE::FLAG_VIDEO,
	PLUGIN_NAME,
	LABEL,
	PLUGIN_INFO,
	items,
	func_proc_video
};

//---------------------------------------------------------------------
//	プラグインDLL初期化関数 (未定義なら呼ばれません)
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) bool InitializePlugin(DWORD version) { // versionは本体のバージョン番号
	return true;
}

//---------------------------------------------------------------------
//	必要とする本体バージョン番号取得関数 (未定義なら呼ばれません)
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) DWORD RequiredVersion() {
	return 2003300;
}

//---------------------------------------------------------------------
//	ログ出力機能初期化関数 (未定義なら呼ばれません)
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) void InitializeLogger(LOG_HANDLE* handle) {
	g_log_handle = handle;
}

//---------------------------------------------------------------------
//	設定関連初期化関数 (未定義なら呼ばれません)
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) void InitializeConfig(CONFIG_HANDLE* handle) {
	g_config_handle = handle;
    g_app_data_path = std::filesystem::path(g_config_handle->app_data_path);

    std::filesystem::path ort_dll = g_app_data_path / L"Plugin" / std::wstring{PLUGIN_NAME} / L"onnxruntime.dll";
    g_ort = LoadLibraryW(ort_dll.c_str());
}

EXTERN_C __declspec(dllexport) void InitializeCache(CACHE_HANDLE* handle) {
    g_cache_handle = handle;
}

//---------------------------------------------------------------------
//	プラグインDLL解放関数 (未定義なら呼ばれません)
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) void UninitializePlugin() {
    g_net = cv::dnn::Net();  // 空のNetで解放
    FreeLibrary(g_ort);
}

//---------------------------------------------------------------------
//	フィルタ構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C __declspec(dllexport) FILTER_PLUGIN_TABLE* GetFilterPluginTable(void) {
	return &filter_plugin_table;
}

//---------------------------------------------------------------------
//	画像フィルタ処理
//---------------------------------------------------------------------
bool func_proc_video(FILTER_PROC_VIDEO* video) {
	auto w      = video->object->width;
    auto h      = video->object->height;
    auto buffer = std::make_unique<PIXEL_RGBA[]>(w * h);
    video->get_image_data(buffer.get());

    if (!g_initialized) {
        model_path = g_app_data_path / L"Plugin" / std::wstring{PLUGIN_NAME} / L"depth_anything_v2_vits.onnx";
        if (!std::filesystem::exists(std::filesystem::status(model_path))) {
            g_log_handle->error(g_log_handle, L"モデルファイルが存在しません");
            return false;
        }
        try {
            g_net = cv::dnn::readNetFromONNX(model_path.string<char>(), cv::dnn::ENGINE_ORT);
            g_initialized = true;
        } catch (...) {
            g_log_handle->error(g_log_handle, L"Failed readNetFromONNX");
            return false;
        }
    }

	std::wstring cache_name = std::to_wstring(video->object->effect_id) + L'-' + std::to_wstring(video->object->frame) + L'-' + std::to_wstring(colormap.value);
	// キャッシュが存在する場合は復元
	{
		auto image = g_cache_handle->get_image_cache(g_cache_handle, cache_name.c_str());
		if (image && image.width == w && image.height == h) {
            video->set_image_data(image.buffer, image.width, image.height);
            return true;
        }
	}

    cv::Mat mat = cv::Mat(h, w, CV_8UC4, buffer.get());
    cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGRA);
    auto [bgr_mat, alpha] = splitBGRandAlpha(mat);

    cv::Mat blob = cv::dnn::blobFromImage(bgr_mat, 1.0 / 255.0, cv::Size(INPUT_W, INPUT_H), cv::Scalar(0.485, 0.456, 0.406), true, false);
    cv::divide(blob, cv::Scalar(0.229, 0.224, 0.225), blob);

    // 推論
    try {
        g_net.setInput(blob);
        auto output = g_net.forward();

        cv::Mat depth_out(INPUT_H, INPUT_W, CV_32F, output.ptr<float>());

        double min_val, max_val;
        cv::minMaxLoc(depth_out, &min_val, &max_val);
        if (min_val != max_val) {
            depth_out = (depth_out - min_val) / (max_val - min_val);
        }
        depth_out *= 255.0;
        depth_out.convertTo(depth_out, CV_8UC1);

        cv::resize(depth_out, depth_out, cv::Size(w, h), 0, 0, cv::INTER_CUBIC);

        // カラーマップを適用
        if (colormap.value != -1) cv::applyColorMap(depth_out, depth_out, colormap.value);

        auto dst = mergeBGRandAlpha(depth_out, alpha);
        cv::cvtColor(dst, dst, cv::COLOR_BGRA2RGBA);
        video->set_image_data(reinterpret_cast<PIXEL_RGBA*>(dst.data), w, h);

        // キャッシュを作成
        {
            auto image = g_cache_handle->create_image_cache(g_cache_handle, cache_name.c_str(), w, h);
            memcpy(image.buffer, dst.data, w * h * sizeof(PIXEL_RGBA));
        }
    } catch (...) {
        g_log_handle->error(g_log_handle, L"Failed forward");
        return false;
    }

	return true;
}

