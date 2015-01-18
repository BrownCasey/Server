/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2006 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/


#include "../common/debug.h"
#include "launcher_list.h"
#include "launcher_link.h"
#include "../common/logsys.h"
#include "eql_config.h"

LauncherList::LauncherList()
: nextID(1)
{
}

LauncherList::~LauncherList() {
	std::vector<LauncherLink *>::iterator cur, end;
	cur = m_pendingLaunchers.begin();
	end = m_pendingLaunchers.end();
	for(; cur != end; ++cur) {
		delete *cur;
	}

	std::map<std::string, EQLConfig *>::iterator curc, endc;
	curc = m_configs.begin();
	endc = m_configs.end();
	for(; curc != endc; ++curc) {
		delete curc->second;
	}

	std::map<std::string, LauncherLink *>::iterator curl, endl;
	curl = m_launchers.begin();
	endl = m_launchers.end();
	for(; curl != endl; ++curl) {
		delete curl->second;
	}
}

void LauncherList::Process() {
	//process pending launchers..
	std::vector<LauncherLink *>::iterator cur;
	cur = m_pendingLaunchers.begin();
	while(cur != m_pendingLaunchers.end()) {
		LauncherLink *l = *cur;
//printf("ProcP %d: %p\n", l->GetID(), l);
		if(!l->Process()) {
			//launcher has died before it identified itself.
			Log.Out(EQEmuLogSys::Detail, EQEmuLogSys::World_Server, "Removing pending launcher %d", l->GetID());
			cur = m_pendingLaunchers.erase(cur);
			delete l;
		} else if(l->HasName()) {
			//launcher has identified itself now.
			//remove ourself from the pending list
			cur = m_pendingLaunchers.erase(cur);
			std::string name = l->GetName();
			//kill off anybody else using our name.
			std::map<std::string, LauncherLink *>::iterator res;
			res = m_launchers.find(name);
			if(res != m_launchers.end()) {
				Log.Out(EQEmuLogSys::Detail, EQEmuLogSys::World_Server, "Ghosting launcher %s", name.c_str());
				delete res->second;
			}
			Log.Out(EQEmuLogSys::Detail, EQEmuLogSys::World_Server, "Removing pending launcher %d. Adding %s to active list.", l->GetID(), name.c_str());
			//put the launcher in the list.
			m_launchers[name] = l;
		} else {
			++cur;
		}
	}

	//process active launchers.
	std::map<std::string, LauncherLink *>::iterator curl;
	curl = m_launchers.begin();
	while(curl != m_launchers.end()) {
		LauncherLink *l = curl->second;
//printf("Proc %s(%d): %p\n", l->GetName(), l->GetID(), l);
		if(!l->Process()) {
			//launcher has died before it identified itself.
			Log.Out(EQEmuLogSys::Detail, EQEmuLogSys::World_Server, "Removing launcher %s (%d)", l->GetName(), l->GetID());
			curl = m_launchers.erase(curl);
			delete l;
		} else {
			++curl;
		}
	}
}

LauncherLink *LauncherList::Get(const char *name) {
	std::map<std::string, LauncherLink *>::iterator res;
	res = m_launchers.find(name);
	if(res == m_launchers.end())
		return(nullptr);
	return(res->second);
/*	std::string goal(name);

	std::vector<LauncherLink *>::iterator cur, end;
	cur = m_launchers.begin();
	end = m_launchers.end();
	for(; cur != end; cur++) {
		if(goal == (*cur)->GetName())
			return(*cur);
	}
	return(nullptr);*/
}

LauncherLink *LauncherList::FindByZone(const char *short_name) {
	std::map<std::string, LauncherLink *>::iterator cur, end;
	cur = m_launchers.begin();
	end = m_launchers.end();
	for(; cur != end; ++cur) {
		if(cur->second->ContainsZone(short_name))
			return(cur->second);
	}
	return(nullptr);
}

void LauncherList::Add(EmuTCPConnection *conn) {
	auto it = new LauncherLink(nextID++, conn);
	Log.Out(EQEmuLogSys::Detail, EQEmuLogSys::World_Server, "Adding pending launcher %d", it->GetID());
	m_pendingLaunchers.push_back(it);
}


int LauncherList::GetLauncherCount() {
	return(m_launchers.size());
}

void LauncherList::GetLauncherNameList(std::vector<std::string> &res) {
	std::map<std::string, EQLConfig *>::iterator cur, end;
	cur = m_configs.begin();
	end = m_configs.end();
	for(; cur != end; ++cur) {
		res.push_back(cur->first);
	}
}

void LauncherList::LoadList() {
	std::vector<std::string> launchers;

	database.GetLauncherList(launchers);

	std::vector<std::string>::iterator cur, end;
	cur = launchers.begin();
	end = launchers.end();
	for(; cur != end; ++cur) {
		m_configs[*cur] = new EQLConfig(cur->c_str());
	}
}

EQLConfig *LauncherList::GetConfig(const char *name) {
	std::map<std::string, EQLConfig *>::iterator res;
	res = m_configs.find(name);
	if(res == m_configs.end()) {
		return(nullptr);
	}
	return(res->second);
}

void LauncherList::CreateLauncher(const char *name, uint8 dynamic_count) {
	m_configs[name] = EQLConfig::CreateLauncher(name, dynamic_count);
}

void LauncherList::Remove(const char *name) {
	std::map<std::string, EQLConfig *>::iterator resc;
	resc = m_configs.find(name);
	if(resc != m_configs.end()) {
		delete resc->second;
		m_configs.erase(resc);
	}

	std::map<std::string, LauncherLink *>::iterator resl;
	resl = m_launchers.find(name);
	if(resl != m_launchers.end()) {
		resl->second->Disconnect();
	}
}

