// File_AribStdB24B37 - Info for DVB Subtitle streams
// Copyright (C) 2012-2012 MediaArea.net SARL, Info@MediaArea.net
//
// This library is free software: you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library. If not, see <http://www.gnu.org/licenses/>.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Information about DVB Subtitle streams
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//---------------------------------------------------------------------------
#ifndef MediaInfo_File_AribStdB24B37H
#define MediaInfo_File_AribStdB24B37H
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#include "MediaInfo/File__Analyze.h"
//---------------------------------------------------------------------------

namespace MediaInfoLib
{

//***************************************************************************
// Class File_AribStdB24B37
//***************************************************************************

class File_AribStdB24B37 : public File__Analyze
{
public :
    //Constructor/Destructor
    File_AribStdB24B37();

    //In
    bool    HasCcis;

    //enums
    enum graphic_set
    {
        // Table 7-3 Classification of code set and Final Byte
        GS_Kanji                = 0x42, //2-byte
        GS_Alphanumeric         = 0x4A,
        GS_Hiragana             = 0x30,
        GS_Katakana             = 0x31,
        GS_Mosaic_A             = 0x32,
        GS_Mosaic_B             = 0x33,
        GS_Mosaic_C             = 0x34,
        GS_Mosaic_D             = 0x35,
        GS_PropAscii            = 0x36,
        GS_PropHiragana         = 0x37,
        GS_PropKatakana         = 0x38,
        GS_JisX0201_Katakana    = 0x49,
        GS_Jis_Kanji_Plane1     = 0x39, //2-byte
        GS_Jis_Kanji_Plane2     = 0x3A, //2-byte
        GS_AddSymbols           = 0x3B, //2-byte
        GS_Macro                = 0x70,

        GS_G                    = 0x0000, // G set
        GS_DRCS                 = 0x0100, // DRCS set
    };

private :
    //Streams management
    void Streams_Fill();
    void Streams_Finish();

    //Buffer - Per element
    void Read_Buffer_Continue();

    //Buffer - Per element
    void Header_Parse();
    void Data_Parse();

    //Elements
    void caption_management();
    void caption_statement();
    void data_unit_data();
    void Character(int8u G_Value, int8u FirstByte, int8u SecondByte);
    void Add(Char Character);
    void DefaultMacro();
    void control_code();
    void NUL();
    void BEL();
    void APB();
    void APF();
    void APD();
    void APU();
    void CS();
    void APR();
    void LS1();
    void LS0();
    void PAPF();
    void CAN();
    void SS2();
    void ESC();
    void APS();
    void SS3();
    void RS();
    void US();
    void xxF();
    void xxZ();
    void SZX();
    void COL();
    void FLC();
    void CDC();
    void POL();
    void WMM();
    void MACRO();
    void HLC();
    void RPC();
    void SPL();
    void STL();
    void CSI();
    void TIME();

    //Stream
    struct stream
    {
        string  ISO_639_language_code;
        int8u   DMF_reception;
        int8u   Format;

        int16u  G[4];
        int8u   G_Width[4];
        int8u   GL;         // Locked
        int8u   GL_SS;      // Single
        int8u   GR;         // Locked

        Ztring Line;

        stream()
        {
            DMF_reception=(int8u)-1;
            Format=(int8u)-1;

            // Table 8-2 Initial status
            G[0]=GS_Kanji;
            G[1]=GS_Alphanumeric;
            G[2]=GS_Hiragana;
            G[3]=GS_DRCS|GS_Macro;
            G_Width[0]=2;
            G_Width[1]=1;
            G_Width[2]=1;
            G_Width[3]=1;
            GL=0;
            GL_SS=0;
            GR=2;
        }
    };
    typedef std::vector<stream> streams;
    streams Streams;

    //Temp
    void JIS (int8u Row, int8u Column);
};

} //NameSpace

#endif

