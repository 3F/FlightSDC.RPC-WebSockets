/*
 * Copyright (C) 2011-2013 FlylinkDC++ Team http://flylinkdc.com/
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

#include "stdinc.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/trim_all.hpp>

#include "../client/version.h"
#include "flyServer.h"
#include "../client/Socket.h"
#include "../client/ClientManager.h"
#include "../client/LogManager.h"
#include "../client/SimpleXML.h"
#include "../client/StringTokenizer.h"
#include "../jsoncpp/include/json/value.h"
#include "../jsoncpp/include/json/reader.h"
#include "../jsoncpp/include/json/writer.h"
#include "../MediaInfoLib/Source/MediaInfo/MediaInfo_Const.h"
#include "../zlib/zlib.h"

const static string g_user_agent = APPNAME " " A_VERSIONSTRING;
static CriticalSection g_cs;
CFlyServerConfig g_fly_server_config;

std::vector<DHTServer>	  CFlyServerConfig::g_dht_servers;
std::vector<CServerItem > CFlyServerConfig::g_mirror_read_only_servers;
std::unordered_set<std::string> CFlyServerConfig::g_parasitic_files;
std::string  CFlyServerConfig::g_server_ip;
uint16_t     CFlyServerConfig::g_server_port = 0;

//======================================================================================================
//	mediainfo_ext ="3gp,avi,divx,flv,m4v,mkv,dts,mov,mp4,mpg,mpeg,vob,wmv,bik,qt,rm,aac,ac3,ape,fla,flac,m4a,mp1,mp2,mp3,ogg,wma,wv,mka,vqf,lqt,ts,tp,mod,m2ts,webm" 
static const char* g_mediainfo_ext[] =
{
	"3gp",
	"avi",
	"divx",
	"flv",
	"m4v",
	"mkv",
	"dts",
	"mov",
	"mp4",
	"mpg",
	"mpeg",
	"vob",
	"wmv",
	"bik",
	"qt",
	"rm",
	"aac",
	"ac3",
	"ape",
	"fla",
	"flac",
	"m4a",
	"mp1",
	"mp2",
	"mp3",
	"ogg",
	"wma",
	"wv",
	"mka",
	"vqf",
	"lqt",
	"ts",
	"tp",
	"mod",
	"m2ts",
	"webm",
#if defined(USE_MEDIAINFO_IMAGES) // Картинки не анализируем
	"tiff",
	"tif",
	"jpg",
	"ico",
	"jpeg",
	"gif",
	"bmp",
	"png",
	"pcx",
	"tga",
#endif
	0 // Не убирать терминатор!
};
//======================================================================================================
bool CFlyServerConfig::isSupportFile(const string& p_file_ext, uint64_t p_size) const
{
	dcassert(!m_scan.empty());
	return p_size > m_min_file_size && m_scan.find(p_file_ext) != m_scan.end(); // [!] IRainman opt.
}
//======================================================================================================
bool CFlyServerConfig::isSupportTag(const string& p_tag) const
{
	const auto l_string_pos = p_tag.find("/String");
	if(l_string_pos != string::npos)
	{
		return false;
	}
	if(m_include_tag.empty())
	{
		return m_exclude_tag.find(p_tag) == m_exclude_tag.end();
	}
	else
	{
		if(m_include_tag.find(p_tag) != m_include_tag.end())
		{
			return true;
		}
		else
			return false;
	}
}
//======================================================================================================
CServerItem CFlyServerConfig::getRandomMirrorServer(bool p_is_set)
{
	if(p_is_set == false && !g_mirror_read_only_servers.empty())
	{
	 const int l_id = Util::rand(g_mirror_read_only_servers.size());
	 return g_mirror_read_only_servers[l_id];
	}
	else // Вернем пишущий сервер
	{
		const CServerItem l_main_server(g_server_ip,g_server_port);
		return l_main_server;
	}
}
//======================================================================================================
bool CFlyServerConfig::isParasitFile(const string& p_file)
{
	return g_parasitic_files.find(p_file) != g_parasitic_files.end(); // [!] IRainman opt.
}
//======================================================================================================
const DHTServer& CFlyServerConfig::getRandomDHTServer()
{
	dcassert(!g_dht_servers.empty());
	const int l_id = Util::rand(g_dht_servers.size());
	return g_dht_servers[l_id];
}
//======================================================================================================
void CFlyServerConfig::initOfflineConfig()
{
	if (m_mediainfo_ext.empty())
	{
	for (size_t i = 0; g_mediainfo_ext[i]; ++i)
		m_mediainfo_ext.insert(g_mediainfo_ext[i]);
	}
	if(g_dht_servers.empty())
	{
		const char* l_url_0 = "http://strongdc.sourceforge.net/bootstrap/";
		const char* l_url_1 = "http://ssa.in.ua/dcDHT.php";
		g_dht_servers.push_back(DHTServer(l_url_0, "StrongDC++ v 2.43"));
		g_dht_servers.push_back(DHTServer(l_url_1));
	}
	m_time_load_config = 0; // Возвращаем попытку загрузки
}
//======================================================================================================
inline static void checkStrKey(const string& p_str)
{
#ifdef _DEBUG
	dcassert(Text::toLower(p_str) == p_str);
	string l_str_copy = p_str;
	boost::algorithm::trim(l_str_copy);
	dcassert(l_str_copy == p_str);
#endif
}
//======================================================================================================
void CFlyServerConfig::ConvertInform(string& p_inform) const
{
	// TODO - убрать лишние строки из результат
	if(!m_exclude_tag_inform.empty())
	{
	string l_result_line;
	int  l_start_line = 0;
	auto l_end_line = string::npos;
	do
	{
	l_end_line = p_inform.find("\r\n",l_start_line);
	if (l_end_line != string::npos)
	 {
		string l_cur_line = p_inform.substr(l_start_line,l_end_line - l_start_line);
		// Фильтруем строчку через маски
		bool l_ignore_tag = false;
		for(auto i = m_exclude_tag_inform.cbegin(); i != m_exclude_tag_inform.cend(); ++i)
		{
			const auto l_tag_begin = l_cur_line.find(*i);
			const auto l_end_index  = i->size() + 1;
			if(l_tag_begin != string::npos 
				&& l_tag_begin == 0  // Тэг в начале строки?
				&& l_cur_line.size() > l_end_index // После него есть место?
				&& (l_cur_line[l_end_index] == ':' || l_cur_line[l_end_index] == ' ') // После тэга пробел или ':'
				)
			{ 
				l_ignore_tag = true;
				break;
			}
		}
		if(l_ignore_tag == false)
		{
			  boost::algorithm::trim_all(l_cur_line);
			  l_result_line += l_cur_line + "\r\n";
		}
		l_start_line = l_end_line + 2;
	 }
	}
	while (l_end_line != string::npos);
    p_inform = l_result_line;	
  }
}
//======================================================================================================
void CFlyServerConfig::loadConfig()
{
  const auto l_cur_tick = GET_TICK();
  if(m_time_load_config == 0 || (l_cur_tick - m_time_load_config) > 1000*60*60*10 )
  {
    m_time_load_config = l_cur_tick;
	CFlyLog l_dht_server_log("[fly-server config loader]");
	std::string l_data;
	const string l_url_config_file = "http://www.fly-server.ru/etc/strongdc-sqlite-config.xml";
	l_dht_server_log.step("Download:" + l_url_config_file);
	if (Util::getDataFromInet(Text::toT(g_user_agent).c_str(), 4096, l_url_config_file, l_data, 0) == 0)
	{
		l_dht_server_log.step("Error download!");
		initOfflineConfig();
	}
	else
	{
		try
		{
			SimpleXML l_xml;
			l_xml.fromXML(l_data);
			if (l_xml.findChild("Bootstraps"))
			{
				if(g_dht_servers.empty()) // DHT забираем один раз
				{
				 l_xml.stepIn();
				 while (l_xml.findChild("server"))
				 {
					const string& l_server = l_xml.getChildAttrib("url");
					const string& l_agent = l_xml.getChildAttrib("agent");
					if (!l_server.empty())
						g_dht_servers.push_back(DHTServer(l_server, l_agent));
				 }
				 l_xml.stepOut();
				}
				l_xml.stepIn();
				while (l_xml.findChild("fly-server"))
				{
					g_server_ip = l_xml.getChildAttrib("ip");
					g_server_port = Util::toInt(l_xml.getChildAttrib("port"));
					m_min_file_size = Util::toInt64(l_xml.getChildAttrib("min_file_size"));
					m_max_size_value = Util::toInt(l_xml.getChildAttrib("max_size_value"));
					m_zlib_compress_level = Util::toInt(l_xml.getChildAttrib("zlib_compress_level"));
					dcassert(m_zlib_compress_level >= 0 && m_zlib_compress_level < 10);
					if(m_zlib_compress_level <= 0 || m_zlib_compress_level > Z_BEST_COMPRESSION)
                                            m_zlib_compress_level = Z_BEST_COMPRESSION;
					m_send_full_mediainfo = Util::toInt(l_xml.getChildAttrib("send_full_mediainfo")) == 1;
					m_type = l_xml.getChildAttrib("type") == "http" ? TYPE_FLYSERVER_HTTP : TYPE_FLYSERVER_TCP ;
					// TODO - убрать копипаст и все токенайзеры унести в сервисный метод.

					const StringTokenizer<string> l_scan(l_xml.getChildAttrib("scan"), ',');
					m_scan.clear();
					for (auto i = l_scan.getTokens().cbegin(); i != l_scan.getTokens().cend(); ++i)
						{
							checkStrKey(*i);
							m_scan.insert(*i);
					    }
					
					const StringTokenizer<string> l_mediainfo_ext(l_xml.getChildAttrib("mediainfo_ext"), ',');
					m_mediainfo_ext.clear();
					for (auto i = l_mediainfo_ext.getTokens().cbegin(); i != l_mediainfo_ext.getTokens().cend(); ++i)
						{
							checkStrKey(*i);
							m_mediainfo_ext.insert(*i);
					    }

					const StringTokenizer<string> l_exclude_tag(l_xml.getChildAttrib("exclude_tag"), ',');
					m_exclude_tag.clear();
					for (auto i = l_exclude_tag.getTokens().cbegin(); i != l_exclude_tag.getTokens().cend(); ++i)
						m_exclude_tag.insert(*i);

					m_exclude_tag_inform.clear();
					const StringTokenizer<string> l_exclude_tag_inform(l_xml.getChildAttrib("exclude_tag_inform"), ',');
					for (auto i = l_exclude_tag_inform.getTokens().cbegin(); i != l_exclude_tag_inform.getTokens().cend(); ++i)
						m_exclude_tag_inform.push_back(*i);

					const StringTokenizer<string> l_include_tag(l_xml.getChildAttrib("include_tag"), ',');
					m_include_tag.clear();
					for (auto i = l_include_tag.getTokens().cbegin(); i != l_include_tag.getTokens().cend(); ++i)
						m_include_tag.insert(*i);
					
					const StringTokenizer<string> l_parasitic_file(l_xml.getChildAttrib("parasitic_file"), ',');
					g_parasitic_files.clear();
					for (auto i = l_parasitic_file.getTokens().cbegin(); i != l_parasitic_file.getTokens().cend(); ++i)
						g_parasitic_files.insert(*i);

					// Достанем RO-зеркала
					const StringTokenizer<string> l_mirror_read_only_server_tag(l_xml.getChildAttrib("mirror_read_only_server"), ',');
					g_mirror_read_only_servers.clear();
					for (auto i = l_mirror_read_only_server_tag.getTokens().cbegin(); i != l_mirror_read_only_server_tag.getTokens().cend(); ++i)
						{
							std::pair<std::string, uint16_t> l_ro_server_item;
							const auto l_port = i->find(':');
							if(l_port != string::npos)
							{
								l_ro_server_item.first = i->substr(0,l_port);
								l_ro_server_item.second = atoi(i->c_str()+l_port+1);
								g_mirror_read_only_servers.push_back(l_ro_server_item);
							}
					   }					
				}
				l_xml.stepOut();
			}
			l_dht_server_log.step("Download and parse - Ok!");
		}
		catch (const Exception& e)
		{
			dcassert(0);
			dcdebug("CFlyServerConfig::loadConfig parseXML ::Problem: %s\n", e.getError().c_str());
			l_dht_server_log.step("parseXML Problem:" + e.getError());
			initOfflineConfig();
		}
	}
	}
}
//======================================================================================================
//string sendTTH(const CFlyServerKey& p_fileInfo, bool p_send_mediainfo)
//{
//	CFlyServerKeyArray l_array;
//	l_array.push_back(p_fileInfo);
//	return sendTTH(l_array,p_send_mediainfo);
//}
//======================================================================================================
string CFlyServerJSON::g_fly_server_id;
//======================================================================================================
void CFlyServerJSON::login()
{
  if(g_fly_server_id.empty())
 {
	CFlyLog l_log("[fly-login]");
	Json::Value  l_root;   
	Json::Value& l_info = l_root["login"];
	l_info["CID"] = ClientManager::getInstance()->getMyCID().toBase32();
	const std::string l_post_query = l_root.toStyledString();
    string l_result_query = postFlyServer(true,"fly-login",l_post_query);
	Json::Value l_result_root;
	Json::Reader l_reader;
	const bool l_parsingSuccessful = l_reader.parse(l_result_query, l_result_root);
	if (!l_parsingSuccessful && !l_result_query.empty())
	 {
			l_log.step("Failed to parse json configuration: l_result_query = " + l_result_query);
	 }
	 else
	 {
		g_fly_server_id = l_result_root["ID"].asString();
		if(!g_fly_server_id.empty())
		  l_log.step("Register OK!");
//		else
//		  g_fly_server_id
	 }
	}
}
//======================================================================================================
void logout()
{
}
//======================================================================================================
string CFlyServerJSON::postFlyServer(bool p_is_set, const char* p_query, const string& p_body)
{
	dcassert(!p_body.empty());
	string l_result_query;
	CFlyLog l_fly_server_log("[www.fly-server.ru]");
    static const char g_hdrs[]		= "Content-Type: application/x-www-form-urlencoded";
    static const size_t g_hdrs_len   = strlen(g_hdrs); // Можно заменить на (sizeof(g_hdrs)-1) ...
    static LPCSTR g_accept[2]	= {"*/*", NULL};
	const static string l_user_agent = A_VERSIONSTRING;
