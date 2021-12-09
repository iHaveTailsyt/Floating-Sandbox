/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-12-09
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <GameCore/GameTypes.h>
#include <GameCore/ImageData.h>

#include <wx/dc.h>
#include <wx/image.h>
#include <wx/panel.h>

namespace ShipBuilder {

class ShipResizeVisualizationControl : public wxPanel
{
public:

    ShipResizeVisualizationControl(
        wxWindow * parent,
        int width,
        int height,
        float initialOffsetX,
        float initialOffsetY);

    void Initialize(
        RgbaImageData const & shipVisualization,
        ShipSpaceToWorldSpaceCoordsRatio const & shipSpaceToWorldSpaceCoordsRatio,
        float offsetX,
        float offsetY);
    
    void SetOffsetX(float offsetX);
    void SetOffsetY(float offsetY);

private:

    void OnPaint(wxPaintEvent & event);
    void OnChange();

    void Render(wxDC & dc);

private:

    wxImage mShipVisualization;
    ShipSpaceToWorldSpaceCoordsRatio mShipSpaceToWorldSpaceCoordsRatio;
    float mOffsetX;
    float mOffsetY;
    
    wxBrush mSeaBrush;
    wxPen mSeaPen;
    wxPen mGuidesPen;

    // Calculated members    
    wxBitmap mResizedShipBitmap;
    wxPoint mResizedShipOrigin;
};

}