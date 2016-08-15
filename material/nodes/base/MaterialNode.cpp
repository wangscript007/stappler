/*
 * MaterialNode.cpp
 *
 *  Created on: 17 нояб. 2014 г.
 *      Author: sbkarr
 */

#include "Material.h"
#include "MaterialNode.h"
#include "MaterialResourceManager.h"

#include "SPShadowSprite.h"
#include "SPRoundedSprite.h"
#include "SPClippingNode.h"
#include "SPEventListener.h"

#include "2d/CCActionInterval.h"
#include "2d/CCLayer.h"
#include "base/CCDirector.h"
#include "SPDataListener.h"

#define MATERIAL_SHADOW_AMBIENT_MOD 4.0f
#define MATERIAL_SHADOW_KEY_MOD 7.0f

NS_MD_BEGIN

class ShadowAction : public cocos2d::ActionInterval {
public:
    /** creates the action with separate rotation angles */
    static ShadowAction* create(float duration, float targetIndex) {
    	ShadowAction *action = new ShadowAction();
    	if (action->init(duration, targetIndex)) {
    		action->autorelease();
    		return action;
    	} else {
    		delete action;
    		return nullptr;
    	}
    }

    virtual bool init(float duration, float targetIndex) {
    	if (!ActionInterval::initWithDuration(duration)) {
    		return false;
    	}

    	_destIndex = targetIndex;
    	return true;
    }

    virtual void startWithTarget(cocos2d::Node *t) override {
    	if (auto target = dynamic_cast<MaterialNode *>(t)) {
        	_sourceIndex = target->getShadowZIndex();
    	}
    	ActionInterval::startWithTarget(t);
    }

    virtual void update(float time) override {
    	auto target = dynamic_cast<MaterialNode *>(_target);
    	target->setShadowZIndex(_sourceIndex + (_destIndex - _sourceIndex) * time);

    }

    virtual void stop() override {
		ActionInterval::stop();
    }

protected:
    float _sourceIndex = 0;
    float _destIndex = 0;
};

bool MaterialNode::init() {
	if (!Node::init()) {
		return false;
	}

	_background = construct<RoundedSprite>((uint32_t)_borderRadius, stappler::screen::density());
	_background->setAnchorPoint(cocos2d::Vec2(0, 0));
	_background->setPosition(0, 0);
	_background->setOpacity(255);
	_background->setColor(Color::White);
	addChild(_background, 0);

	_backgroundClipper = construct<RoundedSprite>((uint32_t)_borderRadius, stappler::screen::density());
	_backgroundClipper->setAnchorPoint(cocos2d::Vec2(0, 0));
	_backgroundClipper->setPosition(0, 0);
	_backgroundClipper->setOpacity(255);
	_backgroundClipper->setColor(Color::White);

	_shadowClipper = construct<ClippingNode>(_backgroundClipper);
	_shadowClipper->setAnchorPoint(cocos2d::Vec2(0, 0));
	_shadowClipper->setPosition(0, 0);
	_shadowClipper->setAlphaThreshold(1.0f);
	_shadowClipper->setInverted(true);
	_shadowClipper->setEnabled(false);
	_shadowClipper->setCascadeColorEnabled(true);
	addChild(_shadowClipper, -1);

	setCascadeOpacityEnabled(true);

	ignoreAnchorPointForPosition(false);
	setAnchorPoint(cocos2d::Vec2(0, 0));

	_ambientShadow = construct<ShadowSprite>(_shadowIndex * MATERIAL_SHADOW_AMBIENT_MOD);
	_ambientShadow->setOpacity(64);
	_ambientShadow->setAnchorPoint(cocos2d::Vec2(0, 0));
	_ambientShadow->setVisible(false);
	_shadowClipper->addChild(_ambientShadow, -1);

	_keyShadow = construct<ShadowSprite>(_shadowIndex * MATERIAL_SHADOW_KEY_MOD);
	_keyShadow->setOpacity(128);
	_keyShadow->setAnchorPoint(cocos2d::Vec2(0, 0));
	_keyShadow->setVisible(false);
	_shadowClipper->addChild(_keyShadow, -2);

	return true;
}

void MaterialNode::onEnter() {
	Node::onEnter();
	if (isAutoLightLevel()) {
		onLightLevel();
	}
}

void MaterialNode::onTransformDirty() {
	Node::onTransformDirty();
	layoutShadows();
}

void MaterialNode::onContentSizeDirty() {
	Node::onContentSizeDirty();
	layoutShadows();
}

