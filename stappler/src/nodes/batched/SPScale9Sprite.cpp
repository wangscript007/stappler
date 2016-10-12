//
//  SPScale9Sprite.cpp
//  stappler
//
//  Created by SBKarr on 2/19/14.
//  Copyright (c) 2014 SBKarr. All rights reserved.
//

#include "SPDefine.h"
#include "SPScale9Sprite.h"
#include "2d/CCSprite.h"
#include "SPFilesystem.h"

USING_NS_SP;

bool Scale9Sprite::init(cocos2d::Texture2D *tex, cocos2d::Rect capInsets) {
	return init(tex, cocos2d::Rect::ZERO, capInsets);
}

bool Scale9Sprite::init(cocos2d::Texture2D *tex, float insetLeft, float insetTop, float insetRight, float insetBottom) {
	return init(tex, cocos2d::Rect::ZERO, cocos2d::Rect(insetLeft, insetBottom, insetRight, insetTop));
}

bool Scale9Sprite::init(cocos2d::Texture2D *tex, cocos2d::Rect rect, cocos2d::Rect capInsets) {
	if (!DynamicBatchNode::init(tex)) {
		return false;
	}

	if (rect.equals(cocos2d::Rect::ZERO)) {
		rect.size = getTexture()->getContentSize();
	}
	_textureRect = rect;
	_capInsets = capInsets;
	_contentSize = _textureRect.size;
	
	setCascadeColorEnabled(true);
	setCascadeOpacityEnabled(true);
	
	return true;
}

void Scale9Sprite::onContentSizeDirty() {
	DynamicBatchNode::onContentSizeDirty();
	updateSprites();
}

void Scale9Sprite::setFlippedX(bool flipX) {
	if (_flipX != flipX) {
		_flipX = flipX;
		_contentSizeDirty = true;
	}
}

void Scale9Sprite::setFlippedY(bool flipY) {
	if (_flipY != flipY) {
		_flipY = flipY;
		_contentSizeDirty = true;
	}
}

void Scale9Sprite::setTexture(cocos2d::Texture2D *texture, const cocos2d::Rect &rect) {
	_contentSizeDirty = true;
	DynamicBatchNode::setTexture(texture);
	if (rect.equals(cocos2d::Rect::ZERO)) {
		setTextureRect(cocos2d::Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height));
	} else {
		setTextureRect(rect);
	}
}
void Scale9Sprite::setTextureRect(const cocos2d::Rect &rect) {
	_textureRect = rect;
	_contentSizeDirty = true;
}
void Scale9Sprite::setInsets(const cocos2d::Rect &capInsets) {
	_capInsets = capInsets;
	_contentSizeDirty = true;
}

/** Grid:

 i
 2  2 5 8
 1  1 4 7
 0  0 3 6

    0 1 2 j

 **/


cocos2d::Rect Scale9Sprite::textureRectForGrid(int i, int j) {
	cocos2d::Rect rect = cocos2d::Rect::ZERO;

	if (i == 2) {
		rect.origin.y = _textureRect.origin.y;
		rect.size.height = _insetRect.origin.y - _textureRect.origin.y;
	} else if (i == 1) {
		rect.origin.y = _insetRect.origin.y;
		rect.size.height = _insetRect.size.height;
	} else if (i == 0) {
		rect.origin.y = _insetRect.origin.y + _insetRect.size.height;
		rect.size.height = _textureRect.size.height - _insetRect.size.height - (_insetRect.origin.y - _textureRect.origin.y);
	}


	if (j == 0) {
		rect.origin.x = _textureRect.origin.x;
		rect.size.width = _insetRect.origin.x - _textureRect.origin.x;
	} else if (j == 1) {
		rect.origin.x = _insetRect.origin.x;
		rect.size.width = _insetRect.size.width;
	} else if (j == 2) {
		rect.origin.x = _insetRect.origin.x + _insetRect.size.width;
		rect.size.width = _textureRect.size.width - _insetRect.size.width - (_insetRect.origin.x - _textureRect.origin.x);
	}

	return rect;
}

