/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __WGUI_H__
#define __WGUI_H__
#include <string>
#include "wguidefinitions.h"
#include "wiskin.h"
#include "wguifactories.h"
#include "wihandle.h"
#include <materialmanager.h>
#include <unordered_map>
#include <algorithm>
#include <wrappers/buffer.h>
#include <iglfw/glfw_window.h>
#include <prosper_context_object.hpp>
#include <sharedutils/chronotime.h>

#undef GetClassName
class WIBase;
class WISkin;
class WIHandle;
namespace GLFW {class Joystick;};
namespace prosper
{
	class UniformResizableBuffer;
	class Buffer;
	class DescriptorSetGroup;
	class Shader;
};

namespace wgui
{
	class Shader;
	class ShaderColored;
	class ShaderColoredRect;
	class ShaderColoredLine;
	class ShaderText;
	class ShaderTextRect;
	class ShaderTextured;
	class ShaderTexturedRect;
};
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLWGUI WGUI
	: public prosper::ContextObject
{
public:
	friend WIBase;
	enum class ElementBuffer : uint32_t
	{
		SizeColor = sizeof(Vector4),
		SizeMVP = sizeof(Mat4),

		OffsetColor = 0,
		OffsetMVP = OffsetColor +SizeColor,

		Size = OffsetMVP +SizeMVP
	};
	enum class ElementVertex : uint32_t
	{
		SizePos = sizeof(Vector2),
		SizeUV = sizeof(Vector2),

		Size = SizePos +SizeUV
	};
	enum class ResultCode : uint8_t
	{
		Ok = 0u,
		UnableToInitializeFontManager,
		ErrorInitializingShaders,
		FontNotFound,
	};
	WGUI(prosper::Context &context,const std::weak_ptr<MaterialManager> &wpMatManager);
	WGUI(const WGUI&)=delete;
	~WGUI();
	WGUI &operator=(const WGUI&)=delete;

	static WGUI &Open(prosper::Context &context,const std::weak_ptr<MaterialManager> &wpMatManager);
	static void Close();
	static WGUI &GetInstance();

	ResultCode Initialize();
	template<class TElement>
		TElement *Create(WIBase *parent=nullptr);
	template<class TElement>
		void Setup(WIBase *el,WIBase *parent=nullptr);
	WIBase *Create(std::string classname,WIBase *parent=nullptr);
	void RemoveSafely(WIBase *gui);
	void Remove(WIBase *gui);
	WIBase *GetBaseElement();
	WIBase *GetFocusedElement();
	void Think();
	void Draw();
	bool HandleJoystickInput(GLFW::Window &window,const GLFW::Joystick &joystick,uint32_t key,GLFW::KeyState state);
	bool HandleMouseInput(GLFW::Window &window,GLFW::MouseButton button,GLFW::KeyState state,GLFW::Modifier mods);
	bool HandleKeyboardInput(GLFW::Window &window,GLFW::Key key,int scanCode,GLFW::KeyState state,GLFW::Modifier mods);
	bool HandleCharInput(GLFW::Window &window,unsigned int c);
	bool HandleScrollInput(GLFW::Window &window,Vector2 offset);
	void SetHandleFactory(WIHandle *(*handleFactory)(WIBase*));
	void SetCreateCallback(void(*callback)(WIBase*));
	void SetRemoveCallback(void(*callback)(WIBase*));
	void GetMousePos(int &x,int &y);
	template<class TSkin>
		TSkin *RegisterSkin(std::string id,bool bReload=false);
	void SetSkin(std::string skin);
	std::string GetSkinName();
	WISkin *GetSkin();
	WISkin *GetSkin(std::string name);
	void SetCursor(GLFW::Cursor::Shape cursor);
	void SetCursor(GLFW::Cursor &cursor);
	void ResetCursor();
	void SetCursorInputMode(GLFW::CursorMode mode);
	GLFW::Cursor::Shape GetCursor();
	GLFW::CursorMode GetCursorInputMode();
	MaterialManager &GetMaterialManager();
	WIBase *GetGUIElement(WIBase *el,int32_t x,int32_t y,const std::function<bool(WIBase*)> &condition);
	WIBase *GetCursorGUIElement(WIBase *el,const std::function<bool(WIBase*)> &condition);
	double GetDeltaTime() const;
	//prosper::util::record_set_scissor(*drawCmd,szScissor[0],szScissor[1],posScissor[0],posScissor[1]); // Use parent scissor values

	wgui::ShaderColored *GetColoredShader();
	wgui::ShaderColoredRect *GetColoredRectShader();
	wgui::ShaderColoredLine *GetColoredLineShader();
	wgui::ShaderText *GetTextShader();
	wgui::ShaderTextRect *GetTextRectShader();
	wgui::ShaderTextured *GetTexturedShader();
	wgui::ShaderTexturedRect *GetTexturedRectShader();
private:
	void SetScissor(uint32_t x,uint32_t y,uint32_t w,uint32_t h);
	void GetScissor(uint32_t &x,uint32_t &y,uint32_t &w,uint32_t &h);
	friend WIBase;
	friend wgui::Shader;
	friend wgui::ShaderColoredRect;

	WISkin *m_skin = nullptr;
	std::unordered_map<std::string,WISkin*> m_skins = {};
	std::weak_ptr<MaterialManager> m_matManager = {};
	std::shared_ptr<prosper::UniformResizableBuffer> m_elementBuffer = nullptr;
	WIHandle m_base = {};
	WIHandle m_focused = {};
	std::queue<WIHandle> m_removeQueue;
	WIHandle *(*m_handleFactory)(WIBase*) = nullptr;
	std::vector<std::unique_ptr<GLFW::Cursor>> m_cursors;
	GLFW::Cursor::Shape m_cursor = GLFW::Cursor::Shape::Arrow;
	GLFW::CursorHandle m_customCursor = {};
	void (*m_createCallback)(WIBase*) = nullptr;
	void (*m_removeCallback)(WIBase*) = nullptr;
	ChronoTime m_time = {};
	double m_tLastThink = 0;
	double m_tDelta = 0.f;

	util::WeakHandle<prosper::Shader> m_shaderColored = {};
	util::WeakHandle<prosper::Shader> m_shaderColoredCheap = {};
	util::WeakHandle<prosper::Shader> m_shaderColoredLine = {};
	util::WeakHandle<prosper::Shader> m_shaderText = {};
	util::WeakHandle<prosper::Shader> m_shaderTextCheap = {};
	util::WeakHandle<prosper::Shader> m_shaderTextured = {};
	util::WeakHandle<prosper::Shader> m_shaderTexturedCheap = {};

	bool SetFocusedElement(WIBase *gui);
	void ClearSkin();
};
REGISTER_BASIC_ARITHMETIC_OPERATORS(WGUI::ElementBuffer);
#pragma warning(pop)

