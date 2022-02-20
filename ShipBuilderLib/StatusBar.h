/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <wx/panel.h>
#include <wx/stattext.h>

#include <optional>

namespace ShipBuilder {

class StatusBar : public wxPanel
{
public:

    StatusBar(
        wxWindow * parent,
        ResourceLocator const & resourceLocator);

    void SetCanvasSize(std::optional<ShipSpaceSize> canvasSize);
    void SetToolCoordinates(std::optional<ShipSpaceCoordinates> coordinates);
    void SetZoom(std::optional<float> zoom);
    void SetSampledMaterial(std::optional<std::string> materialName);

private:

    void RefreshCanvasSize();
    void RefreshToolCoordinates();
    void RefreshZoom();
    void RefreshSampledMaterial();

private:

    // UI
    wxStaticText * mCanvasSizeStaticText;
    wxStaticText * mToolCoordinatesStaticText;
    wxStaticText * mZoomStaticText;
    wxStaticText * mSampledMaterialNameStaticText;

    // State
    std::optional<ShipSpaceSize> mCanvasSize;
    std::optional<ShipSpaceCoordinates> mToolCoordinates;
    std::optional<float> mZoom;
    std::optional<std::string> mSampledMaterialName;
};

}