cocos2d::Vec2 Scale9Sprite::texturePositionForGrid(int i, int j, float csx, float csy) {
	cocos2d::Vec2 point = cocos2d::Vec2::ZERO;

	if (_flipY) {
		i = 2 - i;
	}

	if (_flipX) {
		j = 2 - j;
	}

	if (i == 0) {
		point.y = 0;
	} else if (i == 1) {
		if (_flipY) {
			point.y = _contentSize.height - (_insetRect.origin.y - _textureRect.origin.y + _drawRect.size.height);
		} else {
			point.y = _insetRect.origin.y - _textureRect.origin.y;
		}
	} else if (i == 2) {
		if (_flipY) {
			point.y = _contentSize.height - (_insetRect.origin.y - _textureRect.origin.y) * _globalScaleY;
		} else {
			point.y = (_insetRect.origin.y - _textureRect.origin.y + _drawRect.size.height) * _globalScaleY;
		}
	}

	if (j == 0) {
		point.x = 0;
	} else if (j == 1) {
		if (_flipX) {
			point.x = _contentSize.width - (_insetRect.origin.x - _textureRect.origin.x + _drawRect.size.width);
		} else {
			point.x = _insetRect.origin.x - _textureRect.origin.x;
		}
	} else if (j == 2) {
		if (_flipX) {
			point.x = _contentSize.width - (_insetRect.origin.x - _textureRect.origin.x) * _globalScaleX;
		} else {
			point.x = (_insetRect.origin.x - _textureRect.origin.x + _drawRect.size.width) * _globalScaleX;
		}
	}

	return point;
}

void Scale9Sprite::updateRects() {
	_insetRect = cocos2d::Rect(_textureRect.origin.x + _capInsets.origin.x,
								 _textureRect.origin.y + _capInsets.origin.y,
								 _textureRect.size.width - _capInsets.size.width - _capInsets.origin.x,
								 _textureRect.size.height - _capInsets.size.height - _capInsets.origin.y);

	_drawRect = cocos2d::Rect(_insetRect.origin.x, _insetRect.origin.y,
								_contentSize.width - (_textureRect.size.width - _insetRect.size.width),
								_contentSize.height - (_textureRect.size.height - _insetRect.size.height));

	_globalScaleX = (_drawRect.size.width < 0)?(_contentSize.width/(_textureRect.size.width - _insetRect.size.width)):1;
	_globalScaleY = (_drawRect.size.height < 0)?(_contentSize.width /(_textureRect.size.height - _insetRect.size.height)):1;

	if (_globalScaleX < 1) {
		_drawRect.size.width = 0;
	}

	if (_globalScaleY < 1) {
		_drawRect.size.height = 0;
	}
	
	_minContentSize = cocos2d::Size(_capInsets.origin.x + _capInsets.size.width, _capInsets.origin.y + _capInsets.size.height);
}

void Scale9Sprite::updateSprites() {
	updateRects();
	
	float contentScaleX = 1.0f;
	float contentScaleY = 1.0f;
	
	if (_contentSize.width < _minContentSize.width) {
		contentScaleX = _contentSize.width / _minContentSize.width;
	}
	
	if (_contentSize.height < _minContentSize.height) {
		contentScaleY = _contentSize.height / _minContentSize.height;
	}

    int i, j;

    bool visible;
	float scaleX, scaleY;

    cocos2d::Rect texRect;
    cocos2d::Vec2 spritePos;

    auto tex = getTexture();
    float atlasWidth = (float)tex->getPixelsWide();
    float atlasHeight = (float)tex->getPixelsHigh();

    cocos2d::Color4B color4( _displayedColor.r, _displayedColor.g, _displayedColor.b, _displayedOpacity );
	if (_opacityModifyRGB) {
		color4.r *= _displayedOpacity/255.0f;
		color4.g *= _displayedOpacity/255.0f;
		color4.b *= _displayedOpacity/255.0f;
    }

	size_t quadsCount = 9;
	size_t quadId = 0;

	if (contentScaleY < 1.0f && contentScaleX < 1.0f) {
		quadsCount = 4;
	} else if (contentScaleY < 1.0f || contentScaleX < 1.0f) {
		quadsCount = 6;
	}

	_quads->resize(quadsCount);

	for (int tag = 0; tag < 9; tag++) {
		visible = true;

		i = tag % 3;
		j = tag / 3;

		scaleX = contentScaleX;
		scaleY = contentScaleY;

		if (i == 1) {
			if (contentScaleY < 1.0f) {
				visible = false;
			} else {
				scaleY = _drawRect.size.height / _insetRect.size.height;
			}
		}

		if (j == 1) {
			if (contentScaleX < 1.0f) {
				visible = false;
			} else {
				scaleX = _drawRect.size.width / _insetRect.size.width;
			}
		}

		if (visible) {

			texRect = textureRectForGrid(i, j);
			spritePos = texturePositionForGrid(i, j, contentScaleX, contentScaleY);

			_quads->setTextureRect(quadId, texRect, atlasWidth, atlasHeight, _flipX, _flipY);
			_quads->setGeometry(quadId, spritePos, cocos2d::Size(texRect.size.width * scaleX, texRect.size.height * scaleY), _positionZ);
			_quads->setColor(quadId, color4);

			quadId ++;
		}
	}

	_quads->shrinkToFit();
}
