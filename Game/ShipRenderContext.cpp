/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2018-03-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ShipRenderContext.h"

#include "GameParameters.h"

#include <GameCore/GameException.h>
#include <GameCore/GameMath.h>
#include <GameCore/GameWallClock.h>
#include <GameCore/Log.h>

#include <algorithm>
#include <cstring>

namespace Render {

// Base dimensions of flame quads
static float constexpr BasisHalfFlameQuadWidth = 9.5f * 2.0f;
static float constexpr BasisFlameQuadHeight = 7.5f * 2.0f;

ShipRenderContext::ShipRenderContext(
    ShipId shipId,
    size_t pointCount,
    size_t shipCount,
    RgbaImageData shipTexture,
    ShaderManager<ShaderManagerTraits> & shaderManager,
    TextureAtlasMetadata<ExplosionTextureGroups> const & explosionTextureAtlasMetadata,
    TextureAtlasMetadata<GenericLinearTextureGroups> const & genericLinearTextureAtlasMetadata,
    TextureAtlasMetadata<GenericMipMappedTextureGroups> const & genericMipMappedTextureAtlasMetadata,
    RenderSettings const & renderSettings,
    vec4f const & lampLightColor,
    vec4f const & waterColor,
    float waterContrast,
    float waterLevelOfDetail,
    bool showStressedSprings,
    bool drawHeatOverlay,
    float heatOverlayTransparency,
    ShipFlameRenderModeType shipFlameRenderMode,
    float shipFlameSizeAdjustment)
    : mShipId(shipId)
    , mPointCount(pointCount)
    , mShipCount(shipCount)
    , mMaxMaxPlaneId(0)
    , mIsViewModelDirty(true)
    // Buffers
    , mPointAttributeGroup1Buffer()
    , mPointAttributeGroup1VBO()
    , mPointAttributeGroup2Buffer()
    , mPointAttributeGroup2VBO()
    , mPointColorVBO()
    , mPointTemperatureVBO()
    //
    , mStressedSpringElementBuffer()
    , mStressedSpringElementVBO()
    //
    , mFlameVertexBuffer()
    , mFlameVertexBufferAllocatedSize(0u)
    , mFlameBackgroundCount(0u)
    , mFlameForegroundCount(0u)
    , mFlameVertexVBO()
    , mWindSpeedMagnitudeRunningAverage(0.0f)
    , mCurrentWindSpeedMagnitudeAverage(-1.0f) // Make sure we update the param right away
    //
    , mExplosionPlaneVertexBuffers()
    , mExplosionVBO()
    , mExplosionVBOAllocatedVertexSize(0u)
    //
    , mSparkleVertexBuffer()
    , mSparkleVBO()
    , mSparkleVBOAllocatedVertexSize(0u)
    //
    , mGenericMipMappedTextureAirBubbleVertexBuffer()
    , mGenericMipMappedTexturePlaneVertexBuffers()
    , mGenericMipMappedTextureVBO()
    , mGenericMipMappedTextureVBOAllocatedVertexSize(0u)
    //
    , mHighlightVertexBuffers()
    , mHighlightVBO()
    , mHighlightVBOAllocatedVertexSize(0u)
    //
    , mVectorArrowVertexBuffer()
    , mVectorArrowVBO()
    , mVectorArrowVBOAllocatedVertexSize(0u)
    , mVectorArrowColor(0.0f, 0.0f, 0.0f, 1.0f)
    , mIsVectorArrowColorDirty(true)
    // Element (index) buffers
    , mPointElementBuffer()
    , mEphemeralPointElementBuffer()
    , mSpringElementBuffer()
    , mRopeElementBuffer()
    , mTriangleElementBuffer()
    , mAreElementBuffersDirty(true)
    , mElementVBO()
    , mElementVBOAllocatedIndexSize(0u)
    , mPointElementVBOStartIndex(0)
    , mEphemeralPointElementVBOStartIndex(0)
    , mSpringElementVBOStartIndex(0)
    , mRopeElementVBOStartIndex(0)
    , mTriangleElementVBOStartIndex(0)
    // VAOs
    , mShipVAO()
    , mFlameVAO()
    , mExplosionVAO()
    , mSparkleVAO()
    , mGenericMipMappedTextureVAO()
    , mVectorArrowVAO()
    // Textures
    , mShipTextureOpenGLHandle()
    , mStressedSpringTextureOpenGLHandle()
    , mExplosionTextureAtlasMetadata(explosionTextureAtlasMetadata)
    , mGenericLinearTextureAtlasMetadata(genericLinearTextureAtlasMetadata)
    , mGenericMipMappedTextureAtlasMetadata(genericMipMappedTextureAtlasMetadata)
    // Managers
    , mShaderManager(shaderManager)
    // Parameters
    , mLampLightColor(lampLightColor)
    , mWaterColor(waterColor)
    , mWaterContrast(waterContrast)
    , mWaterLevelOfDetail(waterLevelOfDetail)
    , mShowStressedSprings(showStressedSprings)
    , mDrawHeatOverlay(drawHeatOverlay)
    , mHeatOverlayTransparency(heatOverlayTransparency)
    , mShipFlameRenderMode(shipFlameRenderMode)
    , mShipFlameSizeAdjustment(shipFlameSizeAdjustment)
    , mHalfFlameQuadWidth(0.0f) // Will be calculated
    , mFlameQuadHeight(0.0f) // Will be calculated
{
    GLuint tmpGLuint;

    // Clear errors
    glGetError();


    //
    // Initialize buffers
    //

    GLuint vbos[11];
    glGenBuffers(11, vbos);
    CheckOpenGLError();

    mPointAttributeGroup1VBO = vbos[0];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup1VBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_STREAM_DRAW);
    mPointAttributeGroup1Buffer.reset(new vec4f[pointCount]);
    std::memset(mPointAttributeGroup1Buffer.get(), 0, pointCount * sizeof(vec4f));