void MaterialNode::setShadowZIndex(float value) {
	if (_shadowIndex != value) {
		_shadowIndex = value;

		_ambientShadow->setTextureSize((_shadowIndex) * MATERIAL_SHADOW_AMBIENT_MOD  + _borderRadius);
		_keyShadow->setTextureSize((_shadowIndex) * MATERIAL_SHADOW_KEY_MOD + _borderRadius);

		if (value == 0.0f) {
			_ambientShadow->setVisible(false);
			_keyShadow->setVisible(false);
		} else {
			_ambientShadow->setVisible(true);
			_keyShadow->setVisible(true);
		}

		_ambientShadow->setOpacity(getOpacityForAmbientShadow(value));
		_keyShadow->setOpacity(getOpacityForKeyShadow(value));

		_contentSizeDirty = true;
	}
}

void MaterialNode::setShadowZIndexAnimated(float value, float duration) {
	if (_shadowIndex != value) {
		stopActionByTag(1);
		auto a = ShadowAction::create(duration, value);
		a->setTag(1);
		runAction(a);
	}
}

void MaterialNode::setBorderRadius(uint32_t value) {
	if (_borderRadius != value) {
		_borderRadius = value;

		_background->setTextureSize((uint32_t)_borderRadius);
		_backgroundClipper->setTextureSize((uint32_t)_borderRadius);

		_ambientShadow->setTextureSize((_shadowIndex) * MATERIAL_SHADOW_AMBIENT_MOD  + _borderRadius);
		_keyShadow->setTextureSize((_shadowIndex) * MATERIAL_SHADOW_KEY_MOD + _borderRadius);

		_contentSizeDirty = true;
	}
}
uint32_t MaterialNode::getBorderRadius() const {
	return _borderRadius;
}

void MaterialNode::setBackgroundColor(const Color &c) {
	_background->setColor(c);
	_backgroundClipper->setColor(c);
}
const cocos2d::Color3B &MaterialNode::getBackgroundColor() const {
	return _background->getColor();
}

void MaterialNode::setBackgroundVisible(bool value) {
	_background->setVisible(value);
}
bool MaterialNode::isBackgroundVisible() const {
	return _background->isVisible();
}

void MaterialNode::setUserData(const data::Value &d) {
	_userData = d;
}
void MaterialNode::setUserData(data::Value &&d) {
	_userData = std::move(d);
}
const data::Value &MaterialNode::getUserData() const {
	return _userData;
}

void MaterialNode::setPadding(const stappler::Padding &p) {
	if (p != _padding) {
		_padding = p;
		_contentSizeDirty = true;
	}
}
const Padding &MaterialNode::getPadding() const {
	return _padding;
}

void MaterialNode::layoutShadows() {
	if (_shadowIndex > 0) {
		_ambientShadow->setContentSize(getContentSizeForAmbientShadow(_shadowIndex));
		_ambientShadow->setPosition(getPositionForAmbientShadow(_shadowIndex));
		_ambientShadow->setVisible(true);

		_keyShadow->setContentSize(getContentSizeForKeyShadow(_shadowIndex));
		_keyShadow->setPosition(getPositionForKeyShadow(_shadowIndex));
		_keyShadow->setVisible(true);

		_ambientShadow->setOpacity(getOpacityForAmbientShadow(_shadowIndex));
		_keyShadow->setOpacity(getOpacityForKeyShadow(_shadowIndex));
	} else {
		_ambientShadow->setVisible(false);
		_keyShadow->setVisible(false);
	}

	_background->setContentSize(getContentSizeWithPadding());
	_background->setPosition(getAnchorPositionWithPadding());

	_backgroundClipper->setContentSize(getContentSizeWithPadding());
	_backgroundClipper->setPosition(getAnchorPositionWithPadding());

	_shadowClipper->setContentSize(_contentSize);
	_shadowClipper->setPosition(0, 0);

	_positionsDirty = false;
}

void MaterialNode::updateColor() {
	cocos2d::Node::updateColor();
	if (_displayedOpacity == 255) {
		_shadowClipper->setEnabled(false);
	} else {
		_shadowClipper->setEnabled(true);
	}
}

void MaterialNode::onDataRecieved(stappler::data::Value &) { }

cocos2d::Size MaterialNode::getContentSizeWithPadding() const {
	return cocos2d::Size(MAX(_contentSize.width - _padding.left - _padding.right, 0),
			MAX(_contentSize.height - _padding.top - _padding.bottom, 0));
}
cocos2d::Vec2 MaterialNode::getAnchorPositionWithPadding() const {
	return cocos2d::Vec2(_padding.left, _padding.bottom);
}

cocos2d::Rect MaterialNode::getContentRect() const {
	return cocos2d::Rect(getAnchorPositionWithPadding(), getContentSizeWithPadding());
}

