
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
 *  sample5.cpp -- Renders sphere with a normal shader 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <optix.h>
#include <sutil.h>
#include <optix_world.h>

char path_to_ptx[512];


typedef struct struct_BoxExtent{
    float min[3];
    float max[3];
} BoxExtent;

struct BasicLight
{
 optix::float3 pos;
 optix::float3 color;
 int casts_shadow;
 };


unsigned int width = 1920;
unsigned int height = 1080;

void createContext( RTcontext* context, RTbuffer* buffer );
void createGeometry( RTcontext context, RTgeometry* sphere );
void createMaterial( RTcontext context, RTmaterial* material );
void createInstance( RTcontext context, RTgeometry sphere, RTmaterial material );
void printUsageAndExit( const char* argv0 );


int main(int argc, char* argv[])
{
  /* Primary RTAPI objects */
  RTcontext           context;
  RTbuffer            output_buffer_obj;
  RTgeometry          sphere;
  RTmaterial          material;

  char outfile[512];
  int i;
  int use_glut = 1;

  outfile[0] = '\0';

  /* If "--file" is specified, don't do any GL stuff */
  for( i = 1; i < argc; ++i ) {
    if( strcmp( argv[i], "--file" ) == 0 || strcmp( argv[i], "-f" ) == 0 )
      use_glut = 0;
  }

  /* Process command line args */
  if( use_glut )
    RT_CHECK_ERROR_NO_CONTEXT( sutilInitGlut( &argc, argv ) );
  for( i = 1; i < argc; ++i ) {
    if( strcmp( argv[i], "--help" ) == 0 || strcmp( argv[i], "-h" ) == 0 ) {
      printUsageAndExit( argv[0] );
    } else if( strcmp( argv[i], "--file" ) == 0 || strcmp( argv[i], "-f" ) == 0 ) {
      if( i < argc-1 ) {
        strcpy( outfile, argv[++i] );
      } else {
        printUsageAndExit( argv[0] );
      }
    } else if ( strncmp( argv[i], "--dim=", 6 ) == 0 ) {
      const char *dims_arg = &argv[i][6];
      if ( sutilParseImageDimensions( dims_arg, &width, &height ) != RT_SUCCESS ) {
        fprintf( stderr, "Invalid window dimensions: '%s'\n", dims_arg );
        printUsageAndExit( argv[0] );
      }
    } else {
      fprintf( stderr, "Unknown option '%s'\n", argv[i] );
      printUsageAndExit( argv[0] );
    }
  }

  /* Setup state */
  createContext( &context, &output_buffer_obj );
  createGeometry( context, &sphere );
  createMaterial( context, &material);
  createInstance( context, sphere, material );

  /* Run */
  RT_CHECK_ERROR( rtContextValidate( context ) );
  RT_CHECK_ERROR( rtContextCompile( context ) );
  RT_CHECK_ERROR( rtContextLaunch2D( context, 0, width, height ) );

  /* Display image */
  if( strlen( outfile ) == 0 ) {
    RT_CHECK_ERROR( sutilDisplayBufferInGlutWindow( argv[0], output_buffer_obj ) );
  } else {
    RT_CHECK_ERROR( sutilDisplayFilePPM( outfile, output_buffer_obj ) );
  }

  /* Clean up */
  RT_CHECK_ERROR( rtContextDestroy( context ) );
  return( 0 );
}

