/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include "JSApi/duktape/duktape.h"
#include <d2d1.h>
#include <vector>
#include "Element.h"

namespace Utils {

    std::wstring ToWString(const std::string& str);
    std::string ToString(const std::wstring& wstr);

    class PropertyReader {
    public:
        PropertyReader(duk_context* ctx);

        bool GetString(const char* key, std::wstring& outStr);
        bool GetInt(const char* key, int& outInt);
        bool GetFloat(const char* key, float& outFloat);
        bool GetBool(const char* key, bool& outBool);
        bool GetColor(const char* key, COLORREF& outColor, BYTE& outAlpha);
        bool GetGradientOrColor(const char* key, COLORREF& outColor, BYTE& outAlpha, GradientInfo& outGradient);
        bool GetFloatArray(const char* key, std::vector<float>& outArray, int minSize);
        void GetEvent(const char* key, int& outId);
        bool ParseShadow(TextShadow& shadow);

    private:
        duk_context* m_Ctx;
    };

    std::vector<std::wstring> SplitByComma(const std::wstring& s);
    bool ParseGradientString(const std::wstring& str, GradientInfo& out);
    D2D1_CAP_STYLE GetCapStyle(const std::wstring& str);
    D2D1_LINE_JOIN GetLineJoin(const std::wstring& str);
}
