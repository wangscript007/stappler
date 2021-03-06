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

#include "Material.h"
#include "MaterialResize.h"
#include "2d/CCNode.h"
#include "2d/CCTweenFunction.h"

NS_MD_BEGIN

bool ResizeTo::init(float duration, const Size &size) {
	if (!ActionInterval::initWithDuration(duration)) {
		return false;
	}

	_targetSize = size;
	return true;
}

void ResizeTo::startWithTarget(cocos2d::Node *t) {
	ActionInterval::startWithTarget(t);
	if (t) {
		_sourceSize = t->getContentSize();
	}
}

void ResizeTo::update(float time) {
	if (_target) {
		float w = 0.0f, h = 0.0f;
		if (_sourceSize.width != _targetSize.width && _sourceSize.height != _targetSize.height) {
			if (_sourceSize.width > _targetSize.width) {
				w = _sourceSize.width + (_targetSize.width - _sourceSize.width) * cocos2d::tweenfunc::quadraticIn(time);
			} else {
				w = _sourceSize.width + (_targetSize.width - _sourceSize.width) * cocos2d::tweenfunc::quadraticOut(time);
			}

			if (_sourceSize.height > _targetSize.height) {
				h = _sourceSize.height + (_targetSize.height - _sourceSize.height) * cocos2d::tweenfunc::quadraticOut(time);
			} else {
				h = _sourceSize.height + (_targetSize.height - _sourceSize.height) * cocos2d::tweenfunc::quadraticIn(time);
			}
		} else if (_sourceSize.width == _targetSize.width) {
			w = _sourceSize.width + (_targetSize.width - _sourceSize.width) * time;
			h = _sourceSize.height + (_targetSize.height - _sourceSize.height) * cocos2d::tweenfunc::quadraticInOut(time);
		} else {
			w = _sourceSize.width + (_targetSize.width - _sourceSize.width) * cocos2d::tweenfunc::quadraticInOut(time);
			h = _sourceSize.height + (_targetSize.height - _sourceSize.height) * time;
		}

		_target->setContentSize(Size(w, h));
	}
}

bool ResizeBy::init(float duration, const Size &size) {
	if (!ActionInterval::initWithDuration(duration)) {
		return false;
	}

	_additionalSize = size;
	return true;
}

void ResizeBy::startWithTarget(cocos2d::Node *t) {
	ActionInterval::startWithTarget(t);
	if (t) {
		_sourceSize = t->getContentSize();
	}
}

void ResizeBy::update(float time) {
	if (_target) {
		_target->setContentSize(_sourceSize + (_additionalSize) * time);
	}
}

NS_MD_END