    mPointAttributeGroup2VBO = vbos[1];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup2VBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_STREAM_DRAW);
    mPointAttributeGroup2Buffer.reset(new vec4f[pointCount]);
    std::memset(mPointAttributeGroup2Buffer.get(), 0, pointCount * sizeof(vec4f));

    mPointColorVBO = vbos[2];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(vec4f), nullptr, GL_STATIC_DRAW);

    mPointTemperatureVBO = vbos[3];
    glBindBuffer(GL_ARRAY_BUFFER, *mPointTemperatureVBO);
    glBufferData(GL_ARRAY_BUFFER, pointCount * sizeof(float), nullptr, GL_STREAM_DRAW);

    mStressedSpringElementVBO = vbos[4];
    mStressedSpringElementBuffer.reserve(1024); // Arbitrary

    mFlameVertexVBO = vbos[5];

    mExplosionVBO = vbos[6];

    mSparkleVBO = vbos[7];
    mSparkleVertexBuffer.reserve(256); // Arbitrary

    mGenericMipMappedTextureVBO = vbos[8];

    mHighlightVBO = vbos[9];

    mVectorArrowVBO = vbos[10];

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Initialize element (index) buffers
    //

    glGenBuffers(1, &tmpGLuint);
    mElementVBO = tmpGLuint;

    mPointElementBuffer.reserve(pointCount);
    mEphemeralPointElementBuffer.reserve(GameParameters::MaxEphemeralParticles);
    mSpringElementBuffer.reserve(pointCount * GameParameters::MaxSpringsPerPoint);
    mRopeElementBuffer.reserve(pointCount); // Arbitrary
    mTriangleElementBuffer.reserve(pointCount * GameParameters::MaxTrianglesPerPoint);


    //
    // Initialize Ship VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mShipVAO = tmpGLuint;

        glBindVertexArray(*mShipVAO);
        CheckOpenGLError();

        //
        // Describe vertex attributes
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup1VBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointAttributeGroup1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointAttributeGroup1), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup2VBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointAttributeGroup2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointAttributeGroup2), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointColor));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointColor), 4, GL_FLOAT, GL_FALSE, sizeof(vec4f), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, *mPointTemperatureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::ShipPointTemperature));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::ShipPointTemperature), 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)(0));
        CheckOpenGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //
        // Associate element VBO
        //

        // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
        // in the VAO. So we won't associate the element VBO here, but rather before the drawing call.
        ////glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mPointElementVBO);
        ////CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Flame VAO's
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mFlameVAO = tmpGLuint;

        glBindVertexArray(*mFlameVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mFlameVertexVBO);
        static_assert(sizeof(FlameVertex) == (4 + 2) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Flame1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Flame1), 4, GL_FLOAT, GL_FALSE, sizeof(FlameVertex), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Flame2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Flame2), 2, GL_FLOAT, GL_FALSE, sizeof(FlameVertex), (void*)((4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Explosion VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mExplosionVAO = tmpGLuint;

        glBindVertexArray(*mExplosionVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mExplosionVBO);
        static_assert(sizeof(ExplosionVertex) == (4 + 4 + 2) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Explosion1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Explosion1), 4, GL_FLOAT, GL_FALSE, sizeof(ExplosionVertex), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Explosion2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Explosion2), 4, GL_FLOAT, GL_FALSE, sizeof(ExplosionVertex), (void*)((4) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Explosion3));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Explosion3), 2, GL_FLOAT, GL_FALSE, sizeof(ExplosionVertex), (void*)((4 + 4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Sparkle VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mSparkleVAO = tmpGLuint;

        glBindVertexArray(*mSparkleVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mSparkleVBO);
        static_assert(sizeof(SparkleVertex) == (4 + 4) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Sparkle1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Sparkle1), 4, GL_FLOAT, GL_FALSE, sizeof(SparkleVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Sparkle2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Sparkle2), 4, GL_FLOAT, GL_FALSE, sizeof(SparkleVertex), (void *)((4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize GenericMipMappedTexture VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mGenericMipMappedTextureVAO = tmpGLuint;

        glBindVertexArray(*mGenericMipMappedTextureVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mGenericMipMappedTextureVBO);
        static_assert(sizeof(GenericTextureVertex) == (4 + 4 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture1), 4, GL_FLOAT, GL_FALSE, sizeof(GenericTextureVertex), (void*)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture2), 4, GL_FLOAT, GL_FALSE, sizeof(GenericTextureVertex), (void*)((4) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture3));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::GenericMipMappedTexture3), 3, GL_FLOAT, GL_FALSE, sizeof(GenericTextureVertex), (void*)((4 + 4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Highlight VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mHighlightVAO = tmpGLuint;

        glBindVertexArray(*mHighlightVAO);

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mHighlightVBO);
        static_assert(sizeof(HighlightVertex) == (2 + 2 + 3 + 1 + 1) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Highlight1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Highlight1), 4, GL_FLOAT, GL_FALSE, sizeof(HighlightVertex), (void *)((0) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Highlight2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Highlight2), 4, GL_FLOAT, GL_FALSE, sizeof(HighlightVertex), (void *)((4) * sizeof(float)));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Highlight3));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Highlight3), 1, GL_FLOAT, GL_FALSE, sizeof(HighlightVertex), (void *)((4 + 4) * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize VectorArrow VAO
    //

    {
        glGenVertexArrays(1, &tmpGLuint);
        mVectorArrowVAO = tmpGLuint;

        glBindVertexArray(*mVectorArrowVAO);
        CheckOpenGLError();

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::VectorArrow));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::VectorArrow), 3, GL_FLOAT, GL_FALSE, sizeof(vec3f), (void*)(0));
        CheckOpenGLError();

        glBindVertexArray(0);
    }


    //
    // Initialize Ship texture
    //

    glGenTextures(1, &tmpGLuint);
    mShipTextureOpenGLHandle = tmpGLuint;

    // Bind texture
    mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
    glBindTexture(GL_TEXTURE_2D, *mShipTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadMipmappedTexture(std::move(shipTexture));

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Set texture parameter
    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetTextureParameters<ProgramType::ShipSpringsTexture>();
    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetTextureParameters<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetTextureParameters<ProgramType::ShipTrianglesTexture>();
    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetTextureParameters<ProgramType::ShipTrianglesTextureWithTemperature>();

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Initialize StressedSpring texture
    //

    glGenTextures(1, &tmpGLuint);
    mStressedSpringTextureOpenGLHandle = tmpGLuint;

    // Bind texture
    mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
    glBindTexture(GL_TEXTURE_2D, *mStressedSpringTextureOpenGLHandle);
    CheckOpenGLError();

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CheckOpenGLError();

    // Set filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CheckOpenGLError();

    // Make texture data
    unsigned char buf[] = {
        239, 16, 39, 255,       255, 253, 181,  255,    239, 16, 39, 255,
        255, 253, 181, 255,     239, 16, 39, 255,       255, 253, 181,  255,
        239, 16, 39, 255,       255, 253, 181,  255,    239, 16, 39, 255
    };

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 3, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
    CheckOpenGLError();

    // Unbind texture
    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Update parameters
    //

    ProcessSettingChanges(renderSettings);

    // TODOOLD
    OnLampLightColorUpdated();
    OnWaterColorUpdated();
    OnWaterContrastUpdated();
    OnWaterLevelOfDetailUpdated();
    OnHeatOverlayTransparencyUpdated();
    OnShipFlameSizeAdjustmentUpdated();
}

