#pragma once
#include <C_CrossRenderer/CrossRenderer.h>
#include <stdint.h>

typedef enum
	{
	crWindowState_Minimized,
	crWindowState_Windowed,
	crWindowState_Maximized
	} crWindowState;

typedef enum
	{
	edWindowEventType_WindowClosed,
	edWindowEventType_WindowMoved,
	edWindowEventType_WindowResized,
	edWindowEventType_WindowFocusChanged,
	edWindowEventType_WindowStateChanged,
	edWindowEventType_WindowFramebufferResized,
	edWindowEventType_KeyStateChanged,
	edWindowEventType_MouseButtonStateChanged,
	edWindowEventType_MouseWheel,
	edWindowEventType_MouseMoved,
	edWindowEventType_Quit,
	edWindowEventType_TextInput
	} edWindowEventType;

typedef struct
	{
	edWindowEventType EventType;
	crWindowHandle OwnerHandle;
	union
		{
		struct
			{
			crKeyCode Key;
			bool State;
			} KeyStateChanged;
		struct
			{
			uint32_t Button;
			bool State;
			} MouseButtonStateChanged;
		struct
			{
			int Amount;
			} MouseWheel;
		struct
			{
			ivec2 NewPosition, Delta;
			} MouseMoved;
		struct
			{
			uint32_t Data1, Data2;
			} Raw;
		struct
			{
			ivec2 NewPosition;
			} WindowMoved;
		struct
			{
			uvec2 NewSize;
			} WindowResized, WindowFramebufferResized;
		struct
			{
			bool NewState;
			} WindowFocusChanged;
		struct
			{
			crWindowState NewState;
			} WindowStateChanged;
		struct
			{
			char Text[32];
			} TextInput;
		void *Pointer;
		} EventData;
	} edWindowEvent;

typedef void ( *edWindowEventListenerCallback ) ( const edWindowEvent Event );

void edAddEventListener ( const edWindowEventListenerCallback Callback );
void edRemoveEventListener ( const edWindowEventListenerCallback Callback );
void edSendWindowClosedEvent ( const crWindowHandle WindowHandle );
void edSendKeyStateChangedEvent ( const crWindowHandle WindowHandle, const crKeyCode KeyCode, const bool NewState );
void edSendMouseMovedEvent ( const crWindowHandle WindowHandle, const ivec2 NewPosition, const ivec2 Delta );
void edSendMouseButtonStateChangedEvent ( const crWindowHandle WindowHandle, const uint32_t Button, const bool State );
void edSendMouseWheelEvent ( const crWindowHandle WindowHandle, const int Amount );
