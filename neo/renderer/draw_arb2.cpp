/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

/*
====================
GL_SelectTextureNoClient
====================
*/
static void GL_SelectTextureNoClient( int unit ) {
	backEnd.glState.currenttmu = unit;
	glActiveTextureARB( GL_TEXTURE0_ARB + unit );
	RB_LogComment( "glActiveTextureARB( %i )\n", unit );
}

/*
==================
RB_ARB2_DrawInteraction
==================
*/
void	RB_ARB2_DrawInteraction( const drawInteraction_t *din ) {
	// load all the vertex program parameters
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_ORIGIN, din->localLightOrigin.ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_VIEW_ORIGIN, din->localViewOrigin.ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_S, din->lightProjection[0].ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_T, din->lightProjection[1].ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_PROJECT_Q, din->lightProjection[2].ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_LIGHT_FALLOFF_S, din->lightProjection[3].ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_BUMP_MATRIX_S, din->bumpMatrix[0].ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_BUMP_MATRIX_T, din->bumpMatrix[1].ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_DIFFUSE_MATRIX_S, din->diffuseMatrix[0].ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_DIFFUSE_MATRIX_T, din->diffuseMatrix[1].ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_SPECULAR_MATRIX_S, din->specularMatrix[0].ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_SPECULAR_MATRIX_T, din->specularMatrix[1].ToFloatPtr() );

	static const float zero[4] = { 0, 0, 0, 0 };
	static const float one[4] = { 1, 1, 1, 1 };
	static const float negOne[4] = { -1, -1, -1, -1 };

	switch ( din->vertexColor ) {
	case SVC_IGNORE:
		glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_MODULATE, zero );
		glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_ADD, one );
		break;
	case SVC_MODULATE:
		glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_MODULATE, one );
		glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_ADD, zero );
		break;
	case SVC_INVERSE_MODULATE:
		glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_MODULATE, negOne );
		glProgramEnvParameter4fvARB( GL_VERTEX_PROGRAM_ARB, PP_COLOR_ADD, one );
		break;
	}

	// set the constant colors
	glProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 0, din->diffuseColor.ToFloatPtr() );
	glProgramEnvParameter4fvARB( GL_FRAGMENT_PROGRAM_ARB, 1, din->specularColor.ToFloatPtr() );

	// set the textures

	// texture 1 will be the per-surface bump map
	GL_SelectTextureNoClient( 1 );
	din->bumpImage->Bind();

	// texture 2 will be the light falloff texture
	GL_SelectTextureNoClient( 2 );
	din->lightFalloffImage->Bind();

	// texture 3 will be the light projection texture
	GL_SelectTextureNoClient( 3 );
	din->lightImage->Bind();

	// texture 4 is the per-surface diffuse map
	GL_SelectTextureNoClient( 4 );
	din->diffuseImage->Bind();

	// texture 5 is the per-surface specular map
	GL_SelectTextureNoClient( 5 );
	din->specularImage->Bind();

	// draw it
	RB_DrawElementsWithCounters( din->surf->geo );
}