ShipRenderContext::~ShipRenderContext()
{
}

//////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::UploadStart(PlaneId maxMaxPlaneId)
{
    /* TODOTEST
    //
    // Reset flames, explosions, sparkles, air bubbles, generic textures, highlights,
    // vector arrows;
    // they are all uploaded as needed
    //

    mFlameVertexBuffer.reset();
    mFlameBackgroundCount = 0u;
    mFlameForegroundCount = 0u;
    */

    {
        size_t const newSize = static_cast<size_t>(maxMaxPlaneId) + 1u;
        assert(mExplosionPlaneVertexBuffers.size() <= newSize);

        size_t const clearCount = mExplosionPlaneVertexBuffers.size();
        for (size_t i = 0; i < clearCount; ++i)
        {
            mExplosionPlaneVertexBuffers[i].vertexBuffer.clear();
        }

        if (newSize != mExplosionPlaneVertexBuffers.size())
            mExplosionPlaneVertexBuffers.resize(newSize);
    }


    mSparkleVertexBuffer.clear();

    {
        mGenericMipMappedTextureAirBubbleVertexBuffer.clear();

        size_t const newSize = static_cast<size_t>(maxMaxPlaneId) + 1u;
        assert(mGenericMipMappedTexturePlaneVertexBuffers.size() <= newSize);

        size_t const clearCount = mGenericMipMappedTexturePlaneVertexBuffers.size();
        for (size_t i = 0; i < clearCount; ++i)
        {
            mGenericMipMappedTexturePlaneVertexBuffers[i].vertexBuffer.clear();
        }

        if (newSize != mGenericMipMappedTexturePlaneVertexBuffers.size())
            mGenericMipMappedTexturePlaneVertexBuffers.resize(newSize);
    }

    for (size_t i = 0; i <= static_cast<size_t>(HighlightModeType::_Last); ++i)
    {
        mHighlightVertexBuffers[i].clear();
    }

    mVectorArrowVertexBuffer.clear();

    //
    // Check if the max max plane ID has changed
    //

    if (maxMaxPlaneId != mMaxMaxPlaneId)
    {
        // Update value
        mMaxMaxPlaneId = maxMaxPlaneId;
        mIsViewModelDirty = true;
    }
}

void ShipRenderContext::UploadPointImmutableAttributes(vec2f const * textureCoordinates)
{
    // Uploaded only once, but we treat them as if they could
    // be uploaded any time

    // Interleave texture coordinates into AttributeGroup1 buffer
    vec4f * restrict pDst = mPointAttributeGroup1Buffer.get();
    vec2f const * restrict pSrc = textureCoordinates;
    for (size_t i = 0; i < mPointCount; ++i)
    {
        pDst[i].z = pSrc[i].x;
        pDst[i].w = pSrc[i].y;
    }
}

void ShipRenderContext::UploadPointMutableAttributesStart()
{
    // Nop
}

void ShipRenderContext::UploadPointMutableAttributes(
    vec2f const * position,
    float const * light,
    float const * water,
    size_t lightAndWaterCount)
{
    // Uploaded at each cycle

    // Interleave positions into AttributeGroup1 buffer
    vec4f * restrict pDst1 = mPointAttributeGroup1Buffer.get();
    vec2f const * restrict pSrc = position;
    for (size_t i = 0; i < mPointCount; ++i)
    {
        pDst1[i].x = pSrc[i].x;
        pDst1[i].y = pSrc[i].y;
    }

    // Interleave light and water into AttributeGroup2 buffer
    vec4f * restrict pDst2 = mPointAttributeGroup2Buffer.get();
    float const * restrict pSrc1 = light;
    float const * restrict pSrc2 = water;
    for (size_t i = 0; i < lightAndWaterCount; ++i)
    {
        pDst2[i].x = pSrc1[i];
        pDst2[i].y = pSrc2[i];
    }
}

void ShipRenderContext::UploadPointMutableAttributesPlaneId(
    float const * planeId,
    size_t startDst,
    size_t count)
{
    // Uploaded sparingly, but we treat them as if they could
    // be uploaded at any time

    // Interleave plane ID into AttributeGroup2 buffer
    assert(startDst + count <= mPointCount);
    vec4f * restrict pDst = &(mPointAttributeGroup2Buffer.get()[startDst]);
    float const * restrict pSrc = planeId;
    for (size_t i = 0; i < count; ++i)
        pDst[i].z = pSrc[i];
}

void ShipRenderContext::UploadPointMutableAttributesDecay(
    float const * decay,
    size_t startDst,
    size_t count)
{
    // Uploaded sparingly, but we treat them as if they could
    // be uploaded at any time

    // Interleave decay into AttributeGroup2 buffer
    assert(startDst + count <= mPointCount);
    vec4f * restrict pDst = &(mPointAttributeGroup2Buffer.get()[startDst]);
    float const * restrict pSrc = decay;
    for (size_t i = 0; i < count; ++i)
        pDst[i].w = pSrc[i];
}

void ShipRenderContext::UploadPointMutableAttributesEnd()
{
    // Nop
}

