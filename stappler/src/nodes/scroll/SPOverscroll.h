/*
 * SPOverscroll.h
 *
 *  Created on: 02 апр. 2015 г.
 *      Author: sbkarr
 */

#ifndef LIBS_STAPPLER_NODES_SCROLL_SPOVERSCROLL_H_
#define LIBS_STAPPLER_NODES_SCROLL_SPOVERSCROLL_H_

#include "SPDynamicBatchNode.h"

NS_SP_BEGIN

class Overscroll : public DynamicBatchNode {
public:
	enum Direction {
		Top,
		Left,
		Bottom,
		Right
	};

	static constexpr float OverscrollEdge() { return 0.075f; }
	static constexpr float OverscrollEdgeThreshold() { return 0.5f; }
	static constexpr float OverscrollScale() { return (1.0f / 6.0f); }
	static constexpr float OverscrollMaxHeight() { return 64.0f; }

public:
	static cocos2d::Texture2D *generateTexture(uint32_t width, uint32_t height, cocos2d::Texture2D *tex = nullptr);

	virtual ~Overscroll() { }

	virtual bool init() override;
	virtual bool init(Direction);
	virtual void onContentSizeDirty() override;
	virtual void update(float dt) override;
	virtual void onEnter() override;
	virtual void onExit() override;

    virtual void visit(cocos2d::Renderer *, const cocos2d::Mat4 &, uint32_t, ZPath &zPath) override;

    virtual void setDirection(Direction);
    virtual Direction getDirection() const;

    virtual void setDensity(float val) override;
    virtual float getDensity() const override;

    virtual void setOverscrollOpacity(uint8_t);
    virtual uint8_t getOverscrollOpacity() const;

    void setProgress(float p);

    void incrementProgress(float dt);
    void decrementProgress(float dt);

protected:
    void updateTexture(uint32_t width, uint32_t height);
    void updateProgress();
    void updateSprites();

    bool _progressDirty = true;

    float _density = 1.0f;
	float _progress = 0.0f;
	stappler::Time _delayStart;

	uint8_t _overscrollOpacity = 255;
	uint32_t _width = 0, _height = 0;

	Direction _direction = Direction::Top;
};

NS_SP_END

#endif /* LIBS_STAPPLER_NODES_SCROLL_SPOVERSCROLL_H_ */
