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
#include <unordered_map>
#include "JSUtils.h"

using json = nlohmann::json;

namespace JSApi {
    namespace {
        struct Span {
            size_t start = 0;
            size_t end = 0;
        };

        bool IsWhitespace(char c) {
            return c == ' ' || c == '\t' || c == '\r' || c == '\n';
        }

        void SkipWhitespaceAndComments(const std::string& text, size_t& i, size_t end) {
            while (i < end) {
                if (IsWhitespace(text[i])) {
                    ++i;
                    continue;
                }
                if (text[i] == '/' && i + 1 < end) {
                    if (text[i + 1] == '/') {
                        i += 2;
                        while (i < end && text[i] != '\n') {
                            ++i;
                        }
                        continue;
                    }
                    if (text[i + 1] == '*') {
                        i += 2;
                        while (i + 1 < end && !(text[i] == '*' && text[i + 1] == '/')) {
                            ++i;
                        }
                        if (i + 1 < end) {
                            i += 2;
                        }
                        continue;
                    }
                }
                break;
            }
        }

        bool SkipString(const std::string& text, size_t& i, size_t end) {
            if (i >= end || text[i] != '"') {
                return false;
            }
            ++i;
            while (i < end) {
                char c = text[i++];
                if (c == '\\') {
                    if (i < end) {
                        ++i;
                    }
                    continue;
                }
                if (c == '"') {
                    return true;
                }
            }
            return false;
        }

        bool FindRootObjectSpan(const std::string& text, Span& span) {
            size_t i = 0;
            size_t end = text.size();
            while (i < end) {
                SkipWhitespaceAndComments(text, i, end);
                if (i >= end) {
                    break;
                }
                if (text[i] == '"') {
                    if (!SkipString(text, i, end)) {
                        return false;
                    }
                    continue;
                }
                if (text[i] == '{') {
                    size_t depth = 0;
                    size_t start = i;
                    while (i < end) {
                        char c = text[i];
                        if (c == '"') {
                            if (!SkipString(text, i, end)) {
                                return false;
                            }
                            continue;
                        }
                        if (c == '/' && i + 1 < end) {
                            size_t before = i;
                            SkipWhitespaceAndComments(text, i, end);
                            if (i != before) {
                                continue;
                            }
                        }
                        if (c == '{') {
                            ++depth;
                        }
                        else if (c == '}') {
                            --depth;
                            if (depth == 0) {
                                span.start = start;
                                span.end = i + 1;
                                return true;
                            }
                        }
                        ++i;
                    }
                    return false;
                }
                ++i;
            }
            return false;
        }

        bool ParseValueSpan(const std::string& text, size_t& i, size_t end, Span& span) {
            SkipWhitespaceAndComments(text, i, end);
            if (i >= end) {
                return false;
            }
            span.start = i;
            char c = text[i];
            if (c == '"') {
                if (!SkipString(text, i, end)) {
                    return false;
                }
                span.end = i;
                return true;
            }
            if (c == '{' || c == '[') {
                char open = c;
                char close = (c == '{') ? '}' : ']';
                size_t depth = 0;
                while (i < end) {
                    char ch = text[i];
                    if (ch == '"') {
                        if (!SkipString(text, i, end)) {
                            return false;
                        }
                        continue;
                    }
                    if (ch == '/' && i + 1 < end) {
                        size_t before = i;
                        SkipWhitespaceAndComments(text, i, end);
                        if (i != before) {
                            continue;
                        }
                    }
                    if (ch == open) {
                        ++depth;
                    }
                    else if (ch == close) {
                        --depth;
                        if (depth == 0) {
                            ++i;
                            span.end = i;
                            return true;
                        }
                    }
                    ++i;
                }
                return false;
            }
            while (i < end) {
                char ch = text[i];
                if (ch == ',' || ch == '}' || ch == ']') {
                    break;
                }
                ++i;
            }
            span.end = i;
            return true;
        }

        bool CollectObjectMembers(const std::string& text, const Span& objSpan, std::unordered_map<std::string, Span>& members, size_t& lastValueEnd) {
            size_t i = objSpan.start + 1;
            size_t end = objSpan.end - 1;
            lastValueEnd = objSpan.start + 1;
            while (i < end) {
                SkipWhitespaceAndComments(text, i, end);
                if (i >= end) {
                    break;
                }
                if (text[i] == '}') {
                    break;
                }
                if (text[i] != '"') {
                    return false;
                }
                size_t keyStart = i + 1;
                if (!SkipString(text, i, end)) {
                    return false;
                }
                size_t keyEnd = i - 1;
                std::string key = text.substr(keyStart, keyEnd - keyStart);
                SkipWhitespaceAndComments(text, i, end);
                if (i >= end || text[i] != ':') {
                    return false;
                }
                ++i;
                Span valueSpan;
                if (!ParseValueSpan(text, i, end, valueSpan)) {
                    return false;
                }
                members[key] = valueSpan;
                lastValueEnd = valueSpan.end;
                SkipWhitespaceAndComments(text, i, end);
                if (i < end && text[i] == ',') {
                    ++i;
                }
            }
            return true;
        }

