//-----------------------------------------------------------------------------
//(c) 2011-2013 pavel.pimenov@gmail.com
//-----------------------------------------------------------------------------
#ifndef CFlyMediaInfo_H
#define CFlyMediaInfo_H

#pragma once

#include <string>
#include <vector>
#include "Text.h"

class CFlyMediaInfo
{
	public:
		class ExtItem
		{
			public:
				enum
				{
					channel_all = 255
				};
				bool   m_is_delete;
				uint8_t m_stream_type; // MediaInfoLib::stream_t
				uint8_t m_channel; // ���� = channel_all, �� ��� ����� ����� ��� ����� ���� (���� ������������ ��� ����������� Audio)
				string m_param;
				string m_value;
				ExtItem() : m_stream_type(0), m_channel(0), m_is_delete(false)
				{
				}
		};
		std::vector<ExtItem> m_ext_array;
		uint16_t m_bitrate;
		uint16_t m_mediaX;
		uint16_t m_mediaY;
		std::string m_video;
		std::string m_audio;
		CFlyMediaInfo()
		{
			init();
		}
		CFlyMediaInfo(const std::string& p_WH, uint16_t p_bitrate = 0)
		{
			init(p_WH, p_bitrate);
		}
		bool isMedia() const
		{
			return m_bitrate != 0 || !m_audio.empty() || !m_video.empty();
		}
		bool isMediaAttrExists() const
		{
			return !m_ext_array.empty();
		}
		void init(const std::string& p_WH, uint16_t p_bitrate = 0)
		{
			init(p_bitrate);
			auto l_pos = p_WH.find('x');
			if (l_pos != std::string::npos)
			{
				m_mediaX = atoi(p_WH.c_str());
				m_mediaY = atoi(p_WH.c_str() + l_pos + 1);
			}
		}
		void init(uint16_t p_bitrate = 0)
		{
			m_bitrate = p_bitrate;
			m_mediaX  = 0;
			m_mediaY  = 0;
		}
		
		string getXY() const
		{
			if (m_mediaX && m_mediaY)
			{
				char l_buf[100];
				l_buf[0] = 0;
				snprintf(l_buf, sizeof(l_buf), "%ux%u", m_mediaX, m_mediaY);
				return l_buf;
			}
			return string();
		}
		static void translateDuration(const string& p_audio,
		                              tstring& p_column_audio,
		                              tstring& p_column_duration)  // ��������� ������������ �� ���� Audio
		{
			p_column_duration = _T("");
			p_column_audio    = _T("");
			if (!p_audio.empty())
			{
				const size_t l_pos = p_audio.find('|', 0);
				if (l_pos != string::npos && l_pos)
				{
					if (p_audio.size() > 6 && p_audio[0] >= '1' && p_audio[0] <= '9' && // �������� ���� ������� � ������ ������������
					        (p_audio[1] == 'm' || p_audio[2] == 'n') || // "1mn XXs"
					        (p_audio[2] == 'm' || p_audio[3] == 'n') ||   // "60mn XXs"
					        (p_audio[1] == 'h') ||  // "1h XXmn"
					        (p_audio[2] == 'h')     // "60h XXmn"
					   )
					{
						p_column_duration = Text::toT(p_audio.substr(0, l_pos - 1));
						if (l_pos + 2 < p_audio.length())
							p_column_audio = Text::toT(p_audio.substr(l_pos + 2));
						else
							p_column_audio.clear();
					}
				}
			}
		}
};
#endif
