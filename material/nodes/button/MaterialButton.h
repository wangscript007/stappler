/*
 * MaterialButton.h
 *
 *  Created on: 10 дек. 2014 г.
 *      Author: sbkarr
 */

#ifndef CLASSES_MATERIAL_NODES_MATERIALBUTTON_H_
#define CLASSES_MATERIAL_NODES_MATERIALBUTTON_H_

#include "MaterialNode.h"
#include "MaterialColors.h"
#include "MaterialLabel.h"
#include "MaterialMenuSource.h"

NS_MD_BEGIN

class Button : public MaterialNode {
public:
	using TapCallback = std::function<void()>;
	using TouchFilter = std::function<bool(const cocos2d::Vec2 &)>;

	enum Style {
		Raised,
		FlatWhite,
		FlatBlack,
	};

	virtual bool init(const TapCallback &tapCallback = nullptr, const TapCallback &longTapCallback = nullptr);
	virtual void onContentSizeDirty() override;

	virtual void setStyle(Style style);
	virtual Style getStyle() const;

	virtual void setTapCallback(const TapCallback & tapCallback);
	virtual void setLongTapCallback(const TapCallback & longTapCallback);
	virtual void setTouchFilter(const TouchFilter &);

	virtual void setSwallowTouches(bool value);
	virtual void setGesturePriority(int32_t);
	virtual void setAnimationOpacity(uint8_t);

	virtual bool onPressBegin(const cocos2d::Vec2 &location);
    virtual void onLongPress();
	virtual bool onPressEnd();
	virtual void onPressCancel();

	virtual void setEnabled(bool value);
	virtual bool isEnabled() const;

	virtual void setSelected(bool value, bool instant = false);
	virtual bool isSelected() const;

	virtual void setSpawnDelay(float value);
	virtual float getSpawnDelay() const;

	virtual void setRaisedDefaultZIndex(float value);
	virtual void setRaisedActiveZIndex(float value);

	virtual void setMenuSource(MenuSource *);
	virtual MenuSource * getMenuSource() const;

	virtual void setMenuSourceButton(MenuSourceButton *);
	virtual MenuSourceButton *getMenuSourceButton() const;

	virtual const cocos2d::Vec2 & getTouchPoint() const;

protected:
	virtual void animateSelection();
	virtual void animateDeselection();

	virtual void beginSelection();
	virtual void updateSelection(float progress);
	virtual void endSelection();
	virtual float getSelectionProgress() const;

	virtual stappler::draw::Path * beginSpawn();
	virtual void updateSpawn(float progress, stappler::draw::Path *path, const cocos2d::Size &size, const cocos2d::Point &point);
	virtual void endSpawn();
	virtual void dropSpawn();

	virtual void updateSelectionProgress(float progress);
	virtual void updateStyle();
	virtual void updateEnabled();

	virtual void updateFromSource();

	virtual void onLightLevel() override;

	std::function<void()> _tapCallback = nullptr;
	std::function<void()> _longTapCallback = nullptr;
	TouchFilter _touchFilter = nullptr;
	cocos2d::Component *_listener = nullptr;

	stappler::draw::PathNode *_animationNode = nullptr;
	uint32_t _animationCount = 0;

	cocos2d::Vec2 _touchPoint;

	Style _style = FlatBlack;

	float _animationProgress = 0.0f;
	float _spawnDelay = 0.0f;
	bool _selected = false;
	bool _enabled = true;

	uint8_t _animationOpacity = 36;

	float _raisedDefaultZIndex = 1.5f;
	float _raisedActiveZIndex = 3.0f;

	Rc<MenuSource>_floatingMenuSource;
	data::Listener<MenuSourceButton> _source;
};

NS_MD_END

#endif /* CLASSES_MATERIAL_NODES_MATERIALBUTTON_H_ */
