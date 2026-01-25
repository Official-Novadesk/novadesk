/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSElement.h"
#include "../Widget.h"
#include "../PropertyParser.h"
#include "../Utils.h"
#include "../Logging.h"
#include "../PathUtils.h"
#include "JSContextMenu.h"

namespace JSApi {

    duk_ret_t js_widget_add_image(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!Widget::IsValid(widget) || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;
        duk_dup(ctx, 0);
        PropertyParser::ImageOptions options;
        std::wstring baseDir = PathUtils::GetParentDir(widget->GetOptions().scriptPath);
        PropertyParser::ParseImageOptions(ctx, options, baseDir);
        duk_pop(ctx);

        widget->AddImage(options);
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_add_text(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!Widget::IsValid(widget) || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;
        duk_dup(ctx, 0);
        PropertyParser::TextOptions options;
        std::wstring baseDir = PathUtils::GetParentDir(widget->GetOptions().scriptPath);
        PropertyParser::ParseTextOptions(ctx, options, baseDir);
        duk_pop(ctx);

        widget->AddText(options);
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_add_bar(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!Widget::IsValid(widget) || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;
        duk_dup(ctx, 0);
        PropertyParser::BarOptions options;
        std::wstring baseDir = PathUtils::GetParentDir(widget->GetOptions().scriptPath);
        PropertyParser::ParseBarOptions(ctx, options, baseDir);
        duk_pop(ctx);

        widget->AddBar(options);
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_set_element_properties(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!Widget::IsValid(widget)) return DUK_RET_TYPE_ERROR;
        std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
        if (!duk_is_object(ctx, 1)) return DUK_RET_TYPE_ERROR;
        duk_dup(ctx, 1);
        widget->SetElementProperties(id, ctx);
        duk_pop(ctx);
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_remove_elements(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!Widget::IsValid(widget)) return DUK_RET_TYPE_ERROR;
        if (duk_is_null_or_undefined(ctx, 0)) {
            widget->RemoveElements();
        } else if (duk_is_array(ctx, 0)) {
            std::vector<std::wstring> ids;
            int len = (int)duk_get_length(ctx, 0);
            for (int i = 0; i < len; i++) {
                duk_get_prop_index(ctx, 0, i);
                ids.push_back(Utils::ToWString(duk_get_string(ctx, -1)));
                duk_pop(ctx);
            }
            widget->RemoveElements(ids);
        } else {
            std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
            widget->RemoveElements(id);
        }
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_get_element_property(duk_context* ctx) {
        duk_push_this(ctx);
        duk_get_prop_string(ctx, -1, "\xFF" "widgetPtr");
        Widget* widget = (Widget*)duk_get_pointer(ctx, -1);
        duk_pop_2(ctx);

        if (!Widget::IsValid(widget)) return DUK_RET_TYPE_ERROR;
        std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
        std::string propertyName = duk_get_string(ctx, 1);

        Element* element = widget->FindElementById(id);
        if (!element) {
            duk_push_null(ctx);
            return 1;
        }
        
        // Push all properties to a temporary object and get the requested one
        PropertyParser::PushElementProperties(ctx, element);
        if (duk_get_prop_string(ctx, -1, propertyName.c_str())) {
            duk_remove(ctx, -2); // Remove the properties object, leaving the property value
            return 1;
        }
        
        duk_pop_2(ctx); // Pop undefined and the properties object
        duk_push_undefined(ctx);
        return 1;
    }

    void BindWidgetUIMethods(duk_context* ctx) {
        duk_push_c_function(ctx, js_widget_add_image, 1);
        duk_put_prop_string(ctx, -2, "addImage");
        duk_push_c_function(ctx, js_widget_add_text, 1);
        duk_put_prop_string(ctx, -2, "addText");
        duk_push_c_function(ctx, js_widget_add_bar, 1);
        duk_put_prop_string(ctx, -2, "addBar");

        duk_push_c_function(ctx, js_widget_remove_elements, 1);
        duk_put_prop_string(ctx, -2, "removeElements");
        duk_push_c_function(ctx, js_widget_set_element_properties, 2);
        duk_put_prop_string(ctx, -2, "setElementProperties");
        duk_push_c_function(ctx, js_widget_get_element_property, 2);
        duk_put_prop_string(ctx, -2, "getElementProperty");
    }
}
