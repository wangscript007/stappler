// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPDefine.h"
#include "SPLayer.h"

NS_SP_BEGIN

const cocos2d::Vec2 Gradient::Vertical(0.0f, -1.0f);
const cocos2d::Vec2 Gradient::Horizontal(1.0f, 0.0f);

Gradient Gradient::progress(const Gradient &a, const Gradient &b, float p) {
	Gradient ret;
	ret.colors[0] = stappler::progress(a.colors[0], b.colors[0], p);
	ret.colors[1] = stappler::progress(a.colors[1], b.colors[1], p);
	ret.colors[2] = stappler::progress(a.colors[2], b.colors[2], p);
	ret.colors[3] = stappler::progress(a.colors[3], b.colors[3], p);
	return ret;
}

Gradient::Gradient() {
	colors[0] = Color4B(255, 255, 255, 255);
	colors[1] = Color4B(255, 255, 255, 255);
	colors[2] = Color4B(255, 255, 255, 255);
	colors[3] = Color4B(255, 255, 255, 255);
}

Gradient::Gradient(ColorRef start, ColorRef end, const cocos2d::Vec2 &alongVector) {
	float h = alongVector.getLength();
	if (h == 0) {
		return;
	}

	float c = sqrtf(2.0f);
	cocos2d::Vec2 u(alongVector.x / h, alongVector.y / h);

	// Compressed Interpolation mode
	float h2 = 1 / ( fabsf(u.x) + fabsf(u.y) );
	u = u * (h2 * (float)c);

	cocos2d::Color4B S( start.r, start.g, start.b, start.a  );
	cocos2d::Color4B E( end.r, end.g, end.b, end.a );

    // (-1, -1)
	colors[0].r = E.r + (S.r - E.r) * ((c + u.x + u.y) / (2.0f * c));
	colors[0].g = E.g + (S.g - E.g) * ((c + u.x + u.y) / (2.0f * c));
	colors[0].b = E.b + (S.b - E.b) * ((c + u.x + u.y) / (2.0f * c));
	colors[0].a = E.a + (S.a - E.a) * ((c + u.x + u.y) / (2.0f * c));
    // (1, -1)
	colors[1].r = E.r + (S.r - E.r) * ((c - u.x + u.y) / (2.0f * c));
	colors[1].g = E.g + (S.g - E.g) * ((c - u.x + u.y) / (2.0f * c));
	colors[1].b = E.b + (S.b - E.b) * ((c - u.x + u.y) / (2.0f * c));
	colors[1].a = E.a + (S.a - E.a) * ((c - u.x + u.y) / (2.0f * c));
    // (-1, 1)
	colors[2].r = E.r + (S.r - E.r) * ((c + u.x - u.y) / (2.0f * c));
	colors[2].g = E.g + (S.g - E.g) * ((c + u.x - u.y) / (2.0f * c));
	colors[2].b = E.b + (S.b - E.b) * ((c + u.x - u.y) / (2.0f * c));
	colors[2].a = E.a + (S.a - E.a) * ((c + u.x - u.y) / (2.0f * c));
    // (1, 1)
	colors[3].r = E.r + (S.r - E.r) * ((c - u.x - u.y) / (2.0f * c));
	colors[3].g = E.g + (S.g - E.g) * ((c - u.x - u.y) / (2.0f * c));
	colors[3].b = E.b + (S.b - E.b) * ((c - u.x - u.y) / (2.0f * c));
	colors[3].a = E.a + (S.a - E.a) * ((c - u.x - u.y) / (2.0f * c));
}

Gradient::Gradient(ColorRef bl, ColorRef br, ColorRef tl, ColorRef tr) {
	colors[0] = bl;
	colors[1] = br;
	colors[2] = tl;
	colors[3] = tr;
}

bool Layer::init(const cocos2d::Color4B &c) {
	if (!DynamicBatchNode::init()) {
		return false;
	}

	_stencil = true;

	setColor(cocos2d::Color3B(c));
	setOpacity(c.a);

	updateBlendFunc(nullptr);

	return true;
}

bool Layer::init(const Gradient &grad) {
	if (!DynamicBatchNode::init()) {
		return false;
	}

	_stencil = true;

	setColor(Color3B(255, 255, 255));
	setOpacity(255);
	setGradient(grad);

	updateBlendFunc(nullptr);

	return true;
}

void Layer::onContentSizeDirty() {
	DynamicBatchNode::onContentSizeDirty();
	updateSprites();
}

void Layer::setGradient(const Gradient &g) {
	_gradient = g;
	_contentSizeDirty = true;
}

const Gradient &Layer::getGradient() const {
	return _gradient;
}

void Layer::updateSprites() {
	cocos2d::Color4B color[4];
	for (int i = 0; i < 4; i++) {
		color[i] = cocos2d::Color4B(
				_displayedColor.r * _gradient.colors[i].r / 255,
				_displayedColor.g * _gradient.colors[i].g / 255,
				_displayedColor.b * _gradient.colors[i].b / 255,
				_displayedOpacity * _gradient.colors[i].a / 255 );
		if (_opacityModifyRGB) {
			color[i].r = color[i].r * _displayedOpacity / 255;
			color[i].g = color[i].g * _displayedOpacity / 255;
			color[i].b = color[i].b * _displayedOpacity / 255;
	    }
	}

	_quads->resize(1);
	_quads->setTextureRect(0, cocos2d::Rect(0.0f, 0.0f, 1.0f, 1.0f), 1.0f, 1.0f, false, false);
	_quads->setGeometry(0, cocos2d::Vec2(0.0f, 0.0f), _contentSize, _positionZ);
	_quads->setColor(0, color);
	_quads->shrinkToFit();
}

void Layer::updateColor() {
	if (!_quads->empty()) {
		cocos2d::Color4B color[4];
		for (int i = 0; i < 4; i++) {
			color[i] = cocos2d::Color4B(
					_displayedColor.r * _gradient.colors[i].r / 255,
					_displayedColor.g * _gradient.colors[i].g / 255,
					_displayedColor.b * _gradient.colors[i].b / 255,
					_displayedOpacity * _gradient.colors[i].a / 255 );
			if (_opacityModifyRGB) {
				color[i].r = color[i].r * _displayedOpacity / 255;
				color[i].g = color[i].g * _displayedOpacity / 255;
				color[i].b = color[i].b * _displayedOpacity / 255;
		    }
		}

		for (size_t i = 0; i < _quads->size(); i++) {
			_quads->setColor(i, color);
		}
	}
}

NS_SP_END