// Сжатие
   std::vector<uint8_t> l_post_compress_query;
   if(g_fly_server_config.getZlibCompressLevel() >= 1) // Выполняем сжатие запроса?
   {
   unsigned long l_dest_length = compressBound(p_body.length()) + 2;
   l_post_compress_query.resize(l_dest_length);
   const int l_zlib_result = compress2(l_post_compress_query.data(), &l_dest_length,
	                         (uint8_t*)p_body.data(), 
							 p_body.length(), g_fly_server_config.getZlibCompressLevel()); 
                             // На клиенте пока жмем по максимуму - нагрузка мелкая. 
                             // если декомпрессия будет сажать CPU сервера можем отключить через конфиг (но маловероятно)
                                                        

   if (l_zlib_result == Z_OK)
	{
	if(l_dest_length < p_body.length()) // мелкие пакеты могут стать больше - отказываемся от сжатия.
	  {
		l_post_compress_query.resize(l_dest_length); // TODO - нужен ли этот релокейт?
		l_fly_server_log.step("Compress query:  " + Util::toString(p_body.length()) + " / " + Util::toString(l_dest_length));
      }
     else
	  {
			l_post_compress_query.clear();
	  }
     }
    else
     {
		l_post_compress_query.clear();
		l_fly_server_log.step("Error compress query: l_zlib_result =  " + Util::toString(l_zlib_result));
     }
    }
   const bool l_is_zlib = !l_post_compress_query.empty();

