
#ifndef material_Material_h
#define material_Material_h

#include "SPDefine.h"
#include "SPEventHeader.h"
/* Minimal forward declaration */

#ifdef __cplusplus

#define NS_MD_BEGIN		namespace stappler { namespace material {
#define NS_MD_END		} }
#define USING_NS_MD		using namespace stappler::material

#else

#define NS_MD_BEGIN
#define NS_MD_END
#define USING_NS_MD

#endif

#define MD_COLOR_SPEC_BASE(Name) \
	static Color Name ## _50; \
	static Color Name ## _100; \
	static Color Name ## _200; \
	static Color Name ## _300; \
	static Color Name ## _400; \
	static Color Name ## _500; \
	static Color Name ## _600; \
	static Color Name ## _700; \
	static Color Name ## _800; \
	static Color Name ## _900;

#define MD_COLOR_SPEC_ACCENT(Name) \
	static Color Name ## _A100; \
	static Color Name ## _A200; \
	static Color Name ## _A400; \
	static Color Name ## _A700;

#define MD_COLOR_SPEC(Name) \
		MD_COLOR_SPEC_BASE(Name) \
		MD_COLOR_SPEC_ACCENT(Name)

#define MD_USE_TEXT_FONTS 0

NS_MD_BEGIN

class Font;
class FontSet;
class Label;

class ResourceManager;

class ResizeTo;
class ResizeBy;

enum class IconName : uint32_t;

class StaticIcon;
class DynamicIcon;
class DynamicIconProgress;

class Color;

class MaterialNode;
class MaterialImage;

class Button;
class ButtonStaticIcon;
class ButtonDynamicIcon;
class ButtonLabel;
class ButtonLabelSelector;
class ButtonLabelIcon;
class FloatingActionButton;

class Layout;
class FlexibleLayout;
class SidebarLayout;
class ToolbarLayout;

class MenuSource;
class MenuSourceItem;
class MenuSourceButton;
class MenuSourceCustom;

class FloatingMenu;
class Menu;
class MenuItemInterface;
class MenuButton;
class MenuSeparator;

class LinearProgress;
class RoundedProgress;

class Scene;
class BackgroundLayer;
class ContentLayer;
class NavigationLayer;
class ForegroundLayer;

class Scroll;
class ScrollHandlerFixed;
class ScrollHandlerSlice;
class ScrollHandlerGrid;

class Toolbar;

class FontSizeMenuButton;
class LightLevelMenuButton;

class RichTextListenerView;
class RichTextView;
class RichTextTooltip;
class RichTextSource;
class RichTextImageView;
class ImageLayer;

class Switch;
class TextField;
class SingleLineField;

class GalleryScroll;
class GalleryImage;

NS_MD_END

#include "MaterialColors.h"
#include "MaterialMetrics.h"

#endif
