#ifndef __M_MENU_MENU_H__
#define __M_MENU_MENU_H__




#include "c_cvars.h"
#include "v_font.h"
#include "version.h"
#include "textures.h"
#include "zstring.h"

EXTERN_CVAR(Float, snd_menuvolume)
EXTERN_CVAR(Int, m_use_mouse);

enum EMax
{
	MAXSKILLS = 7,
	MAXVOLUMES = 7,
	MAXMENUGAMEPLAYENTRIES = 7,
};

// These get filled in by the map definition parsers of the front ends.
extern FString gSkillNames[MAXSKILLS];
extern FString gVolumeNames[MAXVOLUMES];
extern FString gVolumeSubtitles[MAXVOLUMES];
extern int32_t gVolumeFlags[MAXVOLUMES];
extern int gDefaultVolume, gDefaultSkill;

const int MENU_TICRATE = 30;
extern bool help_disabled, credits_disabled;
extern int g_currentMenu;

enum
{
	EF_HIDEFROMSP = 1 << 0,
};

enum MenuGameplayEntryFlags
{
	MGE_Locked = 1u << 0u,
	MGE_Hidden = 1u << 1u,
	MGE_UserContent = 1u << 2u,
};

typedef struct MenuGameplayEntry
{
	char name[64];
	uint8_t flags;

	bool isValid() const { return name[0] != '\0'; }
} MenuGameplayEntry;

typedef struct MenuGameplayStemEntry
{
	MenuGameplayEntry entry;
	MenuGameplayEntry subentries[MAXMENUGAMEPLAYENTRIES];
} MenuGameplayStemEntry;

extern MenuGameplayStemEntry g_MenuGameplayEntries[MAXMENUGAMEPLAYENTRIES];


enum EMenuState : int
{
	MENU_Off,			// Menu is closed
	MENU_On,			// Menu is opened
	MENU_WaitKey,		// Menu is opened and waiting for a key in the controls menu
	MENU_OnNoPause,		// Menu is opened but does not pause the game
};

struct event_t;
class FTexture;
class FFont;
enum EColorRange;
class FPlayerClass;
class FKeyBindings;

enum EMenuKey
{
	MKEY_Up,
	MKEY_Down,
	MKEY_Left,
	MKEY_Right,
	MKEY_PageUp,
	MKEY_PageDown,
	//----------------- Keys past here do not repeat.
	MKEY_Enter,
	MKEY_Back,		// Back to previous menu
	MKEY_Clear,		// Clear keybinding/flip player sprite preview
	NUM_MKEYS,

	// These are not buttons but events sent from other menus 

	MKEY_Input,		// Sent when input is confirmed
	MKEY_Abort,		// Input aborted
	MKEY_MBYes,
	MKEY_MBNo,
};

enum ENativeFontValues
{
	NIT_BigFont,
	NIT_SmallFont,
	NIT_TinyFont,

	NIT_ActiveColor = -1,
	NIT_InactiveColor = -2,
	NIT_SelectedColor = -3,

	NIT_ActiveState = 1,
	NIT_InactiveState = 2,
	NIT_SelectedState = 3
	// positive values for color are direct palswap indices.
};


struct FGameStartup
{
	int Episode;
	int Skill;
	int CustomLevel1;
	int CustomLevel2;
};

extern FGameStartup GameStartupInfo;

struct FSaveGameNode
{
	FString Title;
	FString Filename;
	bool bOldVersion;
	bool bMissingWads;
	bool bNoDelete;

	FSaveGameNode() { bNoDelete = false; }
};

extern EMenuState		menuactive;


//=============================================================================
//
// menu descriptor. This is created from the menu definition lump
// Items must be inserted in the order they are cycled through with the cursor
//
//=============================================================================

enum EMenuDescriptorType
{
	MDESC_ListMenu,
	MDESC_OptionsMenu,
};

struct FMenuDescriptor
{
	FName mMenuName;
	FString mNetgameMessage;
	int mType;
	FName mClass;

	virtual ~FMenuDescriptor() {}
};

class FListMenuItem;
class FOptionMenuItem;

enum ListMenuFlags
{
	LMF_Centered = 1,
	LMF_DontSpace = 2,
	LMF_Animate = 4,
};

struct FListMenuDescriptor : public FMenuDescriptor
{
	TDeletingArray<FListMenuItem *> mItems;
	FString mCaption;
	int mSelectedItem;
	int mSelectOfsX;
	int mSelectOfsY;
	FTexture *mSelector;
	int mDisplayTop;
	int mXpos, mYpos, mYbotton;
	int mWLeft, mWRight;
	int mLinespacing;	// needs to be stored for dynamically created menus
	int mAutoselect;	// this can only be set by internal menu creation functions
	int mScriptId;
	int mSecondaryId;
	int mNativeFontNum, mNativePalNum;
	float mNativeFontScale;
	FFont *mFont;
	EColorRange mFontColor;
	EColorRange mFontColor2;
	FMenuDescriptor *mRedirect;	// used to redirect overlong skill and episode menus to option menu based alternatives
	int mFlags;
	int mSpacing;