void ShipRenderContext::UploadPointColors(
    vec4f const * color,
    size_t startDst,
    size_t count)
{
    // Uploaded sparingly

    // We've been invoked on the render thread

    //
    // Upload color range
    //

    assert(startDst + count <= mPointCount);

    glBindBuffer(GL_ARRAY_BUFFER, *mPointColorVBO);

    glBufferSubData(GL_ARRAY_BUFFER, startDst * sizeof(vec4f), count * sizeof(vec4f), color);
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadPointTemperature(
    float const * temperature,
    size_t startDst,
    size_t count)
{
    // Uploaded sparingly

    // We've been invoked on the render thread

    //
    // Upload temperature range
    //

    assert(startDst + count <= mPointCount);

    glBindBuffer(GL_ARRAY_BUFFER, *mPointTemperatureVBO);

    glBufferSubData(GL_ARRAY_BUFFER, startDst * sizeof(float), count * sizeof(float), temperature);
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadElementsStart()
{
    // Elements are uploaded sparingly

    // Empty all buffers - except triangles - as elements will be completely re-populated soon
    // (with a yet-unknown quantity of elements);
    //
    // If the client does not upload new triangles, it means we have to reuse the last known set

    mPointElementBuffer.clear();
    mSpringElementBuffer.clear();
    mRopeElementBuffer.clear();
    mStressedSpringElementBuffer.clear();
    mAreElementBuffersDirty = true;
}

void ShipRenderContext::UploadElementTrianglesStart(size_t trianglesCount)
{
    // Client wants to upload a new set of triangles
    //
    // No need to clear, we'll repopulate everything

    mTriangleElementBuffer.resize(trianglesCount);
}

void ShipRenderContext::UploadElementTrianglesEnd()
{
    // Nop
}

void ShipRenderContext::UploadElementsEnd()
{
    // Nop
}

void ShipRenderContext::UploadElementStressedSpringsStart()
{
    // Empty buffer
    mStressedSpringElementBuffer.clear();
}

void ShipRenderContext::UploadElementStressedSpringsEnd()
{
    //
    // Upload stressed spring elements
    //

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mStressedSpringElementVBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        mStressedSpringElementBuffer.size() * sizeof(LineElement),
        mStressedSpringElementBuffer.data(),
        GL_STREAM_DRAW);
    CheckOpenGLError();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadFlamesStart(
    size_t count,
    float windSpeedMagnitude)
{
    //
    // Prepare buffer - map flame VBO's
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mFlameVertexVBO);

    if (count > mFlameVertexBufferAllocatedSize
        || count < mFlameVertexBufferAllocatedSize - 100)
    {
        // Reallocate
        mFlameVertexBufferAllocatedSize = ((count / 100) + 1) * 100;
        glBufferData(GL_ARRAY_BUFFER, mFlameVertexBufferAllocatedSize * 6 * sizeof(FlameVertex), nullptr, GL_STREAM_DRAW);

    }

    // Map buffer
    mFlameVertexBuffer.map_and_fill(count * 6);
    CheckOpenGLError();


    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Update wind speed
    //

    float newWind = mWindSpeedMagnitudeRunningAverage.Update(windSpeedMagnitude);

    // Set wind speed magnitude parameter, if it has changed
    if (newWind != mCurrentWindSpeedMagnitudeAverage)
    {
        // Calculate wind angle: we do this here once instead of doing it for each and every pixel
        float const windRotationAngle = std::copysign(
            0.6f * SmoothStep(0.0f, 100.0f, std::abs(newWind)),
            -newWind);

        switch (mShipFlameRenderMode)
        {
            case ShipFlameRenderModeType::Mode1:
            {
                mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground1>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground1, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground1>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground1, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                break;
            }

            case ShipFlameRenderModeType::Mode2:
            {
                mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground2>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground2, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground2>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground2, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                break;
            }

            case ShipFlameRenderModeType::Mode3:
            {
                mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground3>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground3, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground3>();
                mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground3, ProgramParameterType::FlameWindRotationAngle>(
                    windRotationAngle);

                break;
            }

            case ShipFlameRenderModeType::NoDraw:
            {
                break;
            }
        }

        mCurrentWindSpeedMagnitudeAverage = newWind;
    }
}

void ShipRenderContext::UploadFlamesEnd()
{
    assert((mFlameBackgroundCount + mFlameForegroundCount) * 6u == mFlameVertexBuffer.size());

    // Unmap flame VBO's
    glBindBuffer(GL_ARRAY_BUFFER, *mFlameVertexVBO);
    mFlameVertexBuffer.unmap();
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ShipRenderContext::UploadElementEphemeralPointsStart()
{
    // Client wants to upload a new set of ephemeral point elements

    // Empty buffer
    mEphemeralPointElementBuffer.clear();
}

void ShipRenderContext::UploadElementEphemeralPointsEnd()
{
    // Nop
}

void ShipRenderContext::UploadVectors(
    size_t count,
    vec2f const * position,
    float const * planeId,
    vec2f const * vector,
    float lengthAdjustment,
    vec4f const & color)
{
    static float const CosAlphaLeftRight = cos(-2.f * Pi<float> / 8.f);
    static float const SinAlphaLeft = sin(-2.f * Pi<float> / 8.f);
    static float const SinAlphaRight = -SinAlphaLeft;

    static vec2f const XMatrixLeft = vec2f(CosAlphaLeftRight, SinAlphaLeft);
    static vec2f const YMatrixLeft = vec2f(-SinAlphaLeft, CosAlphaLeftRight);
    static vec2f const XMatrixRight = vec2f(CosAlphaLeftRight, SinAlphaRight);
    static vec2f const YMatrixRight = vec2f(-SinAlphaRight, CosAlphaLeftRight);

    //
    // Create buffer with endpoint positions of each segment of each arrow
    //

    mVectorArrowVertexBuffer.reserve(count * 3 * 2);

    for (size_t i = 0; i < count; ++i)
    {
        // Stem
        vec2f stemEndpoint = position[i] + vector[i] * lengthAdjustment;
        mVectorArrowVertexBuffer.emplace_back(position[i], planeId[i]);
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint, planeId[i]);

        // Left
        vec2f leftDir = vec2f(-vector[i].dot(XMatrixLeft), -vector[i].dot(YMatrixLeft)).normalise();
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint, planeId[i]);
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint + leftDir * 0.2f, planeId[i]);

        // Right
        vec2f rightDir = vec2f(-vector[i].dot(XMatrixRight), -vector[i].dot(YMatrixRight)).normalise();
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint, planeId[i]);
        mVectorArrowVertexBuffer.emplace_back(stemEndpoint + rightDir * 0.2f, planeId[i]);
    }

    if (color != mVectorArrowColor)
    {
        mVectorArrowColor = color;

        mIsVectorArrowColorDirty = true;
    }
}

void ShipRenderContext::UploadEnd()
{
    // Nop
}