void createContext( RTcontext* context, RTbuffer* output_buffer_obj )
{
  RTprogram  ray_gen_program;
  RTprogram  exception_program;
  RTprogram  miss_program;
  RTvariable output_buffer;
  RTvariable radiance_ray_type, shadow_ray_type;
  RTvariable epsilon;

  /* variables for ray gen program */
  RTvariable eye;
  RTvariable U;
  RTvariable V;
  RTvariable W;
  RTvariable badcolor;

  /* viewing params */
  float cam_eye[3], lookat[3], up[3];
  float hfov, aspect_ratio; 
  float camera_u[3], camera_v[3], camera_w[3];

  /* variables for miss program */
  RTvariable bg_color;

  /* Setup context */
  RT_CHECK_ERROR2( rtContextCreate( context ) );
  RT_CHECK_ERROR2( rtContextSetRayTypeCount( *context, 2 ) );
  RT_CHECK_ERROR2( rtContextSetEntryPointCount( *context, 1 ) );

  RT_CHECK_ERROR2( rtContextDeclareVariable( *context, "output_buffer" , &output_buffer) );
  RT_CHECK_ERROR2( rtContextDeclareVariable( *context, "radiance_ray_type" , &radiance_ray_type) );
  RT_CHECK_ERROR2( rtContextDeclareVariable( *context, "shadow_ray_type" , &shadow_ray_type) );
  
  
  RT_CHECK_ERROR2( rtContextDeclareVariable( *context, "scene_epsilon" , &epsilon) );

  RT_CHECK_ERROR2( rtVariableSet1ui( radiance_ray_type, 0u ) );
  RT_CHECK_ERROR2( rtVariableSet1ui( shadow_ray_type, 1u ) );
  
  RT_CHECK_ERROR2( rtVariableSet1f( epsilon, 1.e-3f ) );

  /* Render result buffer */
  RT_CHECK_ERROR2( rtBufferCreate( *context, RT_BUFFER_OUTPUT, output_buffer_obj ) );
  RT_CHECK_ERROR2( rtBufferSetFormat( *output_buffer_obj, RT_FORMAT_UNSIGNED_BYTE4 ) );
  RT_CHECK_ERROR2( rtBufferSetSize2D( *output_buffer_obj, width, height ) );
  RT_CHECK_ERROR2( rtVariableSetObject( output_buffer, *output_buffer_obj ) );

  /* Ray generation program */
  sprintf( path_to_ptx, "%s/%s", sutilSamplesPtxDir(), "pinhole_camera.ptx" );
  RT_CHECK_ERROR2( rtProgramCreateFromPTXFile( *context, path_to_ptx, "pinhole_camera", &ray_gen_program ) );
  RT_CHECK_ERROR2( rtContextSetRayGenerationProgram( *context, 0, ray_gen_program ) );
  RT_CHECK_ERROR2( rtContextDeclareVariable( *context, "eye" , &eye) );
  RT_CHECK_ERROR2( rtContextDeclareVariable( *context, "U" , &U) );
  RT_CHECK_ERROR2( rtContextDeclareVariable( *context, "V" , &V) );
  RT_CHECK_ERROR2( rtContextDeclareVariable( *context, "W" , &W) );

  cam_eye[0]= 0.0f;  cam_eye[1]= 0.0f;  cam_eye[2]= 5.0f;
  lookat[0] = 0.0f;  lookat[1] = 0.0f;  lookat[2] = 0.0f;
  up[0]     = 0.0f;  up[1]     = 1.0f;  up[2]     = 0.0f;
  hfov      = 60.0f;

  aspect_ratio = (float)width/(float)height;
  sutilCalculateCameraVariables( cam_eye, lookat, up, hfov, aspect_ratio,
      camera_u, camera_v, camera_w );

  RT_CHECK_ERROR2( rtVariableSet3fv( eye, cam_eye ) );
  RT_CHECK_ERROR2( rtVariableSet3fv( U, camera_u ) );
  RT_CHECK_ERROR2( rtVariableSet3fv( V, camera_v ) );
  RT_CHECK_ERROR2( rtVariableSet3fv( W, camera_w ) );

  /* Exception program */
  RT_CHECK_ERROR2( rtContextDeclareVariable( *context, "bad_color" , &badcolor) );
  RT_CHECK_ERROR2( rtVariableSet3f( badcolor, .0f, .0f, 0.5f ) );
  RT_CHECK_ERROR2( rtProgramCreateFromPTXFile( *context, path_to_ptx, "exception", &exception_program ) );
  RT_CHECK_ERROR2( rtContextSetExceptionProgram( *context, 0, exception_program ) );

  /* Miss program */
  sprintf( path_to_ptx, "%s/%s", sutilSamplesPtxDir(), "constantbg.ptx" );
  RT_CHECK_ERROR2( rtProgramCreateFromPTXFile( *context, path_to_ptx, "miss", &miss_program ) );
  RT_CHECK_ERROR2( rtProgramDeclareVariable( miss_program, "bg_color" , &bg_color) );
  RT_CHECK_ERROR2( rtVariableSet3f( bg_color, .0f, .0f, .0f ) );
  RT_CHECK_ERROR2( rtContextSetMissProgram( *context, 0, miss_program ) );
}


