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

#include "Framebuffer.h"
#include "Image.h"
#include "tr_local.h"

fhFramebuffer* fhFramebuffer::currentDrawBuffer = nullptr;
fhFramebuffer* fhFramebuffer::shadowmapFramebuffer = nullptr;
fhFramebuffer* fhFramebuffer::defaultFramebuffer = nullptr;
fhFramebuffer* fhFramebuffer::renderFramebuffer = nullptr;
fhFramebuffer* fhFramebuffer::currentDepthFramebuffer = nullptr;
fhFramebuffer* fhFramebuffer::currentRenderFramebuffer = nullptr;

fhFramebuffer::fhFramebuffer( int w, int h, idImage* color, idImage* depth ) {
	width = w;
	height = h;
	name = (!color && !depth) ? 0 : -1;
	colorAttachment = color;
	depthAttachment = depth;
	samples = 1;
}

static const char* GetFrameBufferStatusMessage( int status ) {
	switch (status) {
	case GL_FRAMEBUFFER_UNDEFINED:
		return "GL_FRAMEBUFFER_UNDEFINED";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
	case GL_FRAMEBUFFER_UNSUPPORTED:
		return "GL_FRAMEBUFFER_UNSUPPORTED";
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
		return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
	case GL_FRAMEBUFFER_COMPLETE:
		return nullptr;
	default:
		return "FRAMEBUFFER_STATUS_UNKNOWN";
	}
}

void fhFramebuffer::Bind() {
	if (currentDrawBuffer == this) {
		return;
	}

	if (name == -1) {
		Allocate();
	}

	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, name );
	currentDrawBuffer = this;

	SetDrawBuffer();
}

bool fhFramebuffer::IsDefault() const {
	return (name == 0);
}

void fhFramebuffer::Resize( int width, int height, int samples ) {
	if (!colorAttachment && !depthAttachment) {
		return;
	}

	if (this->width == width && this->height == height && this->samples == samples) {
		return;
	}


	if (currentDrawBuffer == this) {
		defaultFramebuffer->Bind();
	}

	Purge();

	this->width = width;
	this->height = height;
	this->samples = samples;
}

int fhFramebuffer::GetWidth() const {
	if (!colorAttachment && !depthAttachment) {
		return glConfig.vidWidth;
	}
	return width;
}

int fhFramebuffer::GetHeight() const {
	if (!colorAttachment && !depthAttachment) {
		return glConfig.vidHeight;
	}
	return height;
}

int fhFramebuffer::GetSamples() const {
	if (!colorAttachment && !depthAttachment) {
		return 1;
	}
	return samples;
}

void fhFramebuffer::Purge() {
	if (name != 0 && name != -1) {
		glDeleteFramebuffers( 1, &name );
		name = -1;
	}
}

void fhFramebuffer::SetDrawBuffer() {
	if (!colorAttachment && !depthAttachment) {
		glDrawBuffer( GL_BACK );
	}
	else if (!colorAttachment) {
		glDrawBuffer( GL_NONE );
	}
	else {
		glDrawBuffer( GL_COLOR_ATTACHMENT0 );
	}
}

void fhFramebuffer::Allocate() {
	glGenFramebuffers( 1, &name );
	glBindFramebuffer( GL_DRAW_FRAMEBUFFER, name );
	currentDrawBuffer = this;

	if (colorAttachment) {
		colorAttachment->AllocateMultiSampleStorage( pixelFormat_t::RGBA, width, height, samples );
		colorAttachment->filter = TF_LINEAR;
		colorAttachment->repeat = TR_CLAMP;
		colorAttachment->SetImageFilterAndRepeat();
		glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorAttachment->texnum, 0 );
	}

	if (depthAttachment) {
		depthAttachment->AllocateMultiSampleStorage( pixelFormat_t::DEPTH_24, width, height, samples );
		depthAttachment->filter = TF_LINEAR;
		depthAttachment->repeat = TR_CLAMP;
		depthAttachment->SetImageFilterAndRepeat();
		glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthAttachment->texnum, 0 );
	}

	auto status = GetFrameBufferStatusMessage( glCheckFramebufferStatus( GL_DRAW_FRAMEBUFFER ) );
	if (status) {
		common->Warning( "failed to generate framebuffer: %s", status );
		name = 0;
	}
}

void fhFramebuffer::Init() {
	shadowmapFramebuffer = new fhFramebuffer( 1024 * 4, 1024 * 4, nullptr, globalImages->shadowmapImage );
	defaultFramebuffer = new fhFramebuffer( 0, 0, nullptr, nullptr );
	renderFramebuffer = new fhFramebuffer( 1024, 1024, globalImages->renderColorImage, globalImages->renderDepthImage );
	currentDepthFramebuffer = new fhFramebuffer( 1024, 1024, nullptr, globalImages->currentDepthImage );
	currentRenderFramebuffer = new fhFramebuffer( 1024, 1024, globalImages->currentRenderImage, nullptr );
	currentDrawBuffer = defaultFramebuffer;
}

void fhFramebuffer::PurgeAll() {
	defaultFramebuffer->Bind();

	shadowmapFramebuffer->Purge();
	defaultFramebuffer->Purge();
	renderFramebuffer->Purge();
	currentDepthFramebuffer->Purge();
	currentRenderFramebuffer->Purge();
}

void fhFramebuffer::BlitColor( fhFramebuffer* source, fhFramebuffer* dest ) {
	Blit( source, 0, 0, source->GetWidth(), source->GetHeight(),
		dest, 0, 0, dest->GetWidth(), dest->GetHeight(),
		GL_COLOR_BUFFER_BIT, TF_LINEAR );
}

void fhFramebuffer::BlitColor( fhFramebuffer* source, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight, fhFramebuffer* dest ) {
	Blit( source, sourceX, sourceY, sourceWidth, sourceHeight,
		dest, 0, 0, dest->GetWidth(), dest->GetHeight(),
		GL_COLOR_BUFFER_BIT, TF_LINEAR );
}

void fhFramebuffer::BlitDepth( fhFramebuffer* source, fhFramebuffer* dest ) {
	Blit( source, 0, 0, source->GetWidth(), source->GetHeight(),
		dest, 0, 0, dest->GetWidth(), dest->GetHeight(),
		GL_DEPTH_BUFFER_BIT, TF_NEAREST );
}

void fhFramebuffer::Blit(
	fhFramebuffer* source, uint32 sourceX, uint32 sourceY, uint32 sourceWidth, uint32 sourceHeight,
	fhFramebuffer* dest, uint32 destX, uint32 destY, uint32 destWidth, uint32 destHeight,
	GLint bufferMask, textureFilter_t filter ) {

	if (source == dest) {
		return;
	}

	fhFramebuffer* currentDrawBuffer = fhFramebuffer::GetCurrentDrawBuffer();

	if (dest->name == -1) {
		dest->Allocate();
	}

	glBlitNamedFramebuffer( source->name,
		dest->name,
		sourceX, sourceY, sourceWidth, sourceHeight,
		destX, destY, destWidth, destHeight,
		bufferMask,
		filter == TF_LINEAR ? GL_LINEAR : GL_NEAREST );

	currentDrawBuffer->Bind();
}
