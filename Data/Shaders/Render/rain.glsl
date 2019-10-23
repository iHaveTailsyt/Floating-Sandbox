###VERTEX

#version 120

#define in attribute
#define out varying

//
// This shader's inputs are in NDC coordinates
//

// Inputs
in vec2 inRain; // Position (vec2)

// Outputs
out vec2 uv;

void main()
{
    uv = (vec2(1.0, 1.0) + inRain.xy) / 2.0;
    gl_Position = vec4(inRain.xy, -1.0, 1.0);
}


###FRAGMENT

#version 120

#define in varying

// Inputs
in vec2 uv;

// Parameters        
uniform float paramEffectiveAmbientLightIntensity;
uniform float paramRainDensity;
uniform float paramTime;

void main()
{
    //
    // ---------------------------------------------
    //
    
    #define RainSpatialDensityX 45.0
    #define RainSpatialDensityY 30.0
    #define RainSpeed 20.0
    #define DropletLength 0.65
    #define DropletWidth 0.06
    
    vec2 scaledUV = uv * vec2(RainSpatialDensityX, RainSpatialDensityY);
        
	// Offset Y based off time
    scaledUV.y += paramTime * RainSpeed;
            
    // Calculate tile X coordinate
    float tileX = floor(scaledUV.x);
    
    // Offset Y randomly based off its tile X coordinate
    scaledUV.y += 407.567 * fract(351.5776456 * tileX);
    
    // Calculate tile Y coordinate
    float tileY = floor(scaledUV.y);
    
    // Calculate coords within the tile
    vec2 inTile = fract(scaledUV);
    
    // Shuffle in-tile X based on its tile coordinates;
    float rand = fract(sin(77.7 * tileY + 77.7 * tileX));
    inTile.x += rand - 0.5;    
    
    // Distance from center of tile
    float xDistance = abs(0.5 - inTile.x);
    float yDistance = abs(0.5 - inTile.y);
    	    
    // Thickness of droplet:
    //    > 0.0 only within Width
    //    ...with some tile-specific randomization
    float clampedXDistance = smoothstep(0.0, DropletWidth, xDistance);
    float dropletThickness = (1.0 - clampedXDistance) * smoothstep(1.0 - DropletLength + rand/2.4, 1.0, 1.0 - yDistance);
    
    // Turning off tiles
    float randOnOff = fract(sin(tileX * 71.0 + tileY * 7.0));
    float m = 1.0 - step(paramRainDensity, randOnOff);
    dropletThickness *= m;

    //
    // ---------------------------------------------
    //

    float alpha = .8 * smoothstep(0.4, 1.0, dropletThickness);
    vec3 c = vec3(dropletThickness, dropletThickness, dropletThickness) * paramEffectiveAmbientLightIntensity;

    // Output to screen
    gl_FragColor = vec4(c, alpha);
} 
