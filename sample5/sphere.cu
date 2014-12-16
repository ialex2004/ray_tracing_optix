#include <optix_world.h>

using namespace optix;

rtDeclareVariable(float4, sphere, , );
rtDeclareVariable(float3, sphere_color, , );

rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, ); 
rtDeclareVariable(float3, shading_normal, attribute shading_normal, ); 
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

RT_PROGRAM void intersect(int primIdx)
{
	const float3 center = make_float3(sphere);
	const float3 O = ray.origin - center;
	const float3 D = ray.direction;
	const float radius = sphere.w;

	float b = dot(O, D);
	float c = dot(O, O) - radius * radius;
	float disc = b * b - c;

	if (disc <= 0.0f) return;

	float sdisc = sqrtf(disc);
	float root1 = (-b - sdisc);

	if (rtPotentialIntersection(root1))
	{
		shading_normal = geometric_normal = sphere_color;
		rtReportIntersection(0);
	} 
}

RT_PROGRAM void bounds(int, float result[6])
{
	const float3 center = make_float3(sphere);
	const float radius = sphere.w;

	optix::Aabb* aabb = (optix::Aabb*)result;

	if (radius <= 0.0f || isinf(radius))
	{
		aabb->invalidate();
		return;
	}

	aabb->m_min = center - make_float3(radius);
	aabb->m_max = center + make_float3(radius);
}

