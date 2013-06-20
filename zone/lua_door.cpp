#ifdef LUA_EQEMU

#include "lua.hpp"
#include <luabind/luabind.hpp>
#include <luabind/object.hpp>

#include "doors.h"
#include "lua_door.h"

void Lua_Door::SetDoorName(const char *name) {
	Lua_Safe_Call_Void();
	self->SetDoorName(name);
}

const char *Lua_Door::GetDoorName() {
	Lua_Safe_Call_String();
	return self->GetDoorName();
}

float Lua_Door::GetX() {
	Lua_Safe_Call_Real();
	return self->GetX();
}

float Lua_Door::GetY() {
	Lua_Safe_Call_Real();
	return self->GetY();
}

float Lua_Door::GetZ() {
	Lua_Safe_Call_Real();
	return self->GetZ();
}

float Lua_Door::GetHeading() {
	Lua_Safe_Call_Real();
	return self->GetHeading();
}

void Lua_Door::SetX(float x) {
	Lua_Safe_Call_Void();
	self->SetX(x);
}

void Lua_Door::SetY(float y) {
	Lua_Safe_Call_Void();
	self->SetY(y);
}

void Lua_Door::SetZ(float z) {
	Lua_Safe_Call_Void();
	self->SetZ(z);
}

void Lua_Door::SetHeading(float h) {
	Lua_Safe_Call_Void();
	self->SetHeading(h);
}

void Lua_Door::SetLocation(float x, float y, float z) {
	Lua_Safe_Call_Void();
	self->SetLocation(x, y, z);
}

uint32 Lua_Door::GetDoorDBID() {
	Lua_Safe_Call_Int();
	return self->GetDoorDBID();
}

uint32 Lua_Door::GetDoorID() {
	Lua_Safe_Call_Int();
	return self->GetDoorID();
}

void Lua_Door::SetSize(uint32 sz) {
	Lua_Safe_Call_Void();
	self->SetSize(sz);
}

uint32 Lua_Door::GetSize() {
	Lua_Safe_Call_Int();
	return self->GetSize();
}

void Lua_Door::SetIncline(uint32 incline) {
	Lua_Safe_Call_Void();
	self->SetIncline(incline);
}

uint32 Lua_Door::GetIncline() {
	Lua_Safe_Call_Int();
	return self->GetIncline();
}

void Lua_Door::SetOpenType(uint32 type) {
	Lua_Safe_Call_Void();
	self->SetOpenType(type);
}

uint32 Lua_Door::GetOpenType() {
	Lua_Safe_Call_Int();
	return self->GetOpenType();
}

void Lua_Door::SetLockPick(uint32 pick) {
	Lua_Safe_Call_Void();
	self->SetLockpick(pick);
}

uint32 Lua_Door::GetLockPick() {
	Lua_Safe_Call_Int();
	return self->GetLockpick();
}

void Lua_Door::SetKeyItem(uint32 key) {
	Lua_Safe_Call_Void();
	self->SetKeyItem(key);
}

uint32 Lua_Door::GetKeyItem() {
	Lua_Safe_Call_Int();
	return self->GetKeyItem();
}

void Lua_Door::SetNoKeyring(int type) {
	Lua_Safe_Call_Void();
	self->SetNoKeyring(type);
}

int Lua_Door::GetNoKeyring() {
	Lua_Safe_Call_Int();
	return self->GetNoKeyring();
}

void Lua_Door::CreateDatabaseEntry() {
	Lua_Safe_Call_Void();
	self->CreateDatabaseEntry();
}

luabind::scope lua_register_door() {
	return luabind::class_<Lua_Door, Lua_Entity>("Door")
		.def(luabind::constructor<>())
		.property("null", &Lua_Door::Null)
		.property("valid", &Lua_Door::Valid)
		.def("SetDoorName", (void(Lua_Door::*)(const char*))&Lua_Door::SetDoorName)
		.def("GetDoorName", (const char*(Lua_Door::*)(void))&Lua_Door::GetDoorName)
		.def("GetX", (float(Lua_Door::*)(void))&Lua_Door::GetX)
		.def("GetY", (float(Lua_Door::*)(void))&Lua_Door::GetY)
		.def("GetZ", (float(Lua_Door::*)(void))&Lua_Door::GetZ)
		.def("GetHeading", (float(Lua_Door::*)(void))&Lua_Door::GetHeading)
		.def("SetX", (void(Lua_Door::*)(float))&Lua_Door::SetX)
		.def("SetY", (void(Lua_Door::*)(float))&Lua_Door::SetY)
		.def("SetZ", (void(Lua_Door::*)(float))&Lua_Door::SetZ)
		.def("SetHeading", (void(Lua_Door::*)(float))&Lua_Door::SetHeading)
		.def("SetLocation", (void(Lua_Door::*)(float,float,float))&Lua_Door::SetLocation)
		.def("GetDoorDBID", (uint32(Lua_Door::*)(void))&Lua_Door::GetDoorDBID)
		.def("GetDoorID", (uint32(Lua_Door::*)(void))&Lua_Door::GetDoorID)
		.def("SetSize", (void(Lua_Door::*)(uint32))&Lua_Door::SetSize)
		.def("GetSize", (uint32(Lua_Door::*)(void))&Lua_Door::GetSize)
		.def("SetIncline", (void(Lua_Door::*)(uint32))&Lua_Door::SetIncline)
		.def("GetIncline", (uint32(Lua_Door::*)(void))&Lua_Door::GetIncline)
		.def("SetOpenType", (void(Lua_Door::*)(uint32))&Lua_Door::SetOpenType)
		.def("GetOpenType", (uint32(Lua_Door::*)(void))&Lua_Door::GetOpenType)
		.def("SetLockPick", (void(Lua_Door::*)(uint32))&Lua_Door::SetLockPick)
		.def("GetLockPick", (uint32(Lua_Door::*)(void))&Lua_Door::GetLockPick)
		.def("SetKeyItem", (void(Lua_Door::*)(uint32))&Lua_Door::SetKeyItem)
		.def("GetKeyItem", (uint32(Lua_Door::*)(void))&Lua_Door::GetKeyItem)
		.def("SetNoKeyring", (void(Lua_Door::*)(int))&Lua_Door::SetNoKeyring)
		.def("GetNoKeyring", (int(Lua_Door::*)(void))&Lua_Door::GetNoKeyring)
		.def("CreateDatabaseEntry", (void(Lua_Door::*)(void))&Lua_Door::CreateDatabaseEntry);
}

#endif