void createGeometry( RTcontext context, RTgeometry* sphere )
{
  int num_spheres=2;
  int num_lightSources = 2;
  RT_CHECK_ERROR( rtGeometryCreate( context, sphere ) );
  RT_CHECK_ERROR( rtGeometrySetPrimitiveCount( *sphere, 1u ) );

  RTprogram  intersection_program;
  RTprogram  bounding_box_program;
  sprintf( path_to_ptx, "%s/%s", sutilSamplesPtxDir(), "sphere.ptx" );
  RT_CHECK_ERROR( rtProgramCreateFromPTXFile( context, path_to_ptx, "bounds", &bounding_box_program) );
  RT_CHECK_ERROR( rtGeometrySetBoundingBoxProgram( *sphere, bounding_box_program ) );
  RT_CHECK_ERROR( rtProgramCreateFromPTXFile( context, path_to_ptx, "intersect", &intersection_program) );
  RT_CHECK_ERROR( rtGeometrySetIntersectionProgram( *sphere, intersection_program ) );

  RTvariable spheres,spheres_colors, lights;
  optix::float4 * coords;
  optix::float3 * colors;
  
  RTbuffer buff_coords,buff_colors;
  RTbuffer buff_lights;
  BasicLight * lightsPtr;
 
  RT_CHECK_ERROR(rtBufferCreate(context, RT_BUFFER_INPUT,&buff_lights));
  RT_CHECK_ERROR(rtBufferSetFormat(buff_lights,RT_FORMAT_USER));
  RT_CHECK_ERROR(rtBufferSetElementSize(buff_lights,sizeof(BasicLight)));
  RT_CHECK_ERROR(rtBufferSetSize1D(buff_lights, num_lightSources));
  RT_CHECK_ERROR(rtContextDeclareVariable( context, "lights", &lights ) );
  RT_CHECK_ERROR(rtVariableSetObject( lights, buff_lights ) );
  RT_CHECK_ERROR(rtBufferMap(buff_lights, (void**)&lightsPtr));
 
  RT_CHECK_ERROR(rtBufferCreate(context, RT_BUFFER_INPUT,&buff_coords));
  RT_CHECK_ERROR(rtBufferSetFormat(buff_coords,RT_FORMAT_FLOAT4));
  RT_CHECK_ERROR(rtBufferSetSize1D(buff_coords, num_spheres));
  RT_CHECK_ERROR( rtContextDeclareVariable( context, "spheres", &spheres ) );
  RT_CHECK_ERROR( rtVariableSetObject( spheres, buff_coords ) );
  RT_CHECK_ERROR(rtBufferMap(buff_coords, (void**)&coords));
  
  RT_CHECK_ERROR(rtBufferCreate(context, RT_BUFFER_INPUT,&buff_colors));
  RT_CHECK_ERROR(rtBufferSetFormat(buff_colors,RT_FORMAT_FLOAT3));
  RT_CHECK_ERROR(rtBufferSetSize1D(buff_colors, num_spheres));
  RT_CHECK_ERROR( rtContextDeclareVariable( context, "spheres_colors", &spheres_colors ) );
  RT_CHECK_ERROR( rtVariableSetObject( spheres_colors, buff_colors ) );
  RT_CHECK_ERROR(rtBufferMap(buff_colors, (void**)&colors));
  int i;
  for (i =0; i<num_spheres; i++)
  {
	coords[i].x=i*0.6;
	coords[i].y=i*0.6;
	coords[i].z=0;
	coords[i].w=0.25f;
	colors[i].x=i*0.5f;
	colors[i].y=1-i*0.5f;
	colors[i].z=0;	
  }
  lightsPtr[0].pos.x=-10.0f;
  lightsPtr[0].pos.y=-10.0f;
  lightsPtr[0].pos.z=0.0f;
  
  
  lightsPtr[1].pos.x=10.0f;
  lightsPtr[1].pos.y=-10.0f;
  lightsPtr[1].pos.z=5.0f;
  
  
  RT_CHECK_ERROR(rtBufferUnmap(buff_coords));
  RT_CHECK_ERROR(rtBufferUnmap(buff_colors));
  RT_CHECK_ERROR(rtBufferUnmap(buff_lights));
  
  RTvariable Ka,Kd,Ks,phong_exp;
  RT_CHECK_ERROR(rtContextDeclareVariable( context, "Ka", &Ka ) );
  RT_CHECK_ERROR(rtContextDeclareVariable( context, "Kd", &Kd ) );
  RT_CHECK_ERROR(rtContextDeclareVariable( context, "Ks", &Ks ) );
  //RT_CHECK_ERROR(rtContextDeclareVariable( context, "ambient_color", &ambient_color ) );
  RT_CHECK_ERROR(rtContextDeclareVariable( context, "phong_exp", &phong_exp ) );
  RT_CHECK_ERROR(rtVariableSet3f(Ka, 1.0f,1.0f,1.0f));
  RT_CHECK_ERROR(rtVariableSet3f(Kd, 0.6f,0.6f,0.6f));
  RT_CHECK_ERROR(rtVariableSet3f(Ks, 0.4f,0.4f,0.4f));
  RT_CHECK_ERROR(rtVariableSet3f(Ka, 1.0f,1.0f,1.0f));
  RT_CHECK_ERROR(rtVariableSet1f(phong_exp, 88));

  

}