/*
=============
RB_ARB2_CreateDrawInteractions

=============
*/
void RB_ARB2_CreateDrawInteractions( const drawSurf_t *surf ) {
	if ( !surf ) {
		return;
	}

	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );

	// bind the vertex program
	glBindProgramARB( GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION );
	glBindProgramARB( GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION );

	glEnable(GL_VERTEX_PROGRAM_ARB);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);

	// enable the vertex arrays
	glEnableVertexAttribArrayARB( 8 );
	glEnableVertexAttribArrayARB( 9 );
	glEnableVertexAttribArrayARB( 10 );
	glEnableVertexAttribArrayARB( 11 );
	glEnableClientState( GL_COLOR_ARRAY );

	// texture 0 is the normalization cube map for the vector towards the light
	GL_SelectTextureNoClient( 0 );
	if ( backEnd.vLight->lightShader->IsAmbientLight() ) {
		globalImages->ambientNormalMap->Bind();
	} else {
		globalImages->normalCubeMapImage->Bind();
	}

	// texture 6 is the specular lookup table
	GL_SelectTextureNoClient( 6 );
  globalImages->specularTableImage->Bind();

	for ( ; surf ; surf=surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes

		// set the vertex pointers
		idDrawVert	*ac = (idDrawVert *)vertexCache.Position( surf->geo->ambientCache );
		glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( idDrawVert ), ac->color );
		glVertexAttribPointerARB( 11, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->normal.ToFloatPtr() );
		glVertexAttribPointerARB( 10, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[1].ToFloatPtr() );
		glVertexAttribPointerARB( 9, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[0].ToFloatPtr() );
		glVertexAttribPointerARB( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
		glVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );

		// this may cause RB_ARB2_DrawInteraction to be exacuted multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf, RB_ARB2_DrawInteraction );
	}

	glDisableVertexAttribArrayARB( 8 );
	glDisableVertexAttribArrayARB( 9 );
	glDisableVertexAttribArrayARB( 10 );
	glDisableVertexAttribArrayARB( 11 );
	glDisableClientState( GL_COLOR_ARRAY );

	// disable features
	GL_SelectTextureNoClient( 6 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 5 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 4 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 3 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 2 );
	globalImages->BindNull();

	GL_SelectTextureNoClient( 1 );
	globalImages->BindNull();

	backEnd.glState.currenttmu = -1;
	GL_SelectTexture( 0 );

	glDisable(GL_VERTEX_PROGRAM_ARB);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
}


/*
==================
RB_ARB2_DrawInteractions
==================
*/
void RB_ARB2_DrawInteractions( void ) {
	viewLight_t		*vLight;
	const idMaterial	*lightShader;

	GL_SelectTexture( 0 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	//
	// for each light, perform adding and shadowing
	//
	for ( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		backEnd.vLight = vLight;

		// do fogging later
		if ( vLight->lightShader->IsFogLight() ) {
			continue;
		}
		if ( vLight->lightShader->IsBlendLight() ) {
			continue;
		}

		if ( !vLight->localInteractions && !vLight->globalInteractions
			&& !vLight->translucentInteractions ) {
			continue;
		}

		lightShader = vLight->lightShader;

		// clear the stencil buffer if needed
		if ( vLight->globalShadows || vLight->localShadows ) {
			backEnd.currentScissor = vLight->scissorRect;
			if ( r_useScissor.GetBool() ) {
				glScissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1, 
					backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
					backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
					backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
			}
			glClear( GL_STENCIL_BUFFER_BIT );
		} else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			glStencilFunc( GL_ALWAYS, 128, 255 );
		}

		RB_StencilShadowPass( vLight->globalShadows );
		RB_ARB2_CreateDrawInteractions( vLight->localInteractions );
		RB_StencilShadowPass( vLight->localShadows );
		RB_ARB2_CreateDrawInteractions( vLight->globalInteractions );
		glDisable( GL_VERTEX_PROGRAM_ARB );	// if there weren't any globalInteractions, it would have stayed on

		// translucent surfaces never get stencil shadowed
		if ( r_skipTranslucent.GetBool() ) {
			continue;
		}

		glStencilFunc( GL_ALWAYS, 128, 255 );

		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_ARB2_CreateDrawInteractions( vLight->translucentInteractions );

		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}

	// disable stencil shadow test
	glStencilFunc( GL_ALWAYS, 128, 255 );

	GL_SelectTexture( 0 );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
}

//===================================================================================


typedef struct {
	GLenum			target;
	GLuint			ident;
	char			name[64];
} progDef_t;

static	const int	MAX_GLPROGS = 200;

// a single file can have both a vertex program and a fragment program
static progDef_t	progs[MAX_GLPROGS] = {
	{ GL_VERTEX_PROGRAM_ARB, VPROG_INTERACTION, "interaction.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_INTERACTION, "interaction.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_ENVIRONMENT, "environment.vfp" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_ENVIRONMENT, "environment.vfp" },
	{ GL_VERTEX_PROGRAM_ARB, VPROG_GLASSWARP, "arbVP_glasswarp.txt" },
	{ GL_FRAGMENT_PROGRAM_ARB, FPROG_GLASSWARP, "arbFP_glasswarp.txt" },

	// additional programs can be dynamically specified in materials
};


static glslProgramDef_t glslPrograms[MAX_GLPROGS] = { 0 };

