###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inWorldBorder; // Position (vec2), TextureSpaceCoords (vec2)

// Parameters
uniform mat4 paramOrthoMatrix;

// Outputs
out vec2 textureSpaceCoords;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(inWorldBorder.xy, -1.0, 1.0);
    textureSpaceCoords = inWorldBorder.zw;
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 textureSpaceCoords; // 3.0 => 3 full frames

// The texture
uniform sampler2D paramGenericLinearTexturesAtlasTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;

void main()
{
    //
    // Wrap the atlas texture tile
    //

    // TODO: from param
    vec2 textureFrameBottomLeft = vec2(0.0009765625, 0.501953125);
    vec2 textureFrameSize = vec2(0.0390625, 0.078125);

    // TODO: from param
    //#define DX (0.025 + 0.005)
    #define DX (0.025 * 2.0)
    vec2 uv = clamp(fract(textureSpaceCoords), vec2(DX), vec2(1.0-DX));
    vec2 textureCoords = textureFrameBottomLeft + textureFrameSize * uv;
    vec4 textureColor = texture2D(paramGenericLinearTexturesAtlasTexture, textureCoords);
    gl_FragColor = vec4(textureColor.xyz * paramEffectiveAmbientLightIntensity, 0.75);
} 