void ShipRenderContext::Draw(
    RenderSettings const & renderSettings,
    RenderStatistics & renderStats)
{
    // We've been invoked on the render thread

    //
    // Process changes to settings
    //

    ProcessSettingChanges(renderSettings);


    //
    // Upload Point AttributeGroup1 buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup1VBO);

    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec4f), mPointAttributeGroup1Buffer.get());
    CheckOpenGLError();


    //
    // Upload Point AttributeGroup2 buffer
    //
    glBindBuffer(GL_ARRAY_BUFFER, *mPointAttributeGroup2VBO);

    glBufferSubData(GL_ARRAY_BUFFER, 0, mPointCount * sizeof(vec4f), mPointAttributeGroup2Buffer.get());
    CheckOpenGLError();

    glBindBuffer(GL_ARRAY_BUFFER, 0);


    //
    // Upload element buffers, if needed
    //

    if (mAreElementBuffersDirty)
    {
        //
        // Upload all elements to the VBO, remembering the starting VBO index
        // of each element type which we'll need at primitives' render time
        //

        // Note: byte-granularity indices
        mTriangleElementVBOStartIndex = 0;
        mRopeElementVBOStartIndex = mTriangleElementVBOStartIndex + mTriangleElementBuffer.size() * sizeof(TriangleElement);
        mSpringElementVBOStartIndex = mRopeElementVBOStartIndex + mRopeElementBuffer.size() * sizeof(LineElement);
        mPointElementVBOStartIndex = mSpringElementVBOStartIndex + mSpringElementBuffer.size() * sizeof(LineElement);
        mEphemeralPointElementVBOStartIndex = mPointElementVBOStartIndex + mPointElementBuffer.size() * sizeof(PointElement);
        size_t requiredIndexSize = mEphemeralPointElementVBOStartIndex + mEphemeralPointElementBuffer.size() * sizeof(PointElement);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);

        if (mElementVBOAllocatedIndexSize != requiredIndexSize)
        {
            // Re-allocate VBO buffer
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, requiredIndexSize, nullptr, GL_STATIC_DRAW);
            CheckOpenGLError();

            mElementVBOAllocatedIndexSize = requiredIndexSize;
        }

        // Upload triangles
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            mTriangleElementVBOStartIndex,
            mTriangleElementBuffer.size() * sizeof(TriangleElement),
            mTriangleElementBuffer.data());

        // Upload ropes
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            mRopeElementVBOStartIndex,
            mRopeElementBuffer.size() * sizeof(LineElement),
            mRopeElementBuffer.data());

        // Upload springs
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            mSpringElementVBOStartIndex,
            mSpringElementBuffer.size() * sizeof(LineElement),
            mSpringElementBuffer.data());

        // Upload points
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            mPointElementVBOStartIndex,
            mPointElementBuffer.size() * sizeof(PointElement),
            mPointElementBuffer.data());

        // Upload ephemeral points
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            mEphemeralPointElementVBOStartIndex,
            mEphemeralPointElementBuffer.size() * sizeof(PointElement),
            mEphemeralPointElementBuffer.data());

        CheckOpenGLError();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        mAreElementBuffersDirty = false;
    }

    /* TODOTEST
    //
    // Render background flames
    //

    if (mShipFlameRenderMode == ShipFlameRenderMode::Mode1)
    {
        RenderFlames<ProgramType::ShipFlamesBackground1>(
            0,
            mFlameBackgroundCount);
    }
    else if (mShipFlameRenderMode == ShipFlameRenderMode::Mode2)
    {
        RenderFlames<ProgramType::ShipFlamesBackground2>(
            0,
            mFlameBackgroundCount);
    }
    else if (mShipFlameRenderMode == ShipFlameRenderMode::Mode3)
    {
        RenderFlames<ProgramType::ShipFlamesBackground3>(
            0,
            mFlameBackgroundCount);
    }
    */

    //
    // Draw ship elements
    //

    glBindVertexArray(*mShipVAO);

    {
        //
        // Bind element VBO
        //
        // NOTE: Intel drivers have a bug in the VAO ARB: they do not store the ELEMENT_ARRAY_BUFFER binding
        // in the VAO
        //

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);


        //
        // Bind ship texture
        //

        assert(!!mShipTextureOpenGLHandle);

        mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
        glBindTexture(GL_TEXTURE_2D, *mShipTextureOpenGLHandle);



        //
        // Draw triangles
        //
        // Best to draw triangles (temporally) before springs and ropes, otherwise
        // the latter, which use anti-aliasing, would end up being contoured with background
        // when drawn Z-ally over triangles
        //
        // Also, edge springs might just contain transparent pixels (when textured), which
        // would result in the same artifact
        //

        if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe
            || renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Decay
            || renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Structure
            || renderSettings.DebugShipRenderMode == DebugShipRenderModeType::None)
        {
            if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Decay)
            {
                // Use decay program
                mShaderManager.ActivateProgram<ProgramType::ShipTrianglesDecay>();
            }
            else
            {
                if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::None)
                {
                    // Use texture program
                    if (mDrawHeatOverlay)
                        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
                    else
                        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
                }
                else
                {
                    // Use color program
                    if (mDrawHeatOverlay)
                        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
                    else
                        mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
                }
            }

            if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
                glLineWidth(0.1f);

            // Draw!
            glDrawElements(
                GL_TRIANGLES,
                static_cast<GLsizei>(3 * mTriangleElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)mTriangleElementVBOStartIndex);

            // Update stats
            renderStats.LastRenderedShipTriangles += mTriangleElementBuffer.size();
        }



        //
        // Set line width, for ropes and springs
        //

        glLineWidth(0.1f * 2.0f * renderSettings.View.GetCanvasToVisibleWorldHeightRatio());



        //
        // Draw ropes, unless it's a debug mode that doesn't want them
        //
        // Note: when DebugRenderMode is springs|edgeSprings, ropes would all be uploaded
        // as springs.
        //

        if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Structure
            || renderSettings.DebugShipRenderMode == DebugShipRenderModeType::None)
        {
            if (mDrawHeatOverlay)
                mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
            else
                mShaderManager.ActivateProgram<ProgramType::ShipRopes>();

            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mRopeElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)mRopeElementVBOStartIndex);

            // Update stats
            renderStats.LastRenderedShipRopes += mRopeElementBuffer.size();
        }



        //
        // Draw springs
        //
        // We draw springs when:
        // - DebugRenderMode is springs|edgeSprings, in which case we use colors - so to show
        //   structural springs -, or
        // - DebugRenderMode is structure, in which case we use colors - so to draw 1D chains -, or
        // - DebugRenderMode is none, in which case we use texture - so to draw 1D chains
        //
        // Note: when DebugRenderMode is springs|edgeSprings, ropes would all be here.
        //

        if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Springs
            || renderSettings.DebugShipRenderMode == DebugShipRenderModeType::EdgeSprings
            || renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Structure
            || renderSettings.DebugShipRenderMode == DebugShipRenderModeType::None)
        {
            if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::None)
            {
                // Use texture program
                if (mDrawHeatOverlay)
                    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
                else
                    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
            }
            else
            {
                // Use color program
                if (mDrawHeatOverlay)
                    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
                else
                    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
            }

            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mSpringElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)mSpringElementVBOStartIndex);

            // Update stats
            renderStats.LastRenderedShipSprings += mSpringElementBuffer.size();
        }



        //
        // Draw stressed springs
        //

        if (mShowStressedSprings
            && !mStressedSpringElementBuffer.empty())
        {
            mShaderManager.ActivateProgram<ProgramType::ShipStressedSprings>();

            // Bind stressed spring texture
            mShaderManager.ActivateTexture<ProgramParameterType::SharedTexture>();
            glBindTexture(GL_TEXTURE_2D, *mStressedSpringTextureOpenGLHandle);
            CheckOpenGLError();

            // Bind stressed spring VBO
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mStressedSpringElementVBO);

            // Draw
            glDrawElements(
                GL_LINES,
                static_cast<GLsizei>(2 * mStressedSpringElementBuffer.size()),
                GL_UNSIGNED_INT,
                (GLvoid *)0);

            // Bind again element VBO
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *mElementVBO);
        }



        //
        // Draw points (orphaned/all non-ephemerals, and ephemerals)
        //

        if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Points
            || renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Structure
            || renderSettings.DebugShipRenderMode == DebugShipRenderModeType::None)
        {
            auto const totalPoints = mPointElementBuffer.size() + mEphemeralPointElementBuffer.size();

            if (mDrawHeatOverlay)
                mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
            else
                mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();

            glPointSize(0.3f * renderSettings.View.GetCanvasToVisibleWorldHeightRatio());

            glDrawElements(
                GL_POINTS,
                static_cast<GLsizei>(1 * totalPoints),
                GL_UNSIGNED_INT,
                (GLvoid *)mPointElementVBOStartIndex);

            // Update stats
            renderStats.LastRenderedShipPoints += totalPoints;
        }

        // We are done with the ship VAO
        glBindVertexArray(0);
    }


    /* TODOTEST
    //
    // Render foreground flames
    //

    if (mShipFlameRenderMode == ShipFlameRenderMode::Mode1)
    {
        RenderFlames<ProgramType::ShipFlamesForeground1>(
            mFlameBackgroundCount,
            mFlameForegroundCount);
    }
    else if (mShipFlameRenderMode == ShipFlameRenderMode::Mode2)
    {
        RenderFlames<ProgramType::ShipFlamesForeground2>(
            mFlameBackgroundCount,
            mFlameForegroundCount);
    }
    else if (mShipFlameRenderMode == ShipFlameRenderMode::Mode3)
    {
        RenderFlames<ProgramType::ShipFlamesForeground3>(
            mFlameBackgroundCount,
            mFlameForegroundCount);
    }
    */


    //
    // Render sparkles
    //

    RenderSparkles(renderSettings);


    //
    // Render generic textures
    //

    RenderGenericMipMappedTextures(renderSettings, renderStats);


    //
    // Render explosions
    //

    RenderExplosions(renderSettings);


    //
    // Render highlights
    //

    RenderHighlights(renderSettings);


    //
    // Render vectors
    //

    RenderVectorArrows(renderSettings);


    //
    // Update stats
    //

    renderStats.LastRenderedShipPlanes += mMaxMaxPlaneId + 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////

