#ifndef SONY_ORIGINAL_TEST_GUI_OPTIONS_H
#define SONY_ORIGINAL_TEST_GUI_OPTIONS_H

#include <cstddef>
#include <string>

namespace test_gui {

struct Profile { const wchar_t *id; const wchar_t *label; };
inline constexpr Profile profiles[] = {
    {L"atrac3-66", L"ATRAC3 66 kbps（有损）"},
    {L"atrac3-105", L"ATRAC3 105 kbps（有损）"},
    {L"atrac3-132", L"ATRAC3 132 kbps（有损）"},
    {L"atrac3plus-48", L"ATRAC3plus 48 kbps（有损）"},
    {L"atrac3plus-64", L"ATRAC3plus 64 kbps（有损）"},
    {L"atrac3plus-96", L"ATRAC3plus 96 kbps（有损）"},
    {L"atrac3plus-128", L"ATRAC3plus 128 kbps（有损）"},
    {L"atrac3plus-160", L"ATRAC3plus 160 kbps（有损）"},
    {L"atrac3plus-192", L"ATRAC3plus 192 kbps（有损）"},
    {L"atrac3plus-256", L"ATRAC3plus 256 kbps（有损）"},
    {L"atrac3plus-320", L"ATRAC3plus 320 kbps（有损）"},
    {L"atrac3plus-352", L"ATRAC3plus 352 kbps（有损）"},
    {L"aal3-132", L"AAL3 无损（ATRAC3 132 kbps 核心）"},
    {L"aalp-48", L"AALP 无损（ATRAC3plus 48 kbps 核心）"},
    {L"aalp-64", L"AALP 无损（ATRAC3plus 64 kbps 核心）"},
    {L"aalp-96", L"AALP 无损（ATRAC3plus 96 kbps 核心）"},
    {L"aalp-128", L"AALP 无损（ATRAC3plus 128 kbps 核心）"},
    {L"aalp-160", L"AALP 无损（ATRAC3plus 160 kbps 核心）"},
    {L"aalp-192", L"AALP 无损（ATRAC3plus 192 kbps 核心）"},
    {L"aalp-256", L"AALP 无损（ATRAC3plus 256 kbps 核心）"},
    {L"aalp-320", L"AALP 无损（ATRAC3plus 320 kbps 核心）"},
    {L"aalp-352", L"AALP 无损（ATRAC3plus 352 kbps 核心）"}
};
inline constexpr std::size_t profile_count = sizeof(profiles) / sizeof(profiles[0]);
inline constexpr const wchar_t *default_profile = L"aalp-352";
static_assert(profile_count == 22, "The test GUI must expose every supported profile");

// CreateProcessW receives an explicit application path. Every argument is
// separately quoted using Windows CRT backslash/quote rules; no shell is used.
inline std::wstring quote_argument(const std::wstring &value) {
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (const wchar_t c : value) {
        if (c == L'\\') { ++slashes; continue; }
        if (c == L'\"') {
            result.append(slashes * 2 + 1, L'\\');
        } else {
            result.append(slashes, L'\\');
        }
        slashes = 0;
        result.push_back(c);
    }
    result.append(slashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
}

inline std::wstring companion_executable(const std::wstring &module_path) {
    const auto slash = module_path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return {};
    return module_path.substr(0, slash + 1) + L"atracodec.exe";
}

inline std::wstring command_line(const std::wstring &executable,
    const std::wstring &profile, const std::wstring &input,
    const std::wstring &output, bool overwrite, bool decode = false) {
    return quote_argument(executable) + (decode ? L" decode" : L" encode --profile " + quote_argument(profile)) +
        L" " + quote_argument(input) + L" " + quote_argument(output) +
        (overwrite ? L" --overwrite" : L"");
}

} // namespace test_gui
#endif
