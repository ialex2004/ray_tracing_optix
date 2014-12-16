
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

/*
 * sample1.cpp -- Renders a solid green image.
 *
 * A filename can be given on the command line to write the results to file. 
 */

#include <optix.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sutil.h>

#define ENABLE_CHECK

int main(int argc, char* argv[])
{
	if (argc != 6)
	{
		printf("Usage: %s <n_spheres> <n_lights> <width> <height> <bmp_filename> \n", argv[0]);
		return -1; 
	}

	int n_spheres = atoi( argv[1]);
#ifdef ENABLE_CHECK
	if (n_spheres < 5 || n_spheres > 10)
	{
		printf ("n_spheres is out of range [5:10]\n");
		return -1;
	}
#endif

	int n_lights = atoi( argv[2]);
#ifdef ENABLE_CHECK
	if (n_lights < 1 || n_lights > 2)
	{
		printf ("n_lights is out of range [1:2]\n");
		return -1;
	}
#endif

	int width = atoi( argv[3]);
#ifdef ENABLE_CHECK
	if (width < 800 || width > 1920)
	{
		printf ("width is out of range [800:1920]\n");
		return -1;
	}
#endif

	int height = atoi( argv[4]);
#ifdef ENABLE_CHECK
	if (height < 600 || height > 1080)
	{
		printf ("height is out of range [600:1080]\n");
		return -1;
	}
#endif

#ifdef DEBUG
	printf ("Picture size is width=%d  height=%d \n", width, height);
#endif

    /* Create our objects and set state */
    RTcontext context;
    RT_CHECK_ERROR( rtContextCreate( &context ) );
    RT_CHECK_ERROR( rtContextSetRayTypeCount( context, 1 ) );
    RT_CHECK_ERROR( rtContextSetEntryPointCount( context, 1 ) );

    RTbuffer buffer;
    RT_CHECK_ERROR( rtBufferCreate( context, RT_BUFFER_OUTPUT, &buffer ) );
    RT_CHECK_ERROR( rtBufferSetFormat( buffer, RT_FORMAT_FLOAT4 ) );
    RT_CHECK_ERROR( rtBufferSetSize2D( buffer, width, height ) );
  
    RTvariable result;
    RT_CHECK_ERROR( rtContextDeclareVariable( context, "result", &result ) );
    RT_CHECK_ERROR( rtVariableSetObject( result, buffer ) );

    RTprogram ray_tracing_program;
    RT_CHECK_ERROR( rtProgramCreateFromPTXFile( context, "ray_tracing.ptx", "ray_tracing", &ray_tracing_program ) );

    RTvariable ray_tracing_color;
    RT_CHECK_ERROR( rtProgramDeclareVariable( ray_tracing_program, "ray_tracing_color", &ray_tracing_color ) );
    RT_CHECK_ERROR( rtVariableSet3f( ray_tracing_color, 0.462f, 0.725f, 0.0f ) );

	RTprogram  intersection_program;
	RTprogram  bounding_box_program;
	RTvariable s;
	float sphere_loc[4] =  {0, 0, 0, 1.5};
	RTgeometry sphere;
	RT_CHECK_ERROR( rtGeometryCreate( context, &sphere ) );
	RT_CHECK_ERROR( rtGeometrySetPrimitiveCount( sphere, 1u ) );
	RT_CHECK_ERROR( rtGeometryDeclareVariable( sphere, "sphere" , &s) );
	RT_CHECK_ERROR( rtVariableSet4fv( s, &sphere_loc[0] ) );
	RT_CHECK_ERROR( rtProgramCreateFromPTXFile( context, "ray_tracing.ptx", "bounds", &bounding_box_program) );
	RT_CHECK_ERROR( rtGeometrySetBoundingBoxProgram( sphere, bounding_box_program ) );
//	RT_CHECK_ERROR( rtProgramCreateFromPTXFile( context, "ray_tracing.ptx", "intersect", &intersection_program) );
//	RT_CHECK_ERROR( rtGeometrySetIntersectionProgram( sphere, intersection_program ) );

    RT_CHECK_ERROR( rtContextSetRayGenerationProgram( context, 0, ray_tracing_program ) );

    /* Run */
    RT_CHECK_ERROR( rtContextValidate( context ) );
    RT_CHECK_ERROR( rtContextCompile( context ) );
    RT_CHECK_ERROR( rtContextLaunch2D( context, 0 /* entry point */, width, height ) );

    /* Display image */
    RT_CHECK_ERROR( sutilDisplayFilePPM(argv[5], buffer) );

    /* Clean up */
    RT_CHECK_ERROR( rtBufferDestroy( buffer ) );
    RT_CHECK_ERROR( rtProgramDestroy( ray_tracing_program ) );
    RT_CHECK_ERROR( rtContextDestroy( context ) );

    return( 0 );
}