void createMaterial( RTcontext context, RTmaterial* material )
{
  RTprogram chp, ahp;

  sprintf( path_to_ptx, "%s/%s", sutilSamplesPtxDir(), "normal_shader.ptx" );
  RT_CHECK_ERROR( rtProgramCreateFromPTXFile( context, path_to_ptx, "closest_hit_radiance", &chp ) );
  sprintf( path_to_ptx, "%s/%s", sutilSamplesPtxDir(), "normal_shader.ptx" );
  RT_CHECK_ERROR( rtProgramCreateFromPTXFile( context, path_to_ptx, "any_hit_shadow", &ahp ) );
	
  
  RT_CHECK_ERROR( rtMaterialCreate( context, material ) );
  RT_CHECK_ERROR( rtMaterialSetClosestHitProgram( *material, 0, chp) );
  RT_CHECK_ERROR( rtMaterialSetAnyHitProgram( *material, 1, ahp) );
}


void createInstance( RTcontext context, RTgeometry sphere, RTmaterial material )
{
  RTgeometrygroup geometrygroup;
  RTvariable      top_object, top_shadower;
  RTacceleration  acceleration;
  RTgeometryinstance instance;

  /* Create geometry instance */
  RT_CHECK_ERROR( rtGeometryInstanceCreate( context, &instance ) );
  RT_CHECK_ERROR( rtGeometryInstanceSetGeometry( instance, sphere ) );
  RT_CHECK_ERROR( rtGeometryInstanceSetMaterialCount( instance, 1 ) );
  RT_CHECK_ERROR( rtGeometryInstanceSetMaterial( instance, 0, material ) );

  /* Create geometry group */
  RT_CHECK_ERROR( rtAccelerationCreate( context, &acceleration ) );
  RT_CHECK_ERROR( rtAccelerationSetBuilder( acceleration, "NoAccel" ) );
  RT_CHECK_ERROR( rtAccelerationSetTraverser( acceleration, "NoAccel" ) );
  RT_CHECK_ERROR( rtGeometryGroupCreate( context, &geometrygroup ) );
  RT_CHECK_ERROR( rtGeometryGroupSetChildCount( geometrygroup, 1 ) );
  RT_CHECK_ERROR( rtGeometryGroupSetChild( geometrygroup, 0, instance ) );
  RT_CHECK_ERROR( rtGeometryGroupSetAcceleration( geometrygroup, acceleration ) );

  RT_CHECK_ERROR( rtContextDeclareVariable( context, "top_object", &top_object ) );
  RT_CHECK_ERROR( rtContextDeclareVariable( context, "top_shadower", &top_shadower ) );
  
  RT_CHECK_ERROR( rtVariableSetObject( top_object, geometrygroup ) );
  RT_CHECK_ERROR( rtVariableSetObject( top_shadower, geometrygroup ) );
  

  
  
}


void printUsageAndExit( const char* argv0 )
{
  fprintf( stderr, "Usage  : %s [options]\n", argv0 );
  fprintf( stderr, "Options: --help | -h             Print this usage message\n" );
  fprintf( stderr, "         --dim=<width>x<height>  Set image dimensions; defaults to 1024x768\n" );
  exit(1);
}
