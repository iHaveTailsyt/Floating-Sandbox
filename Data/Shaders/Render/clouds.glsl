###VERTEX

#version 120

#define in attribute
#define out varying

//
// Clouds' inputs are directly in NDC coordinates
//

// Inputs
in vec4 inCloud1; // Position (NDC) (vec2), TextureCoordinates (vec2)
in vec4 inCloud2; // TextureCenterCoordinates (vec2), Darkening, GrowthProgress

// Outputs
out vec2 texturePos;
out vec2 textureCenterPos;
out float darkness;
out float growthProgress;

void main()
{
    texturePos = inCloud1.zw;
    textureCenterPos = inCloud2.xy;
    darkness = inCloud2.z;
    growthProgress = inCloud2.w;

    gl_Position = vec4(inCloud1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader
in vec2 texturePos;
in vec2 textureCenterPos;
in float darkness;
in float growthProgress;

// The texture
uniform sampler2D paramCloudsAtlasTexture;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;

void main()
{
    // Sample texture
    vec4 textureColor = texture2D(paramCloudsAtlasTexture, texturePos);

    // Sample alpha
    vec2 alphaMaskTexturePos = textureCenterPos + (texturePos - textureCenterPos) * growthProgress;
    vec4 alphaMaskSample = texture2D(paramCloudsAtlasTexture, alphaMaskTexturePos);
       
    // Combine into final color
    float alphaMultiplier = sqrt(textureColor.w * alphaMaskSample.w);
    gl_FragColor = vec4(textureColor.xyz * paramEffectiveAmbientLightIntensity * darkness, alphaMultiplier);
} 