cocos2d::Size MaterialNode::getContentSizeForAmbientShadow(float index) const {
	float ambientSeed = index * MATERIAL_SHADOW_AMBIENT_MOD;
	auto ambientSize = getContentSizeWithPadding();
	ambientSize.width += ambientSeed;
	ambientSize.height += ambientSeed;
	return ambientSize;
}
cocos2d::Vec2 MaterialNode::getPositionForAmbientShadow(float index) const {
	float ambientSeed = index * MATERIAL_SHADOW_AMBIENT_MOD;
	return cocos2d::Vec2(-ambientSeed / 2 + _padding.left, -ambientSeed / 2 + _padding.bottom);
}

cocos2d::Size MaterialNode::getContentSizeForKeyShadow(float index) const {
	float keySeed = index * MATERIAL_SHADOW_KEY_MOD;
	auto keySize = getContentSizeWithPadding();
	keySize.width += keySeed;
	keySize.height += keySeed;
	return keySize;
}
cocos2d::Vec2 MaterialNode::getPositionForKeyShadow(float index) const {
	cocos2d::Vec2 vec = convertToWorldSpace(cocos2d::Vec2(_contentSize.width / 2, _contentSize.height / 2));
	cocos2d::Size screenSize = cocos2d::Director::getInstance()->getWinSize();
	cocos2d::Vec2 lightSource(screenSize.width / 2, screenSize.height);
	cocos2d::Vec2 normal = lightSource - vec;
	normal.normalize();

	normal = normal - cocos2d::Vec2(0, -1);

	float keySeed = index * MATERIAL_SHADOW_KEY_MOD;
	return cocos2d::Vec2(-keySeed / 2 + _padding.left, -keySeed / 2 + _padding.bottom) - normal * _shadowIndex * 0.5;
}

uint8_t MaterialNode::getOpacityForAmbientShadow(float value) const {
	uint8_t ret = (value < 2.0)?(value * 0.5f * 168.0f):168;
	auto size = getContentSizeForAmbientShadow(value);
	auto texSize = _ambientShadow->getTextureSize();
	float min = MIN(size.width, size.height) - texSize;
	if (min < 0) {
		ret = 0;
	} else if (min < texSize * 2) {
		ret = (float)ret * (min / (texSize * 2.0f));
	}
	return ret;
}
uint8_t MaterialNode::getOpacityForKeyShadow(float value) const {
	uint8_t ret = (value < 2.0)?(value * 0.5f * 200.0f):200;
	auto size = getContentSizeForKeyShadow(value);
	auto texSize = _keyShadow->getTextureSize();
	float min = MIN(size.width, size.height) - texSize;
	if (min < 0) {
		ret = 0;
	} else if (min < texSize * 2) {
		ret = (float)ret * (min / (texSize * 2.0f));
	}
	return ret;
}

void MaterialNode::setAutoLightLevel(bool value) {
	if (value && !_lightLevelListener) {
		_lightLevelListener = construct<EventListener>();
		_lightLevelListener->onEvent(ResourceManager::onLightLevel, std::bind(&MaterialNode::onLightLevel, this));
		addComponent(_lightLevelListener);
		if (isRunning()) {
			onLightLevel();
		}
	} else if (!value && _lightLevelListener) {
		removeComponent(_lightLevelListener);
		_lightLevelListener = nullptr;
	}
}
bool MaterialNode::isAutoLightLevel() const {
	return _lightLevelListener != nullptr;
}

void MaterialNode::setDimColor(const Color &c) {
	_dimColor = c;
	onLightLevel();
}
const Color &MaterialNode::getDimColor() const {
	return _dimColor;
}

void MaterialNode::setNormalColor(const Color &c) {
	_normalColor = c;
	onLightLevel();
}
const Color &MaterialNode::getNormalColor() const {
	return _normalColor;
}

void MaterialNode::setWashedColor(const Color &c) {
	_washedColor = c;
	onLightLevel();
}
const Color &MaterialNode::getWashedColor() const {
	return _washedColor;
}

void MaterialNode::onLightLevel() {
	if (isAutoLightLevel()) {
		auto level = material::ResourceManager::getInstance()->getLightLevel();
		switch(level) {
		case rich_text::style::LightLevel::Dim:
			setBackgroundColor(_dimColor);
			break;
		case rich_text::style::LightLevel::Normal:
			setBackgroundColor(_normalColor);
			break;
		case rich_text::style::LightLevel::Washed:
			setBackgroundColor(_washedColor);
			break;
		};
	}
}

NS_MD_END
