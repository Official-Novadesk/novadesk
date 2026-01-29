/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSJson.h"
#include "../json/json.hpp"
#include "../PathUtils.h"
#include "../FileUtils.h"
#include "../Utils.h"
#include "../Logging.h"
#include <fstream>
#include "JSUtils.h"

using json = nlohmann::json;

namespace JSApi {

    // Helper to convert nlohmann::json to Duktape
    void PushJsonToDuk(duk_context* ctx, const json& j) {
        if (j.is_null()) {
            duk_push_null(ctx);
        }
        else if (j.is_boolean()) {
            duk_push_boolean(ctx, j.get<bool>());
        }
        else if (j.is_number()) {
            duk_push_number(ctx, j.get<double>());
        }
        else if (j.is_string()) {
            duk_push_string(ctx, j.get<std::string>().c_str());
        }
        else if (j.is_array()) {
            duk_idx_t arr_idx = duk_push_array(ctx);
            int i = 0;
            for (auto& element : j) {
                PushJsonToDuk(ctx, element);
                duk_put_prop_index(ctx, arr_idx, i++);
            }
        }
        else if (j.is_object()) {
            duk_idx_t obj_idx = duk_push_object(ctx);
            for (auto& el : j.items()) {
                PushJsonToDuk(ctx, el.value());
                duk_put_prop_string(ctx, obj_idx, el.key().c_str());
            }
        }
    }

    // Helper to convert Duktape to nlohmann::json
    json PushDukToJson(duk_context* ctx, duk_idx_t idx) {
        if (duk_is_null_or_undefined(ctx, idx)) {
            return nullptr;
        }
        else if (duk_is_boolean(ctx, idx)) {
            return duk_get_boolean(ctx, idx);
        }
        else if (duk_is_number(ctx, idx)) {
            return duk_get_number(ctx, idx);
        }
        else if (duk_is_string(ctx, idx)) {
            return duk_get_string(ctx, idx);
        }
        else if (duk_is_array(ctx, idx)) {
            json j = json::array();
            duk_size_t len = duk_get_length(ctx, idx);
            for (duk_size_t i = 0; i < len; i++) {
                duk_get_prop_index(ctx, idx, (duk_uarridx_t)i);
                j.push_back(PushDukToJson(ctx, -1));
                duk_pop(ctx);
            }
            return j;
        }
        else if (duk_is_object(ctx, idx)) {
            json j = json::object();
            duk_enum(ctx, idx, DUK_ENUM_OWN_PROPERTIES_ONLY);
            while (duk_next(ctx, -1, 1)) {
                // [ ... enum key value ]
                std::string key = duk_get_string(ctx, -2);
                j[key] = PushDukToJson(ctx, -1);
                duk_pop_2(ctx); // pop key and value
            }
            duk_pop(ctx); // pop enum
            return j;
        }
        return nullptr;
    }

    duk_ret_t js_read_json(duk_context* ctx) {
        std::wstring wpath = ResolveScriptPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));

        try {
            std::ifstream f(wpath);
            if (!f.is_open()) {
                Logging::Log(LogLevel::Error, L"Failed to open JSON file: %s", wpath.c_str());
                return 0;
            }

            json j;
            f >> j;
            
            PushJsonToDuk(ctx, j);
            return 1;
        }
        catch (json::parse_error& e) {
            Logging::Log(LogLevel::Error, L"JSON parse error in %s: %s", wpath.c_str(), Utils::ToWString(e.what()).c_str());
            return 0;
        }
        catch (...) {
            return 0;
        }
    }

    duk_ret_t js_write_json(duk_context* ctx) {
        std::wstring wpath = ResolveScriptPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));

        try {
            // Convert Duktape object to json
            json j = PushDukToJson(ctx, 1);
            
            std::ofstream out(wpath);
            if (out.is_open()) {
                out << j.dump(4); // Pretty print with 4 spaces
                out.close();
                duk_push_boolean(ctx, true);
                return 1;
            }
        }
        catch (...) {
            Logging::Log(LogLevel::Error, L"Failed to write JSON file: %s", wpath.c_str());
        }

        duk_push_boolean(ctx, false);
        return 1;
    }

    void BindJsonMethods(duk_context* ctx) {
        // "system" object is already on top of the stack
        
        duk_push_c_function(ctx, js_read_json, 1);
        duk_put_prop_string(ctx, -2, "readJson");
        
        duk_push_c_function(ctx, js_write_json, 2);
        duk_put_prop_string(ctx, -2, "writeJson");
    }
}