/*
=================
R_LoadARBProgram
=================
*/
void R_LoadARBProgram( int progIndex ) {
	int		ofs;
	int		err;
	idStr	fullPath = "glprogs/";
	fullPath += progs[progIndex].name;
	char	*fileBuffer;
	char	*buffer;
	char	*start, *end;

	common->Printf( "%s", fullPath.c_str() );

	// load the program even if we don't support it, so
	// fs_copyfiles can generate cross-platform data dumps
	fileSystem->ReadFile( fullPath.c_str(), (void **)&fileBuffer, NULL );
	if ( !fileBuffer ) {
		common->Printf( ": File not found\n" );
		return;
	}

	// copy to stack memory and free
	buffer = (char *)_alloca( strlen( fileBuffer ) + 1 );
	strcpy( buffer, fileBuffer );
	fileSystem->FreeFile( fileBuffer );

	if ( !glConfig.isInitialized ) {
		return;
	}

	//
	// submit the program string at start to GL
	//
	if ( progs[progIndex].ident == 0 ) {
		// allocate a new identifier for this program
		progs[progIndex].ident = PROG_USER + progIndex;
	}

	// vertex and fragment programs can both be present in a single file, so
	// scan for the proper header to be the start point, and stamp a 0 in after the end

	if ( progs[progIndex].target == GL_VERTEX_PROGRAM_ARB ) {
		if ( !glConfig.ARBVertexProgramAvailable ) {
			common->Printf( ": GL_VERTEX_PROGRAM_ARB not available\n" );
			return;
		}
		start = strstr( (char *)buffer, "!!ARBvp" );
	}
	if ( progs[progIndex].target == GL_FRAGMENT_PROGRAM_ARB ) {
		if ( !glConfig.ARBFragmentProgramAvailable ) {
			common->Printf( ": GL_FRAGMENT_PROGRAM_ARB not available\n" );
			return;
		}
		start = strstr( (char *)buffer, "!!ARBfp" );
	}
	if ( !start ) {
		common->Printf( ": !!ARB not found\n" );
		return;
	}
	end = strstr( start, "END" );

	if ( !end ) {
		common->Printf( ": END not found\n" );
		return;
	}
	end[3] = 0;

	glBindProgramARB( progs[progIndex].target, progs[progIndex].ident );
	glGetError();

	glProgramStringARB( progs[progIndex].target, GL_PROGRAM_FORMAT_ASCII_ARB,
		strlen( start ), (unsigned char *)start );

	err = glGetError();
	glGetIntegerv( GL_PROGRAM_ERROR_POSITION_ARB, (GLint *)&ofs );
	if ( err == GL_INVALID_OPERATION ) {
		const GLubyte *str = glGetString( GL_PROGRAM_ERROR_STRING_ARB );
		common->Printf( "\nGL_PROGRAM_ERROR_STRING_ARB: %s\n", str );
		if ( ofs < 0 ) {
			common->Printf( "GL_PROGRAM_ERROR_POSITION_ARB < 0 with error\n" );
		} else if ( ofs >= (int)strlen( (char *)start ) ) {
			common->Printf( "error at end of program\n" );
		} else {
			common->Printf( "error at %i:\n%s", ofs, start + ofs );
		}
		return;
	}
	if ( ofs != -1 ) {
		common->Printf( "\nGL_PROGRAM_ERROR_POSITION_ARB != -1 without error\n" );
		return;
	}

	common->Printf( "\n" );
}

/*
==================
R_FindARBProgram

Returns a GL identifier that can be bound to the given target, parsing
a text file if it hasn't already been loaded.
==================
*/
int R_FindARBProgram( GLenum target, const char *program ) {
	int		i;
	idStr	stripped = program;

	stripped.StripFileExtension();

	// see if it is already loaded
	for ( i = 0 ; progs[i].name[0] ; i++ ) {
		if ( progs[i].target != target ) {
			continue;
		}

		idStr	compare = progs[i].name;
		compare.StripFileExtension();

		if ( !idStr::Icmp( stripped.c_str(), compare.c_str() ) ) {
			return progs[i].ident;
		}
	}

	if ( i == MAX_GLPROGS ) {
		common->Error( "R_FindARBProgram: MAX_GLPROGS" );
	}

	// add it to the list and load it
	progs[i].ident = (program_t)0;	// will be gen'd by R_LoadARBProgram
	progs[i].target = target;
	strncpy( progs[i].name, program, sizeof( progs[i].name ) - 1 );

	R_LoadARBProgram( i );

	return progs[i].ident;
}

