#include "WidgetUiBindings.h"

#include <algorithm>
#include <string>
#include <vector>

#include "../../domain/Widget.h"
#include "../../render/BarElement.h"
#include "../../render/BitmapElement.h"
#include "../../render/HistogramElement.h"
#include "../../render/AreaGraphElement.h"
#include "../../render/ImageElement.h"
#include "../../render/LineElement.h"
#include "../../render/RoundLineElement.h"
#include "../../render/ShapeElement.h"
#include "../../render/RectangleShape.h"
#include "../../render/EllipseShape.h"
#include "../../render/LineShape.h"
#include "../../render/ArcShape.h"
#include "../../render/CurveShape.h"
#include "../../render/PathShape.h"
#include "../../render/TextElement.h"
#include "../../shared/FileUtils.h"
#include "../../shared/Logging.h"
#include "../../shared/PathUtils.h"
#include "../../shared/Settings.h"
#include "../../shared/Utils.h"
#include "../../shared/ColorUtil.h"
#include "../engine/JSEngine.h"
#include "ModuleSystem.h"
#include "../parser/PropertyParser.h"
#include "WidgetWindowEventBindings.h"

extern std::vector<Widget *> widgets;

namespace novadesk::scripting::quickjs
{
    namespace
    {
        bool g_widgetUiDebug = false;
        JSClassID g_widgetWindowClassId = 0;
        JSClassID g_widgetUiClassId = 0;
        JSRuntime *g_widgetWindowClassRuntime = nullptr;
        JSRuntime *g_widgetUiClassRuntime = nullptr;

        Widget *GetWidget(JSContext *ctx, JSValueConst thisVal)
        {
            return static_cast<Widget *>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
        }

        Widget *GetUiWidget(JSContext *ctx, JSValueConst thisVal)
        {
            return static_cast<Widget *>(JS_GetOpaque2(ctx, thisVal, g_widgetUiClassId));
        }

        Widget *GetAnyWidget(JSContext *ctx, JSValueConst thisVal)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
            {
                widget = GetUiWidget(ctx, thisVal);
            }
            return widget;
        }

        JSValue ThrowTypeError(JSContext *ctx, const char *method, const char *usage)
        {
            return JS_ThrowTypeError(ctx, "%s: %s", method, usage);
        }

