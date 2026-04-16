/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ButtonElement.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include "../shared/Logging.h"

ButtonElement::ButtonElement(const std::wstring &id, int x, int y, const std::wstring &path)
    : Element(ELEMENT_BUTTON, id, x, y, 0, 0)
{
    Logging::Log(LogLevel::Info, L"ButtonElement created: id=%s, path=%s", id.c_str(), path.c_str());
    UpdateImage(path);
}

ButtonElement::~ButtonElement()
{
}

void ButtonElement::UpdateImage(const std::wstring &path)
{
    m_ButtonImage.SetPath(path);
}

int ButtonElement::GetAutoWidth()
{
    int w = m_ButtonImage.GetAutoWidth();
    int h = m_ButtonImage.GetAutoHeight();
    
    // Auto width is w / 3 if it's laid out horizontally
    if (w >= h * 3) {
        return w / 3;
    }
    return w;
}

int ButtonElement::GetAutoHeight()
{
    int w = m_ButtonImage.GetAutoWidth();
    int h = m_ButtonImage.GetAutoHeight();
    
    // Auto height is h / 3 if it's laid out vertically
    if (w >= h * 3) {
        return h;
    }
    return h / 3;
}

void ButtonElement::Render(ID2D1DeviceContext *context)
{
    if (!m_Show)
        return;

    m_ButtonImage.EnsureBitmap(context);

    D2D1_MATRIX_3X2_F originalTransform = D2D1::Matrix3x2F::Identity();
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    if (m_ButtonImage.IsLoaded())
    {
        Microsoft::WRL::ComPtr<ID2D1Image> finalImage;
        if (m_ButtonImage.BuildProcessedImage(context, finalImage) && finalImage)
        {
            float imageWidth = static_cast<float>(m_ButtonImage.GetAutoWidth());
            float imageHeight = static_cast<float>(m_ButtonImage.GetAutoHeight());
            
            bool horizontal = (imageWidth >= imageHeight * 3);
            float frameWidth = imageWidth;
            float frameHeight = imageHeight;

            if (horizontal) {
                frameWidth = imageWidth / 3.0f;
            } else {
                frameHeight = imageHeight / 3.0f;
            }
            
            int frameIndex = 0;
            switch (m_State) {
                case BUTTON_STATE_CLICKED: frameIndex = 1; break;
                case BUTTON_STATE_HOVERED: frameIndex = 2; break;
                default: frameIndex = 0; break;
            }
            
            D2D1_RECT_F srcRect;
            if (horizontal) {
                srcRect = D2D1::RectF(frameWidth * frameIndex, 0, frameWidth * (frameIndex + 1), imageHeight);
            } else {
                srcRect = D2D1::RectF(0, frameHeight * frameIndex, imageWidth, frameHeight * (frameIndex + 1));
            }

            float destW = static_cast<float>(GetWidth());
            float destH = static_cast<float>(GetHeight());
            if (destW <= 0) destW = frameWidth;
            if (destH <= 0) destH = frameHeight;

            D2D1_MATRIX_3X2_F currentTransform;
            context->GetTransform(&currentTransform);
            
            D2D1_MATRIX_3X2_F finalDrawTransform = 
                D2D1::Matrix3x2F::Translation(-srcRect.left, -srcRect.top) *
                D2D1::Matrix3x2F::Scale(destW / frameWidth, destH / frameHeight) *
                D2D1::Matrix3x2F::Translation(static_cast<float>(m_X + m_PaddingLeft), static_cast<float>(m_Y + m_PaddingTop)) *
                currentTransform;

            context->SetTransform(finalDrawTransform);
            context->DrawImage(finalImage.Get(), D2D1::Point2F(srcRect.left, srcRect.top), srcRect);
            
            context->SetTransform(currentTransform);

            Logging::Log(LogLevel::Info, L"Button Render Success: id=%s, frame=%d, dest=[%.1f, %.1f]", m_Id.c_str(), frameIndex, destW, destH);
        }
        else
        {
            Logging::Log(LogLevel::Error, L"Button Render Error: BuildProcessedImage failed for %s", m_Id.c_str());
        }
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}

bool ButtonElement::HitTest(int x, int y)
{
    if (!Element::HitTest(x, y))
        return false;

    if (!m_ButtonImage.IsLoaded())
        return true; // fallback to bounds hit testing
    
    // Pixel perfect alpha test
    float contentX = static_cast<float>(m_X + m_PaddingLeft);
    float contentY = static_cast<float>(m_Y + m_PaddingTop);
    
    float width = static_cast<float>(GetWidth());
    float height = static_cast<float>(GetHeight());

    int autoW = GetAutoWidth();
    int autoH = GetAutoHeight();
    
    if (width <= 0.0f) width = static_cast<float>(autoW);
    if (height <= 0.0f) height = static_cast<float>(autoH);

    if (width <= 0.0f || height <= 0.0f) return true;

    float relX = (x - contentX) / width;
    float relY = (y - contentY) / height;

    if (relX < 0 || relX > 1.0f || relY < 0 || relY > 1.0f)
        return false;

    int imgWidth = m_ButtonImage.GetAutoWidth();
    int imgHeight = m_ButtonImage.GetAutoHeight();

    bool horizontal = (imgWidth >= imgHeight * 3);
    
    float frameWidth = horizontal ? (imgWidth / 3.0f) : static_cast<float>(imgWidth);
    float frameHeight = horizontal ? static_cast<float>(imgHeight) : (imgHeight / 3.0f);

    int frameIndex = 0;
    if (m_State == BUTTON_STATE_CLICKED)
        frameIndex = 1;
    else if (m_State == BUTTON_STATE_HOVERED)
        frameIndex = 2;

    int pixelX = static_cast<int>((relX * frameWidth) + (horizontal ? (frameIndex * frameWidth) : 0));
    int pixelY = static_cast<int>((relY * frameHeight) + (horizontal ? 0 : (frameIndex * frameHeight)));

    BYTE alpha = m_ButtonImage.GetPixelAlpha(pixelX, pixelY);
    
    return alpha > 0;
}
