#include <optix_world.h>

using namespace optix;

struct BasicLight
{
 optix::float3 pos;
 optix::float3 color;
 int casts_shadow;
 };

rtBuffer<float4> spheres;
rtBuffer<float3> spheres_colors;


rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, ); 
rtDeclareVariable(float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable(float3, color_normal, attribute color_normal,);
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

RT_PROGRAM void intersect(int primIdx)
{
	int num_spheres=spheres.size();
	int i;
	
	#pragma unroll
	for (i=0; i<num_spheres;i++)
	{
		const float3 center = make_float3(spheres[i]);
	
		const float3 O = ray.origin - center;
		const float3 D = ray.direction;
		const float radius = spheres[i].w;

		float b = dot(O, D);
		float c = dot(O, O) - radius * radius;
		float disc = b * b - c;

		if (!(disc <= 0.0f))
		{
			float sdisc = sqrtf(disc);
			float root1 = (-b - sdisc);
	
			if (rtPotentialIntersection(root1))
			{
					
				shading_normal = geometric_normal = (O + (root1 )*D)/radius;
				color_normal=spheres_colors[i];
				rtReportIntersection(0);
			}
		}
	}
	return;
}

RT_PROGRAM void bounds(int, float result[6])
{
	int num_spheres=spheres.size();
	int i;
	#pragma unroll
	for (i=0; i<num_spheres;i++)
	{
		const float3 center = make_float3(spheres[i]);
		
		const float radius = spheres[i].w;

		optix::Aabb* aabb = (optix::Aabb*)result;

		if (radius <= 0.0f || isinf(radius))
		{
			aabb->invalidate();
			
		}
		else 
		{
		aabb->m_min = center - make_float3(radius);
		aabb->m_max = center + make_float3(radius);
		}
	}
	return;
}

