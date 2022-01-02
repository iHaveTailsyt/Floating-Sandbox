/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShaderTypes.h"
#include "ShipBuilderTypes.h"
#include "ViewModel.h"

#include <Game/Layers.h>
#include <Game/ResourceLocator.h>

#include <GameCore/Colors.h>
#include <GameCore/ImageData.h>

#include <GameOpenGL/GameOpenGL.h>
#include <GameOpenGL/ShaderManager.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace ShipBuilder {

/*
 * This class is the entry point of the entire OpenGL rendering subsystem, providing
 * the API for rendering, which is agnostic about the render platform implementation.
 *
 * All uploads are sticky, and thus need to be explicitly "undone" when they shouldn't
 * be drawn anymore.
 */
class View
{
public:

    View(
        ShipSpaceSize initialShipSpaceSize,
        DisplayLogicalSize initialDisplaySize,
        int logicalToPhysicalPixelFactor,
        std::function<void()> swapRenderBuffersFunction,
        ResourceLocator const & resourceLocator);

    int GetZoom() const
    {
        return mViewModel.GetZoom();
    }

    int SetZoom(int zoom)
    {
        auto const newZoom = mViewModel.SetZoom(zoom);

        OnViewModelUpdated();

        return newZoom;
    }

    int CalculateIdealZoom() const
    {
        return mViewModel.CalculateIdealZoom();
    }

    ShipSpaceCoordinates const & GetCameraShipSpacePosition() const
    {
        return mViewModel.GetCameraShipSpacePosition();
    }

    ShipSpaceCoordinates SetCameraShipSpacePosition(ShipSpaceCoordinates const & pos)
    {
        auto const newPos = mViewModel.SetCameraShipSpacePosition(pos);

        OnViewModelUpdated();

        return newPos;
    }

    void SetShipSize(ShipSpaceSize const & size)
    {
        mViewModel.SetShipSize(size);

        OnViewModelUpdated();
    }

    void SetDisplayLogicalSize(DisplayLogicalSize const & logicalSize)
    {
        mViewModel.SetDisplayLogicalSize(logicalSize);

        OnViewModelUpdated();
    }

    ShipSpaceSize GetCameraRange() const
    {
        return mViewModel.GetCameraRange();
    }

    ShipSpaceSize GetCameraThumbSize() const
    {
        return mViewModel.GetCameraThumbSize();
    }

    ShipSpaceSize GetDisplayShipSpaceSize() const
    {
        return mViewModel.GetDisplayShipSpaceSize();
    }

    DisplayPhysicalRect GetPhysicalVisibleShipRegion() const
    {
        return mViewModel.GetPhysicalVisibleShipRegion();
    }

    ShipSpaceCoordinates ScreenToShipSpace(DisplayLogicalCoordinates const & displayCoordinates) const
    {
        return mViewModel.ScreenToShipSpace(displayCoordinates);
    }

public:

    void EnableVisualGrid(bool doEnable);

    void SetPrimaryVisualization(VisualizationType visualization)
    {
        mPrimaryVisualization = visualization;
    }

    float GetOtherVisualizationsOpacity() const
    {
        return mOtherVisualizationsOpacity;
    }

    void SetOtherVisualizationsOpacity(float value)
    {
        mOtherVisualizationsOpacity = value;
    }

    // Sticky, always drawn
    void UploadBackgroundTexture(RgbaImageData && texture);

    //
    // Game viz (all sticky)
    //

    void UploadGameVisualization(RgbaImageData const & texture);

    void UpdateGameVisualizationTexture(
        RgbaImageData const & subTexture,
        ImageCoordinates const & origin);

    void RemoveGameVisualization();

    bool HasGameVisualization() const
    {
        return mHasGameVisualization;
    }

    //
    // Structural layer viz (all sticky)
    //

    enum class StructuralLayerVisualizationDrawMode
    {
        MeshMode,
        PixelMode
    };

    void SetStructuralLayerVisualizationDrawMode(StructuralLayerVisualizationDrawMode mode);

    void UploadStructuralLayerVisualization(RgbaImageData const & texture);

    void RemoveStructuralLayerVisualization();

    bool HasStructuralLayerVisualization() const
    {
        return mHasStructuralLayerVisualization;
    }