/*
==================
R_ReloadARBPrograms_f
==================
*/
void R_ReloadARBPrograms_f( const idCmdArgs &args ) {
	int		i;

	common->Printf( "----- R_ReloadARBPrograms -----\n" );
	for ( i = 0 ; progs[i].name[0] ; i++ ) {
		R_LoadARBProgram( i );
	}
	common->Printf( "-------------------------------\n" );
}

/*
==================
R_ARB2_Init

==================
*/
void R_ARB2_Init( void ) {

	common->Printf( "---------- R_ARB2_Init ----------\n" );

	if ( !glConfig.ARBVertexProgramAvailable || !glConfig.ARBFragmentProgramAvailable ) {
		common->Printf( "Not available.\n" );
		return;
	}

	common->Printf( "Available.\n" );

	common->Printf( "---------------------------------\n" );
}

/*
=================
R_LoadGlslShader
=================
*/
static GLuint R_LoadGlslShader(GLenum shaderType, const char* filename) {
  assert(filename);
  assert(filename[0]);
  assert(shaderType == GL_VERTEX_SHADER || shaderType == GL_FRAGMENT_SHADER);

  idStr	fullPath = "glsl/";
  fullPath += filename;

  if(shaderType == GL_VERTEX_SHADER)
    common->Printf("load vertex shader %s\n", fullPath.c_str());
  else
    common->Printf("load fragment shader %s\n", fullPath.c_str());

  // load the program even if we don't support it, so
  // fs_copyfiles can generate cross-platform data dumps
  // 
  char	*fileBuffer = nullptr;
  fileSystem->ReadFile(fullPath.c_str(), (void **)&fileBuffer, NULL);
  if (!fileBuffer) {
    common->Printf(": File not found\n");
    return 0;
  }

  // copy to stack memory and free
  char* buffer = (char *)_alloca(strlen(fileBuffer) + 1);
  strcpy(buffer, fileBuffer);
  fileSystem->FreeFile(fileBuffer);

  GLuint shaderObject = glCreateShader(shaderType);

  const int bufferLen = strlen(buffer);

  glShaderSource(shaderObject, 1, (const GLchar**)&buffer, &bufferLen);
  glCompileShader(shaderObject);

  GLint success = 0;
  glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &success);
  if(success = GL_FALSE) {
    char buffer[1024];
    GLsizei length;
    glGetShaderInfoLog(shaderObject, sizeof(buffer)-1, &length, &buffer[0]);
    buffer[length] = '\0';


    common->Printf("failed to compile shader '%s': %s", filename, &buffer[0]);
    return 0;
  }

  return shaderObject;
}

namespace {
  // Little RAII helper to deal with detaching and deleting of shader objects
  struct GlslShader {
    GlslShader(GLuint ident)
      : ident(ident)
      , program(0)
    {}

    ~GlslShader() {      
      release();        
    }

    void release() {
      if (ident) {
        if (program) {
          glDetachShader(program, ident);
        }
        glDeleteShader(ident);
      }

      ident = 0;
      program = 0;
    }

    void attachToProgram(GLuint program) {
      this->program = program;
      glAttachShader(program, ident);
    }

    GLuint ident;
    GLuint program;
  };
}