// Передача

	if(HINTERNET hSession = InternetOpenA(l_user_agent.c_str(),INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0))
    {
	const CServerItem l_Server = CFlyServerConfig::getRandomMirrorServer(p_is_set);
	if(HINTERNET hConnect = InternetConnectA(hSession, l_Server.first.c_str(),l_Server.second, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 1))
    {
     if(HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", p_query , NULL, NULL, g_accept, 0, 1))
	   {
		   if(HttpSendRequestA(hRequest, g_hdrs, g_hdrs_len, 
			     l_is_zlib ? reinterpret_cast<LPVOID>(l_post_compress_query.data()) : LPVOID(p_body.data()), 
				 l_is_zlib ? l_post_compress_query.size() : p_body.size()))
				{
					DWORD l_dwBytesAvailable = 0;
					std::vector<char> l_zlib_blob;
				    while (InternetQueryDataAvailable(hRequest, &l_dwBytesAvailable, 0, 0))
					{
						if(l_dwBytesAvailable == 0)
							break;
#ifdef _DEBUG
						l_fly_server_log.step(" InternetQueryDataAvailable dwBytesAvailable = " + Util::toString(l_dwBytesAvailable));
#endif
						std::vector<unsigned char> l_MessageBody(l_dwBytesAvailable + 1);
						DWORD dwBytesRead = 0;
						const BOOL bResult = InternetReadFile(hRequest, l_MessageBody.data(),l_dwBytesAvailable, &dwBytesRead);
						if (!bResult)
						{
							l_fly_server_log.step(" InternetReadFile error " + Util::translateError(GetLastError()));
							break;
						}
						if (dwBytesRead == 0)
							break;
					l_MessageBody[dwBytesRead] = 0;
					const int l_cur_size = l_zlib_blob.size();
					l_zlib_blob.resize(l_zlib_blob.size() + dwBytesRead);
					memcpy(l_zlib_blob.data() + l_cur_size, l_MessageBody.data(), dwBytesRead);
					}
#ifdef _DEBUG
/*
std::string l_hex_dump;
				for(unsigned long i=0;i<l_zlib_blob.size(); ++i)
				{
					if(i % 10 == 0)
						l_hex_dump += "\r\n";
					char b[7] = {0};
					sprintf(b, "%#x ", (unsigned char)l_zlib_blob[i] & 0xFF);
					l_hex_dump += b;
				}
					l_fly_server_log.step( "l_hex_dump = " + l_hex_dump);
*/
#endif
					// TODO обощить в утил и подменить в области DHT с другим стартовым коэф-центом. для уменьшения кол-ва релокаций
					unsigned long l_decompress_size = l_zlib_blob.size() * 10;
					std::vector<unsigned char> l_decompress(l_decompress_size); // TODO расширять динамически
					while(1)
					{
					    const int l_un_compress_result = uncompress(l_decompress.data(), &l_decompress_size, (uint8_t*)l_zlib_blob.data(), l_zlib_blob.size());
						l_fly_server_log.step(" zlib-uncompress: " + Util::toString(l_zlib_blob.size()) + " / " +  Util::toString(l_decompress_size));
						if (l_un_compress_result == Z_BUF_ERROR)
							{
								l_decompress_size *= 2;
								l_decompress.resize(l_decompress_size);
								continue;
						    }
						if (l_un_compress_result == Z_OK)
							l_result_query = (const char*) l_decompress.data();
                      break;
					};

#ifdef _DEBUG
					l_fly_server_log.step(" InternetReadFile Ok! size = " + Util::toString(l_result_query.size()));
#endif
		   }
			else
				l_fly_server_log.step("HttpSendRequest error " + Util::translateError(GetLastError()));
	   InternetCloseHandle(hRequest);
	   }
	   else
	  	  l_fly_server_log.step("HttpOpenRequest error " + Util::translateError(GetLastError()));
	  InternetCloseHandle(hConnect);
    }
    else
  	  l_fly_server_log.step("InternetConnect error " + Util::translateError(GetLastError()));
   InternetCloseHandle(hSession);
   }
   else
  	  l_fly_server_log.step("InternetOpen error " + Util::translateError(GetLastError()));
  return l_result_query;
}
//======================================================================================================
string CFlyServerJSON::connectFlyServer(const CFlyServerKeyArray& p_fileInfoArray, bool p_is_fly_set_query )
{
	dcassert(!p_fileInfoArray.empty());
	Lock l(g_cs);
	CFlyServerJSON::login(); // Запросим свой ИД у сервера
	CFlyLog l_log("[flylinkdc-server]",true);
	Json::Value  l_root;   	
	Json::Value& l_arrays = l_root["array"];
	l_root["header"]["ID"] = CFlyServerJSON::g_fly_server_id;
	int  l_count = 0;
	int  l_count_only_ext_info = 0;
	bool is_all_file_only_ext_info = false;
	if( p_is_fly_set_query == false) // Для get-запроса все записи хотят только расширенную инфу?
	{
	 for(auto i = p_fileInfoArray.cbegin(); i != p_fileInfoArray.cend(); ++i)
	  if(i->m_only_ext_info == true)
		++l_count_only_ext_info;
	 if(l_count_only_ext_info ==  p_fileInfoArray.size())
	 {
	   is_all_file_only_ext_info = true;
	   l_root["only_ext_info"] = 1; // Поместим признак в корень чтобы не размножать запрос всем.
	 }
	 if(l_count_only_ext_info > 0 && l_count_only_ext_info != p_fileInfoArray.size())
	 {
	   l_root["different_ext_info"] = 1; // Поместим признак различных способов запроса к медиаинфе 
	                                     // чтобы на стороне сервера не делать лишний поиск параметра для каждого файла.
	 }
	}
	for(auto i = p_fileInfoArray.cbegin(); i != p_fileInfoArray.cend(); ++i)
	{
	// Начинаем создавать JSON пакет для передачи на сервер.
	const string l_tth_base32 = i->m_tth.toBase32();
	dcassert(i->m_file_size && !l_tth_base32.empty());
	if(!i->m_file_size || l_tth_base32.empty())
	{
		l_log.step("Error !i->m_file_size || l_tth_base32.empty(). i->m_file_size = " +
			        Util::toString(i->m_file_size) 
					+ " l_tth_base32 = " + l_tth_base32 /*+ " i->m_file_name = " + i->m_file_name */);
		continue;
	}
	Json::Value& l_array_item = l_arrays[l_count++];
	l_array_item["tth"]  = l_tth_base32;
	l_array_item["size"] = Util::toString(i->m_file_size);
	 // TODO - l_array_item["name"] = i->m_file_name;  имя пока не помнить. оно разное всегда
	 /* Проброс хитов и времени хеша/файла так-же исключаем
	 if(i->m_hit)
	  l_array_item["hit"] = i->m_hit;
	 if(i->m_time_hash)
	 {
		l_array_item["time_hash"] = i->m_time_hash;
		l_array_item["time_file"] = i->m_time_file;
	 }
	 */
	if (p_is_fly_set_query == false)
	{
		if(is_all_file_only_ext_info == false && i->m_only_ext_info == true)
	        l_array_item["only_ext_info"] = 1;
	}
	else
	{
		Json::Value& l_mediaInfo = l_array_item["media"];
		if(!i->m_media.m_audio.empty())
		  l_mediaInfo["fly_audio"] = i->m_media.m_audio;
		if(!i->m_media.m_video.empty())
		  l_mediaInfo["fly_video"] = i->m_media.m_video;
		if(i->m_media.m_bitrate)
		  l_mediaInfo["fly_audio_br"] = i->m_media.m_bitrate;
		if(i->m_media.m_mediaX && i->m_media.m_mediaY)
		{
		  l_mediaInfo["fly_xy"] = i->m_media.getXY();
		}

	if (!i->m_media.m_ext_array.empty())
	{
				Json::Value& l_mediaInfo_ext = l_array_item["media-ext"];
				Json::Value* l_ptr_mediaInfo_ext_general = nullptr;
				Json::Value* l_ptr_mediaInfo_ext_audio   = nullptr;
				Json::Value* l_ptr_mediaInfo_ext_video   = nullptr;
				int l_count_audio_channel = 0;
				int l_last_channel = 0;
				for(auto j = i->m_media.m_ext_array.cbegin(); j!= i->m_media.m_ext_array.cend(); ++j)
				{
						if(j->m_stream_type == MediaInfoLib::Stream_Audio)
					{
						if(j->m_channel != l_last_channel)
						{
							l_last_channel = j->m_channel;
							++l_count_audio_channel;
						}
					}
				}
				std::unordered_map<int,Json::Value*> l_cache_channel;
				for(auto j = i->m_media.m_ext_array.cbegin(); j!= i->m_media.m_ext_array.cend(); ++j)
				{
					if(g_fly_server_config.isSupportTag(j->m_param))
					{
					switch(j->m_stream_type)
					{
					case MediaInfoLib::Stream_General:
						{
							if(!l_ptr_mediaInfo_ext_general)
								l_ptr_mediaInfo_ext_general = &l_mediaInfo_ext["general"];
							Json::Value& l_info = *l_ptr_mediaInfo_ext_general;
							l_info[j->m_param] = j->m_value;
						}
						break;
					case MediaInfoLib::Stream_Video:
						{
							if(!l_ptr_mediaInfo_ext_video)
								l_ptr_mediaInfo_ext_video = &l_mediaInfo_ext["video"];
							Json::Value& l_info = *l_ptr_mediaInfo_ext_video;
							l_info[j->m_param] = j->m_value;
						}
						break;
					case MediaInfoLib::Stream_Audio:
						{
							if(!l_ptr_mediaInfo_ext_audio)
								l_ptr_mediaInfo_ext_audio = &l_mediaInfo_ext["audio"];
							Json::Value& l_info = *l_ptr_mediaInfo_ext_audio;
							if(l_count_audio_channel == 0)
							  {
								  l_info[j->m_param] = j->m_value;
							  }
							else
							  {
								  // Определяем, в какую секцию канала должна попасть информация.
								  uint8_t l_channel_num = j->m_channel;
								  const auto l_ch_item = l_cache_channel.find(l_channel_num);
								  Json::Value* l_channel_info = 0;
								  if(l_ch_item == l_cache_channel.end())
								  {
									  const string l_channel_id = "channel-" + (l_channel_num != CFlyMediaInfo::ExtItem::channel_all ? Util::toString(l_channel_num) : string("all"));
									  l_channel_info = &l_info[l_channel_id];
									  l_cache_channel[l_channel_num] =  l_channel_info;
								  }
								  else
								  {
										  l_channel_info = l_ch_item->second;
								  }
								  (*l_channel_info)[j->m_param] = j->m_value;
								  }
						}
						break;
					}
					}
				}
			}
	}
	}
// string l_tmp;
 string l_result_query;
 if (l_count > 0)  // Есть что передавать на сервер?
 {
#define FLYLINKDC_USE_HTTP_SERVER
#ifdef FLYLINKDC_USE_HTTP_SERVER
   const std::string l_post_query = l_root.toStyledString();
   l_result_query = postFlyServer(p_is_fly_set_query,p_is_fly_set_query ? "fly-set" : "fly-zget",l_post_query);
#endif

// #define FLYLINKDC_USE_SOCKET
// #ifndef FLYLINKDC_USE_SOCKET
//  ::MessageBoxA(NULL,outputConfig.c_str(),"json",MB_OK | MB_ICONINFORMATION);
// #endif
#ifdef FLYLINKDC_USE_SOCKET
	unique_ptr<Socket> l_socket(new Socket);
	try
	{
	l_socket->create(Socket::TYPE_TCP);
	l_socket->setBlocking(true);
//#define FLYLINKDC_USE_MEDIAINFO_SERVER_RESOLVE
#ifdef FLYLINKDC_USE_MEDIAINFO_SERVER_RESOLVE
	const string l_ip = Socket::resolve("flylink.no-ip.org");
	l_log.step("Socket::resolve(flylink.no-ip.org) = ip " + l_ip);
#else
	const string l_ip = g_fly_server_config.m_ip;
#endif
	l_socket->connect(l_ip,g_fly_server_config.m_port); // 
	l_log.step("write");
	const size_t l_size = l_socket->writeAll(outputConfig.c_str(), outputConfig.size(), 500);
	if(l_size != outputConfig.size())
	 {
		 l_log.step("l_socket->write(outputConfig) != outputConfig.size() l_size = " + Util::toString(l_size));
		 return outputConfig;
	 }
	else
	{
		l_log.step("write-OK");
		vector<char> l_buf(64000);
		l_log.step("read");
		l_socket->readAll(&l_buf[0],l_buf.size(),500);
		l_log.step("read-OK");
	}
	l_log.step(outputConfig);
	}
	catch(const SocketException& e)
	{
	l_log.step("Socket error" + e.getError());
	}
#endif
	}
	return l_result_query;
}