template<ProgramType ShaderProgram>
void ShipRenderContext::RenderFlames(
    size_t startFlameIndex,
    size_t flameCount,
    RenderSettings const & renderSettings,
    RenderStatistics & renderStats)
{
    if (flameCount > 0
        && mShipFlameRenderMode != ShipFlameRenderModeType::NoDraw)
    {
        glBindVertexArray(*mFlameVAO);

        mShaderManager.ActivateProgram<ShaderProgram>();

        // Set flame speed parameter
        mShaderManager.SetProgramParameter<ShaderProgram, ProgramParameterType::FlameSpeed>(
            GameWallClock::GetInstance().NowAsFloat() * 0.345f);

        // Bind VBO
        glBindBuffer(GL_ARRAY_BUFFER, *mFlameVertexVBO);

        // Render
        if (mShipFlameRenderMode == ShipFlameRenderModeType::Mode1)
        {
            glDrawArrays(
                GL_TRIANGLES,
                static_cast<GLint>(startFlameIndex * 6u),
                static_cast<GLint>(flameCount * 6u));
        }
        else
        {
            glDrawArraysInstanced(
                GL_TRIANGLES,
                static_cast<GLint>(startFlameIndex * 6u),
                static_cast<GLint>(flameCount * 6u),
                2); // Without border, with border
        }

        glBindVertexArray(0);

        // Update stats
        renderStats.LastRenderedShipFlames += flameCount; // # of quads
    }
}

