/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "RotatorElement.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define CONVERT_TO_DEGREES(X) ((X) * (180.0 / M_PI))

RotatorElement::RotatorElement(const std::wstring &id, int x, int y, const std::wstring &path)
    : Element(ELEMENT_ROTATOR, id, x, y, 0, 0)
{
    m_RotatorImage.SetPath(path);
}

RotatorElement::~RotatorElement()
{
}

int RotatorElement::GetAutoWidth()
{
    return m_RotatorImage.GetAutoWidth();
}

int RotatorElement::GetAutoHeight()
{
    return m_RotatorImage.GetAutoHeight();
}

void RotatorElement::Render(ID2D1DeviceContext *context)
{
    if (!m_Show || !context)
        return;

    m_RotatorImage.EnsureBitmap(context);
    ID2D1Bitmap *bitmap = m_RotatorImage.GetBitmap();
    if (!bitmap)
        return;

    // Apply background / bevel
    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);
    RenderBackground(context);
    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);

    const float imageW = bitmap->GetSize().width;
    const float imageH = bitmap->GetSize().height;
    const float opacity = m_RotatorImage.GetImageAlpha() / 255.0f;

    // Calculate the normalized value (0.0-1.0)
    double normalizedValue = 0.0;
    if (m_ValueRemainder > 0)
    {
        long long rawValue = static_cast<long long>(m_Value);
        normalizedValue = static_cast<double>(rawValue % m_ValueRemainder) / static_cast<double>(m_ValueRemainder);
    }
    else
    {
        // Standard mode: normalize using maxValue
        double range = m_MaxValue - m_MinValue;
        normalizedValue = (range > 0.0) ? (m_Value - m_MinValue) / range : 0.0;
        if (normalizedValue < 0.0)
            normalizedValue = 0.0;
        if (normalizedValue > 1.0)
            normalizedValue = 1.0;
    }

    float angleDeg = static_cast<float>(CONVERT_TO_DEGREES(m_RotationAngle * normalizedValue + m_StartAngle));

    const int contentX = m_X + m_PaddingLeft;
    const int contentY = m_Y + m_PaddingTop;
    const int elementW = GetWidth();
    const int elementH = GetHeight();
    float cx = static_cast<float>(contentX) + static_cast<float>(elementW) / 2.0f;
    float cy = static_cast<float>(contentY) + static_cast<float>(elementH) / 2.0f;

    D2D1_MATRIX_3X2_F currentTransform;
    context->GetTransform(&currentTransform);

    D2D1_MATRIX_3X2_F combinedTransform =
        D2D1::Matrix3x2F::Translation(static_cast<float>(-m_OffsetX), static_cast<float>(-m_OffsetY)) *
        D2D1::Matrix3x2F::Rotation(angleDeg) *
        D2D1::Matrix3x2F::Translation(cx, cy) *
        currentTransform;

    context->SetTransform(combinedTransform);

    Microsoft::WRL::ComPtr<ID2D1Image> finalImage;
    m_RotatorImage.BuildProcessedImage(context, finalImage);

    const D2D1_RECT_F srcRect = D2D1::RectF(0.0f, 0.0f, imageW, imageH);
    const D2D1_RECT_F dstRect = D2D1::RectF(0.0f, 0.0f, imageW, imageH);

    const D2D1_BITMAP_INTERPOLATION_MODE interp = m_AntiAlias
                                                      ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
                                                      : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

    if (finalImage.Get() == static_cast<ID2D1Image *>(bitmap))
    {
        context->DrawBitmap(bitmap, &dstRect, opacity, interp, &srcRect);
    }
    else
    {
        const D2D1_POINT_2F targetOffset = D2D1::Point2F(0.0f, 0.0f);
        context->DrawImage(finalImage.Get(), &targetOffset, &srcRect,
                           m_AntiAlias ? D2D1_INTERPOLATION_MODE_LINEAR : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
    }

    // Restore original transform
    context->SetTransform(currentTransform);
}