        JSValue GetGeneralImagePropertyValue(JSContext *ctx, Element *element, const std::string &prop)
        {
            if (prop == "grayscale")
            {
                bool val = false;
                if (auto *img = dynamic_cast<ImageElement *>(element)) val = img->IsGrayscale();
                else if (auto *btn = dynamic_cast<ButtonElement *>(element)) val = btn->IsGrayscale();
                else if (auto *bmp = dynamic_cast<BitmapElement *>(element)) val = bmp->IsGrayscale();
                return JS_NewBool(ctx, val ? 1 : 0);
            }
            if (prop == "useExifOrientation")
            {
                bool val = false;
                if (auto *img = dynamic_cast<ImageElement *>(element)) val = img->GetUseExifOrientation();
                else if (auto *btn = dynamic_cast<ButtonElement *>(element)) val = btn->GetUseExifOrientation();
                else if (auto *bmp = dynamic_cast<BitmapElement *>(element)) val = bmp->GetUseExifOrientation();
                return JS_NewBool(ctx, val ? 1 : 0);
            }
            if (prop == "imageAlpha")
            {
                BYTE val = 255;
                if (auto *img = dynamic_cast<ImageElement *>(element)) val = img->GetImageAlpha();
                else if (auto *btn = dynamic_cast<ButtonElement *>(element)) val = btn->GetImageAlpha();
                else if (auto *bmp = dynamic_cast<BitmapElement *>(element)) val = bmp->GetImageAlpha();
                return JS_NewInt32(ctx, static_cast<int>(val));
            }
            if (prop == "imageTint")
            {
                bool hasTint = false;
                COLORREF color = 0;
                BYTE alpha = 255;
                if (auto *img = dynamic_cast<ImageElement *>(element)) {
                    hasTint = img->HasImageTint();
                    if (hasTint) { color = img->GetImageTint(); alpha = img->GetImageTintAlpha(); }
                } else if (auto *btn = dynamic_cast<ButtonElement *>(element)) {
                    hasTint = btn->HasImageTint();
                    if (hasTint) { color = btn->GetImageTint(); alpha = btn->GetImageTintAlpha(); }
                } else if (auto *bmp = dynamic_cast<BitmapElement *>(element)) {
                    hasTint = bmp->HasImageTint();
                    if (hasTint) { color = bmp->GetImageTint(); alpha = bmp->GetImageTintAlpha(); }
                }
                if (hasTint) {
                    const std::wstring c = ColorUtil::ToRGBAString(color, alpha);
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }
            }
            if (prop == "imageFlip")
            {
                ImageFlipMode flip = IMAGE_FLIP_NONE;
                if (auto *img = dynamic_cast<ImageElement *>(element)) flip = img->GetImageFlip();
                else if (auto *btn = dynamic_cast<ButtonElement *>(element)) flip = btn->GetImageFlip();
                else if (auto *bmp = dynamic_cast<BitmapElement *>(element)) flip = bmp->GetImageFlip();
                
                const char *flipStr = "none";
                switch (flip) {
                case IMAGE_FLIP_HORIZONTAL: flipStr = "horizontal"; break;
                case IMAGE_FLIP_VERTICAL: flipStr = "vertical"; break;
                case IMAGE_FLIP_BOTH: flipStr = "both"; break;
                default: break;
                }
                return JS_NewString(ctx, flipStr);
            }
            if (prop == "imageCrop")
            {
                bool hasCrop = false;
                float x = 0, y = 0, w = 0, h = 0;
                ImageCropOrigin origin = IMAGE_CROP_ORIGIN_TOP_LEFT;
                if (auto *img = dynamic_cast<ImageElement *>(element)) {
                    hasCrop = img->HasImageCrop();
                    if (hasCrop) { x = img->GetImageCropX(); y = img->GetImageCropY(); w = img->GetImageCropW(); h = img->GetImageCropH(); origin = img->GetImageCropOrigin(); }
                } else if (auto *btn = dynamic_cast<ButtonElement *>(element)) {
                    hasCrop = btn->HasImageCrop();
                    if (hasCrop) { x = btn->GetImageCropX(); y = btn->GetImageCropY(); w = btn->GetImageCropW(); h = btn->GetImageCropH(); origin = btn->GetImageCropOrigin(); }
                }
                if (hasCrop) {
                    JSValue arr = JS_NewArray(ctx);
                    JS_SetPropertyUint32(ctx, arr, 0, JS_NewFloat64(ctx, x));
                    JS_SetPropertyUint32(ctx, arr, 1, JS_NewFloat64(ctx, y));
                    JS_SetPropertyUint32(ctx, arr, 2, JS_NewFloat64(ctx, w));
                    JS_SetPropertyUint32(ctx, arr, 3, JS_NewFloat64(ctx, h));
                    JS_SetPropertyUint32(ctx, arr, 4, JS_NewInt32(ctx, (int)origin));
                    return arr;
                }
            }
            if (prop == "colorMatrix")
            {
                bool hasMatrix = false;
                const float *m = nullptr;
                if (auto *img = dynamic_cast<ImageElement *>(element)) {
                    hasMatrix = img->HasColorMatrix();
                    if (hasMatrix) m = img->GetColorMatrix();
                } else if (auto *btn = dynamic_cast<ButtonElement *>(element)) {
                    hasMatrix = btn->HasColorMatrix();
                    if (hasMatrix) m = btn->GetColorMatrix();
                } else if (auto *bmp = dynamic_cast<BitmapElement *>(element)) {
                    hasMatrix = bmp->HasColorMatrix();
                    if (hasMatrix) m = bmp->GetColorMatrix();
                }
                if (hasMatrix && m) {
                    JSValue arr = JS_NewArray(ctx);
                    for (uint32_t i = 0; i < 20; ++i) JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, m[i]));
                    return arr;
                }
            }
            return JS_UNDEFINED;
        }

        std::wstring GetWidgetScriptBaseDir(Widget *widget)
        {
            if (!widget)
                return JSEngine::GetEntryScriptDir();

            const std::wstring scriptPath = widget->GetOptions().scriptPath;
            if (scriptPath.empty())
                return JSEngine::GetEntryScriptDir();

            const std::wstring absScriptPath = PathUtils::ResolvePath(scriptPath, JSEngine::GetEntryScriptDir());
            return PathUtils::GetParentDir(absScriptPath);
        }

        JSValue JsWidgetAddImage(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addImage", "expected options object");
            PropertyParser::ImageOptions options;
            PropertyParser::ParseImageOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddImage(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddButton(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addButton", "expected options object");
            PropertyParser::ButtonOptions options;
            PropertyParser::ParseButtonOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddButton(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddText(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addText", "expected options object");
            PropertyParser::TextOptions options;
            PropertyParser::ParseTextOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddText(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddBar(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addBar", "expected options object");
            PropertyParser::BarOptions options;
            PropertyParser::ParseBarOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddBar(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddRoundLine(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addRoundLine", "expected options object");
            PropertyParser::RoundLineOptions options;
            PropertyParser::ParseRoundLineOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddRoundLine(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddLine(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addLine", "expected options object");
            PropertyParser::LineOptions options;
            PropertyParser::ParseLineOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddLine(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddHistogram(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addHistogram", "expected options object");
            PropertyParser::HistogramOptions options;
            PropertyParser::ParseHistogramOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddHistogram(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddShape(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addShape", "expected options object");
            PropertyParser::ShapeOptions options;
            PropertyParser::ParseShapeOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddShape(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddBitmap(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addBitmap", "expected options object");
            PropertyParser::BitmapOptions options;
            PropertyParser::ParseBitmapOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddBitmap(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddRotator(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addRotator", "expected options object");
            PropertyParser::RotatorOptions options;
            PropertyParser::ParseRotatorOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddRotator(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetAddAreaGraph(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "addAreaGraph", "expected options object");
            PropertyParser::AreaGraphOptions options;
            PropertyParser::ParseAreaGraphOptions(ctx, argv[0], options, GetWidgetScriptBaseDir(widget));
            widget->AddAreaGraph(options);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetRemoveElements(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;

            if (argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0]))
            {
                return JS_NewBool(ctx, widget->RemoveElements() ? 1 : 0);
            }

            if (JS_IsArray(argv[0]))
            {
                JSValue lenV = JS_GetPropertyStr(ctx, argv[0], "length");
                uint32_t len = 0;
                JS_ToUint32(ctx, &len, lenV);
                JS_FreeValue(ctx, lenV);
                std::vector<std::wstring> ids;
                ids.reserve(static_cast<size_t>(len));
                for (uint32_t i = 0; i < len; ++i)
                {
                    JSValue iv = JS_GetPropertyUint32(ctx, argv[0], i);
                    const char *idUtf8 = JS_ToCString(ctx, iv);
                    if (idUtf8)
                    {
                        ids.emplace_back(Utils::ToWString(idUtf8));
                        JS_FreeCString(ctx, idUtf8);
                    }
                    JS_FreeValue(ctx, iv);
                }
                widget->RemoveElements(ids);
                return JS_NewBool(ctx, 1);
            }

            const char *idUtf8 = JS_ToCString(ctx, argv[0]);
            if (!idUtf8)
                return JS_EXCEPTION;
            std::wstring id = Utils::ToWString(idUtf8);
            JS_FreeCString(ctx, idUtf8);
            return JS_NewBool(ctx, widget->RemoveElements(id) ? 1 : 0);
        }

        JSValue JsWidgetRemoveElementsByGroup(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 1)
                return ThrowTypeError(ctx, "removeElementsByGroup", "expected group id string");
            const char *groupUtf8 = JS_ToCString(ctx, argv[0]);
            if (!groupUtf8)
                return JS_EXCEPTION;
            std::wstring group = Utils::ToWString(groupUtf8);
            JS_FreeCString(ctx, groupUtf8);
            widget->RemoveElementsByGroup(group);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetBeginUpdate(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            widget->BeginUpdate();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetEndUpdate(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            widget->EndUpdate();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetSetElementProperties(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 2 || !JS_IsObject(argv[1]))
                return ThrowTypeError(ctx, "setElementProperties", "expected (id, options)");

            const char *idUtf8 = JS_ToCString(ctx, argv[0]);
            if (!idUtf8)
                return JS_EXCEPTION;
            std::wstring id = Utils::ToWString(idUtf8);
            JS_FreeCString(ctx, idUtf8);

            Element *element = widget->FindElementById(id);
            if (!element)
                return JS_UNDEFINED;
            const std::wstring baseDir = GetWidgetScriptBaseDir(widget);

            if (auto *image = dynamic_cast<ImageElement *>(element))
            {
                PropertyParser::ImageOptions options;
                PropertyParser::PreFillImageOptions(options, image);
                PropertyParser::ParseImageOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyImageOptions(image, options);
            }
            else if (auto *btn = dynamic_cast<ButtonElement *>(element))
            {
                PropertyParser::ButtonOptions options;
                PropertyParser::PreFillButtonOptions(options, btn);
                PropertyParser::ParseButtonOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyButtonOptions(btn, options);
            }
            else if (auto *text = dynamic_cast<TextElement *>(element))
            {
                PropertyParser::TextOptions options;
                PropertyParser::PreFillTextOptions(options, text);
                PropertyParser::ParseTextOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyTextOptions(text, options);
            }
            else if (auto *bar = dynamic_cast<BarElement *>(element))
            {
                PropertyParser::BarOptions options;
                PropertyParser::PreFillBarOptions(options, bar);
                PropertyParser::ParseBarOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyBarOptions(bar, options);
            }
            else if (auto *round = dynamic_cast<RoundLineElement *>(element))
            {
                PropertyParser::RoundLineOptions options;
                PropertyParser::PreFillRoundLineOptions(options, round);
                PropertyParser::ParseRoundLineOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyRoundLineOptions(round, options);
            }
            else if (auto *line = dynamic_cast<LineElement *>(element))
            {
                PropertyParser::LineOptions options;
                PropertyParser::PreFillLineOptions(options, line);
                PropertyParser::ParseLineOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyLineOptions(line, options);
            }
            else if (auto *histogram = dynamic_cast<HistogramElement *>(element))
            {
                PropertyParser::HistogramOptions options;
                PropertyParser::PreFillHistogramOptions(options, histogram);
                PropertyParser::ParseHistogramOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyHistogramOptions(histogram, options);
            }
            else if (auto *shape = dynamic_cast<ShapeElement *>(element))
            {
                PropertyParser::ShapeOptions options;
                PropertyParser::PreFillShapeOptions(options, shape);
                PropertyParser::ParseShapeOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyShapeOptions(shape, options);
            }
            else if (auto *bitmap = dynamic_cast<BitmapElement *>(element))
            {
                PropertyParser::BitmapOptions options;
                PropertyParser::PreFillBitmapOptions(options, bitmap);
                PropertyParser::ParseBitmapOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyBitmapOptions(bitmap, options);
            }
            else if (auto *rotator = dynamic_cast<RotatorElement *>(element))
            {
                PropertyParser::RotatorOptions options;
                PropertyParser::PreFillRotatorOptions(options, rotator);
                PropertyParser::ParseRotatorOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyRotatorOptions(rotator, options);
            }
            else if (auto *graph = dynamic_cast<AreaGraphElement *>(element))
            {
                PropertyParser::AreaGraphOptions options;
                PropertyParser::PreFillAreaGraphOptions(options, graph);
                PropertyParser::ParseAreaGraphOptions(ctx, argv[1], options, baseDir);
                PropertyParser::ApplyAreaGraphOptions(graph, options);
            }

            widget->Redraw();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetSetElementPropertiesByGroup(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 2)
                return ThrowTypeError(ctx, "setElementPropertiesByGroup", "expected (groupId, options)");
            const char *groupUtf8 = JS_ToCString(ctx, argv[0]);
            if (!groupUtf8)
                return JS_EXCEPTION;
            std::wstring group = Utils::ToWString(groupUtf8);
            JS_FreeCString(ctx, groupUtf8);
            widget->SetGroupProperties(group, reinterpret_cast<duk_context *>(ctx));
            widget->Redraw();
            return JS_UNDEFINED;
        }

        JSValue GetElementPropertyValue(JSContext *ctx, Element *element, const std::string &prop)
        {
            const GfxRect contentBounds = element->GetBounds();
            GfxRect outerBounds = element->GetBackgroundBounds();
            if (element->GetBevelType() != 0)
            {
                const int bevelPad = 2;
                outerBounds = GfxRect(
                    outerBounds.X - bevelPad,
                    outerBounds.Y - bevelPad,
                    outerBounds.Width + bevelPad * 2,
                    outerBounds.Height + bevelPad * 2);
            }

            if (prop == "id")
                return JS_NewString(ctx, Utils::ToString(element->GetId()).c_str());
            if (prop == "contentX")
                return JS_NewInt32(ctx, contentBounds.X);
            if (prop == "contentY")
                return JS_NewInt32(ctx, contentBounds.Y);
            if (prop == "contentWidth")
                return JS_NewInt32(ctx, contentBounds.Width);
            if (prop == "contentHeight")
                return JS_NewInt32(ctx, contentBounds.Height);
            if (prop == "x")
                return JS_NewInt32(ctx, outerBounds.X);
            if (prop == "y")
                return JS_NewInt32(ctx, outerBounds.Y);
            if (prop == "width")
                return JS_NewInt32(ctx, outerBounds.Width);
            if (prop == "height")
                return JS_NewInt32(ctx, outerBounds.Height);
            if (prop == "show")
                return JS_NewBool(ctx, element->IsVisible() ? 1 : 0);
            if (prop == "container")
                return JS_NewString(ctx, Utils::ToString(element->GetContainerId()).c_str());
            if (prop == "group")
                return JS_NewString(ctx, Utils::ToString(element->GetGroupId()).c_str());
            if (prop == "mouseEventCursor")
                return JS_NewBool(ctx, element->GetMouseEventCursor() ? 1 : 0);
            if (prop == "mouseEventCursorName")
                return JS_NewString(ctx, Utils::ToString(element->GetMouseEventCursorName()).c_str());
            if (prop == "cursorsDir")
                return JS_NewString(ctx, Utils::ToString(element->GetCursorsDir()).c_str());
            if (prop == "rotate")
                return JS_NewFloat64(ctx, element->GetRotate());
            if (prop == "antiAlias")
                return JS_NewBool(ctx, element->GetAntiAlias() ? 1 : 0);
            if (prop == "backgroundColorRadius")
                return JS_NewInt32(ctx, element->GetCornerRadius());
            if (prop == "backgroundColor" && element->HasSolidColor())
            {
                const std::wstring color = ColorUtil::ToRGBAString(element->GetSolidColor(), element->GetSolidAlpha());
                return JS_NewString(ctx, Utils::ToString(color).c_str());
            }
            if (prop == "bevelType")
            {
                const int bt = element->GetBevelType();
                const char *bevStr = "none";
                switch (bt)
                {
                case 1:
                    bevStr = "raised";
                    break;
                case 2:
                    bevStr = "sunken";
                    break;
                case 3:
                    bevStr = "emboss";
                    break;
                case 4:
                    bevStr = "pillow";
                    break;
                default:
                    break;
                }
                return JS_NewString(ctx, bevStr);
            }
            if (prop == "bevelWidth")
                return JS_NewInt32(ctx, element->GetBevelWidth());
            if (prop == "bevelColor")
            {
                const std::wstring color = ColorUtil::ToRGBAString(element->GetBevelColor(), element->GetBevelAlpha());
                return JS_NewString(ctx, Utils::ToString(color).c_str());
            }
            if (prop == "bevelColor2")
            {
                const std::wstring color = ColorUtil::ToRGBAString(element->GetBevelColor2(), element->GetBevelAlpha2());
                return JS_NewString(ctx, Utils::ToString(color).c_str());
            }
            if (prop == "padding")
            {
                JSValue arr = JS_NewArray(ctx);
                JS_SetPropertyUint32(ctx, arr, 0, JS_NewInt32(ctx, element->GetPaddingLeft()));
                JS_SetPropertyUint32(ctx, arr, 1, JS_NewInt32(ctx, element->GetPaddingTop()));
                JS_SetPropertyUint32(ctx, arr, 2, JS_NewInt32(ctx, element->GetPaddingRight()));
                JS_SetPropertyUint32(ctx, arr, 3, JS_NewInt32(ctx, element->GetPaddingBottom()));
                return arr;
            }
            if (prop == "transformMatrix" && element->HasTransformMatrix())
            {
                JSValue arr = JS_NewArray(ctx);
                const float *m = element->GetTransformMatrix();
                for (uint32_t i = 0; i < 6; ++i)
                {
                    JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, m[i]));
                }
                return arr;
            }

            if (prop == "tooltipText")
                return JS_NewString(ctx, Utils::ToString(element->GetToolTipText()).c_str());
            if (prop == "tooltipTitle")
                return JS_NewString(ctx, Utils::ToString(element->GetToolTipTitle()).c_str());
            if (prop == "tooltipIcon")
                return JS_NewString(ctx, Utils::ToString(element->GetToolTipIcon()).c_str());
            if (prop == "tooltipMaxWidth")
                return JS_NewInt32(ctx, element->GetToolTipMaxWidth());
            if (prop == "tooltipMaxHeight")
                return JS_NewInt32(ctx, element->GetToolTipMaxHeight());
            if (prop == "tooltipBalloon")
                return JS_NewBool(ctx, element->GetToolTipBalloon() ? 1 : 0);
            if (prop == "tooltipDisabled")
                return JS_NewBool(ctx, element->GetToolTipDisabled() ? 1 : 0);

            if (element->GetType() == ELEMENT_TEXT)
            {
                auto *t = static_cast<TextElement *>(element);
                if (prop == "text")
                    return JS_NewString(ctx, Utils::ToString(t->GetText()).c_str());
                if (prop == "fontFace")
                    return JS_NewString(ctx, Utils::ToString(t->GetFontFace()).c_str());
                if (prop == "fontSize")
                    return JS_NewInt32(ctx, t->GetFontSize());
                if (prop == "fontColor")
                {
                    const std::wstring color = ColorUtil::ToRGBAString(t->GetFontColor(), t->GetFontAlpha());
                    return JS_NewString(ctx, Utils::ToString(color).c_str());
                }
                if (prop == "fontWeight")
                    return JS_NewInt32(ctx, t->GetFontWeight());
                if (prop == "italic")
                    return JS_NewBool(ctx, t->IsItalic() ? 1 : 0);
                if (prop == "underLine")
                    return JS_NewBool(ctx, t->GetUnderline() ? 1 : 0);
                if (prop == "strikeThrough")
                    return JS_NewBool(ctx, t->GetStrikethrough() ? 1 : 0);
                if (prop == "letterSpacing")
                    return JS_NewFloat64(ctx, t->GetLetterSpacing());
                if (prop == "fontPath")
                    return JS_NewString(ctx, Utils::ToString(t->GetFontPath()).c_str());
                if (prop == "textAlign")
                {
                    const char *alStr = "lefttop";
                    switch (t->GetTextAlign())
                    {
                    case TEXT_ALIGN_LEFT_TOP:
                        alStr = "lefttop";
                        break;
                    case TEXT_ALIGN_CENTER_TOP:
                        alStr = "centertop";
                        break;
                    case TEXT_ALIGN_RIGHT_TOP:
                        alStr = "righttop";
                        break;
                    case TEXT_ALIGN_LEFT_CENTER:
                        alStr = "leftcenter";
                        break;
                    case TEXT_ALIGN_CENTER_CENTER:
                        alStr = "centercenter";
                        break;
                    case TEXT_ALIGN_RIGHT_CENTER:
                        alStr = "rightcenter";
                        break;
                    case TEXT_ALIGN_LEFT_BOTTOM:
                        alStr = "leftbottom";
                        break;
                    case TEXT_ALIGN_CENTER_BOTTOM:
                        alStr = "centerbottom";
                        break;
                    case TEXT_ALIGN_RIGHT_BOTTOM:
                        alStr = "rightbottom";
                        break;
                    default:
                        break;
                    }
                    return JS_NewString(ctx, alStr);
                }
                if (prop == "textClip")
                {
                    const char *clipStr = "none";
                    switch (t->GettextClip())
                    {
                    case TEXT_CLIP_ON:
                        clipStr = "clip";
                        break;
                    case TEXT_CLIP_ELLIPSIS:
                        clipStr = "ellipsis";
                        break;
                    case TEXT_CLIP_WRAP:
                        clipStr = "wrap";
                        break;
                    default:
                        break;
                    }
                    return JS_NewString(ctx, clipStr);
                }
                if (prop == "case")
                {
                    const char *caseStr = "normal";
                    switch (t->GetTextCase())
                    {
                    case TEXT_CASE_UPPER:
                        caseStr = "upper";
                        break;
                    case TEXT_CASE_LOWER:
                        caseStr = "lower";
                        break;
                    case TEXT_CASE_CAPITALIZE:
                        caseStr = "capitalize";
                        break;
                    case TEXT_CASE_SENTENCE:
                        caseStr = "sentence";
                        break;
                    default:
                        break;
                    }
                    return JS_NewString(ctx, caseStr);
                }
                if (prop == "fontShadow")
                {
                    JSValue arr = JS_NewArray(ctx);
                    const auto &shadows = t->GetShadows();
                    for (uint32_t i = 0; i < static_cast<uint32_t>(shadows.size()); ++i)
                    {
                        JSValue sh = JS_NewObject(ctx);
                        JS_SetPropertyStr(ctx, sh, "x", JS_NewFloat64(ctx, shadows[i].offsetX));
                        JS_SetPropertyStr(ctx, sh, "y", JS_NewFloat64(ctx, shadows[i].offsetY));
                        JS_SetPropertyStr(ctx, sh, "blur", JS_NewFloat64(ctx, shadows[i].blur));
                        const std::wstring c = ColorUtil::ToRGBAString(shadows[i].color, shadows[i].alpha);
                        JS_SetPropertyStr(ctx, sh, "color", JS_NewString(ctx, Utils::ToString(c).c_str()));
                        JS_SetPropertyUint32(ctx, arr, i, sh);
                    }
                    return arr;
                }
            }
            else if (element->GetType() == ELEMENT_IMAGE)
            {
                auto *img = static_cast<ImageElement *>(element);
                if (prop == "path")
                    return JS_NewString(ctx, Utils::ToString(img->GetImagePath()).c_str());
                if (prop == "preserveAspectRatio")
                {
                    const char *aspect = "stretch";
                    switch (img->GetPreserveAspectRatio())
                    {
                    case IMAGE_ASPECT_PRESERVE:
                        aspect = "preserve";
                        break;
                    case IMAGE_ASPECT_CROP:
                        aspect = "crop";
                        break;
                    default:
                        break;
                    }
                    return JS_NewString(ctx, aspect);
                }
                if (prop == "scaleMargins")
                {
                    if (!img->HasScaleMargins())
                    {
                        return JS_UNDEFINED;
                    }
                    JSValue arr = JS_NewArray(ctx);
                    JS_SetPropertyUint32(ctx, arr, 0, JS_NewFloat64(ctx, img->GetScaleMarginLeft()));
                    JS_SetPropertyUint32(ctx, arr, 1, JS_NewFloat64(ctx, img->GetScaleMarginTop()));
                    JS_SetPropertyUint32(ctx, arr, 2, JS_NewFloat64(ctx, img->GetScaleMarginRight()));
                    JS_SetPropertyUint32(ctx, arr, 3, JS_NewFloat64(ctx, img->GetScaleMarginBottom()));
                    return arr;
                }
                if (prop == "tile")
                    return JS_NewBool(ctx, img->IsTile() ? 1 : 0);
                
                return GetGeneralImagePropertyValue(ctx, element, prop);
            }
            else if (element->GetType() == ELEMENT_BUTTON)
            {
                auto *btn = static_cast<ButtonElement *>(element);
                if (prop == "buttonImageName")
                    return JS_NewString(ctx, Utils::ToString(btn->GetImagePath()).c_str());
                
                return GetGeneralImagePropertyValue(ctx, element, prop);
            }
            else if (element->GetType() == ELEMENT_BITMAP)
            {
                auto *bitmap = static_cast<BitmapElement *>(element);
                if (prop == "value")
                    return JS_NewFloat64(ctx, bitmap->GetValue());
                if (prop == "bitmapImageName")
                    return JS_NewString(ctx, Utils::ToString(bitmap->GetImagePath()).c_str());
                if (prop == "bitmapFrames")
                    return JS_NewInt32(ctx, bitmap->GetBitmapFrames());
                if (prop == "bitmapZeroFrame")
                    return JS_NewBool(ctx, bitmap->GetBitmapZeroFrame() ? 1 : 0);
                if (prop == "bitmapExtend")
                    return JS_NewBool(ctx, bitmap->GetBitmapExtend() ? 1 : 0);
                if (prop == "minValue")
                    return JS_NewFloat64(ctx, bitmap->GetMinValue());
                if (prop == "maxValue")
                    return JS_NewFloat64(ctx, bitmap->GetMaxValue());
                if (prop == "bitmapOrientation")
                    return JS_NewString(ctx, Utils::ToString(bitmap->GetBitmapOrientation()).c_str());
                if (prop == "bitmapDigits")
                    return JS_NewInt32(ctx, bitmap->GetBitmapDigits());
                if (prop == "bitmapSeparation")
                    return JS_NewInt32(ctx, bitmap->GetBitmapSeparation());
                if (prop == "bitmapAlign")
                {
                    const char *align = "left";
                    switch (bitmap->GetBitmapAlign())
                    {
                    case BITMAP_ALIGN_CENTER:
                        align = "center";
                        break;
                    case BITMAP_ALIGN_RIGHT:
                        align = "right";
                        break;
                    default:
                        break;
                    }
                    return JS_NewString(ctx, align);
                }

                return GetGeneralImagePropertyValue(ctx, element, prop);
            }
            else if (element->GetType() == ELEMENT_AREA_GRAPH)
            {
                auto *graph = static_cast<AreaGraphElement *>(element);
                if (prop == "data")
                {
                    JSValue arr = JS_NewArray(ctx);
                    const auto &data = graph->GetData();
                    for (uint32_t i = 0; i < static_cast<uint32_t>(data.size()); ++i)
                    {
                        JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, data[i]));
                    }
                    return arr;
                }
                if (prop == "minValue")
                    return JS_NewFloat64(ctx, graph->GetMinValue());
                if (prop == "maxValue")
                    return JS_NewFloat64(ctx, graph->GetMaxValue());
                if (prop == "autoRange")
                    return JS_NewBool(ctx, graph->GetAutoRange() ? 1 : 0);
                if (prop == "lineColor")
                    return JS_NewString(ctx, Utils::ToString(ColorUtil::ToRGBAString(graph->GetLineColor(), 255)).c_str());
                if (prop == "lineWidth")
                    return JS_NewFloat64(ctx, graph->GetLineWidth());
                if (prop == "fillColor")
                    return JS_NewString(ctx, Utils::ToString(ColorUtil::ToRGBAString(graph->GetFillColor(), graph->GetFillAlpha())).c_str());
                if (prop == "fillAlpha")
                    return JS_NewInt32(ctx, graph->GetFillAlpha());
                if (prop == "maxPoints")
                    return JS_NewInt32(ctx, graph->GetMaxPoints());
                if (prop == "gridColor")
                    return JS_NewString(ctx, Utils::ToString(ColorUtil::ToRGBAString(graph->GetGridColor(), graph->GetGridAlpha())).c_str());
                if (prop == "gridAlpha")
                    return JS_NewInt32(ctx, graph->GetGridAlpha());
                if (prop == "gridVisible")
                    return JS_NewBool(ctx, graph->GetGridVisible() ? 1 : 0);
                if (prop == "gridX")
                    return JS_NewInt32(ctx, graph->GetGridXSpacing());
                if (prop == "gridY")
                    return JS_NewInt32(ctx, graph->GetGridYSpacing());
                if (prop == "graphStart")
                    return JS_NewString(ctx, graph->GetGraphStartLeft() ? "left" : "right");
                if (prop == "flip")
                    return JS_NewBool(ctx, graph->GetFlip() ? 1 : 0);
            }
            else if (element->GetType() == ELEMENT_ROTATOR)
            {
                auto *rotator = static_cast<RotatorElement *>(element);
                if (prop == "value")
                    return JS_NewFloat64(ctx, rotator->GetValue());
                if (prop == "rotatorImageName")
                    return JS_NewString(ctx, Utils::ToString(rotator->GetImagePath()).c_str());
                if (prop == "offsetX")
                    return JS_NewFloat64(ctx, rotator->GetOffsetX());
                if (prop == "offsetY")
                    return JS_NewFloat64(ctx, rotator->GetOffsetY());
                if (prop == "startAngle")
                    return JS_NewFloat64(ctx, rotator->GetStartAngle());
                if (prop == "rotationAngle")
                    return JS_NewFloat64(ctx, rotator->GetRotationAngle());
                if (prop == "valueRemainder")
                    return JS_NewInt32(ctx, rotator->GetValueRemainder());
                if (prop == "minValue")
                    return JS_NewFloat64(ctx, rotator->GetMinValue());
                if (prop == "maxValue")
                    return JS_NewFloat64(ctx, rotator->GetMaxValue());

                return GetGeneralImagePropertyValue(ctx, element, prop);
            }
            else if (element->GetType() == ELEMENT_BAR)
            {
                auto *bar = static_cast<BarElement *>(element);
                if (prop == "value")
                    return JS_NewFloat64(ctx, bar->GetValue());
                if (prop == "barCornerRadius")
                    return JS_NewInt32(ctx, bar->GetBarCornerRadius());
                if (prop == "orientation")
                    return JS_NewString(ctx, bar->GetOrientation() == BAR_VERTICAL ? "vertical" : "horizontal");
                if (prop == "barColor" && bar->HasBarColor())
                {
                    const std::wstring c = ColorUtil::ToRGBAString(bar->GetBarColor(), bar->GetBarAlpha());
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }
            }
            else if (element->GetType() == ELEMENT_ROUNDLINE)
            {
                auto *rl = static_cast<RoundLineElement *>(element);
                if (prop == "value")
                    return JS_NewFloat64(ctx, rl->GetValue());
                if (prop == "radius")
                    return JS_NewInt32(ctx, rl->GetRadius());
                if (prop == "thickness")
                    return JS_NewInt32(ctx, rl->GetThickness());
                if (prop == "startAngle")
                    return JS_NewFloat64(ctx, rl->GetStartAngle());
                if (prop == "totalAngle")
                    return JS_NewFloat64(ctx, rl->GetTotalAngle());
                if (prop == "clockwise")
                    return JS_NewBool(ctx, rl->IsClockwise() ? 1 : 0);
                if (prop == "endThickness")
                    return JS_NewInt32(ctx, rl->GetEndThickness());
                if (prop == "ticks")
                    return JS_NewInt32(ctx, rl->GetTicks());
                if (prop == "capType")
                    return JS_NewString(ctx, rl->GetCapType() == ROUNDLINE_CAP_ROUND ? "round" : "flat");
                if (prop == "lineColor" && rl->HasLineColor())
                {
                    const std::wstring c = ColorUtil::ToRGBAString(rl->GetLineColor(), rl->GetLineAlpha());
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }
                if (prop == "lineColorBg" && rl->HasLineColorBg())
                {
                    const std::wstring c = ColorUtil::ToRGBAString(rl->GetLineColorBg(), rl->GetLineAlphaBg());
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }
            }
            else if (element->GetType() == ELEMENT_LINE)
            {
                auto *line = static_cast<LineElement *>(element);
                if (prop == "lineCount")
                    return JS_NewInt32(ctx, line->GetLineCount());
                if (prop == "lineWidth")
                    return JS_NewFloat64(ctx, line->GetLineWidth());
                if (prop == "maxPoints")
                    return JS_NewInt32(ctx, line->GetMaxPoints());
                if (prop == "horizontalLines")
                    return JS_NewBool(ctx, line->GetHorizontalLines() ? 1 : 0);
                if (prop == "horizontalLineColor")
                {
                    const std::wstring c = ColorUtil::ToRGBAString(line->GetHorizontalLineColor(), line->GetHorizontalLineAlpha());
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }
                if (prop == "graphStart")
                    return JS_NewString(ctx, line->GetGraphStartLeft() ? "left" : "right");
                if (prop == "graphOrientation")
                    return JS_NewString(ctx, line->GetGraphHorizontalOrientation() ? "horizontal" : "vertical");
                if (prop == "flip")
                    return JS_NewBool(ctx, line->GetFlip() ? 1 : 0);
                if (prop == "transformStroke")
                    return JS_NewString(ctx, line->GetStrokeTransformType() == D2D1_STROKE_TRANSFORM_TYPE_FIXED ? "fixed" : "normal");
                if (prop == "autoRange")
                    return JS_NewBool(ctx, line->GetAutoRange() ? 1 : 0);
                if (prop == "rangeMin")
                    return JS_NewFloat64(ctx, line->GetScaleMin());
                if (prop == "rangeMax")
                    return JS_NewFloat64(ctx, line->GetScaleMax());

                if (prop == "data")
                {
                    JSValue arr = JS_NewArray(ctx);
                    const auto &dataSets = line->GetDataSets();
                    if (!dataSets.empty())
                    {
                        for (uint32_t i = 0; i < static_cast<uint32_t>(dataSets[0].size()); ++i)
                        {
                            JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, dataSets[0][i]));
                        }
                    }
                    return arr;
                }

                for (int i = 0; i < line->GetLineCount(); ++i)
                {
                    std::string colorKey = (i == 0) ? "lineColor" : ("lineColor" + std::to_string(i + 1));
                    if (prop == colorKey)
                    {
                        const auto &colors = line->GetLineColors();
                        const auto &alphas = line->GetLineAlphas();
                        if (i < (int)colors.size() && i < (int)alphas.size())
                        {
                            const std::wstring c = ColorUtil::ToRGBAString(colors[(size_t)i], alphas[(size_t)i]);
                            return JS_NewString(ctx, Utils::ToString(c).c_str());
                        }
                    }

                    std::string scaleKey = (i == 0) ? "lineScale" : ("lineScale" + std::to_string(i + 1));
                    if (prop == scaleKey)
                    {
                        const auto &scales = line->GetScaleValues();
                        if (i < (int)scales.size())
                        {
                            return JS_NewFloat64(ctx, scales[(size_t)i]);
                        }
                    }

                    std::string dataKey = (i == 0) ? "data" : ("data" + std::to_string(i + 1));
                    if (prop == dataKey)
                    {
                        JSValue arr = JS_NewArray(ctx);
                        const auto &dataSets = line->GetDataSets();
                        if (i < (int)dataSets.size())
                        {
                            for (uint32_t k = 0; k < static_cast<uint32_t>(dataSets[(size_t)i].size()); ++k)
                            {
                                JS_SetPropertyUint32(ctx, arr, k, JS_NewFloat64(ctx, dataSets[(size_t)i][k]));
                            }
                        }
                        return arr;
                    }
                }
            }
            else if (element->GetType() == ELEMENT_HISTOGRAM)
            {
                auto *histogram = static_cast<HistogramElement *>(element);
                if (prop == "data")
                {
                    JSValue arr = JS_NewArray(ctx);
                    const auto &data = histogram->GetData();
                    for (uint32_t i = 0; i < static_cast<uint32_t>(data.size()); ++i)
                    {
                        JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, data[i]));
                    }
                    return arr;
                }
                if (prop == "data2")
                {
                    JSValue arr = JS_NewArray(ctx);
                    const auto &data = histogram->GetData2();
                    for (uint32_t i = 0; i < static_cast<uint32_t>(data.size()); ++i)
                    {
                        JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, data[i]));
                    }
                    return arr;
                }
                if (prop == "autoRange")
                    return JS_NewBool(ctx, histogram->GetAutoRange() ? 1 : 0);
                if (prop == "graphStart")
                    return JS_NewString(ctx, histogram->GetGraphStartLeft() ? "left" : "right");
                if (prop == "graphOrientation")
                    return JS_NewString(ctx, histogram->GetGraphHorizontalOrientation() ? "horizontal" : "vertical");
                if (prop == "flip")
                    return JS_NewBool(ctx, histogram->GetFlip() ? 1 : 0);
                if (prop == "primaryColor")
                {
                    const std::wstring c = ColorUtil::ToRGBAString(histogram->GetPrimaryColor(), histogram->GetPrimaryAlpha());
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }
                if (prop == "secondaryColor")
                {
                    const std::wstring c = ColorUtil::ToRGBAString(histogram->GetSecondaryColor(), histogram->GetSecondaryAlpha());
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }
                if (prop == "bothColor")
                {
                    const std::wstring c = ColorUtil::ToRGBAString(histogram->GetBothColor(), histogram->GetBothAlpha());
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }
            }
            else if (element->GetType() == ELEMENT_SHAPE)
            {
                auto *shape = static_cast<ShapeElement *>(element);

                auto capToStr = [](D2D1_CAP_STYLE cap) -> const char *
                {
                    switch (cap)
                    {
                    case D2D1_CAP_STYLE_ROUND:
                        return "Round";
                    case D2D1_CAP_STYLE_SQUARE:
                        return "Square";
                    case D2D1_CAP_STYLE_TRIANGLE:
                        return "Triangle";
                    default:
                        return "Flat";
                    }
                };
                auto joinToStr = [](D2D1_LINE_JOIN join) -> const char *
                {
                    switch (join)
                    {
                    case D2D1_LINE_JOIN_BEVEL:
                        return "Bevel";
                    case D2D1_LINE_JOIN_ROUND:
                        return "Round";
                    case D2D1_LINE_JOIN_MITER_OR_BEVEL:
                        return "MiterOrBevel";
                    default:
                        return "Miter";
                    }
                };

                if (prop == "shapeType")
                {
                    if (dynamic_cast<RectangleShape *>(shape))
                        return JS_NewString(ctx, "rectangle");
                    if (dynamic_cast<EllipseShape *>(shape))
                        return JS_NewString(ctx, "ellipse");
                    if (dynamic_cast<LineShape *>(shape))
                        return JS_NewString(ctx, "line");
                    if (dynamic_cast<ArcShape *>(shape))
                        return JS_NewString(ctx, "arc");
                    if (dynamic_cast<CurveShape *>(shape))
                        return JS_NewString(ctx, "curve");
                    if (dynamic_cast<PathShape *>(shape))
                        return JS_NewString(ctx, "path");
                }

                if (prop == "strokeWidth")
                    return JS_NewFloat64(ctx, shape->GetStrokeWidth());
                if (prop == "strokeColor" && shape->HasStroke())
                {
                    const std::wstring c = ColorUtil::ToRGBAString(shape->GetStrokeColor(), shape->GetStrokeAlpha());
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }
                if (prop == "fillColor" && shape->HasFill())
                {
                    const std::wstring c = ColorUtil::ToRGBAString(shape->GetFillColor(), shape->GetFillAlpha());
                    return JS_NewString(ctx, Utils::ToString(c).c_str());
                }

                if (prop == "radiusX")
                    return JS_NewFloat64(ctx, shape->GetRadiusX());
                if (prop == "radiusY")
                    return JS_NewFloat64(ctx, shape->GetRadiusY());
                if (prop == "startX")
                    return JS_NewFloat64(ctx, shape->GetStartX());
                if (prop == "startY")
                    return JS_NewFloat64(ctx, shape->GetStartY());
                if (prop == "endX")
                    return JS_NewFloat64(ctx, shape->GetEndX());
                if (prop == "endY")
                    return JS_NewFloat64(ctx, shape->GetEndY());
                if (prop == "curveType")
                    return JS_NewString(ctx, Utils::ToString(shape->GetCurveType()).c_str());
                if (prop == "controlX")
                    return JS_NewFloat64(ctx, shape->GetControlX());
                if (prop == "controlY")
                    return JS_NewFloat64(ctx, shape->GetControlY());
                if (prop == "control2X")
                    return JS_NewFloat64(ctx, shape->GetControl2X());
                if (prop == "control2Y")
                    return JS_NewFloat64(ctx, shape->GetControl2Y());
                if (prop == "startAngle")
                    return JS_NewFloat64(ctx, shape->GetStartAngle());
                if (prop == "endAngle")
                    return JS_NewFloat64(ctx, shape->GetEndAngle());
                if (prop == "clockwise")
                    return JS_NewBool(ctx, shape->IsClockwise() ? 1 : 0);
                if (prop == "pathData")
                    return JS_NewString(ctx, Utils::ToString(shape->GetPathData()).c_str());

                if (prop == "strokeStartCap")
                    return JS_NewString(ctx, capToStr(shape->GetStrokeStartCap()));
                if (prop == "strokeEndCap")
                    return JS_NewString(ctx, capToStr(shape->GetStrokeEndCap()));
                if (prop == "strokeDashCap")
                    return JS_NewString(ctx, capToStr(shape->GetStrokeDashCap()));
                if (prop == "strokeLineJoin")
                    return JS_NewString(ctx, joinToStr(shape->GetStrokeLineJoin()));
                if (prop == "strokeDashOffset")
                    return JS_NewFloat64(ctx, shape->GetStrokeDashOffset());
                if (prop == "strokeDashes")
                {
                    JSValue arr = JS_NewArray(ctx);
                    const auto &dashes = shape->GetStrokeDashes();
                    for (uint32_t i = 0; i < static_cast<uint32_t>(dashes.size()); ++i)
                    {
                        JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, dashes[i]));
                    }
                    return arr;
                }

                if (auto *pathShape = dynamic_cast<PathShape *>(shape))
                {
                    if (prop == "isCombine")
                        return JS_NewBool(ctx, pathShape->IsCombineShape() ? 1 : 0);
                    if (prop == "combineBaseId" || prop == "combineConsumeAll" || prop == "combineOps")
                    {
                        std::wstring baseId;
                        std::vector<PathShape::CombineOp> ops;
                        bool consumeBase = false;
                        pathShape->GetCombineData(baseId, ops, consumeBase);
                        if (prop == "combineBaseId")
                            return JS_NewString(ctx, Utils::ToString(baseId).c_str());
                        if (prop == "combineConsumeAll")
                            return JS_NewBool(ctx, consumeBase ? 1 : 0);
                        JSValue arr = JS_NewArray(ctx);
                        for (uint32_t i = 0; i < static_cast<uint32_t>(ops.size()); ++i)
                        {
                            JSValue op = JS_NewObject(ctx);
                            JS_SetPropertyStr(ctx, op, "id", JS_NewString(ctx, Utils::ToString(ops[i].id).c_str()));
                            const char *mode = "union";
                            switch (ops[i].mode)
                            {
                            case D2D1_COMBINE_MODE_INTERSECT:
                                mode = "intersect";
                                break;
                            case D2D1_COMBINE_MODE_XOR:
                                mode = "xor";
                                break;
                            case D2D1_COMBINE_MODE_EXCLUDE:
                                mode = "exclude";
                                break;
                            default:
                                break;
                            }
                            JS_SetPropertyStr(ctx, op, "mode", JS_NewString(ctx, mode));
                            JS_SetPropertyStr(ctx, op, "consume", JS_NewBool(ctx, ops[i].consume ? 1 : 0));
                            JS_SetPropertyUint32(ctx, arr, i, op);
                        }
                        return arr;
                    }
                }
            }

            return JS_UNDEFINED;
        }

        JSValue JsWidgetGetElementProperty(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetAnyWidget(ctx, thisVal);
            if (!widget)
                return JS_EXCEPTION;
            if (argc < 2)
                return ThrowTypeError(ctx, "getElementProperty", "expected (id, propertyName)");

            const char *idUtf8 = JS_ToCString(ctx, argv[0]);
            const char *propUtf8 = JS_ToCString(ctx, argv[1]);
            if (!idUtf8 || !propUtf8)
            {
                if (idUtf8)
                    JS_FreeCString(ctx, idUtf8);
                if (propUtf8)
                    JS_FreeCString(ctx, propUtf8);
                return JS_EXCEPTION;
            }

            const std::wstring id = Utils::ToWString(idUtf8);
            const std::string prop = propUtf8;
            JS_FreeCString(ctx, idUtf8);
            JS_FreeCString(ctx, propUtf8);

            Element *element = widget->FindElementById(id);
            if (!element)
            {
                return JS_NULL;
            }
            return GetElementPropertyValue(ctx, element, prop);
        }

        void JsWidgetFinalizer(JSRuntime *, JSValue)
        {
            // Native widget lifetime is owned by Novadesk's global widget list.
        }

        const JSCFunctionListEntry kWidgetProtoFuncs[] = {
            JS_CFUNC_DEF("addImage", 1, JsWidgetAddImage),
            JS_CFUNC_DEF("addButton", 1, JsWidgetAddButton),
            JS_CFUNC_DEF("addText", 1, JsWidgetAddText),
            JS_CFUNC_DEF("addBar", 1, JsWidgetAddBar),
            JS_CFUNC_DEF("addLine", 1, JsWidgetAddLine),
            JS_CFUNC_DEF("addHistogram", 1, JsWidgetAddHistogram),
            JS_CFUNC_DEF("addRoundLine", 1, JsWidgetAddRoundLine),
            JS_CFUNC_DEF("addShape", 1, JsWidgetAddShape),
            JS_CFUNC_DEF("addBitmap", 1, JsWidgetAddBitmap),
            JS_CFUNC_DEF("addRotator", 1, JsWidgetAddRotator),
            JS_CFUNC_DEF("addAreaGraph", 1, JsWidgetAddAreaGraph),
            JS_CFUNC_DEF("setElementProperty", 2, JsWidgetSetElementProperties),
            JS_CFUNC_DEF("setElementProperties", 2, JsWidgetSetElementProperties),
            JS_CFUNC_DEF("setElementPropertyByGroup", 2, JsWidgetSetElementPropertiesByGroup),
            JS_CFUNC_DEF("setElementPropertiesByGroup", 2, JsWidgetSetElementPropertiesByGroup),
            JS_CFUNC_DEF("getElementProperty", 2, JsWidgetGetElementProperty),
            JS_CFUNC_DEF("removeElements", 1, JsWidgetRemoveElements),
            JS_CFUNC_DEF("removeElementsByGroup", 1, JsWidgetRemoveElementsByGroup),
            JS_CFUNC_DEF("beginUpdate", 0, JsWidgetBeginUpdate),
            JS_CFUNC_DEF("endUpdate", 0, JsWidgetEndUpdate),
        };

        bool RunWidgetUiScriptImpl(JSContext *ctx, Widget *widget, const std::wstring &scriptPath)
        {
            if (!widget || scriptPath.empty())
                return true;

            {
                std::wstring lower = scriptPath;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
                const std::wstring requiredSuffix = L".ui.js";
                if (lower.size() < requiredSuffix.size() ||
                    lower.compare(lower.size() - requiredSuffix.size(), requiredSuffix.size(), requiredSuffix) != 0)
                {
                    Logging::Log(LogLevel::Error, L"[novadesk] widget ui script must end with '.ui.js': %s", scriptPath.c_str());
                    return false;
                }
            }

            std::wstring absPath;
            std::wstring baseDir;
            if (PathUtils::IsPathRelative(scriptPath))
            {
                baseDir = GetWidgetScriptBaseDir(widget);
                absPath = PathUtils::ResolvePath(
                    scriptPath,
                    baseDir.empty() ? PathUtils::GetWidgetsDir() : baseDir);
            }
            else
            {
                absPath = PathUtils::NormalizePath(scriptPath);
            }
            const std::string scriptSource = FileUtils::ReadFileContent(absPath);
            if (scriptSource.empty())
            {
                return false;
            }

            JSRuntime *rt = JS_GetRuntime(ctx);
            if (g_widgetUiClassId == 0)
            {
                JS_NewClassID(rt, &g_widgetUiClassId);
            }
            if (g_widgetUiClassRuntime != rt)
            {
                JSClassDef uiCls{};
                uiCls.class_name = "WidgetUiBridge";
                JS_NewClass(rt, g_widgetUiClassId, &uiCls);
                JSValue uiProto = JS_NewObject(ctx);
                JS_SetPropertyFunctionList(ctx, uiProto, kWidgetProtoFuncs, sizeof(kWidgetProtoFuncs) / sizeof(kWidgetProtoFuncs[0]));
                JS_SetClassProto(ctx, g_widgetUiClassId, uiProto);
                g_widgetUiClassRuntime = rt;
            }

            JSValue uiObj = JS_NewObjectClass(ctx, g_widgetUiClassId);
            if (JS_IsException(uiObj))
                return false;
            JS_SetOpaque(uiObj, widget);

            JSValue global = JS_GetGlobalObject(ctx);
            JSValue ipcObj = JSEngine::CreateUiIpcObject(ctx);
            JS_SetPropertyStr(ctx, global, "ui", JS_DupValue(ctx, uiObj));
            JS_SetPropertyStr(ctx, global, "ipcRenderer", JS_DupValue(ctx, ipcObj));
            const std::string fileName = Utils::ToString(absPath);
            const std::string dirName = Utils::ToString(PathUtils::GetParentDir(absPath));
            const std::string widgetDirName = Utils::ToString(PathUtils::GetWidgetsDir());
            const std::string addonsPathName = Utils::ToString(PathUtils::GetAddonsDir());
            JS_SetPropertyStr(ctx, global, "__filename", JS_NewString(ctx, fileName.c_str()));
            JS_SetPropertyStr(ctx, global, "__dirname", JS_NewString(ctx, dirName.c_str()));
            JS_SetPropertyStr(ctx, global, "__widgetDir", JS_NewString(ctx, widgetDirName.c_str()));
            JS_SetPropertyStr(ctx, global, "__addonsPath", JS_NewString(ctx, addonsPathName.c_str()));
            JS_FreeValue(ctx, global);

            const std::string scriptPrelude =
                "(function(ui, ipcRenderer, __filename, __dirname, __widgetDir){\n"
                "const setTimeout = undefined;\n"
                "const setInterval = undefined;\n"
                "const clearTimeout = undefined;\n"
                "const clearInterval = undefined;\n";
            const std::string scriptSuffix =
                "\n})(globalThis.ui, globalThis.ipcRenderer, globalThis.__filename, globalThis.__dirname, globalThis.__widgetDir);\n";
            const std::string scriptSourceWithPrelude = scriptPrelude + scriptSource + scriptSuffix;

            JSValue evalResult = JS_Eval(ctx, scriptSourceWithPrelude.c_str(), scriptSourceWithPrelude.size(), fileName.c_str(), JS_EVAL_TYPE_GLOBAL);

            JSValue global2 = JS_GetGlobalObject(ctx);
            JSAtom uiAtom = JS_NewAtom(ctx, "ui");
            JS_DeleteProperty(ctx, global2, uiAtom, 0);
            JS_FreeAtom(ctx, uiAtom);
            JSAtom ipcAtom = JS_NewAtom(ctx, "ipcRenderer");
            JS_DeleteProperty(ctx, global2, ipcAtom, 0);
            JS_FreeAtom(ctx, ipcAtom);
            JSAtom filename2Atom = JS_NewAtom(ctx, "__filename");
            JS_DeleteProperty(ctx, global2, filename2Atom, 0);
            JS_FreeAtom(ctx, filename2Atom);
            JSAtom dirname2Atom = JS_NewAtom(ctx, "__dirname");
            JS_DeleteProperty(ctx, global2, dirname2Atom, 0);
            JS_FreeAtom(ctx, dirname2Atom);
            JSAtom widgetDirAtom = JS_NewAtom(ctx, "__widgetDir");
            JS_DeleteProperty(ctx, global2, widgetDirAtom, 0);
            JS_FreeAtom(ctx, widgetDirAtom);
            JSAtom addonsPathAtom = JS_NewAtom(ctx, "__addonsPath");
            JS_DeleteProperty(ctx, global2, addonsPathAtom, 0);
            JS_FreeAtom(ctx, addonsPathAtom);
            JS_FreeValue(ctx, global2);
            JS_FreeValue(ctx, uiObj);
            JS_FreeValue(ctx, ipcObj);

            if (JS_IsException(evalResult))
            {
                JSValue ex = JS_GetException(ctx);
                const char *msg = JS_ToCString(ctx, ex);
                if (msg)
                {
                    Logging::Log(LogLevel::Error, L"[novadesk] ui script error (%s): %S", absPath.c_str(), msg);
                    JS_FreeCString(ctx, msg);
                }
                else
                {
                    Logging::Log(LogLevel::Error, L"[novadesk] ui script error (%s)", absPath.c_str());
                }
                JS_FreeValue(ctx, ex);
                JS_FreeValue(ctx, evalResult);
                return false;
            }

            JS_FreeValue(ctx, evalResult);
            return true;
        }
    }

    void SetWidgetUiDebug(bool debug)
    {
        g_widgetUiDebug = debug;
    }

    JSClassID EnsureWidgetWindowClass(JSContext *ctx)
    {
        JSRuntime *rt = JS_GetRuntime(ctx);
        if (g_widgetWindowClassId == 0)
        {
            JS_NewClassID(rt, &g_widgetWindowClassId);
        }
        if (g_widgetWindowClassRuntime != rt)
        {
            JSClassDef cls{};
            cls.class_name = "widgetWindow";
            cls.finalizer = JsWidgetFinalizer;
            JS_NewClass(rt, g_widgetWindowClassId, &cls);
            InitWidgetWindowEventBindings(g_widgetWindowClassId);

            JSValue proto = JS_NewObject(ctx);
            JS_SetPropertyFunctionList(ctx, proto, kWidgetProtoFuncs, sizeof(kWidgetProtoFuncs) / sizeof(kWidgetProtoFuncs[0]));
            AttachWidgetWindowEventMethods(ctx, proto);
            JS_SetClassProto(ctx, g_widgetWindowClassId, proto);
            g_widgetWindowClassRuntime = rt;
        }
        return g_widgetWindowClassId;
    }

    JSValue JsWidgetWindowCtor(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
    {
        parser::WidgetWindowOptions parsed;
        if (argc > 0 && JS_IsObject(argv[0]))
        {
            parser::ParseWidgetWindowOptions(ctx, argv[0], parsed);
        }

        WidgetOptions options;
        options.id = parsed.id;

        if (!options.id.empty())
        {
            auto existingIt = std::find_if(
                widgets.begin(),
                widgets.end(),
                [&](Widget *existing)
                {
                    return existing && existing->GetOptions().id == options.id;
                });

            if (existingIt != widgets.end())
            {
                Widget *existing = *existingIt;
                widgets.erase(existingIt);
                delete existing;
            }
        }

        if (!options.id.empty())
        {
            Settings::LoadWidget(options.id, options);
        }

        if (parsed.hasX)
            options.x = parsed.x;
        if (parsed.hasY)
            options.y = parsed.y;

        if (parsed.hasWidth)
        {
            options.width = parsed.width;
            options.m_WDefined = (parsed.width > 0);
        }
        else
        {
            options.width = 0;
            options.m_WDefined = false;
        }
        if (parsed.hasHeight)
        {
            options.height = parsed.height;
            options.m_HDefined = (parsed.height > 0);
        }
        else
        {
            options.height = 0;
            options.m_HDefined = false;
        }

        if (parsed.hasShow)
            options.show = parsed.show;
        if (parsed.hasScriptPath)
            options.scriptPath = parsed.scriptPath;
        if (!options.scriptPath.empty())
        {
            if (PathUtils::IsPathRelative(options.scriptPath))
            {
                std::wstring base = JSEngine::GetCurrentScriptDir();
                if (base.empty())
                {
                    base = JSEngine::GetEntryScriptDir();
                }
                if (!base.empty())
                {
                    options.scriptPath = PathUtils::ResolvePath(options.scriptPath, base);
                }
                else
                {
                    options.scriptPath = PathUtils::ResolvePath(options.scriptPath, PathUtils::GetWidgetsDir());
                }
            }
            else
            {
                options.scriptPath = PathUtils::NormalizePath(options.scriptPath);
            }
        }
        if (parsed.hasDraggable)
            options.draggable = parsed.draggable;
        if (parsed.hasClickThrough)
            options.clickThrough = parsed.clickThrough;
        if (parsed.hasKeepOnScreen)
            options.keepOnScreen = parsed.keepOnScreen;
        if (parsed.hasSnapEdges)
            options.snapEdges = parsed.snapEdges;
        if (parsed.hasShowInToolbar)
            options.showInToolbar = parsed.showInToolbar;
        if (parsed.hasToolbarTitle)
            options.toolbarTitle = parsed.toolbarTitle;
        if (parsed.hasToolbarIcon)
            options.toolbarIcon = parsed.toolbarIcon;
        if (parsed.hasBackgroundColor)
        {
            options.backgroundColor = parsed.backgroundColor;
            options.color = parsed.color;
            options.bgAlpha = parsed.bgAlpha;
            options.bgGradient = parsed.bgGradient;
        }
        if (parsed.hasWindowOpacity)
            options.windowOpacity = parsed.windowOpacity;
        if (parsed.hasZPos)
        {
            options.zPos = static_cast<ZPOSITION>(parsed.zPos);
        }

        if (!options.toolbarIcon.empty())
        {
            if (PathUtils::IsPathRelative(options.toolbarIcon))
            {
                std::wstring base = JSEngine::GetCurrentScriptDir();
                if (base.empty())
                {
                    base = JSEngine::GetEntryScriptDir();
                }
                if (!base.empty())
                {
                    options.toolbarIcon = PathUtils::ResolvePath(options.toolbarIcon, base);
                }
                else
                {
                    options.toolbarIcon = PathUtils::ResolvePath(options.toolbarIcon, PathUtils::GetWidgetsDir());
                }
            }
            else
            {
                options.toolbarIcon = PathUtils::NormalizePath(options.toolbarIcon);
            }
        }

        Widget *widget = new Widget(options);
        if (!widget->Create())
        {
            delete widget;
            return JS_ThrowInternalError(ctx, "Failed to create widget window");
        }

        if (options.show)
        {
            widget->Show();
        }
        widgets.push_back(widget);
        JSEngine::RegisterWidgetOwner(widget, JSEngine::GetCurrentScriptPath());

        if (g_widgetUiDebug)
        {
            Logging::Log(LogLevel::Debug, L"[novadesk] JsCreateWindow id=%s width=%d height=%d", options.id.c_str(), options.width, options.height);
        }

        JSValue obj = JS_NewObjectClass(ctx, EnsureWidgetWindowClass(ctx));
        if (JS_IsException(obj))
            return obj;
        JS_SetOpaque(obj, widget);

        if (!options.scriptPath.empty())
        {
            RunWidgetUiScriptImpl(ctx, widget, options.scriptPath);
        }
        return obj;
    }

    bool ExecuteWidgetUiScript(JSContext *ctx, Widget *widget, const std::wstring &scriptPath)
    {
        return RunWidgetUiScriptImpl(ctx, widget, scriptPath);
    }
} // namespace novadesk::scripting::quickjs
