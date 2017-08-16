/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 2016 Johannes Ohlemacher (http://github.com/eXistence/fhDOOM)

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

#pragma once
#include <type_traits>


class fhBaseRenderList {
public:
	static void Init();
	static void Shutdown();
	static void EndFrame();
protected:
	template<typename T>
	T* AllocateArray(uint32 num) {
		static_assert(std::is_trivial<T>::value, "T must be trivial");
		return static_cast<T*>(AllocateBytes(sizeof(T) * num));
	}

private:
	void* AllocateBytes(uint32 bytes);
};


template<typename T>
class fhRenderList : public fhBaseRenderList {
public:
	fhRenderList()
		: array(nullptr)
		, capacity(0)
		, size(0) {
	}

	void Append(const T& t) {
		if(size == capacity) {
			capacity += 1024;
			T* t = AllocateArray<T>(capacity);

			if (size > 0) {
				memcpy(t, array, sizeof(T) * size);
			}

			array = t;
		}

		assert(size < capacity);
		array[size] = t;
		++size;
	}

	void Clear() {
		size = 0;
	}

	int Num() const {
		return size;
	}

	bool IsEmpty() const {
		return Num() == 0;
	}

	const T& operator[](int i) const {
		assert(i < size);
		return array[i];
	}

private:
	T*  array;
	int capacity;
	int size;
};

struct drawDepth_t {
	const drawSurf_t*  surf;
	idImage*           texture;
	idVec4             textureMatrix[2];
	idVec4             color;
	float              polygonOffset;
	bool               isSubView;
	float              alphaTestThreshold;
};

struct drawShadow_t {
	const srfTriangles_t* tris;
	const idRenderEntityLocal* entity;
	idImage*           texture;
	idVec4             textureMatrix[2];
	bool               hasTextureMatrix;
	float              alphaTestThreshold;
	unsigned           visibleFlags;
};

struct drawStage_t {
	const drawSurf_t* surf;
	const fhRenderProgram*  program;
	idImage*          textures[4];
	idCinematic*      cinematic;
	float             textureMatrix[16];
	bool              hasBumpMatrix;
	idVec4            bumpMatrix[2];
	depthBlendMode_t  depthBlendMode;
	float             depthBlendRange;
	stageVertexColor_t vertexColor;
	float             polygonOffset;
	idVec4            localViewOrigin;
	idVec4            diffuseColor;
	cullType_t        cullType;
	int               drawStateBits;
	idVec4            shaderparms[4];
	int               numShaderparms;
	fhVertexLayout    vertexLayout;
};

static_assert(std::is_trivial<drawDepth_t>::value, "must be trivial");
static_assert(std::is_trivial<drawShadow_t>::value, "must be trivial");
static_assert(std::is_trivial<drawStage_t>::value, "must be trivial");
static_assert(std::is_trivial<drawInteraction_t>::value, "must be trivial");

class DepthRenderList : public fhRenderList<drawDepth_t> {
public:
	void AddDrawSurfaces( drawSurf_t **surf, int numDrawSurfs );
	void Submit();
};

class StageRenderList : public fhRenderList<drawStage_t> {
public:
	void AddDrawSurfaces( drawSurf_t **surf, int numDrawSurfs );
	void Submit();
};

class InteractionList : public fhRenderList<drawInteraction_t> {
public:
	void AddDrawSurfacesOnLight(const drawSurf_t *surf);
	void Submit();
};

class ShadowRenderList : public fhRenderList<drawShadow_t> {
public:
	ShadowRenderList();
	void AddInteractions( viewLight_t* vlight, const shadowMapFrustum_t* shadowFrustrums, int numShadowFrustrums );
	void Submit( const float* shadowViewMatrix, const float* shadowProjectionMatrix, int side, int lod ) const;
private:
	void AddSurfaceInteraction( const idRenderEntityLocal *entityDef, const srfTriangles_t *tri, const idMaterial* material, unsigned visibleSides );
	idRenderEntityLocal dummy;
};

int  RB_GLSL_CreateStageRenderList( drawSurf_t **drawSurfs, int numDrawSurfs, StageRenderList& renderlist, int maxSort );
void RB_GLSL_SubmitStageRenderList( const StageRenderList& renderlist );