#include <wgui/wibase.h>
#include "wihandle.h"

template<class TSkin>
	TSkin *WGUI::RegisterSkin(std::string id,bool bReload)
{
	std::transform(id.begin(),id.end(),id.begin(),::tolower);
	std::unordered_map<std::string,WISkin*>::iterator it = m_skins.find(id);
	if(it != m_skins.end())
	{
		if(bReload == false)
			return NULL;
		if(m_skin == it->second)
			ClearSkin();
		delete it->second;
		m_skins.erase(it);
	}
	TSkin *skin = new TSkin(id);
	m_skins.insert(std::unordered_map<std::string,WISkin*>::value_type(id,skin));
	return skin;
}

template<class TElement>
	void WGUI::Setup(WIBase *el,WIBase *parent)
{
	if(m_handleFactory != nullptr)
	{
		WIHandle *handle = m_handleFactory(el);
		if(handle != nullptr)
			el->InitializeHandle(handle);
		else
			el->InitializeHandle();
	}
	else
		el->InitializeHandle();
	if(parent != nullptr)
		el->SetParent(parent);
	else if(m_base.IsValid())
		el->SetParent(m_base.get());
	el->Initialize();
	WGUIClassMap *map = GetWGUIClassMap();
	std::string classname;
	if(map->GetClassName(typeid(TElement),&classname))
		el->m_class = classname;
	if(m_createCallback != nullptr)
		m_createCallback(el);
}

template<class TElement>
	TElement *WGUI::Create(WIBase *parent)
{
	TElement *el = new TElement;
	Setup<TElement>(el,parent);
	return el;
}

#endif