#pragma once
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#ifndef IM_ASSERT
#include <assert.h>
#define IM_ASSERT(_EXPR)            assert(_EXPR)
#endif

typedef int ImGuiSliderFlags;
typedef int ImGuiPopupFlags;
typedef int ImGuiInputTextFlags;
typedef int ImGuiTableFlags;
typedef int ImGuiWindowFlags;
typedef int ImGuiTableRowFlags;
enum ImGuiSliderFlags_
{
    ImGuiSliderFlags_None                   = 0,
    ImGuiSliderFlags_AlwaysClamp            = 1 << 4,
    ImGuiSliderFlags_Logarithmic            = 1 << 5,
    ImGuiSliderFlags_NoRoundToFormat        = 1 << 6,
    ImGuiSliderFlags_NoInput                = 1 << 7,
    ImGuiSliderFlags_InvalidMask_           = 0x7000000F
};
enum ImGuiInputTextFlags_
{
    ImGuiInputTextFlags_None                = 0,
    ImGuiInputTextFlags_CharsDecimal        = 1 << 0,
    ImGuiInputTextFlags_CharsHexadecimal    = 1 << 1,
    ImGuiInputTextFlags_CharsUppercase      = 1 << 2,
    ImGuiInputTextFlags_CharsNoBlank        = 1 << 3,
    ImGuiInputTextFlags_AutoSelectAll       = 1 << 4,
    ImGuiInputTextFlags_EnterReturnsTrue    = 1 << 5,
    ImGuiInputTextFlags_CallbackCompletion  = 1 << 6,
    ImGuiInputTextFlags_CallbackHistory     = 1 << 7,
    ImGuiInputTextFlags_CallbackAlways      = 1 << 8,
    ImGuiInputTextFlags_CallbackCharFilter  = 1 << 9,
    ImGuiInputTextFlags_AllowTabInput       = 1 << 10,
    ImGuiInputTextFlags_CtrlEnterForNewLine = 1 << 11,
    ImGuiInputTextFlags_NoHorizontalScroll  = 1 << 12,
    ImGuiInputTextFlags_AlwaysOverwrite     = 1 << 13,
    ImGuiInputTextFlags_ReadOnly            = 1 << 14,
    ImGuiInputTextFlags_Password            = 1 << 15,
    ImGuiInputTextFlags_NoUndoRedo          = 1 << 16,
    ImGuiInputTextFlags_CharsScientific     = 1 << 17,
    ImGuiInputTextFlags_CallbackResize      = 1 << 18,
    ImGuiInputTextFlags_CallbackEdit        = 1 << 19
};
enum ImGuiPopupFlags_
{
    ImGuiPopupFlags_None                    = 0,
    ImGuiPopupFlags_MouseButtonLeft         = 0,
    ImGuiPopupFlags_MouseButtonRight        = 1,
    ImGuiPopupFlags_MouseButtonMiddle       = 2,
    ImGuiPopupFlags_MouseButtonMask_        = 0x1F,
    ImGuiPopupFlags_MouseButtonDefault_     = 1,
    ImGuiPopupFlags_NoOpenOverExistingPopup = 1 << 5,
    ImGuiPopupFlags_NoOpenOverItems         = 1 << 6,
    ImGuiPopupFlags_AnyPopupId              = 1 << 7,
    ImGuiPopupFlags_AnyPopupLevel           = 1 << 8,
    ImGuiPopupFlags_AnyPopup                = ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel
};
enum ImGuiWindowFlags_
{
    ImGuiWindowFlags_None                   = 0,
    ImGuiWindowFlags_NoTitleBar             = 1 << 0,
    ImGuiWindowFlags_NoResize               = 1 << 1,
    ImGuiWindowFlags_NoMove                 = 1 << 2,
    ImGuiWindowFlags_NoScrollbar            = 1 << 3,
    ImGuiWindowFlags_NoScrollWithMouse      = 1 << 4,
    ImGuiWindowFlags_NoCollapse             = 1 << 5,
    ImGuiWindowFlags_AlwaysAutoResize       = 1 << 6,
    ImGuiWindowFlags_NoBackground           = 1 << 7,
    ImGuiWindowFlags_NoSavedSettings        = 1 << 8,
    ImGuiWindowFlags_NoMouseInputs          = 1 << 9,
    ImGuiWindowFlags_MenuBar                = 1 << 10,
    ImGuiWindowFlags_HorizontalScrollbar    = 1 << 11,
    ImGuiWindowFlags_NoFocusOnAppearing     = 1 << 12,
    ImGuiWindowFlags_NoBringToFrontOnFocus  = 1 << 13,
    ImGuiWindowFlags_AlwaysVerticalScrollbar= 1 << 14,
    ImGuiWindowFlags_AlwaysHorizontalScrollbar=1<< 15,
    ImGuiWindowFlags_AlwaysUseWindowPadding = 1 << 16,
    ImGuiWindowFlags_NoNavInputs            = 1 << 18,
    ImGuiWindowFlags_NoNavFocus             = 1 << 19,
    ImGuiWindowFlags_UnsavedDocument        = 1 << 20,
    ImGuiWindowFlags_NoNav                  = ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
    ImGuiWindowFlags_NoDecoration           = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse,
    ImGuiWindowFlags_NoInputs               = ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus,
};
enum ImGuiTableFlags_
{
    // Features
    ImGuiTableFlags_None                       = 0,
    ImGuiTableFlags_Resizable                  = 1 << 0,
    ImGuiTableFlags_Reorderable                = 1 << 1,
    ImGuiTableFlags_Hideable                   = 1 << 2,
    ImGuiTableFlags_Sortable                   = 1 << 3,
    ImGuiTableFlags_NoSavedSettings            = 1 << 4, 
    ImGuiTableFlags_ContextMenuInBody          = 1 << 5, 
    // Decorations
    ImGuiTableFlags_RowBg                      = 1 << 6, 
    ImGuiTableFlags_BordersInnerH              = 1 << 7,   
    ImGuiTableFlags_BordersOuterH              = 1 << 8,  
    ImGuiTableFlags_BordersInnerV              = 1 << 9,  
    ImGuiTableFlags_BordersOuterV              = 1 << 10, 
    ImGuiTableFlags_BordersH                   = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersOuterH,
    ImGuiTableFlags_BordersV                   = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersOuterV,
    ImGuiTableFlags_BordersInner               = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_BordersInnerH,
    ImGuiTableFlags_BordersOuter               = ImGuiTableFlags_BordersOuterV | ImGuiTableFlags_BordersOuterH,
    ImGuiTableFlags_Borders                    = ImGuiTableFlags_BordersInner | ImGuiTableFlags_BordersOuter,
    ImGuiTableFlags_NoBordersInBody            = 1 << 11,
    ImGuiTableFlags_NoBordersInBodyUntilResize = 1 << 12,
    ImGuiTableFlags_SizingFixedFit             = 1 << 13, 
    ImGuiTableFlags_SizingFixedSame            = 2 << 13, 
    ImGuiTableFlags_SizingStretchProp          = 3 << 13,  
    ImGuiTableFlags_SizingStretchSame          = 4 << 13,
    ImGuiTableFlags_NoHostExtendX              = 1 << 16,
    ImGuiTableFlags_NoHostExtendY              = 1 << 17, 
    ImGuiTableFlags_NoKeepColumnsVisible       = 1 << 18,  
    ImGuiTableFlags_PreciseWidths              = 1 << 19, 
    ImGuiTableFlags_NoClip                     = 1 << 20,
    ImGuiTableFlags_PadOuterX                  = 1 << 21,
    ImGuiTableFlags_NoPadOuterX                = 1 << 22,
    ImGuiTableFlags_NoPadInnerX                = 1 << 23,
    ImGuiTableFlags_ScrollX                    = 1 << 24,
    ImGuiTableFlags_ScrollY                    = 1 << 25,
    ImGuiTableFlags_SortMulti                  = 1 << 26,
    ImGuiTableFlags_SortTristate               = 1 << 27,
    ImGuiTableFlags_SizingMask_                = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_SizingStretchSame
};
struct ImVec2
{
    float                                   x, y;
    ImVec2()                                { x = y = 0.0f; }
    ImVec2(float _x, float _y)              { x = _x; y = _y; }
    float  operator[] (size_t idx) const    { IM_ASSERT(idx <= 1); return (&x)[idx]; }
    float& operator[] (size_t idx)          { IM_ASSERT(idx <= 1); return (&x)[idx]; } 
};
enum ImGuiTableRowFlags_
{
    ImGuiTableRowFlags_None                         = 0,
    ImGuiTableRowFlags_Headers                      = 1 << 0
};
struct ImVec4
{
    float                                           x, y, z, w;
    ImVec4()                                        { x = y = z = w = 0.0f; }
    ImVec4(float _x, float _y, float _z, float _w)  { x = _x; y = _y; z = _z; w = _w; }
};