	FListMenuDescriptor()
	{
		Reset();
	}

	void Reset()
	{
		// Reset the default settings (ignore all other values in the struct)
		mSelectOfsX = 0;
		mSelectOfsY = 0;
		mSelector = nullptr;
		mDisplayTop = 0;
		mXpos = 0;
		mYpos = 0;
		mLinespacing = 0;
		mNetgameMessage = "";
		mFont = NULL;
		mFontColor = CR_UNTRANSLATED;
		mFontColor2 = CR_UNTRANSLATED;
		mScriptId = -1;
		mSecondaryId = 0;
		mNativeFontNum = NIT_BigFont;
		mNativePalNum = NIT_ActiveColor;
		mNativeFontScale = 1.f;
		mFlags = 0;
		mSpacing = 0;
	}
};

struct FOptionMenuSettings
{
	EColorRange mTitleColor;
	EColorRange mFontColor;
	EColorRange mFontColorValue;
	EColorRange mFontColorMore;
	EColorRange mFontColorHeader;
	EColorRange mFontColorHighlight;
	EColorRange mFontColorSelection;
	int mLinespacing;
};

struct FOptionMenuDescriptor : public FMenuDescriptor
{
	TDeletingArray<FOptionMenuItem *> mItems;
	FString mTitle;
	int mSelectedItem;
	int mDrawTop;
	int mScrollTop;
	int mScrollPos;
	int mIndent;
	int mPosition;
	bool mDontDim;

	void CalcIndent();
	FOptionMenuItem *GetItem(FName name);
	void Reset()
	{
		// Reset the default settings (ignore all other values in the struct)
		mPosition = 0;
		mScrollTop = 0;
		mIndent = 0;
		mDontDim = 0;
	}

};
						

typedef TMap<FName, FMenuDescriptor *> MenuDescriptorList;

extern FOptionMenuSettings OptionSettings;
extern MenuDescriptorList MenuDescriptors;

#define CURSORSPACE (14 * CleanXfac_1)

//=============================================================================
//
//
//
//=============================================================================

struct FMenuRect
{
	int x, y;
	int width, height;

	void set(int _x, int _y, int _w, int _h)
	{
		x = _x;
		y = _y;
		width = _w;
		height = _h;
	}

	bool inside(int _x, int _y)
	{
		return _x >= x && _x < x+width && _y >= y && _y < y+height;
	}

};


class DMenu
{
protected:
	bool mMouseCapture;
	bool mBackbuttonSelected;

public:
	enum
	{
		MOUSE_Click,
		MOUSE_Move,
		MOUSE_Release
	};

	enum
	{
		BACKBUTTON_TIME = 4*MENU_TICRATE
	};

	static DMenu *CurrentMenu;
	static int MenuTime;

	DMenu *mParentMenu;
	DVector2 origin = { 0,0 };
	int scriptID = INT_MAX;
	bool canAnimate = false;

	DMenu(DMenu *parent = NULL);
	virtual ~DMenu() = default;
	virtual bool Responder (event_t *ev);
	virtual bool MenuEvent (int mkey, bool fromcontroller);
	virtual void Ticker ();
	virtual void PreDraw() {}
	virtual void PostDraw() {}
	virtual void Drawer ();
	virtual bool DimAllowed ();
	virtual bool TranslateKeyboardEvents();
	virtual void Close();
	virtual bool MouseEvent(int type, int x, int y);
	virtual void Destroy() {}
	bool MouseEventBack(int type, int x, int y);
	void SetCapture();
	void ReleaseCapture();
	void SetOrigin();
	bool HasCapture()
	{
		return mMouseCapture;
	}
};

//=============================================================================
//
// base class for menu items
//
//=============================================================================

class DListMenu;

class FListMenuItem
{
protected:
	int mXpos, mYpos;
	int mHeight;
	FName mAction;

public:
	bool mEnabled, mHidden;

	FListMenuItem(int xpos = 0, int ypos = 0, FName action = NAME_None)
	{
		mXpos = xpos;
		mYpos = ypos;
		mAction = action;
		mEnabled = true;
		mHidden = false;
	}

	virtual ~FListMenuItem();