void ShipRenderContext::RenderSparkles(RenderSettings const & renderSettings)
{
    if (mSparkleVertexBuffer.size() > 0)
    {
        //
        // Upload buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mSparkleVBO);

        if (mSparkleVBOAllocatedVertexSize != mSparkleVertexBuffer.size())
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mSparkleVertexBuffer.size() * sizeof(SparkleVertex), mSparkleVertexBuffer.data(), GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            mSparkleVBOAllocatedVertexSize = mSparkleVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mSparkleVertexBuffer.size() * sizeof(SparkleVertex), mSparkleVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //
        // Render
        //

        glBindVertexArray(*mSparkleVAO);

        mShaderManager.ActivateProgram<ProgramType::ShipSparkles>();

        if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        assert(0 == (mSparkleVertexBuffer.size() % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mSparkleVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderGenericMipMappedTextures(
    RenderSettings const & renderSettings,
    RenderStatistics & renderStats)
{
    size_t const nonAirBubblesTotalVertexCount = std::accumulate(
        mGenericMipMappedTexturePlaneVertexBuffers.cbegin(),
        mGenericMipMappedTexturePlaneVertexBuffers.cend(),
        size_t(0),
        [](size_t const total, auto const & vec)
        {
            return total + vec.vertexBuffer.size();
        });

    size_t const totalVertexCount = mGenericMipMappedTextureAirBubbleVertexBuffer.size() + nonAirBubblesTotalVertexCount;

    if (totalVertexCount > 0)
    {
        //
        // Buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mGenericMipMappedTextureVBO);

        if (totalVertexCount > mGenericMipMappedTextureVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer
            glBufferData(GL_ARRAY_BUFFER, totalVertexCount * sizeof(GenericTextureVertex), nullptr, GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            mGenericMipMappedTextureVBOAllocatedVertexSize = totalVertexCount;
        }

        // Map vertex buffer
        auto mappedBuffer = reinterpret_cast<uint8_t *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
        CheckOpenGLError();

        // Upload air bubbles
        if (!mGenericMipMappedTextureAirBubbleVertexBuffer.empty())
        {
            size_t const byteCopySize = mGenericMipMappedTextureAirBubbleVertexBuffer.size() * sizeof(GenericTextureVertex);
            std::memcpy(mappedBuffer, mGenericMipMappedTextureAirBubbleVertexBuffer.data(), byteCopySize);
            mappedBuffer += byteCopySize;
        }

        // Upload all planes of other textures
        for (auto const & plane : mGenericMipMappedTexturePlaneVertexBuffers)
        {
            if (!plane.vertexBuffer.empty())
            {
                size_t const byteCopySize = plane.vertexBuffer.size() * sizeof(GenericTextureVertex);
                std::memcpy(mappedBuffer, plane.vertexBuffer.data(), byteCopySize);
                mappedBuffer += byteCopySize;
            }
        }

        // Unmap vertex buffer
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //
        // Render
        //

        glBindVertexArray(*mGenericMipMappedTextureVAO);

        mShaderManager.ActivateProgram<ProgramType::ShipGenericMipMappedTextures>();

        if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        assert(0 == (totalVertexCount % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(totalVertexCount));

        glBindVertexArray(0);

        //
        // Update stats
        //

        renderStats.LastRenderedShipGenericMipMappedTextures += totalVertexCount / 6; // # of quads
    }
}

void ShipRenderContext::RenderExplosions(RenderSettings const & renderSettings)
{
    size_t const totalVertexCount = std::accumulate(
        mExplosionPlaneVertexBuffers.cbegin(),
        mExplosionPlaneVertexBuffers.cend(),
        size_t(0),
        [](size_t const total, auto const & vec)
        {
            return total + vec.vertexBuffer.size();
        });

    if (totalVertexCount > 0)
    {
        //
        // Buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mExplosionVBO);

        if (totalVertexCount != mExplosionVBOAllocatedVertexSize)
        {
            // Re-allocate VBO buffer
            glBufferData(GL_ARRAY_BUFFER, totalVertexCount * sizeof(ExplosionVertex), nullptr, GL_STREAM_DRAW);
            CheckOpenGLError();

            mExplosionVBOAllocatedVertexSize = totalVertexCount;
        }

        // Map vertex buffer
        auto mappedBuffer = reinterpret_cast<uint8_t *>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
        CheckOpenGLError();

        // Upload all planes
        for (auto const & plane : mExplosionPlaneVertexBuffers)
        {
            if (!plane.vertexBuffer.empty())
            {
                size_t const byteCopySize = plane.vertexBuffer.size() * sizeof(ExplosionVertex);
                std::memcpy(mappedBuffer, plane.vertexBuffer.data(), byteCopySize);
                mappedBuffer += byteCopySize;
            }
        }

        // Unmap vertex buffer
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //
        // Render
        //

        glBindVertexArray(*mExplosionVAO);

        mShaderManager.ActivateProgram<ProgramType::ShipExplosions>();

        if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
            glLineWidth(0.1f);

        assert(0 == (totalVertexCount % 6));
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(totalVertexCount));

        glBindVertexArray(0);
    }
}

void ShipRenderContext::RenderHighlights(RenderSettings const & renderSettings)
{
    for (size_t i = 0; i <= static_cast<size_t>(HighlightModeType::_Last); ++i)
    {
        if (!mHighlightVertexBuffers[i].empty())
        {
            //
            // Buffer
            //

            glBindBuffer(GL_ARRAY_BUFFER, *mHighlightVBO);

            if (mHighlightVBOAllocatedVertexSize != mHighlightVertexBuffers[i].size())
            {
                // Re-allocate VBO buffer and upload
                glBufferData(GL_ARRAY_BUFFER, mHighlightVertexBuffers[i].size() * sizeof(HighlightVertex), mHighlightVertexBuffers[i].data(), GL_DYNAMIC_DRAW);
                CheckOpenGLError();

                mHighlightVBOAllocatedVertexSize = mVectorArrowVertexBuffer.size();
            }
            else
            {
                // No size change, just upload VBO buffer
                glBufferSubData(GL_ARRAY_BUFFER, 0, mHighlightVertexBuffers[i].size() * sizeof(HighlightVertex), mHighlightVertexBuffers[i].data());
                CheckOpenGLError();
            }

            glBindBuffer(GL_ARRAY_BUFFER, 0);

            //
            // Render
            //

            glBindVertexArray(*mHighlightVAO);

            switch (static_cast<HighlightModeType>(i))
            {
                case HighlightModeType::Circle:
                {
                    mShaderManager.ActivateProgram<ProgramType::ShipCircleHighlights>();
                    break;
                }

                case HighlightModeType::ElectricalElement:
                {
                    mShaderManager.ActivateProgram<ProgramType::ShipElectricalElementHighlights>();
                    break;
                }

                default:
                {
                    assert(false);
                    break;
                }
            }

            if (renderSettings.DebugShipRenderMode == DebugShipRenderModeType::Wireframe)
                glLineWidth(0.1f);

            assert(0 == (mHighlightVertexBuffers[i].size() % 6));
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mHighlightVertexBuffers[i].size()));

            glBindVertexArray(0);
        }
    }
}

void ShipRenderContext::RenderVectorArrows(RenderSettings const & /*renderSettings*/)
{
    if (!mVectorArrowVertexBuffer.empty())
    {
        if (mIsVectorArrowColorDirty)
        {
            mShaderManager.ActivateProgram<ProgramType::ShipVectors>();

            mShaderManager.SetProgramParameter<ProgramType::ShipVectors, ProgramParameterType::MatteColor>(
                mVectorArrowColor.x,
                mVectorArrowColor.y,
                mVectorArrowColor.z,
                mVectorArrowColor.w);

            mIsVectorArrowColorDirty = false;
        }

        //
        // Buffer
        //

        glBindBuffer(GL_ARRAY_BUFFER, *mVectorArrowVBO);

        if (mVectorArrowVBOAllocatedVertexSize != mVectorArrowVertexBuffer.size())
        {
            // Re-allocate VBO buffer and upload
            glBufferData(GL_ARRAY_BUFFER, mVectorArrowVertexBuffer.size() * sizeof(vec3f), mVectorArrowVertexBuffer.data(), GL_DYNAMIC_DRAW);
            CheckOpenGLError();

            mVectorArrowVBOAllocatedVertexSize = mVectorArrowVertexBuffer.size();
        }
        else
        {
            // No size change, just upload VBO buffer
            glBufferSubData(GL_ARRAY_BUFFER, 0, mVectorArrowVertexBuffer.size() * sizeof(vec3f), mVectorArrowVertexBuffer.data());
            CheckOpenGLError();
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //
        // Render
        //

        glBindVertexArray(*mVectorArrowVAO);

        mShaderManager.ActivateProgram<ProgramType::ShipVectors>();

        glLineWidth(0.5f);

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mVectorArrowVertexBuffer.size()));

        glBindVertexArray(0);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////

void ShipRenderContext::ProcessSettingChanges(RenderSettings const & renderSettings)
{
    if (renderSettings.IsViewDirty || mIsViewModelDirty)
    {
        ApplyViewModelChanges(renderSettings);
        mIsViewModelDirty = false;
    }

    if (renderSettings.IsEffectiveAmbientLightIntensityDirty)
    {
        ApplyEffectiveAmbientLightIntensityChanges(renderSettings);
    }
}

void ShipRenderContext::ApplyViewModelChanges(RenderSettings const & renderSettings)
{
    //
    // Each plane Z segment is divided into a number of layers, one for each type of rendering we do for a ship:
    //      - 0: Ropes (always behind)
    //      - 1: Flames - background
    //      - 2: Springs
    //      - 3: Triangles
    //          - Triangles are always drawn temporally before ropes and springs though, to avoid anti-aliasing issues
    //      - 4: Stressed springs
    //      - 5: Points
    //      - 6: Flames - foreground
    //      - 7: Sparkles
    //      - 8: Generic textures
    //      - 9: Explosions
    //      - 10: Highlights
    //      - 11: Vectors
    //

    constexpr float ShipRegionZStart = 1.0f; // Far
    constexpr float ShipRegionZWidth = -2.0f; // Near (-1)

    constexpr int NLayers = 12;

    ViewModel::ProjectionMatrix shipOrthoMatrix;

    //
    // Layer 0: Ropes
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        0,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 1: Flames - background
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        1,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground1>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground1, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground2>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground2, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesBackground3>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesBackground3, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);


    //
    // Layer 2: Springs
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        2,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 3: Triangles
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        3,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesDecay>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesDecay, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 4: Stressed Springs
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        4,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipStressedSprings>();
    mShaderManager.SetProgramParameter<ProgramType::ShipStressedSprings, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 5: Points
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        5,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 6: Flames - foreground
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        6,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground1>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground1, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground2>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground2, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipFlamesForeground3>();
    mShaderManager.SetProgramParameter<ProgramType::ShipFlamesForeground3, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 7: Sparkles
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        7,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipSparkles>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSparkles, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 8: Generic Textures
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        8,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipGenericMipMappedTextures>();
    mShaderManager.SetProgramParameter<ProgramType::ShipGenericMipMappedTextures, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 9: Explosions
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        9,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipExplosions>();
    mShaderManager.SetProgramParameter<ProgramType::ShipExplosions, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 10: Highlights
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        10,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipElectricalElementHighlights>();
    mShaderManager.SetProgramParameter<ProgramType::ShipElectricalElementHighlights, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipCircleHighlights>();
    mShaderManager.SetProgramParameter<ProgramType::ShipCircleHighlights, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);

    //
    // Layer 11: Vectors
    //

    renderSettings.View.CalculateShipOrthoMatrix(
        ShipRegionZStart,
        ShipRegionZWidth,
        static_cast<int>(mShipId),
        static_cast<int>(mShipCount),
        static_cast<int>(mMaxMaxPlaneId),
        11,
        NLayers,
        shipOrthoMatrix);

    mShaderManager.ActivateProgram<ProgramType::ShipVectors>();
    mShaderManager.SetProgramParameter<ProgramType::ShipVectors, ProgramParameterType::OrthoMatrix>(
        shipOrthoMatrix);
}

void ShipRenderContext::ApplyEffectiveAmbientLightIntensityChanges(RenderSettings const & renderSettings)
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesDecay>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesDecay, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);

    mShaderManager.ActivateProgram<ProgramType::ShipGenericMipMappedTextures>();
    mShaderManager.SetProgramParameter<ProgramType::ShipGenericMipMappedTextures, ProgramParameterType::EffectiveAmbientLightIntensity>(
        renderSettings.EffectiveAmbientLightIntensity);
}

