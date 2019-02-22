/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx_wgui.h"
#include "wgui/wgui.h"
#include "wgui/fontmanager.h"
#include "wgui/wibase.h"
#include "wgui/types/wiroot.h"
#include "wgui/wihandle.h"
#include <algorithm>
#include "wgui/wiskin.h"
#include "wgui/types/wiarrow.h"
#include "wgui/types/witext.h"
#include "wgui/shaders/wishader_colored.hpp"
#include "wgui/shaders/wishader_coloredline.hpp"
#include "wgui/shaders/wishader_text.hpp"
#include "wgui/shaders/wishader_textured.hpp"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <buffers/prosper_buffer.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#include <misc/memory_allocator.h>

static std::unique_ptr<WGUI> s_wgui = nullptr;
WGUI &WGUI::Open(prosper::Context &context,const std::weak_ptr<MaterialManager> &wpMatManager)
{
	s_wgui = nullptr;
	s_wgui = std::make_unique<WGUI>(context,wpMatManager);
	return GetInstance();
}
void WGUI::Close()
{
	if(s_wgui == nullptr)
		return;
	if(s_wgui->m_base.IsValid())
		delete s_wgui->m_base.get();
	s_wgui = nullptr;
	FontManager::Close();
	WIText::ClearTextBuffer();
}
WGUI &WGUI::GetInstance() {return *s_wgui;}

WGUI::WGUI(prosper::Context &context,const std::weak_ptr<MaterialManager> &wpMatManager)
	: prosper::ContextObject(context),m_matManager(wpMatManager)
{}

WGUI::~WGUI() {}

double WGUI::GetDeltaTime() const {return m_tDelta;}

wgui::ShaderColored *WGUI::GetColoredShader() {return static_cast<wgui::ShaderColored*>(m_shaderColored.get());}
wgui::ShaderColoredRect *WGUI::GetColoredRectShader() {return static_cast<wgui::ShaderColoredRect*>(m_shaderColoredCheap.get());}
wgui::ShaderColoredLine *WGUI::GetColoredLineShader() {return static_cast<wgui::ShaderColoredLine*>(m_shaderColoredLine.get());}
wgui::ShaderText *WGUI::GetTextShader() {return static_cast<wgui::ShaderText*>(m_shaderText.get());}
wgui::ShaderTextRect *WGUI::GetTextRectShader() {return static_cast<wgui::ShaderTextRect*>(m_shaderTextCheap.get());}
wgui::ShaderTextured *WGUI::GetTexturedShader() {return static_cast<wgui::ShaderTextured*>(m_shaderTextured.get());}
wgui::ShaderTexturedRect *WGUI::GetTexturedRectShader() {return static_cast<wgui::ShaderTexturedRect*>(m_shaderTexturedCheap.get());}

static std::array<uint32_t,4> s_scissor = {0u,0,0u,0u};
void WGUI::SetScissor(uint32_t x,uint32_t y,uint32_t w,uint32_t h)
{
#ifdef WGUI_ENABLE_SANITY_EXCEPTIONS
	if(x +w > std::numeric_limits<int32_t>::max() || y +h > std::numeric_limits<int32_t>::max())
		throw std::logic_error("Scissor out of bounds!");
#endif
	s_scissor = {x,y,w,h};
}
void WGUI::GetScissor(uint32_t &x,uint32_t &y,uint32_t &w,uint32_t &h)
{
	x = s_scissor.at(0u);
	y = s_scissor.at(1u);
	w = s_scissor.at(2u);
	h = s_scissor.at(3u);
}

WGUI::ResultCode WGUI::Initialize()
{
	if(!FontManager::Initialize())
		return ResultCode::UnableToInitializeFontManager;
	m_cursors.reserve(6);
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::Arrow));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::IBeam));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::Crosshair));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::Hand));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::HResize));
	m_cursors.push_back(GLFW::Cursor::Create(GLFW::Cursor::Shape::VResize));

	m_time.Update();
	m_tLastThink = static_cast<double>(m_time());
	auto &context = GetContext();
	auto &shaderManager = context.GetShaderManager();
	m_shaderColored = shaderManager.RegisterShader("wguicolored",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderColored(context,identifier);});
	m_shaderColoredCheap = shaderManager.RegisterShader("wguicolored_cheap",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderColoredRect(context,identifier);});
	m_shaderColoredLine = shaderManager.RegisterShader("wguicoloredline",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderColoredLine(context,identifier);});
	m_shaderText = shaderManager.RegisterShader("wguitext",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderText(context,identifier);});
	m_shaderTextCheap = shaderManager.RegisterShader("wguitext_cheap",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderTextRect(context,identifier);});
	m_shaderTextured = shaderManager.RegisterShader("wguitextured",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderTextured(context,identifier);});
	m_shaderTexturedCheap = shaderManager.RegisterShader("wguitextured_cheap",[](prosper::Context &context,const std::string &identifier) {return new wgui::ShaderTexturedRect(context,identifier);});
	
	if(wgui::Shader::DESCRIPTOR_SET.IsValid() == false)
		return ResultCode::ErrorInitializingShaders;

	// Font has to be loaded AFTER shaders have been initialized (Requires wguitext shader)
	auto font = FontManager::LoadFont("default","vera/VeraBd.ttf",14);
	FontManager::LoadFont("default_large","vera/VeraBd.ttf",18);
	FontManager::LoadFont("default_small","vera/VeraBd.ttf",10);
	FontManager::LoadFont("default_tiny","vera/VeraBd.ttf",8);
	if(font != nullptr)
		FontManager::SetDefaultFont(*font);
	else
		return ResultCode::FontNotFound;
	auto *base = new WIRoot;
	base->Initialize();
	base->InitializeHandle();
	m_base = base->GetHandle();
	base->SetSize(context.GetWindowWidth(),context.GetWindowHeight());
	base->Setup();
	return ResultCode::Ok;
}

