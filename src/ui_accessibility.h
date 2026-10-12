#pragma once
#include <windows.h>
#include <functional>
#include <string>
#include <vector>
namespace Ui::Accessibility {
struct Control { int id; RECT rect; std::wstring name,description,value; LONG role,state; };
using Controls=std::function<std::vector<Control>()>;
void Attach(HWND hwnd,Controls controls,std::function<void(int)> activate,std::function<void(int)> focus,std::function<bool(int,double)> value = {});
void Focus(HWND hwnd,int id);
bool NameNative(HWND hwnd,const std::wstring& name,const std::wstring& description);
}