    //
    // Electrical layer viz (all sticky)
    //

    void UploadElectricalLayerVisualization(RgbaImageData const & texture);

    void RemoveElectricalLayerVisualization();

    bool HasElectricalLayerVisualization() const
    {
        return mHasElectricalLayerVisualization;
    }

    //
    // Ropes layer viz (all sticky)
    //

    void UploadRopesLayerVisualization(RopeBuffer const & ropeBuffer);

    void RemoveRopesLayerVisualization();

    bool HasRopesLayerVisualization() const
    {
        return mRopeCount > 0;
    }

    //
    // Texture layer viz (all sticky)
    //

    void UploadTextureLayerVisualization(RgbaImageData const & texture);

    void RemoveTextureLayerVisualization();

    bool HasTextureLayerVisualization() const
    {
        return mHasTextureLayerVisualization;
    }

    //
    // Overlays (all sticky)
    //

    enum class OverlayMode
    {
        Default,
        Error
    };

    void UploadCircleOverlay(
        ShipSpaceCoordinates const & center,
        OverlayMode mode);

    void RemoveCircleOverlay();

    void UploadRectOverlay(
        ShipSpaceRect const & rect,
        OverlayMode mode);

    void RemoveRectOverlay();

    void UploadDashedLineOverlay(
        ShipSpaceCoordinates const & start,
        ShipSpaceCoordinates const & end,
        OverlayMode mode);

    void RemoveDashedLineOverlay();

public:

    void Render();

private:

    void OnViewModelUpdated();

    void UpdateCanvas();
    void UpdateGrid();
    void UpdateStructuralLayerVisualization();
    void UpdateCircleOverlay();
    void UpdateRectOverlay();
    void UpdateDashedLineOverlay();

    void UploadTextureVertices(GameOpenGLVBO const & vbo);

    void RenderGameVisualization();
    void RenderStructuralLayerVisualization();
    void RenderElectricalLayerVisualization();
    void RenderRopesLayerVisualization();
    void RenderTextureLayerVisualization();

    vec3f GetOverlayColor(OverlayMode mode) const;

private:

    ViewModel mViewModel;
    std::unique_ptr<ShaderManager<ShaderManagerTraits>> mShaderManager;
    std::function<void()> const mSwapRenderBuffersFunction;

    //
    // Types
    //

#pragma pack(push, 1)

    struct GridVertex
    {
        vec2f positionShip; // Ship space
        vec2f positionPixel; // Pixel space
        float midXPixel; // Pixel space

        GridVertex() = default;

        GridVertex(
            vec2f const & _positionShip,
            vec2f _positionPixel,
            float _midXPixel)
            : positionShip(_positionShip)
            , positionPixel(_positionPixel)
            , midXPixel(_midXPixel)
        {}
    };

    struct CanvasVertex
    {
        vec2f positionShip; // Ship space
        vec2f positionNorm; //  0->1

        CanvasVertex() = default;

        CanvasVertex(
            vec2f _positionShip,
            vec2f _positionNorm)
            : positionShip(_positionShip)
            , positionNorm(_positionNorm)
        {}
    };

    struct TextureVertex
    {
        vec2f positionShip; // Ship space
        vec2f textureCoords; // Texture space

        TextureVertex() = default;

        TextureVertex(
            vec2f const & _positionShip,
            vec2f _textureCoords)
            : positionShip(_positionShip)
            , textureCoords(_textureCoords)
        {}
    };

    struct TextureNdcVertex
    {
        vec2f positionNdc;
        vec2f textureCoords; // Texture space

        TextureNdcVertex() = default;

        TextureNdcVertex(
            vec2f const & _positionNdc,
            vec2f _textureCoords)
            : positionNdc(_positionNdc)
            , textureCoords(_textureCoords)
        {}
    };

    struct RopeVertex
    {
        vec2f positionShip; // Ship space
        vec4f color;

        RopeVertex() = default;

        RopeVertex(
            vec2f const & _positionShip,
            vec4f _color)
            : positionShip(_positionShip)
            , color(_color)
        {}
    };

    struct CircleOverlayVertex
    {
        vec2f positionShip; // Ship space
        vec2f positionNorm; //  0->1
        vec3f overlayColor;

