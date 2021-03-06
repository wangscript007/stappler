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

#ifndef LIBS_MATERIAL_NODES_SCENE_MATERIALSCENE_H_
#define LIBS_MATERIAL_NODES_SCENE_MATERIALSCENE_H_

#include "Material.h"
#include "MaterialContentLayer.h"
#include "MaterialBackgroundLayer.h"
#include "MaterialNavigationLayer.h"
#include "MaterialForegroundLayer.h"
#include "SPDynamicBatchScene.h"

/* Default scene layout:
 *
 * --- Foreground
 * 			All popups and floating menus must be placed here
 * 			to capture touch input on the top of scene graph
 *
 * --- Navigation
 * 			Special level for navigation drawer
 *
 * --- Content
 * 			All content pages should be on this level
 *
 * --- Background
 * 			Global background, usually for color or image background
 *
 */

NS_MD_BEGIN

class Scene : public DynamicBatchScene {
public:
	using Transition = cocos2d::FiniteTimeAction;

	static stappler::EventHeader onBackKey;

public:
	static material::Scene *getRunningScene();
	static void run(Scene *scene = nullptr);

	virtual bool init() override;
	virtual void onEnter() override;
	virtual void onEnterTransitionDidFinish() override;
	virtual void onExitTransitionDidStart() override;

	virtual const Size &getViewSize() const;

	virtual ForegroundLayer *getForegroundLayer() const;
	virtual NavigationLayer *getNavigationLayer() const;
	virtual ContentLayer *getContentLayer() const;
	virtual BackgroundLayer *getBackgroundLayer() const;

	virtual void setNavigationEnabled(bool);
	virtual bool isNavigationEnabled() const;

	virtual void setAutoLightLevel(bool value);
	virtual bool isAutoLightLevel() const;

	virtual void setDisplayStats(bool value);
	virtual bool isDisplayStats() const;

	virtual void setVisualizeTouches(bool value);
	virtual bool isVisualizeTouches() const;

	virtual void setExitProtection(bool value);
	virtual bool isExitProtection() const;

	virtual void update(float dt) override;

	virtual void captureContentForNode(cocos2d::Node *);
	virtual void releaseContentForNode(cocos2d::Node *);
	virtual bool isContentCaptured();

	virtual void replaceContentNode(Layout *, Transition *enter = nullptr);
	virtual void replaceTopContentNode(Layout *, Transition *enter = nullptr, Transition *exit = nullptr);

	virtual void pushContentNode(Layout *, Transition *enter = nullptr, Transition *exit = nullptr);
	virtual void popContentNode(Layout *);

	virtual void pushOverlayNode(OverlayLayout *, Transition *enter = nullptr, Transition *exit = nullptr);
	virtual void popOverlayNode(OverlayLayout *);

	virtual void pushFloatNode(cocos2d::Node *, int);
	virtual void popFloatNode(cocos2d::Node *);

	virtual void pushForegroundNode(cocos2d::Node *, const Function<void()> & = nullptr);
	virtual void popForegroundNode(cocos2d::Node *);

	virtual void popNode(cocos2d::Node *);

	virtual void setNavigationMenuSource(material::MenuSource *);

	virtual Vec2 convertToScene(const Vec2 &) const;

	virtual void pushBackButtonCallback(stappler::Ref *, const Function<void()> &);
	virtual void popBackButtonCallback(stappler::Ref *);

public:
	using SnackbarData = ForegroundLayer::SnackbarData;

	virtual void showSnackbar(SnackbarData &&);

protected:
	virtual Rc<ForegroundLayer> createForegroundLayer();
	virtual Rc<NavigationLayer> createNavigationLayer();
	virtual Rc<ContentLayer> createContentLayer();
	virtual Rc<BackgroundLayer> createBackgroundLayer();

	virtual void layoutSubviews();
	virtual void layoutLayer(cocos2d::Node *);

	virtual void onLightLevel();

	void updateStatsLabel();
	void updateStatsValue();
	void updateCapturedContent();

	void onBackKeyPressed();
	void takeScreenshoot();

	void saveScreenshot(const String &, cocos2d::Texture2D *);

	ForegroundLayer *_foreground = nullptr;
	NavigationLayer *_navigation = nullptr;
	ContentLayer *_content = nullptr;
	BackgroundLayer *_background = nullptr;

	Label *_stats = nullptr;
	Layer *_statsColor = nullptr;

	bool _autoLightLevel = false;
	bool _visualizeTouches = false;
	bool _displayStats = false;
	bool _registred = false;
	bool _exitProtection = true;

	bool _shouldExit = false;
	bool _contentCaptured = false;
	Set<cocos2d::Node *> _contentCaptureNodes;

	cocos2d::Node *_touchesNode = nullptr;
	Map<int, cocos2d::Node *> _touchesNodes;
	Vector<cocos2d::GroupCommand> _group;

	bool _contentCapturing = false;
	Rc<draw::Canvas> _captureCanvas;

	Vector<Pair<Rc<stappler::Ref>, Function<void()>>> _backButtonStack;
};

NS_MD_END

#endif /* LIBS_MATERIAL_NODES_SCENE_MATERIALSCENE_H_ */