/*
=================
R_LoadGlslProgram
=================
*/
static void R_LoadGlslProgram(glslProgramDef_t& programDef) {
  if(!programDef.vertexShaderName[0] || !programDef.fragmentShaderName[0])
    return;

  GlslShader vertexShader = R_LoadGlslShader(GL_VERTEX_SHADER, programDef.vertexShaderName);
  if (!vertexShader.ident) {
    common->Warning("failed to load GLSL vertex shader: %s", programDef.vertexShaderName);
    return;
  }

  GlslShader fragmentShader = R_LoadGlslShader(GL_FRAGMENT_SHADER, programDef.fragmentShaderName);
  if (!fragmentShader.ident) {
    common->Warning("failed to load GLSL fragment shader: %s", programDef.fragmentShaderName);
    return;
  }

  const GLuint program = glCreateProgram();
  if (!program) {
    common->Warning("failed to create GLSL program object");
    return;
  }

  vertexShader.attachToProgram(program);
  fragmentShader.attachToProgram(program);
  glLinkProgram(program);

  GLint isLinked = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
  if (isLinked == GL_FALSE) {

    char buffer[1024];
    GLsizei length;
    glGetProgramInfoLog(program, sizeof(buffer)-1, &length, &buffer[0]);
    buffer[length] = '\0';

    vertexShader.release();
    fragmentShader.release();
    glDeleteProgram(program);

    common->Warning("failed to link GLSL shaders to program: %s", &buffer[0]);
    return;
  }

  if (programDef.ident)
    glDeleteProgram(programDef.ident);

  programDef.ident = program;

  programDef.localLightOriginLocation = glGetUniformLocation(program, "rpLocalLightOrigin");
  programDef.shaderParmLocations[0] = glGetUniformLocation(program, "shaderParm0");
  programDef.shaderParmLocations[1] = glGetUniformLocation(program, "shaderParm1");
  programDef.shaderParmLocations[2] = glGetUniformLocation(program, "shaderParm2");
  programDef.shaderParmLocations[3] = glGetUniformLocation(program, "shaderParm3");
  programDef.samplerLocations[0] = glGetUniformLocation(program, "texture0");
  programDef.samplerLocations[1] = glGetUniformLocation(program, "texture1");
  programDef.samplerLocations[2] = glGetUniformLocation(program, "texture2");
  programDef.samplerLocations[3] = glGetUniformLocation(program, "texture3");
  programDef.samplerLocations[4] = glGetUniformLocation(program, "texture4");
  programDef.samplerLocations[5] = glGetUniformLocation(program, "texture5");
  programDef.samplerLocations[6] = glGetUniformLocation(program, "texture6");
  programDef.samplerLocations[7] = glGetUniformLocation(program, "texture7");
}

/*
=================
R_FindGlslProgram
=================
*/
const glslProgramDef_t* R_FindGlslProgram(const char* vertexShaderName, const char* fragmentShaderName) {
  assert(vertexShaderName && vertexShaderName[0]);
  assert(fragmentShaderName && fragmentShaderName[0]);

  const int vertexShaderNameLen = strlen(vertexShaderName);
  const int fragmentShaderNameLen = strlen(fragmentShaderName);

  int i;
  for (i = 0; i < MAX_GLPROGS; ++i) {
    const int vsLen = strlen(glslPrograms[i].vertexShaderName);
    const int fsLen = strlen(glslPrograms[i].fragmentShaderName);
    
    if(!vsLen || !fsLen)
      break;

    if(vsLen != vertexShaderNameLen || fsLen != fragmentShaderNameLen)
      continue;

    if(idStr::Icmpn(vertexShaderName, glslPrograms[i].vertexShaderName, vsLen) != 0)
      continue;

    if (idStr::Icmpn(fragmentShaderName, glslPrograms[i].fragmentShaderName, fsLen) != 0)
      continue;

    return &glslPrograms[i];
  }

  if(i >= MAX_GLPROGS) {
    common->Error("cannot create GLSL program, maximum number of programs reached");
    return nullptr;
  }

  strncpy(glslPrograms[i].vertexShaderName, vertexShaderName, vertexShaderNameLen);
  strncpy(glslPrograms[i].fragmentShaderName, fragmentShaderName, fragmentShaderNameLen);

  R_LoadGlslProgram(glslPrograms[i]);

  return &glslPrograms[i];
}


/*
=================
R_ReloadGlslPrograms_f
=================
*/
void R_ReloadGlslPrograms_f( const idCmdArgs &args ) {
  common->Printf( "----- R_ReloadGlslPrograms -----\n" );

  for(int i=0; i<MAX_GLPROGS; ++i) {
    R_LoadGlslProgram(glslPrograms[i]);
  }

  common->Printf( "----- R_ReloadGlslPrograms -----\n" );
}