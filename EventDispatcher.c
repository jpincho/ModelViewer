#include "EventDispatcher.h"
#include <Platform/PointerList.h>

static PointerList EventListeners = { 0 };

void edAddEventListener ( const edWindowEventListenerCallback Callback )
	{
	PointerListNode *Node = PointerList_Find ( &EventListeners, NULL, Callback );
	if ( Node == NULL )
		PointerList_AddAtEnd ( &EventListeners, Callback );
	}

void edRemoveEventListener ( const edWindowEventListenerCallback Callback )
	{
	PointerListNode *Node = PointerList_Find ( &EventListeners, NULL, Callback );
	if ( Node != NULL )
		PointerList_DestroyNode ( &EventListeners, Node );
	}

void edSendWindowClosedEvent ( const crWindowHandle WindowHandle )
	{
	edWindowEvent NewEvent = { 0 };
	NewEvent.OwnerHandle = WindowHandle;
	NewEvent.EventType = edWindowEventType_WindowClosed;
	for ( PointerListNode *Node = PointerList_GetFirst ( &EventListeners ); Node != NULL; Node = PointerList_GetNextNode ( Node ) )
		{
		edWindowEventListenerCallback Callback = PointerList_GetNodeData ( Node );
		Callback ( NewEvent );
		}
	}

void edSendKeyStateChangedEvent ( const crWindowHandle WindowHandle, const crKeyCode KeyCode, const bool NewState )
	{
	edWindowEvent NewEvent = { 0 };
	NewEvent.OwnerHandle = WindowHandle;
	NewEvent.EventType = edWindowEventType_KeyStateChanged;
	NewEvent.EventData.KeyStateChanged.Key = KeyCode;
	NewEvent.EventData.KeyStateChanged.State = NewState;
	for ( PointerListNode *Node = PointerList_GetFirst ( &EventListeners ); Node != NULL; Node = PointerList_GetNextNode ( Node ) )
		{
		edWindowEventListenerCallback Callback = PointerList_GetNodeData ( Node );
		Callback ( NewEvent );
		}
	}

void edSendMouseMovedEvent ( const crWindowHandle WindowHandle, const ivec2 NewPosition, const ivec2 Delta )
	{
	edWindowEvent NewEvent = { 0 };
	NewEvent.OwnerHandle = WindowHandle;
	NewEvent.EventType = edWindowEventType_MouseMoved;
	NewEvent.EventData.MouseMoved.NewPosition = NewPosition;
	NewEvent.EventData.MouseMoved.Delta = Delta;
	for ( PointerListNode *Node = PointerList_GetFirst ( &EventListeners ); Node != NULL; Node = PointerList_GetNextNode ( Node ) )
		{
		edWindowEventListenerCallback Callback = PointerList_GetNodeData ( Node );
		Callback ( NewEvent );
		}
	}

void edSendMouseButtonStateChangedEvent ( const crWindowHandle WindowHandle, const uint32_t Button, const bool State )
	{
	edWindowEvent NewEvent = { 0 };
	NewEvent.OwnerHandle = WindowHandle;
	NewEvent.EventType = edWindowEventType_MouseButtonStateChanged;
	NewEvent.EventData.MouseButtonStateChanged.Button = Button;
	NewEvent.EventData.MouseButtonStateChanged.State = State;
	for ( PointerListNode *Node = PointerList_GetFirst ( &EventListeners ); Node != NULL; Node = PointerList_GetNextNode ( Node ) )
		{
		edWindowEventListenerCallback Callback = PointerList_GetNodeData ( Node );
		Callback ( NewEvent );
		}
	}

void edSendMouseWheelEvent ( const crWindowHandle WindowHandle, const int Amount )
	{
	edWindowEvent NewEvent = { 0 };
	NewEvent.OwnerHandle = WindowHandle;
	NewEvent.EventType = edWindowEventType_MouseWheel;
	NewEvent.EventData.MouseWheel.Amount = Amount;
	for ( PointerListNode *Node = PointerList_GetFirst ( &EventListeners ); Node != NULL; Node = PointerList_GetNextNode ( Node ) )
		{
		edWindowEventListenerCallback Callback = PointerList_GetNodeData ( Node );
		Callback ( NewEvent );
		}
	}