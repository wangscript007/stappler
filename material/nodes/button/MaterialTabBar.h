/*
 * MaterialTabBar.h
 *
 *  Created on: 4 окт. 2016 г.
 *      Author: sbkarr
 */

#ifndef MATERIAL_NODES_BUTTON_MATERIALTABBAR_H_
#define MATERIAL_NODES_BUTTON_MATERIALTABBAR_H_

#include "MaterialMenuSource.h"
#include "MaterialButton.h"
#include "SPDataListener.h"
#include "MaterialLabel.h"

NS_MD_BEGIN

class TabBar : public cocos2d::Node {
public:
	enum class ButtonStyle {
		Title,
		Icon,
		TitleIcon,
	};

	enum class BarStyle {
		Layout,
		Scroll,
	};

	using Alignment = Label::Alignment;

	virtual ~TabBar() { }

	virtual bool init(MenuSource *, ButtonStyle = ButtonStyle::Title, BarStyle = BarStyle::Layout, Alignment = Alignment::Center);
	virtual void onContentSizeDirty() override;

	virtual void setMenuSource(MenuSource *);
	virtual MenuSource *getMenuSource() const;

	virtual void setTextColor(const Color &);
	virtual Color getTextColor() const;

	virtual void setSelectedColor(const Color &);
	virtual Color getSelectedColor() const;

	virtual void setAccentColor(const Color &);
	virtual Color getAccentColor() const;

	virtual void setButtonStyle(const ButtonStyle &);
	virtual const ButtonStyle & getButtonStyle() const;

	virtual void setBarStyle(const BarStyle &);
	virtual const BarStyle & getBarStyle() const;

	virtual void setAlignment(const Alignment &);
	virtual const Alignment & getAlignment() const;

	virtual void setSelectedIndex(size_t);
	virtual size_t getSelectedIndex() const;

protected:
	virtual void setSelectedTabIndex(size_t);
	virtual void onMenuSource();
	virtual void onScrollPosition();

	virtual float getItemSize(const String &, bool extended = false) const;
	virtual cocos2d::Node *onItem(MenuSourceButton *, bool wrapped);
	virtual void onTabButton(Button *, MenuSourceButton *);


	Alignment _alignment = Alignment::Center;
	ButtonStyle _buttonStyle = ButtonStyle::Title;
	BarStyle _barStyle = BarStyle::Layout;
	Color _textColor = Color::Black;
	Color _accentColor = Color::Black;
	Color _selectedColor = Color::Black;
	ScrollView *_scroll = nullptr;

	cocos2d::Node *_layerNode = nullptr;
	Layer *_layer = nullptr;
	IconSprite *_left = nullptr;
	IconSprite *_right = nullptr;
	data::Listener<material::MenuSource> _source;
	Rc<material::MenuSource> _extra;
	size_t _buttonCount = 0;
	float _scrollWidth = 0.0f;

	size_t _selectedIndex = maxOf<size_t>();
	Vector<Pair<float, float>> _positions;
};

class TabBarButton : public Button {
public:
	using TabButtonCallback = Function<void(Button *, MenuSourceButton *)>;

	virtual bool init(MenuSourceButton *, const TabButtonCallback &cb, TabBar::ButtonStyle, bool wrapped);
	virtual bool init(MenuSource *, const TabButtonCallback &cb, TabBar::ButtonStyle, bool wrapped);

	virtual void onContentSizeDirty() override;

	virtual void setTextColor(const Color &);
	virtual void setSelectedColor(const Color &);
	virtual void setTabSelected(bool);

protected:
	virtual void initialize(const TabButtonCallback &, TabBar::ButtonStyle, bool wrapped);

	virtual void updateFromSource() override;
	virtual void onOpenMenuSource() override;
	virtual void onTabButton();

	Color _textColor;
	Color _selectedColor;

	Label *_label = nullptr;
	IconSprite *_icon = nullptr;
	TabBar::ButtonStyle _tabStyle;
	bool _wrapped = false;
	bool _tabSelected = false;
	TabButtonCallback _tabButtonCallback = nullptr;
};

NS_MD_END

#endif /* MATERIAL_NODES_BUTTON_MATERIALTABBAR_H_ */