        CircleOverlayVertex() = default;

        CircleOverlayVertex(
            vec2f _positionShip,
            vec2f _positionNorm,
            vec3f _overlayColor)
            : positionShip(_positionShip)
            , positionNorm(_positionNorm)
            , overlayColor(_overlayColor)
        {}
    };

    struct RectOverlayVertex
    {
        vec2f positionShip; // Ship space
        vec2f positionNorm; //  0->1
        vec3f overlayColor;

        RectOverlayVertex() = default;

        RectOverlayVertex(
            vec2f _positionShip,
            vec2f _positionNorm,
            vec3f _overlayColor)
            : positionShip(_positionShip)
            , positionNorm(_positionNorm)
            , overlayColor(_overlayColor)
        {}
    };

    struct DashedLineOverlayVertex
    {
        vec2f positionShip; // Ship space
        float pixelCoord; //  PixelSpace
        vec3f overlayColor;

        DashedLineOverlayVertex() = default;

        DashedLineOverlayVertex(
            vec2f _positionShip,
            float _pixelCoord,
            vec3f _overlayColor)
            : positionShip(_positionShip)
            , pixelCoord(_pixelCoord)
            , overlayColor(_overlayColor)
        {}
    };

#pragma pack(pop)

    //
    // Rendering
    //

    // Background texture
    GameOpenGLVAO mBackgroundTextureVAO;
    GameOpenGLVBO mBackgroundTextureVBO;
    GameOpenGLTexture mBackgroundTexture;
    bool mHasBackgroundTexture;

    // Canvas
    GameOpenGLVAO mCanvasVAO;
    GameOpenGLVBO mCanvasVBO;

    // Game visualization
    GameOpenGLVAO mGameVisualizationVAO;
    GameOpenGLVBO mGameVisualizationVBO;
    GameOpenGLTexture mGameVisualizationTexture;
    bool mHasGameVisualization;

    // Structural layer visualization
    GameOpenGLVAO mStructuralLayerVisualizationVAO;
    GameOpenGLVBO mStructuralLayerVisualizationVBO;
    GameOpenGLTexture mStructuralLayerVisualizationTexture;
    bool mHasStructuralLayerVisualization;
    ProgramType mStructuralLayerVisualizationShader;

    // Electrical layer visualization
    GameOpenGLVAO mElectricalLayerVisualizationVAO;
    GameOpenGLVBO mElectricalLayerVisualizationVBO;
    GameOpenGLTexture mElectricalLayerVisualizationTexture;
    bool mHasElectricalLayerVisualization;

    // Ropes layer visualization
    GameOpenGLVAO mRopesVAO;
    GameOpenGLVBO mRopesVBO;
    size_t mRopeCount;

    // Texture layer visualization
    GameOpenGLVAO mTextureLayerVisualizationVAO;
    GameOpenGLVBO mTextureLayerVisualizationVBO;
    GameOpenGLTexture mTextureLayerVisualizationTexture;
    bool mHasTextureLayerVisualization;

    // Visualizations opacity
    float mOtherVisualizationsOpacity;

    // Grid
    GameOpenGLVAO mGridVAO;
    GameOpenGLVBO mGridVBO;
    bool mIsGridEnabled;

    // CircleOverlay
    GameOpenGLVAO mCircleOverlayVAO;
    GameOpenGLVBO mCircleOverlayVBO;
    ShipSpaceCoordinates mCircleOverlayCenter;
    vec3f mCircleOverlayColor;
    bool mHasCircleOverlay;

    // RectOverlay
    GameOpenGLVAO mRectOverlayVAO;
    GameOpenGLVBO mRectOverlayVBO;
    ShipSpaceRect mRectOverlayRect;
    vec3f mRectOverlayColor;
    bool mHasRectOverlay;

    // DashedLineOverlay
    GameOpenGLVAO mDashedLineOverlayVAO;
    GameOpenGLVBO mDashedLineOverlayVBO;
    std::vector<std::pair<ShipSpaceCoordinates, ShipSpaceCoordinates>> mDashedLineOverlaySet;
    vec3f mDashedLineOverlayColor;

    //
    // Settings from outside
    //

    VisualizationType mPrimaryVisualization;
};

}