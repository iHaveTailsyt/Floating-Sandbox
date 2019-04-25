/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2018-01-21
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "GameParameters.h"
#include "IGameEventHandler.h"
#include "MaterialDatabase.h"
#include "Physics.h"
#include "RenderContext.h"
#include "ResourceLoader.h"
#include "ShipDefinition.h"

#include <GameCore/AABB.h>
#include <GameCore/Vectors.h>

#include <cstdint>
#include <memory>
#include <set>
#include <vector>

namespace Physics
{

class World
{
public:

    World(
        std::shared_ptr<IGameEventHandler> gameEventHandler,
        GameParameters const & gameParameters,
        ResourceLoader & resourceLoader);

    float GetCurrentSimulationTime() const
    {
        return mCurrentSimulationTime;
    }

    ShipId AddShip(
        ShipDefinition const & shipDefinition,
        MaterialDatabase const & materialDatabase,
        GameParameters const & gameParameters);

    size_t GetShipCount() const;

    size_t GetShipPointCount(ShipId shipId) const;

    inline float GetOceanSurfaceHeightAt(float x) const
    {
        return mOceanSurface.GetHeightAt(x);
    }

    inline bool IsUnderwater(vec2f const & position) const
    {
        return position.y < GetOceanSurfaceHeightAt(position.x);
    }

    inline float GetOceanFloorHeightAt(float x) const
    {
        return mOceanFloor.GetHeightAt(x);
    }

    inline vec2f const & GetCurrentWindSpeed() const
    {
        return mWind.GetCurrentWindSpeed();
    }

    std::optional<ElementId> Pick(
        vec2f const & pickPosition,
        GameParameters const & gameParameters);

    void MoveBy(
        ElementId elementId,
        vec2f const & offset,
        GameParameters const & gameParameters);

    void MoveAllBy(
        ShipId shipId,
        vec2f const & offset,
        GameParameters const & gameParameters);

    void RotateBy(
        ElementId elementId,
        float angle,
        vec2f const & center,
        GameParameters const & gameParameters);

    void RotateAllBy(
        ShipId shipId,
        float angle,
        vec2f const & center,
        GameParameters const & gameParameters);

    void DestroyAt(
        vec2f const & targetPos,
        float radiusFraction,
        GameParameters const & gameParameters);

    void RepairAt(
        vec2f const & targetPos,
        float radiusMultiplier,
        GameParameters const & gameParameters);

    void SawThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        GameParameters const & gameParameters);

    void DrawTo(
        vec2f const & targetPos,
        float strengthFraction,
        GameParameters const & gameParameters);

    void SwirlAt(
        vec2f const & targetPos,
        float strengthFraction,
        GameParameters const & gameParameters);

    void TogglePinAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool InjectBubblesAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    bool FloodAt(
        vec2f const & targetPos,
        float waterQuantityMultiplier,
        GameParameters const & gameParameters);

    void ToggleAntiMatterBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleImpactBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleRCBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void ToggleTimerBombAt(
        vec2f const & targetPos,
        GameParameters const & gameParameters);

    void DetonateRCBombs();

    void DetonateAntiMatterBombs();

    void AdjustOceanSurfaceTo(
        float x,
        float y);

    bool AdjustOceanFloorTo(
        float x1,
        float targetY1,
        float x2,
        float targetY2);

    bool ScrubThrough(
        vec2f const & startPos,
        vec2f const & endPos,
        GameParameters const & gameParameters);

    std::optional<ElementId> GetNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

    void QueryNearestPointAt(
        vec2f const & targetPos,
        float radius) const;

    void Update(
        GameParameters const & gameParameters,
        Render::RenderContext const & renderContext);

    void Render(
        GameParameters const & gameParameters,
        Render::RenderContext & renderContext) const;

private:

    // The current simulation time
    float mCurrentSimulationTime;

    // Repository
    std::vector<std::unique_ptr<Ship>> mAllShips;
    Stars mStars;
    Wind mWind;
    Clouds mClouds;
    OceanSurface mOceanSurface;
    OceanFloor mOceanFloor;

    // The game event handler
    std::shared_ptr<IGameEventHandler> mGameEventHandler;
};

}
