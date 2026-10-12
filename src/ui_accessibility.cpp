#include "ui_accessibility.h"
#include <oleacc.h>
#include <commctrl.h>
#include <algorithm>
#include <cmath>
#include <cwchar>
namespace Ui::Accessibility {
class Enumeration final:public IEnumVARIANT {
    LONG refs=1;std::vector<int> ids;size_t cursor=0;
public:
    Enumeration(const std::vector<int>& values,size_t offset=0):ids(values),cursor(offset){}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(iid==IID_IUnknown||iid==IID_IEnumVARIANT){*out=static_cast<IEnumVARIANT*>(this);AddRef();return S_OK;}return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs);}
    ULONG STDMETHODCALLTYPE Release()override{LONG n=InterlockedDecrement(&refs);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE Next(ULONG count,VARIANT* values,ULONG* fetched)override{if(!values||(!fetched&&count!=1))return E_POINTER;ULONG n=0;while(n<count&&cursor<ids.size()){VariantInit(&values[n]);values[n].vt=VT_I4;values[n++].lVal=ids[cursor++];}if(fetched)*fetched=n;return n==count?S_OK:S_FALSE;}
    HRESULT STDMETHODCALLTYPE Skip(ULONG n)override{size_t remaining=ids.size()-cursor;cursor+=std::min<size_t>(n,remaining);return n<=remaining?S_OK:S_FALSE;}
    HRESULT STDMETHODCALLTYPE Reset()override{cursor=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE Clone(IEnumVARIANT** out)override{if(!out)return E_POINTER;*out=new Enumeration(ids,cursor);return S_OK;}
};
class Provider final:public IAccessible {
    LONG refs=1; HWND window; Controls controls;std::function<void(int)> activate,focus;std::function<bool(int,double)> setValue;
    std::vector<Control> previous;
    std::vector<Control> Items(){return IsWindow(window)?controls():std::vector<Control>{};}
    bool Item(VARIANT child,Control& item){if(child.vt!=VT_I4)return false;for(auto c:Items())if(c.id==child.lVal){item=c;return true;}return false;}
    HRESULT String(VARIANT child,BSTR* result,int kind){if(!result)return E_POINTER;*result=nullptr;Control c;
        if(!IsWindow(window))return CO_E_OBJNOTCONNECTED;
        if(child.vt==VT_I4&&child.lVal==CHILDID_SELF){if(kind==2)return S_FALSE;wchar_t name[256]{};GetWindowTextW(window,name,256);*result=SysAllocString(name);return S_OK;}
        if(!Item(child,c))return E_INVALIDARG;
        auto& text=kind==0?c.name:kind==1?c.description:c.value;*result=SysAllocString(text.c_str());return *result?S_OK:E_OUTOFMEMORY;}
public:
    Provider(HWND hwnd,Controls c,std::function<void(int)> a,std::function<void(int)> f,std::function<bool(int,double)> v):window(hwnd),controls(c),activate(a),focus(f),setValue(v){previous=Items();}
    void Changed(){
        if(!IsWindow(window))return;
        auto current=Items();
        if(current.size()!=previous.size())NotifyWinEvent(EVENT_OBJECT_REORDER,window,OBJID_CLIENT,CHILDID_SELF);
        for(const auto& c:current){auto old=std::find_if(previous.begin(),previous.end(),[&](const Control& p){return p.id==c.id;});
            if(old==previous.end()||old->state!=c.state)NotifyWinEvent(EVENT_OBJECT_STATECHANGE,window,OBJID_CLIENT,c.id);
            if(old==previous.end()||old->value!=c.value)NotifyWinEvent(EVENT_OBJECT_VALUECHANGE,window,OBJID_CLIENT,c.id);
            if(old==previous.end()||old->name!=c.name)NotifyWinEvent(EVENT_OBJECT_NAMECHANGE,window,OBJID_CLIENT,c.id);
            if(old==previous.end()||old->description!=c.description)NotifyWinEvent(EVENT_OBJECT_DESCRIPTIONCHANGE,window,OBJID_CLIENT,c.id);
        }
        previous=std::move(current);
    }
    void Disconnect(){window=nullptr;controls={};activate={};focus={};setValue={};}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** out)override{if(!out)return E_POINTER;*out=nullptr;if(iid==IID_IEnumVARIANT){std::vector<int> ids;for(auto c:Items())ids.push_back(c.id);*out=static_cast<IEnumVARIANT*>(new Enumeration(ids));return S_OK;}if(iid==IID_IUnknown||iid==IID_IDispatch||iid==IID_IAccessible){*out=static_cast<IAccessible*>(this);AddRef();return S_OK;}return E_NOINTERFACE;}
    ULONG STDMETHODCALLTYPE AddRef()override{return InterlockedIncrement(&refs);}
    ULONG STDMETHODCALLTYPE Release()override{LONG n=InterlockedDecrement(&refs);if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* n)override{if(!n)return E_POINTER;*n=0;return S_OK;}
    HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT,LCID,ITypeInfo**)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID,LPOLESTR*,UINT,LCID,DISPID*)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE Invoke(DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE get_accParent(IDispatch** out)override{if(!out)return E_POINTER;*out=nullptr;if(!IsWindow(window))return CO_E_OBJNOTCONNECTED;return AccessibleObjectFromWindow(window,OBJID_WINDOW,IID_IDispatch,reinterpret_cast<void**>(out));}
    HRESULT STDMETHODCALLTYPE get_accChildCount(LONG* n)override{if(!n)return E_POINTER;*n=LONG(Items().size());return IsWindow(window)?S_OK:CO_E_OBJNOTCONNECTED;}
    HRESULT STDMETHODCALLTYPE get_accChild(VARIANT child,IDispatch** out)override{if(!out)return E_POINTER;*out=nullptr;if(child.vt==VT_I4){HWND native=GetDlgItem(window,child.lVal);if(native)return AccessibleObjectFromWindow(native,OBJID_CLIENT,IID_IDispatch,reinterpret_cast<void**>(out));}return S_FALSE;}
    HRESULT STDMETHODCALLTYPE get_accName(VARIANT c,BSTR* s)override{return String(c,s,0);}
    HRESULT STDMETHODCALLTYPE get_accValue(VARIANT c,BSTR* s)override{return String(c,s,2);}
    HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT c,BSTR* s)override{return String(c,s,1);}
    HRESULT STDMETHODCALLTYPE get_accRole(VARIANT child,VARIANT* out)override{if(!out)return E_POINTER;VariantInit(out);Control c;if(child.vt==VT_I4&&child.lVal==0){out->vt=VT_I4;out->lVal=ROLE_SYSTEM_PANE;return S_OK;}if(!Item(child,c))return E_INVALIDARG;out->vt=VT_I4;out->lVal=c.role;return S_OK;}
    HRESULT STDMETHODCALLTYPE get_accState(VARIANT child,VARIANT* out)override{if(!out)return E_POINTER;VariantInit(out);Control c;out->vt=VT_I4;if(child.vt==VT_I4&&child.lVal==0){out->lVal=IsWindowVisible(window)?0:STATE_SYSTEM_INVISIBLE;return S_OK;}if(!Item(child,c))return E_INVALIDARG;out->lVal=c.state|(IsWindowVisible(window)?0:STATE_SYSTEM_INVISIBLE);return S_OK;}
    HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT c,BSTR* s)override{return String(c,s,1);}
    HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR* s,VARIANT,LONG* n)override{if(s)*s=nullptr;if(n)*n=-1;return S_FALSE;}
    HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT,BSTR* s)override{if(!s)return E_POINTER;*s=nullptr;return S_FALSE;}
    HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT* out)override{if(!out)return E_POINTER;VariantInit(out);for(auto c:Items())if(c.state&STATE_SYSTEM_FOCUSED){out->vt=VT_I4;out->lVal=c.id;return S_OK;}return S_FALSE;}
    HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT* out)override{if(!out)return E_POINTER;VariantInit(out);for(auto c:Items())if(c.state&STATE_SYSTEM_SELECTED){out->vt=VT_I4;out->lVal=c.id;return S_OK;}return S_FALSE;}
    HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT c,BSTR* s)override{if(!s)return E_POINTER;*s=nullptr;Control item;if(!Item(c,item))return E_INVALIDARG;if(item.role==ROLE_SYSTEM_SLIDER||item.role==ROLE_SYSTEM_STATICTEXT)return S_FALSE;return String(c,s,1);}
    HRESULT STDMETHODCALLTYPE accSelect(LONG flags,VARIANT child)override{Control c;if(!Item(child,c))return E_INVALIDARG;if(flags&SELFLAG_TAKEFOCUS){SetFocus(window);focus(c.id);return S_OK;}return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE accLocation(LONG* x,LONG* y,LONG* w,LONG* h,VARIANT child)override{if(!x||!y||!w||!h)return E_POINTER;RECT r{};Control c;
        if(child.vt==VT_I4&&child.lVal==0)GetClientRect(window,&r);else if(Item(child,c))r=c.rect;else return E_INVALIDARG;
        POINT p{r.left,r.top};ClientToScreen(window,&p);*x=p.x;*y=p.y;*w=r.right-r.left;*h=r.bottom-r.top;return S_OK;}
    HRESULT STDMETHODCALLTYPE accNavigate(LONG direction,VARIANT child,VARIANT* out)override{if(!out)return E_POINTER;VariantInit(out);auto items=Items();if(items.empty())return S_FALSE;
        if(child.vt!=VT_I4)return E_INVALIDARG;
        if(child.lVal==0&&(direction==NAVDIR_FIRSTCHILD||direction==NAVDIR_LASTCHILD)){out->vt=VT_I4;out->lVal=direction==NAVDIR_FIRSTCHILD?items.front().id:items.back().id;return S_OK;}
        for(size_t i=0;i<items.size();++i)if(items[i].id==child.lVal){int n=int(i)+(direction==NAVDIR_NEXT?1:direction==NAVDIR_PREVIOUS?-1:0);if(n>=0&&size_t(n)<items.size()&&n!=int(i)){out->vt=VT_I4;out->lVal=items[n].id;return S_OK;}}
        return S_FALSE;}
    HRESULT STDMETHODCALLTYPE accHitTest(LONG x,LONG y,VARIANT* out)override{if(!out)return E_POINTER;VariantInit(out);POINT p{x,y};ScreenToClient(window,&p);for(auto c:Items())if(PtInRect(&c.rect,p)){out->vt=VT_I4;out->lVal=c.id;return S_OK;}out->vt=VT_I4;out->lVal=CHILDID_SELF;return S_OK;}
    HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT child)override{Control c;if(!Item(child,c))return E_INVALIDARG;if(c.state&STATE_SYSTEM_UNAVAILABLE)return S_FALSE;if(c.role==ROLE_SYSTEM_SLIDER||c.role==ROLE_SYSTEM_STATICTEXT)return E_NOTIMPL;activate(c.id);Changed();return S_OK;}
    HRESULT STDMETHODCALLTYPE put_accName(VARIANT,BSTR)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE put_accValue(VARIANT child,BSTR text)override{Control c;if(!Item(child,c))return E_INVALIDARG;if(!setValue)return E_NOTIMPL;if(!text||(c.state&STATE_SYSTEM_UNAVAILABLE))return E_INVALIDARG;wchar_t* end=nullptr;double value=std::wcstod(text,&end);if(end==text||*end||!std::isfinite(value))return E_INVALIDARG;if(!setValue(c.id,value))return E_INVALIDARG;Changed();return S_OK;}
};
static LRESULT CALLBACK Subclass(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR ref){auto p=reinterpret_cast<Provider*>(ref);
    if(msg==WM_GETOBJECT&&LONG(lp)==OBJID_CLIENT)return LresultFromObject(IID_IAccessible,wp,p);
    p->AddRef(); // A control action can synchronously destroy its window.
    if(msg==WM_NCDESTROY){p->Disconnect();RemoveWindowSubclass(hwnd,Subclass,98);p->Release();}
    LRESULT result=DefSubclassProc(hwnd,msg,wp,lp);
    // Bounded input/position notifications, never the visualization frame timer.
    if(msg==WM_KEYDOWN||msg==WM_LBUTTONUP||msg==WM_LBUTTONDBLCLK||msg==WM_APPCOMMAND||msg==WM_COMMAND||msg==WM_HOTKEY||msg==WM_COPYDATA||msg==WM_APP+56||msg==WM_APP+55||msg==WM_DPICHANGED||(msg==WM_TIMER&&wp==1))p->Changed();
    p->Release();
    return result;
}
void Attach(HWND hwnd,Controls controls,std::function<void(int)> activate,std::function<void(int)> focus,std::function<bool(int,double)> value){auto p=new Provider(hwnd,controls,activate,focus,value);if(!SetWindowSubclass(hwnd,Subclass,98,reinterpret_cast<DWORD_PTR>(p)))p->Release();}
static LRESULT CALLBACK NativeSubclass(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
    if(msg==WM_NCDESTROY){IAccPropServices* properties=nullptr;if(SUCCEEDED(CoCreateInstance(CLSID_AccPropServices,nullptr,CLSCTX_INPROC_SERVER,IID_IAccPropServices,reinterpret_cast<void**>(&properties)))){MSAAPROPID ids[]={PROPID_ACC_NAME,PROPID_ACC_DESCRIPTION};properties->ClearHwndProps(hwnd,OBJID_CLIENT,CHILDID_SELF,ids,2);properties->Release();}RemoveWindowSubclass(hwnd,NativeSubclass,100);}
    return DefSubclassProc(hwnd,msg,wp,lp);
}
bool NameNative(HWND hwnd,const std::wstring& name,const std::wstring& description){
    IAccPropServices* properties=nullptr;if(FAILED(CoCreateInstance(CLSID_AccPropServices,nullptr,CLSCTX_INPROC_SERVER,IID_IAccPropServices,reinterpret_cast<void**>(&properties))))return false;
    HRESULT named=properties->SetHwndPropStr(hwnd,OBJID_CLIENT,CHILDID_SELF,PROPID_ACC_NAME,name.c_str());
    HRESULT described=properties->SetHwndPropStr(hwnd,OBJID_CLIENT,CHILDID_SELF,PROPID_ACC_DESCRIPTION,description.c_str());properties->Release();SetWindowSubclass(hwnd,NativeSubclass,100,0);
    NotifyWinEvent(EVENT_OBJECT_NAMECHANGE,hwnd,OBJID_CLIENT,CHILDID_SELF);return SUCCEEDED(named)&&SUCCEEDED(described);
}
void Focus(HWND hwnd,int id){NotifyWinEvent(EVENT_OBJECT_FOCUS,hwnd,OBJID_CLIENT,id);}
}