MaterialManager &WGUI::GetMaterialManager() {return *m_matManager.lock();}

void WGUI::SetCursor(GLFW::Cursor::Shape cursor)
{
	if(m_customCursor == nullptr && cursor == m_cursor)
		return;
	if(cursor == GLFW::Cursor::Shape::Hidden)
	{
		SetCursorInputMode(GLFW::CursorMode::Hidden);
		return;
	}
	else if(m_cursor == GLFW::Cursor::Shape::Hidden)
		SetCursorInputMode(GLFW::CursorMode::Normal);
	auto icursor = static_cast<uint32_t>(cursor) -static_cast<uint32_t>(GLFW::Cursor::Shape::Arrow);
	if(icursor > static_cast<uint32_t>(GLFW::Cursor::Shape::VResize))
		return;
	SetCursor(*m_cursors[icursor].get());
	m_cursor = cursor;
	m_customCursor = GLFW::CursorHandle();
}
void WGUI::SetCursor(GLFW::Cursor &cursor)
{
	if(m_customCursor.IsValid() && m_customCursor.get() == &cursor)
		return;
	GetContext().GetWindow().SetCursor(cursor);
	m_customCursor = cursor.GetHandle();
}
void WGUI::SetCursorInputMode(GLFW::CursorMode mode) {GetContext().GetWindow().SetCursorInputMode(mode);}
GLFW::Cursor::Shape WGUI::GetCursor() {return m_cursor;}
GLFW::CursorMode WGUI::GetCursorInputMode() {return GetContext().GetWindow().GetCursorInputMode();}
void WGUI::ResetCursor() {SetCursor(GLFW::Cursor::Shape::Arrow);}

void WGUI::GetMousePos(int &x,int &y)
{
	auto &window = GetContext().GetWindow();
	auto cursorPos = window.GetCursorPos();
	x = static_cast<int>(cursorPos.x);
	y = static_cast<int>(cursorPos.y);
}

void WGUI::SetHandleFactory(WIHandle *(*handleFactory)(WIBase*)) {m_handleFactory = handleFactory;}

void WGUI::Think()
{
	while(!m_removeQueue.empty())
	{
		auto &hEl = m_removeQueue.front();
		if(hEl.IsValid())
			hEl->Remove();
		m_removeQueue.pop();
	}
	m_time.Update();
	auto t = m_time();
	m_tDelta = static_cast<double>(t -m_tLastThink);
	m_tLastThink = static_cast<double>(t);
	if(m_base.IsValid())
		m_base->Think();
}

void WGUI::Draw()
{
	auto &context = GetContext();
	auto drawCmd = context.GetDrawCommandBuffer();
	WIBase::RENDER_ALPHA = 1.f;
	if(!m_base.IsValid())
		return;
	auto *p = m_base.get();
	auto size = context.GetWindowSize();
	if(p->IsVisible())
		p->Draw(size.at(0),size.at(1));
}

WIBase *WGUI::Create(std::string classname,WIBase *parent)
{
	StringToLower(classname);
	WGUIClassMap *map = GetWGUIClassMap();
	WIBase*(*factory)(void) = map->FindFactory(classname);
	if(factory != NULL)
	{
		WIBase *p = factory();
		p->m_class = classname;
		if(parent != NULL)
			p->SetParent(parent);
		return p;
	}
	return NULL;
}

WIBase *WGUI::GetBaseElement()
{
	return m_base.get();
}