	virtual bool CheckCoordinate(int x, int y);
	virtual void Ticker();
	virtual void Drawer(DListMenu *menu, const DVector2& origin, bool selected);
	virtual bool Selectable();
	virtual bool Activate(FName caller);
	virtual FName GetAction(int *pparam);
	virtual bool SetString(int i, const char *s);
	virtual bool GetString(int i, char *s, int len);
	virtual bool SetValue(int i, int value);
	virtual bool GetValue(int i, int *pvalue);
	virtual void Enable(bool on);
	virtual bool MenuEvent (int mkey, bool fromcontroller);
	virtual bool MouseEvent(int type, int x, int y);
	virtual bool CheckHotkey(int c);
	virtual int GetWidth();
	virtual void DrawSelector(int xofs, int yofs, FTexture *tex);
	void OffsetPositionY(int ydelta) { mYpos += ydelta; }
	int GetY() { return mYpos; }
	int GetX() { return mXpos; }
	void SetX(int x) { mXpos = x; }
	void SetY(int x) { mYpos = x; }
	void SetHeight(int x) { mHeight = x; }
	void SetAction(FName action) { mAction = action; }
};

class FListMenuItemStaticPatch : public FListMenuItem
{
protected:
	FTexture *mTexture;
	bool mCentered;

public:
	FListMenuItemStaticPatch(int x, int y, FTexture *patch, bool centered);
	void Drawer(DListMenu* menu, const DVector2& origin, bool selected);
};

class FListMenuItemStaticText : public FListMenuItem
{
protected:
	const char *mText;
	FFont *mFont;
	EColorRange mColor;
	bool mCentered;

public:
	FListMenuItemStaticText(int x, int y, const char *text, FFont *font, EColorRange color, bool centered);
	~FListMenuItemStaticText();
	void Drawer(DListMenu* menu, const DVector2& origin, bool selected) override;
};

//=============================================================================
//
// selectable items
//
//=============================================================================

class FListMenuItemSelectable : public FListMenuItem
{
protected:
	int mHotkey;
	int mParam;

public:
	FListMenuItemSelectable(int x, int y, int height, FName childmenu, int mParam = -1);
	bool CheckCoordinate(int x, int y) override;
	bool Selectable() override;
	bool CheckHotkey(int c) override;
	bool Activate(FName caller) override;
	bool MouseEvent(int type, int x, int y) override;
	FName GetAction(int *pparam) override;
};

class FListMenuItemText : public FListMenuItemSelectable
{
	FString mText;
	FFont *mFont;
	EColorRange mColor;
	EColorRange mColorSelected;
public:
	FListMenuItemText(int x, int y, int height, int hotkey, const FString &text, FFont *font, EColorRange color, EColorRange color2, FName child, int param = 0);
	~FListMenuItemText();
	void Drawer(DListMenu* menu, const DVector2& origin, bool selected) override;
	int GetWidth() override;
};

class FListMenuItemNativeText : public FListMenuItemSelectable
{
	// This draws the item with the game frontend's native text drawer and uses a front end defined font, it takes only symbolic constants as parameters.
	FString mText;
	int mFontnum;
	int mPalnum;
	float mFontscale;
public:
	FListMenuItemNativeText(int x, int y, int height, int hotkey, const FString& text, int fontnum, int palnum, float fontscale, FName child, int param = 0);
	~FListMenuItemNativeText();
	void Drawer(DListMenu* menu, const DVector2& origin, bool selected) override;
	int GetWidth() override;
	void DrawSelector(int xofs, int yofs, FTexture* tex) override { } // The text drawer handles this itself.
};


class FListMenuItemPatch : public FListMenuItemSelectable
{
	FTexture* mTexture;
public:
	FListMenuItemPatch(int x, int y, int height, int hotkey, FTexture* patch, FName child, int param = 0);
	void Drawer(DListMenu* menu, const DVector2& origin, bool selected) override;
	int GetWidth() override;
};

//=============================================================================
//
// list menu class runs a menu described by a FListMenuDescriptor
//
//=============================================================================

class DListMenu : public DMenu
{
	typedef DMenu Super;
protected:
	FListMenuDescriptor *mDesc = nullptr;
	FListMenuItem *mFocusControl = nullptr;

public:
	DListMenu(DMenu *parent = NULL, FListMenuDescriptor *desc = NULL);
	virtual void Init(DMenu *parent = NULL, FListMenuDescriptor *desc = NULL);
	FListMenuItem *GetItem(FName name);
	bool Responder (event_t *ev);
	bool MenuEvent (int mkey, bool fromcontroller);
	bool MouseEvent(int type, int x, int y);
	void Ticker ();
	void Drawer ();
	void SetFocus(FListMenuItem *fc)
	{
		mFocusControl = fc;
	}
	bool CheckFocus(FListMenuItem *fc)
	{
		return mFocusControl == fc;
	}
	void ReleaseFocus()
	{
		mFocusControl = NULL;
	}
	const FListMenuDescriptor* Descriptor() const
	{
		return mDesc;
	}
};


