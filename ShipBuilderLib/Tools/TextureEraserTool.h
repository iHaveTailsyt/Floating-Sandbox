/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-06-27
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Tool.h"

#include <Game/Layers.h>
#include <Game/Materials.h>
#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

class TextureEraserTool : public Tool
{
public:

    TextureEraserTool(
        Controller & controller,
        ResourceLocator const & resourceLocator);

    ~TextureEraserTool();

    void OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates) override;
    void OnLeftMouseDown() override;
    void OnLeftMouseUp() override;
    void OnRightMouseDown() override {}
    void OnRightMouseUp() override {}
    void OnShiftKeyDown() override;
    void OnShiftKeyUp() override;

private:

    void StartEngagement(ImageCoordinates const & mouseCoordinates);

    void DoEdit(ImageCoordinates const & mouseCoordinates);

    void EndEngagement();

    void DoTempVisualization(ImageRect const & affectedRect);

    void MendTempVisualization();

    std::optional<ImageRect> CalculateApplicableRect(ImageCoordinates const & coords) const;

private:

    // Original layer
    TextureLayerData mOriginalLayerClone;

    // Texture region dirtied so far with temporary visualization
    std::optional<ImageRect> mTempVisualizationDirtyTextureRegion;

    struct EngagementData
    {
        // Rectangle of edit operation
        std::optional<ImageRect> EditRegion;

        // Dirty state
        ModelDirtyState OriginalDirtyState;

        // Position of previous engagement (when this is second, third, etc.)
        std::optional<ImageCoordinates> PreviousEngagementPosition;

        // Position of SHIFT lock start (when exists)
        std::optional<ImageCoordinates> ShiftLockInitialPosition;

        // Direction of SHIFT lock (when exists)
        std::optional<bool> ShiftLockIsVertical;

        EngagementData(
            ModelDirtyState const & dirtyState,
            std::optional<ImageCoordinates> shiftLockInitialPosition)
            : EditRegion()
            , OriginalDirtyState(dirtyState)
            , PreviousEngagementPosition()
            , ShiftLockInitialPosition(shiftLockInitialPosition)
            , ShiftLockIsVertical()
        {}
    };

    // Engagement data - when set, it means we're engaged
    std::optional<EngagementData> mEngagementData;

    // Whether SHIFT is currently down or not
    bool mIsShiftDown;
};

}