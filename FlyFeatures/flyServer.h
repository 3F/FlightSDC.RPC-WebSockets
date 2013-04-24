/*
 * Copyright (C) 2011-2012 FlylinkDC++ Team http://flylinkdc.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if  !defined(_FLY_SERVER_H)
#define _FLY_SERVER_H

#include <unordered_set>
#include <string>

#include "../client/Thread.h"
#include "../client/CFlyMediaInfo.h"
#include "../client/MerkleTree.h"
#include "../client/Util.h"

//=======================================================================
typedef std::pair<std::string, uint16_t> CServerItem;
class DHTServer
{
	public:
		DHTServer(const string& p_url, const string& p_agent = Util::emptyString) : m_url(p_url)
		{
			if (!p_agent.empty())
				m_agent = p_agent;
		}
		const string& getUrl() const
		{
			return m_url;
		}
		const string& getAgent() const
		{
			return m_agent;
		}
	private:
		string m_url;
		string m_agent;
};
//=======================================================================
class CFlyServerConfig
{
  void initOfflineConfig();
 public:

  enum type_server
  {
    TYPE_FLYSERVER_TCP = 1,
    TYPE_FLYSERVER_HTTP = 2 // TODO
  };

  CFlyServerConfig() : 
    m_max_size_value(0), 
    m_min_file_size(0), 
    m_type(TYPE_FLYSERVER_TCP), 
    m_time_load_config(0), 
    m_send_full_mediainfo(false),
    m_zlib_compress_level(9) // Z_BEST_COMPRESSION
  {
  }
  ~CFlyServerConfig()
  {
  }
 static std::string   g_server_ip;
 static uint16_t      g_server_port;
private: // TODO - все мемебры спрятать в приват
 bool     m_send_full_mediainfo; // Если = true на сервер шлем данные если есть полная информация о медиа-файле
 int      m_zlib_compress_level;

public:
 type_server   m_type;
 uint64_t m_min_file_size;
 uint16_t m_max_size_value;
 // TODO boost::flat_set
 std::unordered_set<std::string> m_scan;  
 std::unordered_set<std::string> m_exclude_tag; 
 std::vector<std::string> m_exclude_tag_inform;
 std::unordered_set<std::string> m_include_tag; 
 std::unordered_set<std::string> m_mediainfo_ext; 
 uint64_t m_time_load_config;
//
 static std::vector<CServerItem > g_mirror_read_only_servers;
 static std::vector<DHTServer> g_dht_servers;
 static std::unordered_set<std::string> g_parasitic_files;
 static CServerItem getRandomMirrorServer(bool p_is_set);
 static const DHTServer& getRandomDHTServer();
 static bool isParasitFile(const string& p_file);

public:
  void ConvertInform(string& p_inform) const;
  void loadConfig();
  bool isSupportTag (const string& p_tag) const;
  bool isSupportFile(const string& p_file_ext, uint64_t p_size) const;
  bool isInit() const
  {
	  return !m_scan.empty();
  }
  bool isFullMediainfo() const
  {
	  return m_send_full_mediainfo;
  }  
  int getZlibCompressLevel() const
  {
	  return m_zlib_compress_level;
  }  
  
};
//=======================================================================
extern CFlyServerConfig g_fly_server_config;
//=======================================================================
class CFlyServerKey
{
public:
	TTHValue m_tth;
//	string m_file_name;
	int64_t m_file_size;
	uint32_t m_hit;
	uint32_t m_time_hash;
	uint32_t m_time_file;
	CFlyMediaInfo m_media;
	bool m_only_ext_info; // Запрашиваем только расширенную информацию
  CFlyServerKey(const TTHValue& p_tth,int64_t p_file_size):
    m_tth(p_tth),m_file_size(p_file_size), m_hit(0),m_time_hash(0),m_time_file(0), m_only_ext_info(false)
	{
	//	m_file_name = Util::getFileName(p_file_name);
	}
};
//=======================================================================
class CFlyServerAppendix
{
  public:
	string m_fly_server_inform; 
	bool m_is_fly_server_item; 
    CFlyServerAppendix():m_is_fly_server_item(false)
    {
    }
};
//=======================================================================
typedef std::vector<CFlyServerKey> CFlyServerKeyArray;
//=======================================================================
class CFlyServerJSON
{
public:
		CFlyServerJSON():m_IdleTime(0), m_merge_process(0)
		{
		}
		virtual ~CFlyServerJSON()
		{
		}
		CFlyServerKeyArray	m_GetFlyServerArray; // Запросы на получения медиаинформации. TODO - сократить размер структуры для запроса.
		CFlyServerKeyArray	m_SetFlyServerArray; // Запросы на передачу медиаинформации если она у нас есть в базе и ее ниразу не слали.
		LONG				m_merge_process;		// Блокировка вложенного запуска внутри процесса
		::CriticalSection	m_merge_fly_server_cs;
		int					m_IdleTime;
		void clear()
		{
			m_merge_process = 0;
			m_GetFlyServerArray.clear();
			m_SetFlyServerArray.clear();
		}
		void merge_fly_server_info();
		static void login();
		static void logout();
		static string g_fly_server_id;
		static string connectFlyServer(const CFlyServerKeyArray& p_fileInfoArray, bool p_is_fly_set_query);
		static string postFlyServer(bool p_is_set, const char* p_query, const string& p_body);
};

#endif // _FLY_SERVER_H