//=============================================================================
//
// base class for menu items
//
//=============================================================================

class FOptionMenuItem : public FListMenuItem
{
protected:
	FString mLabel;
	bool mCentered = false;

	void drawLabel(int indent, int y, EColorRange color, bool grayed = false);
public:

	FOptionMenuItem(const char *text, FName action = NAME_None, bool center = false)
		: FListMenuItem(0, 0, action)
	{
		mLabel = text;
		mCentered = center;
	}

	~FOptionMenuItem();
	virtual int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected);
	virtual bool Selectable();
	virtual int GetIndent();
	virtual bool MouseEvent(int type, int x, int y);
};	

//=============================================================================
//
//
//
//=============================================================================
struct FOptionValues
{
	struct Pair
	{
		double Value;
		FString TextValue;
		FString Text;
	};

	TArray<Pair> mValues;
};

typedef TMap< FName, FOptionValues* > FOptionMap;

extern FOptionMap OptionValues;


//=============================================================================
//
// Option menu class runs a menu described by a FOptionMenuDescriptor
//
//=============================================================================

class DOptionMenu : public DMenu
{
	using Super = DMenu;
	bool CanScrollUp;
	bool CanScrollDown;
	int VisBottom;
	FOptionMenuItem *mFocusControl;

protected:
	FOptionMenuDescriptor *mDesc;

public:
	FOptionMenuItem *GetItem(FName name);
	DOptionMenu(DMenu *parent = NULL, FOptionMenuDescriptor *desc = NULL);
	virtual void Init(DMenu *parent = NULL, FOptionMenuDescriptor *desc = NULL);
	int FirstSelectable();
	bool Responder (event_t *ev);
	bool MenuEvent (int mkey, bool fromcontroller);
	bool MouseEvent(int type, int x, int y);
	void Ticker ();
	void Drawer ();
	const FOptionMenuDescriptor *GetDescriptor() const { return mDesc; }
	void SetFocus(FOptionMenuItem *fc)
	{
		mFocusControl = fc;
	}
	bool CheckFocus(FOptionMenuItem *fc)
	{
		return mFocusControl == fc;
	}
	void ReleaseFocus()
	{
		mFocusControl = NULL;
	}
};


//=============================================================================
//
// Input some text
//
//=============================================================================

class DTextEnterMenu : public DMenu
{
	using Super = DMenu;
	TArray<char> mEnterString;
	FString* mOutString;
	unsigned int mEnterSize;
	unsigned int mEnterPos;
	int mSizeMode; // 1: size is length in chars. 2: also check string width
	bool mInputGridOkay;

	int InputGridX;
	int InputGridY;

	// [TP]
	bool AllowColors;

public:

	// [TP] Added allowcolors
	DTextEnterMenu(DMenu *parent, FString &textbuffer, int sizemode, bool showgrid, bool allowcolors = false);

	void Drawer ();
	bool MenuEvent (int mkey, bool fromcontroller);
	bool Responder(event_t *ev);
	bool TranslateKeyboardEvents();
	bool MouseEvent(int type, int x, int y);

};




struct event_t;
void M_EnableMenu (bool on) ;
bool M_Responder (event_t *ev);
void M_Ticker (void);
void M_Drawer (void);
void M_Init (void);
void M_CreateMenus();
void M_ActivateMenu(DMenu *menu);
void M_ClearMenus ();
void M_ParseMenuDefs();
void M_StartupSkillMenu(FGameStartup *gs);
int M_GetDefaultSkill();
void M_StartControlPanel (bool makeSound);
bool M_SetMenu(FName menu, int param = -1, FName callingMenu = NAME_None);
void M_NotifyNewSave (const char *file, const char *title, bool okForQuicksave);
void M_StartMessage(const char *message, int messagemode, FName action = NAME_None);
void M_UnhideCustomMenu(int menu, int itemmask);


void I_SetMouseCapture();
void I_ReleaseMouseCapture();

struct MenuClassDescriptor;
extern TArray<MenuClassDescriptor*> menuClasses;

struct MenuClassDescriptor
{
	FName mName;
	MenuClassDescriptor(const char* name) : mName(name)
	{
		//menuClasses.Push(this);
	}

	virtual DMenu* CreateNew() = 0;
};

template<class Menu> struct TMenuClassDescriptor : public MenuClassDescriptor
{
	TMenuClassDescriptor(const char* name) : MenuClassDescriptor(name)
	{}
	DMenu* CreateNew()
	{
		return new Menu;
	}
};

#endif