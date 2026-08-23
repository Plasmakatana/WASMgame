#version 100

precision mediump float;

varying vec3 fragPosition;
varying vec3 fragNormal;

uniform vec4 colDiffuse;

#define MAX_LIGHTS 4

#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1

struct Light
{
    int enabled;
    int type;

    vec3 position;
    vec3 target;

    vec4 color;

    float intensity;
};

uniform Light lights[MAX_LIGHTS];

uniform vec4 ambient;
uniform vec3 viewPos;

void main()
{
    vec3 normal = normalize(fragNormal);

    vec3 viewDirection =
        normalize(viewPos - fragPosition);

    // Start with ambient illumination.
    vec3 result =
        colDiffuse.rgb * ambient.rgb;

    // --------------------------------------------------------
    // Lights
    // --------------------------------------------------------

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 1)
        {
            vec3 lightDirection;

            float attenuation = 1.0;

            // ------------------------------------------------
            // Directional light
            // ------------------------------------------------

            if (lights[i].type == LIGHT_DIRECTIONAL)
            {
                lightDirection =
                    normalize(
                        lights[i].position -
                        lights[i].target
                    );
            }

            // ------------------------------------------------
            // Point light
            // ------------------------------------------------

            else
            {
                vec3 toLight =
                    lights[i].position -
                    fragPosition;

                float distanceToLight =
                    length(toLight);

                lightDirection =
                    normalize(toLight);

                // Much gentler falloff than before.
                attenuation =
                    1.0 /
                    (
                        1.0 +
                        0.005 *
                        distanceToLight *
                        distanceToLight
                    );
            }

            // ------------------------------------------------
            // Diffuse lighting
            // ------------------------------------------------

            float NdotL =
                max(
                    dot(
                        normal,
                        lightDirection
                    ),
                    0.0
                );

            result +=
                colDiffuse.rgb *
                lights[i].color.rgb *
                NdotL *
                lights[i].intensity *
                attenuation;


            // ------------------------------------------------
            // Specular highlight
            // ------------------------------------------------

            if (NdotL > 0.0)
            {
                vec3 reflected =
                    reflect(
                        -lightDirection,
                        normal
                    );

                float specular =
                    pow(
                        max(
                            dot(
                                viewDirection,
                                reflected
                            ),
                            0.0
                        ),
                        32.0
                    );

                result +=
                    lights[i].color.rgb *
                    specular *
                    0.4 *
                    lights[i].intensity *
                    attenuation;
            }
        }
    }

    // --------------------------------------------------------
    // Prevent completely black objects
    // --------------------------------------------------------

    result =
        max(
            result,
            colDiffuse.rgb * 0.12
        );

    // --------------------------------------------------------
    // Gamma correction
    // --------------------------------------------------------

    result =
        pow(
            result,
            vec3(1.0 / 2.2)
        );

    gl_FragColor =
        vec4(
            result,
            colDiffuse.a
        );
}