        size_t FindInsertPosition(const std::string& text, const Span& objSpan) {
            size_t i = objSpan.end - 1;
            while (i > objSpan.start) {
                size_t before = i;
                SkipWhitespaceAndComments(text, i, objSpan.end);
                if (i != before) {
                    continue;
                }
                if (IsWhitespace(text[i])) {
                    --i;
                    continue;
                }
                break;
            }
            return objSpan.end - 1;
        }

        bool MergeObjectInText(std::string& text, Span& objSpan, const json& patch) {
            if (!patch.is_object()) {
                return false;
            }

            std::string indent = "    ";
            bool hasMembers = true;
            size_t insertPos = FindInsertPosition(text, objSpan);

            for (auto& item : patch.items()) {
                std::unordered_map<std::string, Span> members;
                size_t lastValueEnd = objSpan.start + 1;
                if (!CollectObjectMembers(text, objSpan, members, lastValueEnd)) {
                    return false;
                }

                const std::string& key = item.key();
                auto it = members.find(key);
                hasMembers = !members.empty();

                if (it != members.end()) {
                    Span span = it->second;
                    if (item.value().is_object()) {
                        size_t i = span.start;
                        SkipWhitespaceAndComments(text, i, span.end);
                        if (i < span.end && text[i] == '{') {
                            Span nestedObj;
                            if (FindRootObjectSpan(text.substr(span.start, span.end - span.start), nestedObj)) {
                                Span adjusted;
                                adjusted.start = span.start + nestedObj.start;
                                adjusted.end = span.start + nestedObj.end;
                                size_t beforeSize = text.size();
                                if (MergeObjectInText(text, adjusted, item.value())) {
                                    size_t afterSize = text.size();
                                    if (afterSize != beforeSize) {
                                        objSpan.end += (afterSize - beforeSize);
                                    }
                                    continue;
                                }
                            }
                        }
                    }

                    std::string serializedValue = item.value().dump(4);
                    size_t beforeSize = text.size();
                    text.replace(span.start, span.end - span.start, serializedValue);
                    size_t afterSize = text.size();
                    if (afterSize != beforeSize) {
                        objSpan.end += (afterSize - beforeSize);
                    }
                }
                else {
                    std::string serializedValue = item.value().dump(4);
                    std::string entry = (hasMembers ? "," : "") + std::string("\n") + indent + "\"" + key + "\": " + serializedValue;
                    size_t beforeSize = text.size();
                    text.insert(insertPos, entry);
                    size_t afterSize = text.size();
                    if (afterSize != beforeSize) {
                        objSpan.end += (afterSize - beforeSize);
                    }
                    insertPos += entry.size();
                    hasMembers = true;
                }
                insertPos = FindInsertPosition(text, objSpan);
            }

            if (hasMembers) {
                if (insertPos < text.size() && text[insertPos] != '\n') {
                    text.insert(insertPos, "\n");
                }
            }
            return true;
        }

        bool MergeIntoJsonText(std::string& text, const json& patch) {
            if (!patch.is_object()) {
                return false;
            }

            Span root;
            if (!FindRootObjectSpan(text, root)) {
                return false;
            }

            return MergeObjectInText(text, root, patch);
        }
    }

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
            return duk_get_boolean(ctx, idx) != 0;
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

            if (f.peek() == std::ifstream::traits_type::eof()) {
                // Treat empty files as an empty JSON object for convenience.
                duk_push_object(ctx);
                return 1;
            }

            json j = json::parse(f, nullptr, true, true);
            
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

            std::ifstream existing(wpath);
            if (existing.is_open()) {
                std::string text((std::istreambuf_iterator<char>(existing)), std::istreambuf_iterator<char>());
                existing.close();

                bool hasContent = (text.find_first_not_of(" \t\r\n") != std::string::npos);

                if (MergeIntoJsonText(text, j)) {
                    std::ofstream out(wpath, std::ios::binary);
                    if (out.is_open()) {
                        out << text;
                        out.close();
                        duk_push_boolean(ctx, true);
                        return 1;
                    }
                }
                if (hasContent) {
                    Logging::Log(LogLevel::Warn, L"JSON merge failed; rewriting without preserving comments: %s", wpath.c_str());
                }
            }

            std::ofstream out(wpath, std::ios::binary);
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