// TODOOLD

void ShipRenderContext::OnLampLightColorUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::LampLightColor>(
        mLampLightColor.x, mLampLightColor.y, mLampLightColor.z, mLampLightColor.w);
}

void ShipRenderContext::OnWaterColorUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::WaterColor>(
        mWaterColor.x, mWaterColor.y, mWaterColor.z, mWaterColor.w);
}

void ShipRenderContext::OnWaterContrastUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::WaterContrast>(
        mWaterContrast);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::WaterContrast>(
        mWaterContrast);
}

void ShipRenderContext::OnWaterLevelOfDetailUpdated()
{
    // Transform: 0->1 == 2.0->0.01
    float waterLevelThreshold = 2.0f + mWaterLevelOfDetail * (-2.0f + 0.01f);

    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopes>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopes, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColor, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTexture, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColor, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTexture>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTexture, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColor>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColor, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::WaterLevelThreshold>(
        waterLevelThreshold);
}

void ShipRenderContext::OnHeatOverlayTransparencyUpdated()
{
    //
    // Set parameter in all programs
    //

    mShaderManager.ActivateProgram<ProgramType::ShipRopesWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipRopesWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsColorWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipSpringsTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipSpringsTextureWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesColorWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipTrianglesTextureWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipTrianglesTextureWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);

    mShaderManager.ActivateProgram<ProgramType::ShipPointsColorWithTemperature>();
    mShaderManager.SetProgramParameter<ProgramType::ShipPointsColorWithTemperature, ProgramParameterType::HeatOverlayTransparency>(
        mHeatOverlayTransparency);
}

void ShipRenderContext::OnShipFlameSizeAdjustmentUpdated()
{
    // Recalculate quad dimensions
    mHalfFlameQuadWidth = BasisHalfFlameQuadWidth * mShipFlameSizeAdjustment;
    mFlameQuadHeight = BasisFlameQuadHeight * mShipFlameSizeAdjustment;
}

}