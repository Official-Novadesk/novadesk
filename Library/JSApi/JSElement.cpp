/* Copyright (C) 2026 OfficialNovadesk 
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

    namespace {
        template <typename Options, typename ParseFn, typename AddFn>
        duk_ret_t AddElementCommon(duk_context* ctx, ParseFn parseFn, AddFn addFn) {
            Widget* widget = GetWidgetFromThis(ctx);
            if (!widget || !duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

            duk_dup(ctx, 0);
            Options options;
            std::wstring baseDir = PathUtils::GetParentDir(widget->GetOptions().scriptPath);
            parseFn(ctx, options, baseDir);
            duk_pop(ctx);

            addFn(widget, options);
            duk_push_this(ctx);
            return 1;
        }
    }

    duk_ret_t js_widget_add_image(duk_context* ctx) {
        return AddElementCommon<PropertyParser::ImageOptions>(
            ctx,
            PropertyParser::ParseImageOptions,
            [](Widget* w, const PropertyParser::ImageOptions& options) { w->AddImage(options); }
        );
    }

    duk_ret_t js_widget_add_text(duk_context* ctx) {
        return AddElementCommon<PropertyParser::TextOptions>(
            ctx,
            PropertyParser::ParseTextOptions,
            [](Widget* w, const PropertyParser::TextOptions& options) { w->AddText(options); }
        );
    }

    duk_ret_t js_widget_add_bar(duk_context* ctx) {
        return AddElementCommon<PropertyParser::BarOptions>(
            ctx,
            PropertyParser::ParseBarOptions,
            [](Widget* w, const PropertyParser::BarOptions& options) { w->AddBar(options); }
        );
    }

    duk_ret_t js_widget_add_round_line(duk_context* ctx) {
        return AddElementCommon<PropertyParser::RoundLineOptions>(
            ctx,
            PropertyParser::ParseRoundLineOptions,
            [](Widget* w, const PropertyParser::RoundLineOptions& options) { w->AddRoundLine(options); }
        );
    }

    duk_ret_t js_widget_add_shape(duk_context* ctx) {
        return AddElementCommon<PropertyParser::ShapeOptions>(
            ctx,
            PropertyParser::ParseShapeOptions,
            [](Widget* w, const PropertyParser::ShapeOptions& options) { w->AddShape(options); }
        );
    }

    duk_ret_t js_widget_set_element_properties(duk_context* ctx) {
        Widget* widget = GetWidgetFromThis(ctx);
        if (!widget) return DUK_RET_TYPE_ERROR;
        std::wstring id = Utils::ToWString(duk_get_string(ctx, 0));
        if (!duk_is_object(ctx, 1)) return DUK_RET_TYPE_ERROR;
        duk_dup(ctx, 1);
        widget->SetElementProperties(id, ctx);
        duk_pop(ctx);
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_set_group_properties(duk_context* ctx) {
        Widget* widget = GetWidgetFromThis(ctx);
        if (!widget) return DUK_RET_TYPE_ERROR;
        std::wstring group = Utils::ToWString(duk_get_string(ctx, 0));
        if (!duk_is_object(ctx, 1)) return DUK_RET_TYPE_ERROR;
        duk_dup(ctx, 1);
        widget->SetGroupProperties(group, ctx);
        duk_pop(ctx);
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_remove_elements(duk_context* ctx) {
        Widget* widget = GetWidgetFromThis(ctx);
        if (!widget) return DUK_RET_TYPE_ERROR;
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

    duk_ret_t js_widget_remove_elements_by_group(duk_context* ctx) {
        Widget* widget = GetWidgetFromThis(ctx);
        if (!widget) return DUK_RET_TYPE_ERROR;
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::wstring group = Utils::ToWString(duk_get_string(ctx, 0));
        widget->RemoveElementsByGroup(group);
        duk_push_this(ctx);
        return 1;
    }

    duk_ret_t js_widget_get_element_property(duk_context* ctx) {
        Widget* widget = GetWidgetFromThis(ctx);
        if (!widget) return DUK_RET_TYPE_ERROR;
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

    duk_ret_t js_widget_begin_update(duk_context* ctx) {
        Widget* widget = GetWidgetFromThis(ctx);
        if (widget) widget->BeginUpdate();
        return 0;
    }

    duk_ret_t js_widget_end_update(duk_context* ctx) {
        Widget* widget = GetWidgetFromThis(ctx);
        if (widget) widget->EndUpdate();
        return 0;
    }

    void BindWidgetUIMethods(duk_context* ctx) {
        const JsBinding bindings[] = {
            { "addImage", js_widget_add_image, 1 },
            { "addText", js_widget_add_text, 1 },
            { "addBar", js_widget_add_bar, 1 },
            { "addRoundLine", js_widget_add_round_line, 1 },
            { "addShape", js_widget_add_shape, 1 },
            { "removeElements", js_widget_remove_elements, 1 },
            { "removeElementsByGroup", js_widget_remove_elements_by_group, 1 },
            { "setElementProperties", js_widget_set_element_properties, 2 },
            { "setElementPropertiesByGroup", js_widget_set_group_properties, 2 },
            { "getElementProperty", js_widget_get_element_property, 2 },
            { "beginUpdate", js_widget_begin_update, 0 },
            { "endUpdate", js_widget_end_update, 0 }
        };
        BindMethods(ctx, bindings, sizeof(bindings) / sizeof(bindings[0]));
    }
}