bool WGUI::SetFocusedElement(WIBase *gui)
{
	auto &context = GetContext();
	auto &window = context.GetWindow();
	if(gui != NULL && m_focused.IsValid())
	{
		WIBase *root = GetBaseElement();
		WIBase *parent = m_focused.get();
		while(parent != NULL && parent != root && parent != m_focused.get())
		{
			if(parent->IsFocusTrapped())
				return false;
			parent = parent->GetParent();
		}
		m_focused->KillFocus(true);
	}
	if(gui == NULL)
	{
		window.SetCursorInputMode(GLFW::CursorMode::Hidden);
		m_focused = {};
		return true;
	}
	window.SetCursorInputMode(GLFW::CursorMode::Normal);
	m_focused = gui->GetHandle();
	return true;
}

WIBase *WGUI::GetFocusedElement() {return m_focused.get();}

void WGUI::RemoveSafely(WIBase *gui)
{
	if(gui == nullptr)
		return;
	m_removeQueue.push(gui->GetHandle());
}

void WGUI::Remove(WIBase *gui)
{
	if(gui == m_base.get())
		return;
	auto hEl = gui->GetHandle();
	if(m_removeCallback != NULL)
		m_removeCallback(gui);
	if(!hEl.IsValid())
		return;
	if(gui == GetFocusedElement())
	{
		gui->TrapFocus(false);
		gui->KillFocus();
	}
	delete gui;
}

void WGUI::ClearSkin()
{
	if(m_skin == nullptr)
		return;
	if(!m_base.IsValid())
	{
		m_skin = nullptr;
		return;
	}
	m_base->ResetSkin();
	m_skin = nullptr;
}

void WGUI::SetSkin(std::string skin)
{
	if(!m_base.IsValid())
		return;
	StringToLower(skin);
	std::unordered_map<std::string,WISkin*>::iterator it = m_skins.find(skin);
	if(m_skin != nullptr && it != m_skins.end() && it->second == m_skin)
		return;
	m_base->ResetSkin();
	if(it == m_skins.end())
	{
		m_skin = nullptr;
		return;
	}
	m_skin = it->second;
	if(m_skin == nullptr)
		return;
	m_base->ApplySkin(m_skin);
}
WISkin *WGUI::GetSkin() {return m_skin;}
WISkin *WGUI::GetSkin(std::string name)
{
	std::unordered_map<std::string,WISkin*>::iterator it = m_skins.find(name);
	if(it == m_skins.end())
		return NULL;
	return it->second;
}
std::string WGUI::GetSkinName()
{
	if(m_skin == NULL)
		return "";
	return m_skin->m_identifier;
}

void WGUI::SetCreateCallback(void(*callback)(WIBase*))
{
	m_createCallback = callback;
}

void WGUI::SetRemoveCallback(void(*callback)(WIBase*))
{
	m_removeCallback = callback;
}

bool WGUI::HandleJoystickInput(GLFW::Window &window,const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state)
{
	return WIBase::__wiJoystickCallback(window,joystick,key,state);
}

bool WGUI::HandleMouseInput(GLFW::Window &window,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods)
{
	return WIBase::__wiMouseButtonCallback(window,button,state,mods);
}

bool WGUI::HandleKeyboardInput(GLFW::Window &window,GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods)
{
	return WIBase::__wiKeyCallback(window,key,scanCode,state,mods);
}

bool WGUI::HandleCharInput(GLFW::Window &window,unsigned int c)
{
	return WIBase::__wiCharCallback(window,c);
}

bool WGUI::HandleScrollInput(GLFW::Window &window,Vector2 offset)
{
	return WIBase::__wiScrollCallback(window,offset);
}

static WIBase *check_children(WIBase *gui,int x,int y,int32_t *bestZPos,const std::function<bool(WIBase*)> &condition)
{
	if(!gui->PosInBounds(x,y))
		return nullptr;
	auto *children = gui->GetChildren();
	int32_t localBestZPos = -1;
	for(auto &hnd : *children)
	{
		auto *el = hnd.get();
		if(el != nullptr && el->IsVisible())
		{
			auto *child = check_children(el,x,y,bestZPos,condition);
			if(child != nullptr && (condition == nullptr || condition(child) == true))
			{
				auto zPos = el->GetZPos(); // child->GetZPos();
				if(zPos >= localBestZPos)
				{
					localBestZPos = zPos;
					if(zPos > *bestZPos)
						*bestZPos = zPos;
					gui = child;
				}
			}
		}
	}
	return gui;
}
static WIBase *check_children(WIBase *gui,int x,int y,const std::function<bool(WIBase*)> &condition)
{
	int32_t bestZPos = -1;
	return check_children(gui,x,y,&bestZPos,condition);
}

WIBase *WGUI::GetGUIElement(WIBase *el,int32_t x,int32_t y,const std::function<bool(WIBase*)> &condition)
{
	return check_children((el != nullptr) ? el : GetBaseElement(),x,y,condition);
}

WIBase *WGUI::GetCursorGUIElement(WIBase *el,const std::function<bool(WIBase*)> &condition)
{
	int32_t x,y;
	GetMousePos(x,y);
	return GetGUIElement(el,x,y,condition);
}