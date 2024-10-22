#version 330 core

#define CAMERA_NUM 7

in vec2 vScreenPos;

uniform sampler2D color[CAMERA_NUM];
uniform sampler2D vertices[CAMERA_NUM];
uniform sampler2D normals[CAMERA_NUM];
uniform sampler2D depth[CAMERA_NUM];

uniform sampler2D miniWeightsA;
uniform sampler2D miniWeightsB;

uniform bool isCameraActive[CAMERA_NUM];

uniform bool useFusion = false;
uniform vec4 cameraVector;
uniform mat4 view;


struct Light {
    vec4 position;
    vec4 intensity;
    float range;
};

uniform Light lights[4];

vec3 doLighting(vec3 position, vec3 normal){
	vec3 kD = vec3(1.0, 1.0, 1.0);

    vec3 iOut = kD * 0.1;

    vec3 n = normalize(normal);


    for(int i=0; i < 4; ++i){
        Light light = lights[i];
        vec3 l = light.position.xyz - position;

        float d = length(l);
        l = normalize(l);

        if(dot(n, l) < 0)
            continue;

        float intensity = d > light.range ? 0 : pow((light.range - d) / light.range,2);

        iOut += kD * light.intensity.rgb * dot(n, l) * intensity;

       // vec3 h = normalize(normalize(-position) + l);
       // iOut += kD * light.specular * pow(clamp(dot(n, h),0.0,1.0), shininess) * intensity;
    }

	return iOut;
}


vec3 codeCameraToColor(int camID){
	vec3 base = vec3(0.0, 0.0, 0.0);

	if(camID < 3)
		base = vec3(camID % 3 == 0 ? 1.0 : 0.0, camID % 3 == 1 ? 1.0 : 0.0, camID % 3 == 2 ? 1.0 : 0.0);
	else if(camID == 3)
		base = vec3(1.0, 1.0, 0.0);
	else if(camID == 4)
		base = vec3(1.0, 0.0, 1.0);
	else if(camID == 5)
		base = vec3(0.0, 1.0, 1.0);
	else if(camID == 6)
		base = vec3(1.0, 1.0, 1.0);	
	

	return base; 
}

out vec4 FragColor;

void main()
{		
	float distanceTreshold = 0.05;

	vec3 sumColor = vec3(0.0, 0.0, 0.0);
	vec3 sumNormal = vec3(0.0, 0.0, 0.0);
	float sumDepth = 0.0;
	float sumAlpha = 0.0;
	
	float lastDistToCam = 10000;
	
	float smoothBlend[8];
	vec4 miniWValueA = texture(miniWeightsA, vScreenPos).rgba;
	vec4 miniWValueB = texture(miniWeightsB, vScreenPos).rgba;
	
	smoothBlend[0] = miniWValueA.x;
	smoothBlend[1] = miniWValueA.y;
	smoothBlend[2] = miniWValueA.z;
	smoothBlend[3] = miniWValueA.a;
	smoothBlend[4] = miniWValueB.x;
	smoothBlend[5] = miniWValueB.y;
	smoothBlend[6] = miniWValueB.z;
	smoothBlend[7] = miniWValueB.a;

	for(int i=0; i < CAMERA_NUM; ++i){
		if(isCameraActive[i]){
			vec4 currentVertex = vec4(texture(vertices[i], vScreenPos).xyz, 1.0);
			vec2 blendFactors = vec2(texture(vertices[i], vScreenPos).a, texture(normals[i], vScreenPos).a);
			
			float currentAlpha = smoothBlend[i] * (blendFactors.y);
			
			float distToCam = length(currentVertex.xyz);
		
			if(!(distToCam >= 0.01) || !(currentAlpha > 0.0))
				continue;
				
			if(distToCam + distanceTreshold < lastDistToCam){
				sumColor = vec3(0.0, 0.0, 0.0);
				sumNormal = vec3(0.0, 0.0, 0.0);
				sumDepth = 0.0;
				sumAlpha = 0.0;
				lastDistToCam = distToCam;
			} else if(distToCam - distanceTreshold > lastDistToCam){
				continue;
			}
		
			vec3 currentNormal = texture(normals[i], vScreenPos).xyz;
			//vec3 currentColor = codeCameraToColor(i); //texture(color[i], vScreenPos).rgb;
			vec3 currentColor = texture(color[i], vScreenPos).rgb;
			float currentDepth = texture(depth[i], vScreenPos).r;
			
			sumColor += currentColor * currentAlpha;
			sumNormal += currentNormal * currentAlpha;
			sumDepth += currentDepth * currentAlpha;
			sumAlpha += currentAlpha;
		}
	}
	
		
	//FragColor = vec4(sumColor / sumAlpha + miniWValueA.rgb * 0.4, 1.0);


	if(sumAlpha > 0.009){
		FragColor = vec4(sumColor / sumAlpha, 1.0);
		gl_FragDepth = sumDepth / sumAlpha;
	} else {
		FragColor = vec4(0.0, 0.0, 0.0, 0.0);
		gl_FragDepth = 1.0;
	}
	
	
/*
	for(int i=0; i < CAMERA_NUM; ++i){
		if(isCameraActive[i]){
			vec4 currentVertex = vec4(texture(vertices[i], vScreenPos).xyz, 0.0);
			float dist = length(currentVertex);
			
			if(dist < 0.01)
				continue;
				
			vec3 currentNormal = normalize(texture(normals[i], vScreenPos).xyz);
			vec3 currentColor = texture(color[i], vScreenPos).rgb;
			float alpha =  texture(vertices[i], vScreenPos).a;
				
			bool replace = dist < resultDistance;
			
			if(useFusion && alpha >= 0.0 && resultAlpha >= 0.0){		
				// If the pixel is very much nearer:
				replace = dist < resultDistance - distanceTreshold;
				
				if(!replace){
					float currentDotLast = dot(currentNormal, resultNormal);
					
					//currentColor = vec3(currentDotLast);
					
					// If the pixel is approx the same, check for alpha value:
					replace = replace || (abs(dist - resultDistance) < distanceTreshold && currentDotLast > 0.5 && alpha > resultAlpha);
				}
			}
			
			if(replace){
				resultDistance = dist;
				resultColor = currentColor;//doLighting(currentVertex.xyz, (view * vec4(-currentNormal, 0.0)).xyz);//vec3(i==2?1.0:0.0, i==1?1.0:0.0, i==0?1.0:0.0);//currentColor;////currentColor;
				resultAlpha = alpha;
				resultNormal = currentNormal;
				resultDepth = texture(depth[i], vScreenPos).r;
			}
		}	
	}
		
	FragColor.rgb = resultColor;
	gl_FragDepth = resultDepth;
	*/
}
