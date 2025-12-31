/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSUnits.h"
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cmath>

namespace JSApi {

    // Helper to format bytes
    // formatBytes(bytes, decimals = 2)
    duk_ret_t js_units_format_bytes(duk_context* ctx) {
        double bytes = duk_get_number_default(ctx, 0, 0.0);
        int decimals = duk_get_int_default(ctx, 1, 2);
        if (decimals < 0) decimals = 0;

        if (bytes == 0) {
            duk_push_string(ctx, "0 Bytes");
            return 1;
        }

        const int k = 1024;
        const std::vector<std::string> sizes = { "Bytes", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

        int i = (int)std::floor(std::log(bytes) / std::log(k));
        if (i < 0) i = 0;
        if (i >= sizes.size()) i = (int)sizes.size() - 1;

        double value = bytes / std::pow(k, i);

        std::stringstream ss;
        ss << std::fixed << std::setprecision(decimals) << value << " " << sizes[i];
        
        duk_push_string(ctx, ss.str().c_str());
        return 1;
    }

    // Helper to format seconds
    // formatSeconds(seconds) -> "HH:MM:SS"
    duk_ret_t js_units_format_seconds(duk_context* ctx) {
        double totalSeconds = duk_get_number_default(ctx, 0, 0.0);
        
        int hours = (int)(totalSeconds / 3600);
        int minutes = (int)((totalSeconds - (hours * 3600)) / 60);
        int seconds = (int)(totalSeconds - (hours * 3600) - (minutes * 60));

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << hours << ":"
           << std::setfill('0') << std::setw(2) << minutes << ":"
           << std::setfill('0') << std::setw(2) << seconds;

        duk_push_string(ctx, ss.str().c_str());
        return 1;
    }

    // Helper to convert temperature
    // convertTemperature(value, from, to)
    duk_ret_t js_units_convert_temperature(duk_context* ctx) {
        double value = duk_get_number_default(ctx, 0, 0.0);
        std::string from = duk_get_string_default(ctx, 1, "C");
        std::string to = duk_get_string_default(ctx, 2, "C");

        // Convert input to Celsius first
        double celsius = value;
        if (from == "F" || from == "f") {
            celsius = (value - 32.0) * 5.0 / 9.0;
        } else if (from == "K" || from == "k") {
            celsius = value - 273.15;
        }

        // Convert Celsius to output
        double result = celsius;
        if (to == "F" || to == "f") {
            result = (celsius * 9.0 / 5.0) + 32.0;
        } else if (to == "K" || to == "k") {
            result = celsius + 273.15;
        }

        duk_push_number(ctx, result);
        return 1;
    }

    // Helper to format percentage
    // formatPercentage(value, decimals = 0)
    duk_ret_t js_units_format_percentage(duk_context* ctx) {
        double value = duk_get_number_default(ctx, 0, 0.0);
        int decimals = duk_get_int_default(ctx, 1, 0);
        if (decimals < 0) decimals = 0;

        double percentage = value * 100.0;
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(decimals) << percentage << "%";

        duk_push_string(ctx, ss.str().c_str());
        return 1;
    }

    // Helper to calculate progress (0.0 - 1.0)
    // calculateProgress(value, total)
    duk_ret_t js_units_calculate_progress(duk_context* ctx) {
        double value = duk_get_number_default(ctx, 0, 0.0);
        double total = duk_get_number_default(ctx, 1, 100.0);
        
        if (total == 0) {
            duk_push_number(ctx, 0.0);
            return 1;
        }

        double progress = value / total;
        if (progress < 0.0) progress = 0.0;
        if (progress > 1.0) progress = 1.0;

        duk_push_number(ctx, progress);
        return 1;
    }

    // Helper to parse percentage string/number to normalized float
    // parsePercentage(value) -> 0.0 - 1.0
    // "50%" -> 0.5
    // 50 -> 0.5
    // 0.5 -> 0.5
    duk_ret_t js_units_parse_percentage(duk_context* ctx) {
        if (duk_is_string(ctx, 0)) {
            std::string str = duk_get_string(ctx, 0);
            if (!str.empty() && str.back() == '%') {
                str.pop_back();
                try {
                    double val = std::stod(str);
                    duk_push_number(ctx, val / 100.0);
                    return 1;
                } catch (...) {}
            }
        } 
        
        double val = duk_get_number_default(ctx, 0, 0.0);
        if (val > 1.0) {
            // Assume 0-100 scale
            val = val / 100.0;
        }
        
        if (val < 0.0) val = 0.0;
        if (val > 1.0) val = 1.0;

        duk_push_number(ctx, val);
        return 1;
    }

    void BindUnitsMethods(duk_context* ctx) {
        duk_get_global_string(ctx, "novadesk");
        duk_push_object(ctx); // Create 'units' object

        duk_push_c_function(ctx, js_units_format_bytes, 2);
        duk_put_prop_string(ctx, -2, "formatBytes");

        duk_push_c_function(ctx, js_units_format_seconds, 1);
        duk_put_prop_string(ctx, -2, "formatSeconds");

        duk_push_c_function(ctx, js_units_convert_temperature, 3);
        duk_put_prop_string(ctx, -2, "convertTemperature");

        duk_push_c_function(ctx, js_units_format_percentage, 2);
        duk_put_prop_string(ctx, -2, "formatPercentage");

        duk_push_c_function(ctx, js_units_calculate_progress, 2);
        duk_put_prop_string(ctx, -2, "calculateProgress");

        duk_push_c_function(ctx, js_units_parse_percentage, 1);
        duk_put_prop_string(ctx, -2, "parsePercentage");

        // Attach 'units' to 'novadesk'
        duk_put_prop_string(ctx, -2, "units");
        duk_pop(ctx); // Pop 'novadesk'
    }
}
