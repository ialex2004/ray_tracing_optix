
/*
 * Copyright (c) 2008 - 2009 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property and proprietary
 * rights in and to this software, related documentation and any modifications thereto.
 * Any use, reproduction, disclosure or distribution of this software and related
 * documentation without an express license agreement from NVIDIA Corporation is strictly
 * prohibited.
 *
 * TO THE MAXIMUM EXTENT PERMITTED BY APPLICABLE LAW, THIS SOFTWARE IS PROVIDED *AS IS*
 * AND NVIDIA AND ITS SUPPLIERS DISCLAIM ALL WARRANTIES, EITHER EXPRESS OR IMPLIED,
 * INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL NVIDIA OR ITS SUPPLIERS BE LIABLE FOR ANY
 * SPECIAL, INCIDENTAL, INDIRECT, OR CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT
 * LIMITATION, DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF
 * BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS) ARISING OUT OF THE USE OF OR
 * INABILITY TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES
 */

#include <optix.h>
#include <optixu/optixu_math_namespace.h>

using namespace optix;

struct BasicLight
{
 optix::float3 pos;
 optix::float3 color;
 int casts_shadow;
 };

 struct PerRayData_radiance
{
  float3 result;
  float  importance;
  int depth;
};

struct PerRayData_shadow
{
  float3 attenuation;
};
 

rtDeclareVariable(float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, ); 
rtDeclareVariable(float3, color_normal, attribute color_normal,);

rtDeclareVariable(optix::Ray, ray,          rtCurrentRay, );
rtDeclareVariable(float,      t_hit,        rtIntersectionDistance, );

rtDeclareVariable(PerRayData_radiance, prd_radiance, rtPayload, );
rtDeclareVariable(PerRayData_shadow,   prd_shadow,   rtPayload, );

rtDeclareVariable(unsigned int, radiance_ray_type, , );
rtDeclareVariable(unsigned int, shadow_ray_type , , );
rtDeclareVariable(float,        scene_epsilon, , );
rtDeclareVariable(rtObject,     top_object, , );

rtDeclareVariable(float3,   Ka, , ); 
rtDeclareVariable(float3,   Ks, , ); 
rtDeclareVariable(float,    phong_exp, , );
rtDeclareVariable(float3,   Kd, , ); 
rtDeclareVariable(float3,   ambient_light_color, , );
rtBuffer<BasicLight>        lights;
rtDeclareVariable(rtObject, top_shadower, , );






RT_PROGRAM void any_hit_shadow()
{
  // this material is opaque, so it fully attenuates all shadow rays
	prd_shadow.attenuation = make_float3(0.0f);
  
	rtTerminateRay();
}

RT_PROGRAM void closest_hit_radiance()
{
	float3 world_geo_normal   = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, geometric_normal ) );
	float3 world_shade_normal = normalize( rtTransformNormal( RT_OBJECT_TO_WORLD, shading_normal ) );
	float3 ffnormal     = faceforward( world_shade_normal, -ray.direction, world_geo_normal );
	float3 color = Ka * ambient_light_color;

	float3 hit_point = ray.origin + t_hit * ray.direction;

	for(int i = 0; i <lights.size(); ++i) {
		BasicLight light = lights[i];
		float3 L = normalize(light.pos - hit_point);
		float nDl = dot( ffnormal, L);

		if( nDl >= 0.0f ){
			// cast shadow ray
			PerRayData_shadow shadow_prd;
			shadow_prd.attenuation = make_float3(1.0f);
			float Ldist = length(light.pos - hit_point);
			optix::Ray shadow_ray( hit_point, L, shadow_ray_type, scene_epsilon, Ldist );
			rtTrace(top_shadower, shadow_ray, shadow_prd);
			float3 light_attenuation = shadow_prd.attenuation;

			if( fmaxf(light_attenuation) > 0.0f ){
				float3 Lc = Kd*color_normal;// * light_attenuation;
				color += Lc;
			}

		}
	}
	prd_radiance.result = Ks*color_normal+color/lights.size();
}


//normalize(rtTransformNormal(RT_OBJECT_TO_WORLD, shading_normal));