/**
Copyright (c) 2018 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#include "SPDrawGLRenderSurface.h"
#include "SPThreadManager.h"
#include "renderer/ccGLStateCache.h"
#include "base/CCConfiguration.h"
#include "SPTextureCache.h"

NS_SP_EXT_BEGIN(draw)

static void updateUniformColor(cocos2d::GLProgram *p, cocos2d::Texture2D::PixelFormat internal,
		cocos2d::Texture2D::PixelFormat reference, const Color4B &color) {

	switch (internal) {
	case cocos2d::Texture2D::PixelFormat::R8:
		switch (reference) {
		case cocos2d::Texture2D::PixelFormat::I8:
			p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
					((color.r * 299 + color.g * 587 + color.b * 114 + 500) / 1000) / 255.0f, 0.0f, 0.0f, 1.0f);
			break;
		case cocos2d::Texture2D::PixelFormat::A8:
			p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
					color.a / 255.0f, color.a / 255.0f, color.a / 255.0f, 1.0f);
			break;
		default:
			p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
					color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
			break;
		}
		break;
	case cocos2d::Texture2D::PixelFormat::RG88:
		if (reference == cocos2d::Texture2D::PixelFormat::AI88) {
			p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
					((color.r * 299 + color.g * 587 + color.b * 114 + 500) / 1000) / 255.0f, color.a / 255.0f, 0.0f, 1.0f);
			break;
		} else {
			p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
					color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
		}
		break;
	default:
		p->setUniformLocationWith4f(p->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_AMBIENT_COLOR),
				color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
	}
}


GLRenderSurface::GLRenderSurface() {
	onEvent(Device::onRegenerateResources, [this] (const Event &) {
		resetSurface();
	});
}

GLRenderSurface::~GLRenderSurface() {
	finalizeSurface();
}

bool GLRenderSurface::initializeSurface(StencilDepthFormat fmt) {
	_depthFormat = fmt;

	_clearFlags = GL_COLOR_BUFFER_BIT;
	switch (_depthFormat) {
	case StencilDepthFormat::Depth16:
		_clearFlags |= GL_DEPTH_BUFFER_BIT;
		break;
	case StencilDepthFormat::Depth24Stencil8:
		_clearFlags |= (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		break;
	case StencilDepthFormat::Stencil8:
		_clearFlags |= GL_STENCIL_BUFFER_BIT;
		break;
	default: break;
	}

	glGenFramebuffers(1, &_fbo);
	if (_depthFormat != StencilDepthFormat::None) {
		glGenRenderbuffers(1, &_rbo);
	}
	glGenBuffers(3, _vbo);
	if (_fbo != 0 && _vbo[0] != 0 && _vbo[1] != 0 && _vbo[2] != 0) {
		return true;
	}

	return false;
}

void GLRenderSurface::finalizeSurface() {
	if (_vbo[0] || _vbo[1] || _vbo[2]) {
		glDeleteBuffers(3, _vbo);
	}

	if (_rbo) {
		glDeleteRenderbuffers(1, &_rbo);
		_rbo = 0;
	}

	if (_fbo) {
		glDeleteFramebuffers(1, &_fbo);
		_fbo = 0;
	}
}

void GLRenderSurface::dropSurface() {
	_vbo[0] = 0;
	_vbo[1] = 0;
	_vbo[2] = 0;

	_rbo = 0;
	_fbo = 0;
}

void GLRenderSurface::resetSurface() {
	dropSurface();
	initializeSurface(_depthFormat);
	cleanup();
}

bool GLRenderSurface::begin(cocos2d::Texture2D *tex, const Color4B &color, bool clear) {
	load();
	auto w = tex->getPixelsWide();
	auto h = tex->getPixelsHigh();
	_internalFormat = tex->getPixelFormat();
	_referenceFormat = tex->getReferenceFormat();
	_premultipliedAlpha = tex->hasPremultipliedAlpha();
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_oldFbo);

	if (ThreadManager::getInstance()->isMainThread()) {
		cocos2d::GL::useProgram(0);
		cocos2d::GL::enableVertexAttribs(0);
		cocos2d::GL::bindTexture2D(0);
		cocos2d::GL::bindVAO(0);
	}

	_valid = doUpdateAttachments(tex, w, h);

	if (_valid) {
		glViewport(0, 0, (GLsizei)_width, (GLsizei)_height);

		if (clear) {
			doSafeClear(color);
		}

		int32_t gcd = sp_gcd(w, h);
		int32_t dw = (int32_t)w / gcd;
		int32_t dh = (int32_t)h / gcd;
		int32_t dwh = gcd * dw * dh;

		float mod = 1.0f;
		while (dwh * mod > 8_KiB) {
			mod /= 2.0f;
		}

		_viewTransform = Mat4::IDENTITY;
		_viewTransform.scale(dh * mod, dw * mod, -1.0);
		_viewTransform.m[12] = -dwh * mod / 2.0f;
		_viewTransform.m[13] = -dwh * mod / 2.0f;
		_viewTransform.m[14] = dwh * mod / 2.0f - 1;
		_viewTransform.m[15] = dwh * mod / 2.0f + 1;
		_viewTransform.m[11] = -1.0f;

		auto desc = GLProgramDesc( GLProgramDesc::Attr::MatrixMVP | GLProgramDesc::Attr::Color | GLProgramDesc::Attr::MediumP,
				_internalFormat, _referenceFormat );

		_drawProgram = TextureCache::getInstance()->getPrograms()->getProgram(desc);

		_uniformColor = Color4B(0, 0, 0, 0);
		_uniformTransform = Mat4::ZERO;

	    CHECK_GL_ERROR_DEBUG();
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, _oldFbo);
	}

	return _valid;
}

void GLRenderSurface::end() {
	if (_valid) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, _oldFbo);
	    CHECK_GL_ERROR_DEBUG();
	}

	_valid = false;

	cleanup();
}

void GLRenderSurface::flushVectorCanvas(layout::Canvas *canvas) {
	if (!_valid) {
		return;
	}

	auto &tess = canvas->getTess();
	auto &stroke = canvas->getStroke();
	auto &transform = canvas->getTransform();

	Time t = Time::now();

	if (!tess.empty()) {
		TESSResult * res = tessVecResultTriangles((TESStesselator **)tess.data(), int(tess.size()));
		if (res && (res->contour.vertexCount > 0 || res->triangles.vertexCount > 0)) {
			auto verts = res->contour.vertexBuffer;
			auto nverts = res->contour.vertexCount;

			auto elts = res->contour.elementsBuffer;
			auto nelts = res->contour.elementCount;

			bool aa = (nelts > 0);

			t = Time::now();

			bindTexture(0);
			useProgram(_drawProgram->getProgram());
			setUniformTransform(transform);

			glBindBuffer(GL_ARRAY_BUFFER, _vbo[0]);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vbo[2]);

			size_t vertexBufferOffset = aa ? ((nverts * sizeof(TESSPoint) + 0xF) & ~0xF) : 0;
			size_t vertexBufferSize = aa ? (vertexBufferOffset + ((res->triangles.vertexCount * sizeof(TESSPoint) + 0xF) & ~0xF)) : (res->triangles.vertexCount * sizeof(TESSPoint));
			size_t indexBufferSize = sizeof(TESSshort) * res->triangles.elementCount * 3;

			glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, nullptr, GL_STREAM_DRAW);
			if (aa) {
				glBufferSubData(GL_ARRAY_BUFFER, 0,  nverts * sizeof(TESSPoint), verts);
				glBufferSubData(GL_ARRAY_BUFFER, vertexBufferOffset, res->triangles.vertexCount * sizeof(TESSPoint), res->triangles.vertexBuffer);
			} else {
				glBufferSubData(GL_ARRAY_BUFFER, vertexBufferOffset, vertexBufferSize, res->triangles.vertexBuffer);
			}
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, res->triangles.elementsBuffer, GL_STREAM_DRAW);

			enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION | cocos2d::GL::VERTEX_ATTRIB_FLAG_COLOR);

			if (aa) {
				setStrokeBlending();
				glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(TESSPoint), 0);
				glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
						sizeof(TESSPoint), (GLvoid*) offsetof(TESSPoint, c));

				glEnable(GL_STENCIL_TEST);
				glStencilMask(1);
				glClear(GL_STENCIL_BUFFER_BIT);
				glStencilFunc(GL_ALWAYS, 1, 1);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

				for (int i = 0; i < nelts; ++ i) {
					auto pts = elts[2 * i];
					auto npts = elts[2 * i + 1];

					glDrawArrays(GL_TRIANGLE_STRIP, pts, npts);
				}

				glStencilFunc(GL_NOTEQUAL, 1, 1);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

				LOG_GL_ERROR();
			}

			elts = res->triangles.elementsBuffer;
			nelts = res->triangles.elementCount;

			setFillBlending();
			glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(TESSPoint), (GLvoid*)vertexBufferOffset);
			glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE,
					sizeof(TESSPoint), (GLvoid*) (vertexBufferOffset + offsetof(TESSPoint, c)));

			glDrawElements(GL_TRIANGLES, (GLsizei) nelts * 3, GL_UNSIGNED_SHORT, (GLvoid *) (0));

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			if (aa) {
				glDisable(GL_STENCIL_TEST);
			}

			LOG_GL_ERROR();
		} else {
			log::text("Canvas", "fail to tesselate contour");
		}
	}

	if (!stroke.empty()) {
		bindTexture(0);
		useProgram(_drawProgram->getProgram());
		setUniformTransform(transform);

		size_t size = 0;
		size_t offset = 0;
		for (auto & it : stroke) {
			size += it.outline.size();
			if (it.antialiased) {
				size += it.inner.size();
				size += it.outer.size();
			}
		}

		auto vertexBufferSize = (size * sizeof(TESSPoint));

		setStrokeBlending();
		glBindBuffer(GL_ARRAY_BUFFER, _vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, nullptr, GL_DYNAMIC_DRAW);

		for (auto & it : stroke) {
			auto s = it.outline.size() * sizeof(TESSPoint);
			glBufferSubData(GL_ARRAY_BUFFER, offset, s, it.outline.data());
			offset += s;
			if (it.antialiased) {
				s = it.inner.size() * sizeof(TESSPoint);
				glBufferSubData(GL_ARRAY_BUFFER, offset, s, it.inner.data());
				offset += s;

				s = it.outer.size() * sizeof(TESSPoint);
				glBufferSubData(GL_ARRAY_BUFFER, offset, s, it.outer.data());
				offset += s;
			}
		}

		enableVertexAttribs(cocos2d::GL::VERTEX_ATTRIB_FLAG_POSITION | cocos2d::GL::VERTEX_ATTRIB_FLAG_COLOR);
		glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(TESSPoint), 0);
		glVertexAttribPointer(cocos2d::GLProgram::VERTEX_ATTRIB_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TESSPoint), (GLvoid*) offsetof(TESSPoint, c));

		offset = 0;
		for (auto & it : stroke) {
			if (!it.antialiased) {
				auto s = it.outline.size();
				glDrawArrays(GL_TRIANGLE_STRIP, GLint(offset), GLint(s));
				offset += s;
			} else {
				glEnable(GL_STENCIL_TEST);
				glStencilMask(1);
				glClear(GL_STENCIL_BUFFER_BIT);
				glStencilFunc(GL_ALWAYS, 1, 1);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

				auto s = it.outline.size();
				glDrawArrays(GL_TRIANGLE_STRIP, GLint(offset), GLint(s));
				offset += s;

				glStencilFunc(GL_NOTEQUAL, 1, 1);
				glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

				s = it.inner.size();
				glDrawArrays(GL_TRIANGLE_STRIP, GLint(offset), GLint(s));
				offset += s;

				s = it.outer.size();
				glDrawArrays(GL_TRIANGLE_STRIP, GLint(offset), GLint(s));
				offset += s;

				glDisable(GL_STENCIL_TEST);
			}
			CHECK_GL_ERROR_DEBUG();
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}

bool GLRenderSurface::isValid() const {
	return _valid;
}

Bitmap GLRenderSurface::read(uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
	bool truncate = false;
	Bitmap::PixelFormat fmt, targetFmt;
	Bitmap::Alpha alpha = Bitmap::Alpha::Opaque;
	GLenum readFmt;
	switch (_internalFormat) {
	case cocos2d::Texture2D::PixelFormat::A8:
		targetFmt = fmt = Bitmap::PixelFormat::A8;
		alpha = Bitmap::Alpha::Unpremultiplied;
		readFmt = GL_ALPHA;
		break;
	case cocos2d::Texture2D::PixelFormat::I8:
		if (cocos2d::Configuration::isRenderTargetSupported(cocos2d::Configuration::RenderTarget::R8)) {
			targetFmt = fmt = Bitmap::PixelFormat::I8;
			readFmt = GL_RED_EXT;
		} else {
			targetFmt = Bitmap::PixelFormat::I8;
			fmt = Bitmap::PixelFormat::RGB888;
			readFmt = GL_RGB;
		}
		break;
	case cocos2d::Texture2D::PixelFormat::R8:
		if (cocos2d::Configuration::isRenderTargetSupported(cocos2d::Configuration::RenderTarget::R8)) {
			targetFmt = fmt = Bitmap::PixelFormat::A8;
			readFmt = GL_RED_EXT;
		} else {
			targetFmt = Bitmap::PixelFormat::A8;
			fmt = Bitmap::PixelFormat::RGB888;
			readFmt = GL_RGB;
			truncate = true;
		}
		break;
	case cocos2d::Texture2D::PixelFormat::AI88:
		targetFmt = Bitmap::PixelFormat::IA88;
		fmt = Bitmap::PixelFormat::RGBA8888;
		readFmt = GL_RGBA;
		alpha = Bitmap::Alpha::Unpremultiplied;
		break;
	case cocos2d::Texture2D::PixelFormat::RG88:
		if (cocos2d::Configuration::isRenderTargetSupported(cocos2d::Configuration::RenderTarget::RG8)) {
			targetFmt = fmt = Bitmap::PixelFormat::IA88;
			alpha = Bitmap::Alpha::Unpremultiplied;
			readFmt = GL_RG_EXT;
		} else {
			targetFmt = Bitmap::PixelFormat::IA88;
			fmt = Bitmap::PixelFormat::RGB888;
			readFmt = GL_RGB;
			alpha = Bitmap::Alpha::Unpremultiplied;
			truncate = true;
		}
		break;
	case cocos2d::Texture2D::PixelFormat::RGBA8888:
	case cocos2d::Texture2D::PixelFormat::RGBA4444:
	case cocos2d::Texture2D::PixelFormat::RGB5A1:
		targetFmt = fmt = Bitmap::PixelFormat::RGBA8888;
		alpha = Bitmap::Alpha::Unpremultiplied;
		readFmt = GL_RGBA;
		break;
	case cocos2d::Texture2D::PixelFormat::RGB565:
	case cocos2d::Texture2D::PixelFormat::RGB888:
		targetFmt = fmt = Bitmap::PixelFormat::RGB888;
		readFmt = GL_RGB;
		break;
	default:
		log::format("draw::Canvas", "Unsupported texture format: %d", (int)_internalFormat);
		return Bitmap();
		break;
	}

	if (alpha != Bitmap::Alpha::Opaque) {
		if (_premultipliedAlpha) {
			alpha = Bitmap::Alpha::Premultiplied;
		}
	}

	Bitmap ret;
	ret.alloc(_width, _height, fmt, alpha);
	glReadPixels(x, y, w, h, readFmt, GL_UNSIGNED_BYTE, (GLvoid *)ret.dataPtr());

	if (targetFmt != fmt) {
		if (truncate) {
			ret.truncate(targetFmt);
		} else {
			ret.convert(targetFmt);
		}
	}

	return ret;
}

Bitmap GLRenderSurface::read() {
	return read(0, 0, _width, _height);
}

void GLRenderSurface::setUniformColor(const Color4B &color) {
	if (_uniformColor != color) {
		_uniformColor = color;
		updateUniformColor(_drawProgram, _internalFormat, _referenceFormat, color);
	}
}

void GLRenderSurface::setUniformTransform(const Mat4 &t) {
	if (_uniformTransform != t) {
		_uniformTransform = t;
		Mat4 mv = _viewTransform * t;
		_drawProgram->setUniformLocationWithMatrix4fv(_drawProgram->getUniformLocationForName(cocos2d::GLProgram::UNIFORM_MVP_MATRIX), mv.m, 1);
		LOG_GL_ERROR();
	}
}

void GLRenderSurface::setFillBlending() {
	blendFunc(BlendFunc::ALPHA_NON_PREMULTIPLIED);
}
void GLRenderSurface::setStrokeBlending() {
	switch (_referenceFormat) {
	case cocos2d::Texture2D::PixelFormat::A8:
	case cocos2d::Texture2D::PixelFormat::R8:
		switch (_internalFormat) {
		case cocos2d::Texture2D::PixelFormat::R8:
			setBlending(GLBlending(BlendFunc::ALPHA_NON_PREMULTIPLIED, GLBlending::Max));
			break;
		default:
			setBlending(GLBlending(BlendFunc::ALPHA_PREMULTIPLIED, GLBlending::FuncAdd, GLBlending::Max));
		}
		break;
	default:
		setBlending(GLBlending(BlendFunc::ALPHA_NON_PREMULTIPLIED, GLBlending::FuncAdd, GLBlending::Max));
		break;
	}
}

void GLRenderSurface::doSafeClear(const Color4B &color) {
    GLfloat oldClearColor[4] = {0.0f};
    GLfloat oldDepthClearValue = 0.0f;
    GLint oldStencilClearValue = 0;

    // backup and set
    if (_clearFlags & GL_COLOR_BUFFER_BIT) {
        glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor);
		glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
    }

    if (_clearFlags & GL_DEPTH_BUFFER_BIT) {
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &oldDepthClearValue);
        glClearDepth(0.0f);
    }

    if (_clearFlags & GL_STENCIL_BUFFER_BIT) {
        glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &oldStencilClearValue);
        glClearStencil(0);
    }

	glClear(_clearFlags);

    if (_clearFlags & GL_COLOR_BUFFER_BIT) {
        glClearColor(oldClearColor[0], oldClearColor[1], oldClearColor[2], oldClearColor[3]);
    }
    if (_clearFlags & GL_DEPTH_BUFFER_BIT) {
        glClearDepth(oldDepthClearValue);
    }
    if (_clearFlags & GL_STENCIL_BUFFER_BIT) {
        glClearStencil(oldStencilClearValue);
    }
}

bool GLRenderSurface::doUpdateAttachments(cocos2d::Texture2D *tex, uint32_t w, uint32_t h) {
	if (_rbo && (uint32_t(w) != _width || uint32_t(h) != _height)) {
		GLint oldRbo = 0;
		glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldRbo);
		_width = w;
		_height = h;
		glBindRenderbuffer(GL_RENDERBUFFER, _rbo);
		GLenum fmt = GL_STENCIL_INDEX8;
		switch (_depthFormat) {
		case StencilDepthFormat::Depth16: fmt = GL_DEPTH_COMPONENT16; break;
		case StencilDepthFormat::Depth24Stencil8: fmt = GL_DEPTH24_STENCIL8; break;
		default: break;
		}
		glRenderbufferStorage(GL_RENDERBUFFER, fmt, _width, _height);
		glBindRenderbuffer(GL_RENDERBUFFER, oldRbo);
	    CHECK_GL_ERROR_DEBUG();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex->getName(), 0);

    if (_clearFlags & GL_DEPTH_BUFFER_BIT) {
    	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _rbo);
    }

    if (_clearFlags & GL_STENCIL_BUFFER_BIT) {
    	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _rbo);
    }

    CHECK_GL_ERROR_DEBUG();

	auto check = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (check) {
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		log::format("Framebuffer", "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		log::format("Framebuffer", "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		log::format("Framebuffer", "GL_FRAMEBUFFER_UNSUPPORTED");
		break;
#ifndef GL_ES_VERSION_2_0
	case GL_FRAMEBUFFER_UNDEFINED:
		log::format("Framebuffer", "GL_FRAMEBUFFER_UNDEFINED");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
		log::format("Framebuffer", "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");
		break;
#endif
	case GL_FRAMEBUFFER_COMPLETE:
		return true;
		break;
	case 0:
		log::format("Framebuffer", "Success");
		break;
	default:
		log::format("Framebuffer", "Undefined %d", check);
		break;
	}
	return false;
}

NS_SP_EXT_END(draw)
