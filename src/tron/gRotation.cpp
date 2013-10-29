/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2000  Manuel Moos (manuel@moosnet.de)

**************************************************************************

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

***************************************************************************

*/

#include "gRotation.h"
#include "gGame.h"
#include "gParser.h"

#include "tConfiguration.h"
#include "tDirectories.h"
#include "tRecorder.h"
#include "tResourceManager.h"

#include "eGrid.h"

int gRotation::counter_ = 0;
bool gRotation::queueActive_ = false;

tList<gQueuePlayers> gQueuePlayers::queuePlayers;

gRotation *mapRotation    = new gRotation();    //  class data for map rotation files
gRotation *configRotation = new gRotation();    //  class data for config rotation files

gRotationRound *mapRoundRotation    = new gRotationRound(); //  class data for the map round rotation
gRotationRound *configRoundRotation = new gRotationRound(); //  class data for the config round rotation

gRotation *mapStorage    = new gRotation();     //  class data for storing non-rotation maps
gRotation *configStorage = new gRotation();     //  class data for storing non-rotation configs

gQueueing *sg_MapQueueing    = new gQueueing();     //  class data for storing queueing maps
gQueueing *sg_ConfigQueueing = new gQueueing();     //  class data for storing queueing configs

static void sg_mapStorage(std::istream &s)
{
    tString mapsT;
    mapsT.ReadLine(s);

    if (mapsT.Filter() == "")
        return;

    //  clear previous map cache in class data
    mapStorage->Clear();

    tArray<tString> mapFiles = mapsT.Split(";");
    for(int i = 0; i < mapFiles.Len(); i++)
    {
        tString map = mapFiles[i];

        if (map.Filter() != "")
        {
            gRotationItem *gRotItem = new gRotationItem(map);
            mapStorage->Add(gRotItem);
        }
    }
}
static tConfItemFunc sg_mapStorageCcnf("MAP_STORAGE", &sg_mapStorage);

static void sg_configStorage(std::istream &s)
{
    tString configsT;
    configsT.ReadLine(s);

    if (configsT.Filter() == "")
        return;

    //  clear previous config cache in class data
    configStorage->Clear();

    tArray<tString> configFiles = configsT.Split(";");
    for(int i = 0; i < configFiles.Len(); i++)
    {
        tString config = configFiles[i];

        if (config.Filter() != "")
        {
            gRotationItem *gRotItem = new gRotationItem(config);
            configStorage->Add(gRotItem);
        }
    }
}
static tConfItemFunc sg_configStorageCcnf("CONFIG_STORAGE", &sg_configStorage);

int sg_configRotationType = 0;
bool restrictConfigRotationTypes(const int &newValue)
{
    if ((newValue < 0) || newValue > 1)
    {
        return false;
    }
    return true;
}
static tSettingItem<int> sg_configRotationTypeConf("CONFIG_ROTATION_TYPE", sg_configRotationType, &restrictConfigRotationTypes);

enum gRotationType
{
    gROTATION_NEVER = 0,
    gROTATION_ORDERED_ROUND = 1,
    gROTATION_ORDERED_MATCH = 2,
    gROTATION_RANDOM_ROUND = 3,
    gROTATION_RANDOM_MATCH = 4,
    gROTATION_COUNTER = 5,
    gROTATION_ROUND = 6
};
tCONFIG_ENUM( gRotationType );

gRotationType rotationtype = gROTATION_NEVER;
bool restrictRotatiobType(const gRotationType &newValue)
{
    if ((newValue < gROTATION_NEVER) || (newValue > gROTATION_ROUND))
    {
        return false;
    }
    return true;
}
static tSettingItem<gRotationType> conf_rotationtype("ROTATION_TYPE", rotationtype, &restrictRotatiobType);

// config item for semi-colon deliminated list of maps/configs, needs semi-colon at the end
// ie, original/map-1.0.1.xml|round1;original/map-1.0.1.xml|round2;
// |round is optional to add
static void sg_mapRotation(std::istream &s)
{
    tString mapsT;
    mapsT.ReadLine(s);

    if (mapsT.Filter() == "")
        return;

    //  clear previous map cache in rotation
    mapRotation->Clear();
    mapRoundRotation->Clear();

    /*
    int strpos = 0;
    int nextsemicolon = mapsT.StrPos(";");

    if (nextsemicolon != -1)
    {
        do
        {
            tString const &map = mapsT.SubStr(strpos, nextsemicolon - strpos);

            strpos = nextsemicolon + 1;
            nextsemicolon = mapsT.StrPos(strpos, ";");

            mapRotation->Add(map);
        }
        while ((nextsemicolon = mapsT.StrPos(strpos, ";")) != -1);
    }
    */

    tArray<tString> mapFiles = mapsT.Split(";");
    for(int i = 0; i < mapFiles.Len(); i++)
    {
        tString map = mapFiles[i];
        tArray<tString> mapsParams = map.Split("|");

        if (map.Filter() != "")
        {
            gRotationItem *gRotItem = NULL;
            if (mapsParams.Len() >= 2)
                gRotItem = new gRotationItem(mapsParams[0], atoi(mapsParams[1]));
            else
                gRotItem = new gRotationItem(mapsParams[0], i);
            mapRotation->Add(gRotItem);
        }
    }

    for(int j = 0; j < mapRotation->Size(); j++)
    {
        gRotationItem *gRotItem = mapRotation->Get(j);
        if (gRotItem)
        {
            gRotationRoundSelection *roundSelection = NULL;

            //! Check for round existance in records
            if (!mapRoundRotation->Check(gRotItem->Round()))
            {
                roundSelection = new gRotationRoundSelection(gRotItem->Round());
                mapRoundRotation->Add(roundSelection);
            }
            else
            {
                roundSelection = mapRoundRotation->Get(gRotItem->Round());
            }

            if (roundSelection)
            {
                roundSelection->Add(gRotItem);
            }
        }
    }
}
static tConfItemFunc sg_mapRotationConf("MAP_ROTATION", &sg_mapRotation);

static void sg_mapRotationAdd(std::istream &s)
{
    tString params, mapStr, roundStr;
    params.ReadLine(s);
    int pos = 0;

    //  don't continue unless the params aren't empty
    if (params.Filter() == "") return;

    mapStr   = params.ExtractNonBlankSubString(pos);
    roundStr = params.ExtractNonBlankSubString(pos);

    if (mapStr.Filter() == "") return;
    if (roundStr.Filter() == "") return;

    //  check whether map already exists within rotation
    for(int i = 0; i < mapRotation->Size(); i++)
    {
        gRotationItem *gRotItemSelect = mapRotation->Get(i);
        if (gRotItemSelect)
        {
            if (mapStr == gRotItemSelect->Name()) return;
        }
    }

    gRotationItem *gRotItem = new gRotationItem(mapStr, atoi(roundStr));
    mapRotation->Add(gRotItem);

    //  adding new addition to the round selection list
    gRotationRoundSelection *roundSelection = NULL;

    //! Check for round existance in records
    if (!mapRoundRotation->Check(gRotItem->Round()))
    {
        roundSelection = new gRotationRoundSelection(gRotItem->Round());
        mapRoundRotation->Add(roundSelection);
    }
    else
    {
        roundSelection = mapRoundRotation->Get(gRotItem->Round());
    }

    if (roundSelection)
    {
        roundSelection->Add(gRotItem);
    }
}
static tConfItemFunc sg_mapRotationAddConf("MAP_ROTATION_ADD", &sg_mapRotationAdd);

static void sg_mapRotationSet(std::istream &s)
{
    tString params, mapStr, roundStr;
    params.ReadLine(s);
    int pos = 0;

    //  don't continue unless the params aren't empty
    if (params.Filter() == "") return;

    mapStr   = params.ExtractNonBlankSubString(pos);
    roundStr = params.ExtractNonBlankSubString(pos);

    if (mapStr.Filter() == "") return;
    if (roundStr.Filter() == "") return;

    //  check whether map already exists within rotation
    for(int i = 0; i < mapRotation->Size(); i++)
    {
        gRotationItem *gRotItemSelect = mapRotation->Get(i);
        if (gRotItemSelect)
        {
            if (mapStr == gRotItemSelect->Name())
            {
                gRotationRoundSelection *roundSelectionOld = mapRoundRotation->Get(gRotItemSelect->Round());
                if (roundSelectionOld)
                {
                    roundSelectionOld->Remove(gRotItemSelect);
                }

                //  set the new round value
                gRotItemSelect->SetRound(atoi(roundStr));

                gRotationRoundSelection *roundSelectionNew = NULL;
                //! Check for round existance in records
                if (!mapRoundRotation->Check(gRotItemSelect->Round()))
                {
                    roundSelectionNew = new gRotationRoundSelection(gRotItemSelect->Round());
                    mapRoundRotation->Add(roundSelectionNew);
                }
                else
                {
                    roundSelectionNew = mapRoundRotation->Get(gRotItemSelect->Round());
                }

                if (roundSelectionNew)
                {
                    roundSelectionNew->Add(gRotItemSelect);
                }
                break;
            }
        }
    }
}
static tConfItemFunc sg_mapRotationSetConf("MAP_ROTATION_SET", &sg_mapRotationSet);

static void sg_mapRotationLoad(std::istream &s)
{
    int map_id = 0;
    s >> map_id;

    if ((map_id > 0) && (map_id < mapRotation->Size()))
    {
        gRotationItem *gRotItem = mapRotation->Get(map_id);
        if (gRotItem)
        {
            //  load in the map for that round
            LoadMap(gRotItem->Name());
            mapRotation->SetID(map_id);
        }
    }
}
static tConfItemFunc sg_mapRotationLoadConf("MAP_ROTATION_LOAD", &sg_mapRotationLoad);
static tAccessLevelSetter sg_mapRotationLoadConfLvl(sg_mapRotationLoadConf, tAccessLevel_Moderator);

static void sg_mapRotationRemove(std::istream &s)
{
    tString params, mapStr;
    params.ReadLine(s);
    int pos = 0;

    //  don't continue unless the params aren't empty
    if (params.Filter() == "") return;

    mapStr = params.ExtractNonBlankSubString(pos);

    if (mapStr.Filter() == "") return;

    //  check whether map already exists within rotation
    for(int i = 0; i < mapRotation->Size(); i++)
    {
        gRotationItem *gRotItemSelect = mapRotation->Get(i);
        if (gRotItemSelect)
        {
            if (mapStr == gRotItemSelect->Name())
            {
                gRotationRoundSelection *roundSelection = mapRoundRotation->Get(gRotItemSelect->Round());
                if (roundSelection)
                {
                    roundSelection->Remove(gRotItemSelect);
                }

                mapRotation->Remove(gRotItemSelect);

                tOutput msg;
                msg.SetTemplateParameter(1, gRotItemSelect->Name());
                msg << "$map_remove_success";
                sn_ConsoleOut(msg);

                delete gRotItemSelect;
                return;
            }
        }
    }

    tOutput msg;
    msg.SetTemplateParameter(1, mapStr);
    msg << "$map_remove_failed";
    sn_ConsoleOut(msg, 0);
}
static tConfItemFunc sg_mapRotationRemoveConf("MAP_ROTATION_REMOVE", &sg_mapRotationRemove);

static void sg_configRotation(std::istream &s)
{
    tString configsT;
    configsT.ReadLine(s);

    if (configsT.Filter() == "")
        return;

    //  clear previous config cache in rotation
    configRotation->Clear();
    configRoundRotation->Clear();

    /*int strpos = 0;
    int nextsemicolon = configsT.StrPos(";");

    if (nextsemicolon != -1)
    {
        do
        {
            tString const &map = configsT.SubStr(strpos, nextsemicolon - strpos);

            strpos = nextsemicolon + 1;
            nextsemicolon = configsT.StrPos(strpos, ";");

            configRotation->Add(map);
        }
        while ((nextsemicolon = configsT.StrPos(strpos, ";")) != -1);
    }*/

    tArray<tString> configFiles = configsT.Split(";");
    for(int i = 0; i < configFiles.Len(); i++)
    {
        tString config = configFiles[i];

        if (config.Filter() != "")
        {
            gRotationItem *gRotItem = new gRotationItem(config, i);
            configRotation->Add(gRotItem);
        }
    }

    for(int j = 0; j < configRotation->Size(); j++)
    {
        gRotationItem *gRotItem = configRotation->Get(j);
        if (gRotItem)
        {
            gRotationRoundSelection *roundSelection = NULL;

            //! Check for round existance in records
            if (!configRoundRotation->Check(gRotItem->Round()))
            {
                roundSelection = new gRotationRoundSelection(gRotItem->Round());
                configRoundRotation->Add(roundSelection);
            }
            else
            {
                roundSelection = configRoundRotation->Get(gRotItem->Round());
            }

            if (roundSelection)
            {
                roundSelection->Add(gRotItem);
            }
        }
    }
}
static tConfItemFunc sg_configRotationConf("CONFIG_ROTATION", sg_configRotation);

static void sg_configRotationAdd(std::istream &s)
{
    tString params, configStr, roundStr;
    params.ReadLine(s);
    int pos = 0;

    //  don't continue unless the params aren't empty
    if (params.Filter() == "") return;

    configStr   = params.ExtractNonBlankSubString(pos);
    roundStr = params.ExtractNonBlankSubString(pos);

    if (configStr.Filter() == "") return;
    if (roundStr.Filter() == "") return;

    //  check whether map already exists within rotation
    for(int i = 0; i < configRotation->Size(); i++)
    {
        gRotationItem *gRotItemSelect = configRotation->Get(i);
        if (gRotItemSelect)
        {
            if (configStr == gRotItemSelect->Name()) return;
        }
    }

    gRotationItem *gRotItem = new gRotationItem(configStr, atoi(roundStr));
    configRotation->Add(gRotItem);

    //  adding new addition to the round selection list
    gRotationRoundSelection *roundSelection = NULL;

    //! Check for round existance in records
    if (!configRoundRotation->Check(gRotItem->Round()))
    {
        roundSelection = new gRotationRoundSelection(gRotItem->Round());
        configRoundRotation->Add(roundSelection);
    }
    else
    {
        roundSelection = configRoundRotation->Get(gRotItem->Round());
    }

    if (roundSelection)
    {
        roundSelection->Add(gRotItem);
    }
}
static tConfItemFunc sg_configRotationAddConf("CONFIG_ROTATION_ADD", &sg_configRotationAdd);

static void sg_configRotationSet(std::istream &s)
{
    tString params, configStr, roundStr;
    params.ReadLine(s);
    int pos = 0;

    //  don't continue unless the params aren't empty
    if (params.Filter() == "") return;

    configStr   = params.ExtractNonBlankSubString(pos);
    roundStr = params.ExtractNonBlankSubString(pos);

    if (configStr.Filter() == "") return;
    if (roundStr.Filter() == "") return;

    //  check whether map already exists within rotation
    for(int i = 0; i < configRotation->Size(); i++)
    {
        gRotationItem *gRotItemSelect = configRotation->Get(i);
        if (gRotItemSelect)
        {
            if (configStr == gRotItemSelect->Name())
            {
                gRotationRoundSelection *roundSelectionOld = configRoundRotation->Get(gRotItemSelect->Round());
                if (roundSelectionOld)
                {
                    roundSelectionOld->Remove(gRotItemSelect);
                }

                //  set the new round value
                gRotItemSelect->SetRound(atoi(roundStr));

                gRotationRoundSelection *roundSelectionNew = NULL;
                //! Check for round existance in records
                if (!configRoundRotation->Check(gRotItemSelect->Round()))
                {
                    roundSelectionNew = new gRotationRoundSelection(gRotItemSelect->Round());
                    configRoundRotation->Add(roundSelectionNew);
                }
                else
                {
                    roundSelectionNew = configRoundRotation->Get(gRotItemSelect->Round());
                }

                if (roundSelectionNew)
                {
                    roundSelectionNew->Add(gRotItemSelect);
                }
                break;
            }
        }
    }
}
static tConfItemFunc sg_configRotationSetConf("CONFIG_ROTATION_SET", &sg_configRotationSet);

static void sg_configRotationLoad(std::istream &s)
{
    int config_id = 0;
    s >> config_id;

    if ((config_id > 0) && (config_id < configRotation->Size()))
    {
        gRotationItem *gRotItem = configRotation->Get(config_id);
        if (gRotItem)
        {
            if (sg_configRotationType == 0)
            {
                st_Include( gRotItem->Name() );
            }
            else if (sg_configRotationType == 1)
            {
                tString rclcl = tResourceManager::locateResource(NULL, gRotItem->Name());
                if ( rclcl ) {
                    std::ifstream rc(rclcl);
                    tConfItemBase::LoadAll(rc, false );
                    return;
                }

                con << tOutput( "$config_rinclude_not_found", gRotItem->Name());
            }

            configRotation->SetID(config_id);
        }
    }
}
static tConfItemFunc sg_configRotationLoadConf("CONFIG_ROTATION_LOAD", &sg_configRotationLoad);
static tAccessLevelSetter sg_configRotationLoadConfLvl(sg_configRotationLoadConf, tAccessLevel_Moderator);

static void sg_configRotationRemove(std::istream &s)
{
    tString params, configStr;
    params.ReadLine(s);
    int pos = 0;

    //  don't continue unless the params aren't empty
    if (params.Filter() == "") return;

    configStr = params.ExtractNonBlankSubString(pos);

    if (configStr.Filter() == "") return;

    //  check whether map already exists within rotation
    for(int i = 0; i < configRotation->Size(); i++)
    {
        gRotationItem *gRotItemSelect = configRotation->Get(i);
        if (gRotItemSelect)
        {
            if (configStr == gRotItemSelect->Name())
            {
                gRotationRoundSelection *roundSelection = mapRoundRotation->Get(gRotItemSelect->Round());
                if (roundSelection)
                {
                    roundSelection->Remove(gRotItemSelect);
                }

                configRotation->Remove(gRotItemSelect);

                tOutput msg;
                msg.SetTemplateParameter(1, gRotItemSelect->Name());
                msg << "$config_remove_success";
                sn_ConsoleOut(msg);

                delete gRotItemSelect;
                break;
            }
        }
    }

    tOutput msg;
    msg.SetTemplateParameter(1, configStr);
    msg << "$config_remove_failed";
    sn_ConsoleOut(msg, 0);
}
static tConfItemFunc sg_configRotationRemoveConf("CONFIG_ROTATION_REMOVE", &sg_configRotationRemove);

void sg_DisplayRotationList(ePlayerNetID *p, std::istream &s, tString command)
{
    if (p)
    {
        int max = 10;
        int showing = 0;

        tString params;
        int pos = 0;
        params.ReadLine(s);

        int showAmount = 0;
        tString amount = params.ExtractNonBlankSubString(pos);
        if (amount.Filter() != "") showAmount = atoi(amount);

        if (command == "/mr")
        {
            if (mapRotation->Size() > 0)
            {
                if (showAmount < mapRotation->Size())
                {
                    if (mapRotation->Size() < max) max = mapRotation->Size();
                    if ((mapRotation->Size() - showAmount) < max) max = mapRotation->Size() - showAmount;

                    if (max > 0)
                    {
                        sn_ConsoleOut(tOutput("$map_rotation_list"), p->Owner());

                        for(int i = 0; i < max; i++)
                        {
                            int rotID = showAmount + i;
                            gRotationItem *mapRotItem = mapRotation->Get(rotID);
                            if (mapRotItem)
                            {
                                tColoredString output;
                                output << "0x55ffff" << rotID << ") 0xff55ff";
                                output << mapRotItem->Name();
                                output << tColoredString::ColorString(1, 1, 1) << "\n";
                                sn_ConsoleOut(output, p->Owner());

                                showing++;
                            }
                        }

                        tOutput show;
                        show.SetTemplateParameter(1, showing);
                        show.SetTemplateParameter(2, mapRotation->Size());

                        show << "$map_rotation_list_show";
                        sn_ConsoleOut(show, p->Owner());
                    }
                }
            }
        }
        else if (command == "/msr")
        {
            if (mapStorage->Size() > 0)
            {
                if (showAmount < mapStorage->Size())
                {
                    if (mapStorage->Size() < max) max = mapStorage->Size();
                    if ((mapStorage->Size() - showAmount) < max) max = mapStorage->Size() - showAmount;

                    if (max > 0)
                    {
                        sn_ConsoleOut(tOutput("$map_storage_list"), p->Owner());

                        for(int i = 0; i < max; i++)
                        {
                            int rotID = showAmount + i;
                            gRotationItem *mapRotItem = mapStorage->Get(rotID);
                            if (mapRotItem)
                            {
                                tColoredString output;
                                output << "0x55ffff" << rotID << ") 0xff55ff";
                                output << mapRotItem->Name();
                                output << tColoredString::ColorString(1, 1, 1) << "\n";
                                sn_ConsoleOut(output, p->Owner());

                                showing++;
                            }
                        }

                        tOutput show;
                        show.SetTemplateParameter(1, showing);
                        show.SetTemplateParameter(2, mapStorage->Size());

                        show << "$map_storage_list_show";
                        sn_ConsoleOut(show, p->Owner());
                    }
                }
            }
        }
        else if (command == "/cr")
        {
            sn_ConsoleOut(tOutput("$config_rotation_list"), p->Owner());

            if (configRotation->Size() > 0)
            {
                if (showAmount < configRotation->Size())
                {
                    if (configRotation->Size() < max) max = configRotation->Size();
                    if ((configRotation->Size() - showAmount) < max) max = configRotation->Size() - showAmount;

                    if (max > 0)
                    {
                        for(int i = 0; i < max; i++)
                        {
                            int rotID = showAmount + i;
                            gRotationItem *configRotItem = configRotation->Get(rotID);
                            if (configRotItem)
                            {
                                tColoredString output;
                                output << "0x55ffff" << rotID << ") 0xff55ff";
                                output << configRotItem->Name();
                                output << tColoredString::ColorString(1, 1, 1) << "\n";
                                sn_ConsoleOut(output, p->Owner());

                                showing++;
                            }
                        }

                        tOutput show;
                        show.SetTemplateParameter(1, showing);
                        show.SetTemplateParameter(2, configRotation->Size());

                        show << "$config_rotation_list_show";
                        sn_ConsoleOut(show, p->Owner());
                    }
                }
            }
        }
        else if (command == "/csr")
        {
            sn_ConsoleOut(tOutput("$config_storage_list"), p->Owner());

            if (configStorage->Size() > 0)
            {
                if (showAmount < configStorage->Size())
                {
                    if (configStorage->Size() < max) max = configStorage->Size();
                    if ((configStorage->Size() - showAmount) < max) max = configStorage->Size() - showAmount;

                    if (max > 0)
                    {
                        for(int i = 0; i < max; i++)
                        {
                            int rotID = showAmount + i;
                            gRotationItem *configRotItem = configStorage->Get(rotID);
                            if (configRotItem)
                            {
                                tColoredString output;
                                output << "0x55ffff" << rotID << ") 0xff55ff";
                                output << configRotItem->Name();
                                output << tColoredString::ColorString(1, 1, 1) << "\n";
                                sn_ConsoleOut(output, p->Owner());

                                showing++;
                            }
                        }

                        tOutput show;
                        show.SetTemplateParameter(1, showing);
                        show.SetTemplateParameter(2, configStorage->Size());

                        show << "$config_storage_list_show";
                        sn_ConsoleOut(show, p->Owner());
                    }
                }
            }
        }
    }
}

void sg_ResetRotation()
{
    mapRotation->Reset();
    configRotation->Reset();
    if ( rotationtype != gROTATION_NEVER )
        con << tOutput( "$reset_rotation_message" ) << '\n';
}

void sg_ResetRotation( std::istream & )
{
    sg_ResetRotation();
}

static tConfItemFunc sg_resetRotationConfItemFunc( "RESET_ROTATION", sg_ResetRotation );

static bool sg_resetRotationOnNewMatch = false;
static tSettingItem< bool > sg_resetRotationOnNewMatchSettingItem( "RESET_ROTATION_ON_START_NEW_MATCH", sg_resetRotationOnNewMatch );

int sg_rotationMax = sg_currentSettings->limitRounds;
bool restrictRotationMax(const int &newValue)
{
    if (newValue <= 0) return false;

    return true;
}
static tSettingItem<int> sg_rotationMaxConf("ROTATION_MAX", sg_rotationMax);

static void sg_MapQueueingFunc(std::istream &s)
{
    tString params;
    params.ReadLine(s);
    int pos = 0;

    if (params != "")
    {
        tString items = params.ExtractNonBlankSubString(pos);
        if (items != "")
        {
            tString item = items;
            bool mapFound = false;
            tArray<tString> searchFindings;
            for (int i=0; i < mapRotation->Size(); i++)
            {
                gRotationItem *mapRotItem = mapRotation->Get(i);
                if (mapRotItem)
                {
                    tString filteredName, filteredSearch;
                    filteredName = ePlayerNetID::FilterName(mapRotItem->Name());
                    filteredSearch = ePlayerNetID::FilterName(item);
                    if (filteredName.Contains(filteredSearch))
                    {
                        mapFound = true;
                        searchFindings.Insert(mapRotItem->Name());
                    }
                }
            }
            if (!mapFound)
            {
                tOutput Output;
                Output.SetTemplateParameter(1, item);
                Output << "$map_queueing_file_not_found";
                sn_ConsoleOut(Output, 0);
            }
            else
            {
                if (searchFindings.Len() == 1)
                {
                    bool found = false;
                    tString mapName = searchFindings[0];
                    for (int j=0; j < sg_MapQueueing->Size(); j++)
                    {
                        tString item_name = sg_MapQueueing->Get(j);
                        if (item_name == mapName)
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, stripMapNameOnly(mapName));
                            Output.SetTemplateParameter(2, j+1);
                            Output << "$map_queueing_stored_file_found";
                            sn_ConsoleOut(Output, 0);
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        sg_MapQueueing->Insert(mapName);
                        tOutput Output;
                        Output.SetTemplateParameter(1, stripMapNameOnly(mapName));
                        Output << "$map_queueing_file_stored";
                        sn_ConsoleOut(Output);
                    }
                }
                else
                {
                    tOutput Output;
                    Output.SetTemplateParameter(1, item);
                    Output.SetTemplateParameter(2, searchFindings.Len());
                    Output << "$map_queueing_found_toomanyfiles";
                    sn_ConsoleOut(Output, 0);
                }
            }

            searchFindings.Clear();
        }
    }
}
static tConfItemFunc sg_MapQueueingFuncConf("QUEUE_MAP", &sg_MapQueueingFunc);

static void sg_ConfigQueueingFunc(std::istream &s)
{
    tString params;
    params.ReadLine(s);
    int pos = 0;

    tString items = params.ExtractNonBlankSubString(pos);
    if (items != "")
    {
        tString item = items;
        bool configFound = false;
        tArray<tString> searchFindings;
        for (int i=0; i < configRotation->Size(); i++)
        {
            gRotationItem *configRotItem = configRotation->Get(i);
            if (configRotItem)
            {
                tString filteredName, filteredSearch;
                filteredName = ePlayerNetID::FilterName(configRotItem->Name());
                filteredSearch = ePlayerNetID::FilterName(item);
                if (filteredName.Contains(filteredSearch))
                {
                    configFound = true;
                    searchFindings.Insert(configRotItem->Name());
                }
            }
        }
        if (!configFound)
        {
            tOutput Output;
            Output.SetTemplateParameter(1, item);
            Output << "$config_queueing_file_not_found";
            sn_ConsoleOut(Output, 0);
        }
        else
        {
            if (searchFindings.Len() == 1)
            {
                bool found = false;
                tString configName = searchFindings[0];
                for (int j=0; j < sg_ConfigQueueing->Size(); j++)
                {
                    tString item_name =  sg_ConfigQueueing->Get(j);
                    if (item_name == configName)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, configName);
                        Output.SetTemplateParameter(2, j+1);
                        Output << "$config_queueing_stored_file_found";
                        sn_ConsoleOut(Output, 0);
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    sg_ConfigQueueing->Insert(configName);
                    tOutput Output;
                    Output.SetTemplateParameter(1, configName);
                    Output << "config_queueing_file_stored";
                    sn_ConsoleOut(Output);
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, item);
                Output.SetTemplateParameter(2, searchFindings.Len());
                Output << "$config_queueing_found_toomanyfiles";
                sn_ConsoleOut(Output, 0);
            }
        }

        searchFindings.Clear();
    }
}
static tConfItemFunc sg_ConfigQueueingFuncConf("QUEUE_CONFIG", &sg_ConfigQueueingFunc);

void sg_ResetMapqueueing( std::istream &s)
{
    for (int i = 0; i < sg_MapQueueing->Size(); i++)
    {
        sg_MapQueueing->Remove(i);
    }
}
static tConfItemFunc sg_resetMapqueueingConf( "RESET_MAP_QUEUEING", &sg_ResetMapqueueing );

void sg_ResetConfigqueueing( std::istream &s )
{
    for (int i=0; i < sg_ConfigQueueing->Size(); i++)
    {
        sg_ConfigQueueing->Remove(i);
    }
}
static tConfItemFunc sg_ResetConfigqueueingConf( "RESET_CONFIG_QUEUEING", &sg_ResetConfigqueueing );

int sg_queueLimit = 20;
static tSettingItem<int> sg_queueLimitConf("QUEUE_LIMIT", sg_queueLimit);

int sg_queueLimitExcempt = 2;
static tSettingItem<int> sg_queueLimitExcemptConf("QUEUE_LIMIT_EXCEMPT", sg_queueLimitExcempt);

bool sg_queueLimitEnabled = false;
static tSettingItem<bool> sg_queueLimitEnabledConf("QUEUE_LIMIT_ENABLED", sg_queueLimitEnabled);

int sg_queueIncrement = 0;
static tSettingItem<int> sg_queueIncrementConf("QUEUE_INCREMENT", sg_queueIncrement);

REAL sg_queueRefillTime = 1;
bool restrictRefillTime(const REAL &newValue)
{
    if (newValue <= 0) return false;
    return true;
}
static tSettingItem<REAL> sg_queueRefillTimeConf("QUEUE_REFILL_TIME", sg_queueRefillTime, &restrictRefillTime);

bool sg_queueRefillActive = false;
static tSettingItem<bool> sg_queueRefillActiveConf("QUEUE_REFILL_ACTIVE", sg_queueRefillActive);

static int sg_queueMax = 30;
static tSettingItem<int> sg_queueMaxConf("QUEUE_MAX", sg_queueMax);

static void sg_queueRefill(std::istream &s)
{
    tString player;
    tOutput msg;

    s >> player;

#ifdef DEBUG
    con << "Queuer Name:" << player << "\n";
#endif

    tList<gQueuePlayers> listOfFinds;

    if (gQueuePlayers::queuePlayers.Len() > 0)
    {
        for(int i = 0; i < gQueuePlayers::queuePlayers.Len(); i++)
        {
            gQueuePlayers *qPlayer = gQueuePlayers::queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name().Filter().Contains(player.Filter()))
                {
                    listOfFinds.Insert(qPlayer);
                }
            }
        }
    }

    switch (listOfFinds.Len())
    {
        case 1:
        {
            gQueuePlayers *qPlayer = listOfFinds[0];
            if (qPlayer)
            {
                qPlayer->SetQueue(qPlayer->QueueDefault());

                msg.SetTemplateParameter(1, qPlayer->Name());
                msg << "$queue_refill_success";
                sn_ConsoleOut(msg);

                gQueuePlayers::Save();
            }
            break;
        }

        default:
        {
            msg.SetTemplateParameter(1, player);
            msg.SetTemplateParameter(2, listOfFinds.Len());
            msg << "$queue_find_failed";
            sn_ConsoleOut(msg, 0);
            break;
        }
    }
}
static tConfItemFunc sg_queueRefillConf("QUEUE_REFILL", &sg_queueRefill);

static void sg_queueGive(std::istream &s)
{
    tString player, queueStr;
    tOutput msg;

    s >> player;

    s >> queueStr;
    int new_queues = atoi(queueStr);

#ifdef DEBUG
    con << "Queuer Name:" << player << "\n";
#endif

    tList<gQueuePlayers> listOfFinds;

    if (gQueuePlayers::queuePlayers.Len() > 0)
    {
        for(int i = 0; i < gQueuePlayers::queuePlayers.Len(); i++)
        {
            gQueuePlayers *qPlayer = gQueuePlayers::queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name().Filter().Contains(player.Filter()))
                {
                    listOfFinds.Insert(qPlayer);
                }
            }
        }
    }

    switch (listOfFinds.Len())
    {
        case 1:
        {
            if (new_queues > 0)
            {
                gQueuePlayers *qPlayer = listOfFinds[0];

                //  send message
                msg.SetTemplateParameter(1, qPlayer->Name());
                msg.SetTemplateParameter(2, qPlayer->Queues());
                msg.SetTemplateParameter(3, new_queues);
                msg << "$queue_give_success";
                sn_ConsoleOut(msg);

                //  apply
                qPlayer->SetQueue(new_queues);

                gQueuePlayers::Save();
            }
            break;
        }

        default:
        {
            msg.SetTemplateParameter(1, player);
            msg.SetTemplateParameter(2, listOfFinds.Len());
            msg << "$queue_find_failed";
            sn_ConsoleOut(msg, 0);
            break;
        }
    }
}
static tConfItemFunc sg_queueGiveConf("QUEUE_GIVE", &sg_queueGive);

gQueuePlayers::gQueuePlayers(ePlayerNetID *player)
{
    this->name_ = player->GetUserName();
    this->owner_ = NULL;

    this->played_ = 0;
    this->refill_ = -1;

    this->queuesDefault = sg_queueLimit;
    this->queues_ = this->queuesDefault;

    this->lastTime_ = se_GameTime();

    queuePlayers.Insert(this);
}

gQueuePlayers::gQueuePlayers(tString name)
{
    this->name_ = name;
    this->owner_ = NULL;

    this->played_ = 0;
    this->refill_ = 0;

    this->queuesDefault = sg_queueLimit;
    this->queues_ = this->queuesDefault;

    this->lastTime_ = se_GameTime();

    queuePlayers.Insert(this);
}

bool gQueuePlayers::PlayerExists(ePlayerNetID *player)
{
    if (queuePlayers.Len() > 0)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers * qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name() == player->GetUserName())
                        return true;
            }
        }
    }
    return false;
}

bool gQueuePlayers::PlayerExists(tString name)
{
    if (queuePlayers.Len() > 0)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers * qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name() == name)
                        return true;
            }
        }
    }
    return false;
}

gQueuePlayers *gQueuePlayers::GetData(ePlayerNetID *player)
{
    if (queuePlayers.Len() > 0)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers * qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name() == player->GetUserName())
                        return qPlayer;
            }
        }
    }
    return NULL;
}

gQueuePlayers *gQueuePlayers::GetData(tString name)
{
    if (queuePlayers.Len() > 0)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers * qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Name() == name)
                        return qPlayer;
            }
        }
    }
    return NULL;
}

void gQueuePlayers::RemovePlayer()
{
    owner_ = NULL;
}

bool gQueuePlayers::Timestep(REAL time)
{
    if ((queuePlayers.Len() > 0) && sg_queueLimitEnabled)
    {
        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers *qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                if (qPlayer->Player() && (qPlayer->Name() == ""))
                    qPlayer->name_ = qPlayer->Player()->GetUserName();
            }
        }

        for(int i = 0; i < queuePlayers.Len(); i++)
        {
            gQueuePlayers *qPlayer = queuePlayers[i];
            if (qPlayer)
            {
                // account for play time
                REAL tick = time - qPlayer->lastTime_;

                if ((qPlayer->Player() && sg_queueRefillActive) || !sg_queueRefillActive)
                {
                    //qPlayer->played_ += tick;
                    qPlayer->played_ += 0.35f;

                    if (qPlayer->played_ >= qPlayer->refill_)
                    {
                        //  if queue increment is enabled
                        if (sg_queueIncrement > 0)
                        {
                            //  if queue max is below 0
                            if (sg_queueMax <= 0)
                            {
                                qPlayer->queuesDefault += sg_queueIncrement;
                            }
                            //  if queue max is above 0
                            else
                            {
                                if (qPlayer->queuesDefault < sg_queueMax)
                                {
                                    //  increase their default queue limit by this amount
                                    qPlayer->queuesDefault += sg_queueIncrement;
                                }
                            }

                            //  create the new time for the next time to refill
                            qPlayer->refill_ = qPlayer->played_ + sg_queueRefillTime;
                        }

                        //  refill queues with their original amount
                        if (qPlayer->queues_ == 0)
                        {
                            qPlayer->queues_ = qPlayer->queuesDefault;

                            tOutput msg;
                            msg.SetTemplateParameter(1, qPlayer->Name());
                            msg << "$queue_refill_complete";
                            sn_ConsoleOut(msg);
                        }
                    }
                }

                qPlayer->lastTime_ = time;
            }
        }
    }
}

void gQueuePlayers::Save()
{
    if ((queuePlayers.Len() > 0) && sg_queueLimitEnabled)
    {
        std::ofstream o;
        if (tDirectories::Var().Open(o, "queuers.txt"))
        {
            for(int i = 0; i < queuePlayers.Len(); i++)
            {
                gQueuePlayers *qPlayer = queuePlayers[i];
                if (qPlayer)
                {
                    o << qPlayer->Name() << " " << qPlayer->Queues() << " " << qPlayer->QueueDefault() << " " << qPlayer->PlayedTime()<< " " << qPlayer->RefillTime() << "\n";
                }
            }
        }
    }
}

void gQueuePlayers::Load()
{
    if (queuePlayers.Len() > 0)
        queuePlayers.Clear();

    std::ifstream r;
    if (tDirectories::Var().Open(r, "queuers.txt"))
    {
        while (!r.eof())
        {
            //std::stringstream s(r.getline());

            std::string sayLine;
            std::getline(r, sayLine);
            std::istringstream s(sayLine);
            tString params;

            params.ReadLine(s);
            int pos = 0;

            if (params.Filter() != "")
            {
                gQueuePlayers *qPlayer = NULL;

                tString name = params.ExtractNonBlankSubString(pos);
                if (!PlayerExists(name))
                {
                    qPlayer = new gQueuePlayers(name);
                }
                else
                {
                    qPlayer = GetData(name);
                }

                qPlayer->queues_ = atoi(params.ExtractNonBlankSubString(pos));
                qPlayer->queuesDefault = atoi(params.ExtractNonBlankSubString(pos));
                qPlayer->played_ = atof(params.ExtractNonBlankSubString(pos));
                qPlayer->refill_ = atof(params.ExtractNonBlankSubString(pos));
            }
        }
    }
    r.close();
}

void gQueuePlayers::Reset(REAL time)
{
    for (int i = 0; i < queuePlayers.Len(); i++)
    {
        gQueuePlayers *qPlayer = queuePlayers[i];
        if (qPlayer)
            qPlayer->lastTime_ = time;
    }

    Save();
}

bool gQueuePlayers::CanQueue(ePlayerNetID *p)
{
    if (sg_queueLimitEnabled)
    {
        //  allow access level of players from excempted to queue
        if (p->GetAccessLevel() <= sg_queueLimitExcempt ) return true;

        gQueuePlayers *qPlayer = NULL;
        if (!gQueuePlayers::PlayerExists(p))
            qPlayer = new gQueuePlayers(p);
        else
            qPlayer = gQueuePlayers::GetData(p);
        qPlayer->SetOwner(p);

        if (!qPlayer) return false;

        if (qPlayer->Queues() == 0)
        {
            tOutput warning;
            warning.SetTemplateParameter(1, (qPlayer->RefillTime() - (qPlayer->PlayedTime())));
            warning << "$queue_refill_waiting";
            sn_ConsoleOut(warning, p->Owner());

            return false;
        }

        if (qPlayer->Queues() > 0)
        {
            qPlayer->queues_ --;

            if (qPlayer->Queues() == 0)
            {
                REAL refillTime = sg_queueRefillTime;
                qPlayer->refill_ = qPlayer->PlayedTime() + refillTime;

                tOutput warning;
                warning.SetTemplateParameter(1, refillTime);
                warning << "$queue_limit_reached";
                sn_ConsoleOut(warning, p->Owner());
            }
        }
    }
    return true;
}

bool sg_QueueLog = false;
static tSettingItem<bool> sg_QueueLogConf("QUEUE_LOG", sg_QueueLog);

void sg_LogQueue(ePlayerNetID *p, tString command, tString params, tString item)
{
    if (!sg_QueueLog) return;

    std::ofstream o;
    if (tDirectories::Var().Open(o, "queuelog.txt", std::ios::app))
    {
        o << "[" << st_GetCurrentTime("%Y/%m/%d-%H:%M:%S") << "] " << p->GetName() << " | " << command << " " << params << " " << item << "\n";
    }
    o.close();
}

static void sg_queuersList(std::istream &s)
{
    int max = 10;
    int showing = 0;

    tString params;
    int pos = 0;
    params.ReadLine(s);

    int showAmount = 0;
    tString amount = params.ExtractNonBlankSubString(pos);
    if (amount.Filter() != "") showAmount = atoi(amount);

    if (gQueuePlayers::queuePlayers.Len() > 0)
    {
        if (showAmount < gQueuePlayers::queuePlayers.Len())
        {
            if (gQueuePlayers::queuePlayers.Len() < max) max = gQueuePlayers::queuePlayers.Len();
            if ((gQueuePlayers::queuePlayers.Len() - showAmount) < max) max = gQueuePlayers::queuePlayers.Len() - showAmount;

            if (max > 0)
            {
                sn_ConsoleOut(tOutput("$queuers_list"), 0);

                for(int i = 0; i < max; i++)
                {
                    int rotID = showAmount + i;
                    gQueuePlayers *queuePlayer = gQueuePlayers::queuePlayers[rotID];
                    if (queuePlayer)
                    {
                        tColoredString send;
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << "( ";
                        send << "0x55ffff" << queuePlayer->Name();
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << " | 0xff55ff" << queuePlayer->Queues() << " qs";
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << " | 0x88ff22" << queuePlayer->PlayedTime() << " s";
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << " | 0x88ff22" << queuePlayer->RefillTime() << " s";
                        send << tColoredString::ColorString( 1,1,.5 );
                        send << " )\n";
                        sn_ConsoleOut(send, 0);

                        showing++;
                    }
                }

                tOutput show;
                show.SetTemplateParameter(1, showing);
                show.SetTemplateParameter(2, gQueuePlayers::queuePlayers.Len());
                show << "$queuers_list_show";
                sn_ConsoleOut(show, 0);
            }
        }
    }
}
static tConfItemFunc sg_queuersListConf("QUEUERS_LIST", &sg_queuersList);
static tAccessLevelSetter sg_queuersListConfLevel( sg_queuersListConf, tAccessLevel_Program );

//  show user their current queue limit values, refill time and play time
void QueueShowPlayer(ePlayerNetID *player)
{
    gQueuePlayers *queuePlayer = gQueuePlayers::GetData(player);

    if (queuePlayer)
    {
        tColoredString send;
        send << tColoredString::ColorString( 1,1,.5 );
        send << "( ";
        send << "0x55ffff" << queuePlayer->Name();
        send << tColoredString::ColorString( 1,1,.5 );
        send << " | 0xff55ff" << queuePlayer->Queues() << " qs";
        send << tColoredString::ColorString( 1,1,.5 );
        send << " | 0x88ff22" << queuePlayer->PlayedTime() << " s";
        send << tColoredString::ColorString( 1,1,.5 );
        send << " | 0x88ff22" << queuePlayer->RefillTime() << " s";
        send << tColoredString::ColorString( 1,1,.5 );
        send << " )\n";
        sn_ConsoleOut(send, player->Owner());
    }
    else
    {
        tColoredString send;
        send << tColoredString::ColorString( 1,1,.5 );
        send << "QUEUE LIMIT: You have not made any queues to show your limits.";
        send << "\n";
        sn_ConsoleOut(send, player->Owner());
    }
}

//  show user the map they are searching for and its id in the MAP/CONFIG rotation
void RotationShowPlayer(ePlayerNetID *player, std::istream &s)
{
    tString params;
    params.ReadLine(s);
    int pos = 0;

    if (params.Filter() != "")
    {
        tString type = params.ExtractNonBlankSubString(pos).ToLower();
        if (type == "map")
        {
            tString map = params.ExtractNonBlankSubString(pos);
            tList<gRotationItem> foundMaps;
            for(int i = 0; i < mapRotation->Size(); i++)
            {
                gRotationItem *gRotItem = mapRotation->Get(i);
                if (gRotItem)
                {
                    if (gRotItem->Name().Filter().Contains(map.Filter()))
                    {
                        foundMaps.Insert(gRotItem);
                    }
                }
            }

            if (foundMaps.Len() > 1)
            {
                sn_ConsoleOut(tOutput("$rotation_search_unknown", map), player->Owner());
                sn_ConsoleOut(tOutput("$rotation_search_other"), player->Owner());

                int len = map.Len()-1;
                int printMax = 1 + 3 * len * len * len;

                if (printMax <= 0 )
                {
                    tOutput msg;
                    msg.SetTemplateParameter(1, foundMaps.Len());
                    msg.SetTemplateParameter(2, map);
                    msg << "$rotation_search_found_toomanyfiles";
                    sn_ConsoleOut(msg, player->Owner());
                }
            }
            else
            {

            }
        }
        else if (type == "cfg")
        {
            tString map = params.ExtractNonBlankSubString(pos);
        }
        else
        {
            tOutput msg;
            msg.SetTemplateParameter(1, "Type of rotation to look into not set");
            msg << "$rotation_search_failed";
            sn_ConsoleOut(msg, player->Owner());
        }
    }
    else
    {
        tOutput msg;
        msg << "$rotation_search_usage";
        sn_ConsoleOut(msg, player->Owner());
    }
}

// minimal access level to add items to the map list
static tAccessLevel se_mapqueueingAccessLevel = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_mapqueueingAccessLevelConf( "ACCESS_LEVEL_QUEUE_MAPS", se_mapqueueingAccessLevel );

// minimal access level to add items to the config list
static tAccessLevel se_configqueueingAccessLevel = tAccessLevel_Program;
static tSettingItem< tAccessLevel > se_configqueueingAccessLevelConf( "ACCESS_LEVEL_QUEUE_CONFIGS", se_configqueueingAccessLevel );

void sg_AddqueueingItems(ePlayerNetID *p, std::istream &s, tString command)
{
    tString params;
    int pos = 0;
    params.ReadLine(s);

    if (command == "/mq")
    {
        tString argument = params.ExtractNonBlankSubString(pos);
        if ((argument == "list") || (argument == "ls"))
        {
            tColoredString Output;
            Output << "0xaaffffList of maps in the que by order:\n";
            if (sg_MapQueueing->Size() > 0)
            {
                for (int i=0; i < sg_MapQueueing->Size(); i++)
                {
                    if (i == 0)
                    {
                        tString name = sg_MapQueueing->Get(i);
                        Output << "0xffaa00" << name;
                    }
                    else
                    {
                        tString name = sg_MapQueueing->Get(i);
                        Output << "0x00aaff, " << "0xffaa00" << name;
                    }
                }
            }
            else
            {
                Output << "0xddff00There are no items currently in the map queueing system.";
                Output << "\n";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
        else if (argument == "add")
        {
            if (p->GetAccessLevel() <= se_mapqueueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool mapFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < mapRotation->Size(); i++)
                    {
                        gRotationItem *mapRotItem = mapRotation->Get(i);
                        if (mapRotItem)
                        {
                            tString filteredName, filteredSearch;
                            filteredName = ePlayerNetID::FilterName(mapRotItem->Name());
                            filteredSearch = ePlayerNetID::FilterName(item);
                            if (filteredName.Contains(filteredSearch))
                            {
                                mapFound = true;
                                searchFindings.Insert(mapRotItem->Name());
                            }
                        }
                    }
                    if (!mapFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$map_queueing_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            if (!gQueuePlayers::CanQueue(p))
                                return;

                            bool found = false;
                            tString mapName = searchFindings[0];
                            for (int j=0; j < sg_MapQueueing->Size(); j++)
                            {
                                tString item_name = sg_MapQueueing->Get(j);
                                if (item_name == mapName)
                                {
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, stripMapNameOnly(mapName));
                                    Output.SetTemplateParameter(2, j+1);
                                    Output << "$map_queueing_stored_file_found";
                                    sn_ConsoleOut(Output, p->Owner());
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                sg_MapQueueing->Insert(mapName);
                                tOutput Output;
                                Output.SetTemplateParameter(1, stripMapNameOnly(mapName));
                                Output.SetTemplateParameter(2, p->GetName());
                                Output << "$map_queueing_file_stored";
                                sn_ConsoleOut(Output);

                                sg_LogQueue(p, command, argument, mapName);
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output.SetTemplateParameter(2, searchFindings.Len());
                            Output << "$map_queueing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }

                    searchFindings.Clear();
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_mapqueueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$map_queueing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
        else if (argument == "remove")
        {
            if (p->GetAccessLevel() <= se_mapqueueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool mapFound = false;
                    tArray<tString> searchFindings;
                    for (int i = 0; i < sg_MapQueueing->Size(); i++)
                    {
                        tString mapRotItem = sg_MapQueueing->Get(i);
                        if (mapRotItem)
                        {
                            tString filteredName, filteredSearch;
                            filteredName = ePlayerNetID::FilterName(mapRotItem);
                            filteredSearch = ePlayerNetID::FilterName(item);
                            if (filteredName.Contains(filteredSearch))
                            {
                                mapFound = true;
                                searchFindings.Insert(mapRotItem);
                            }
                        }
                    }
                    if (!mapFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$map_queueing_que_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            tString mapName = searchFindings[0];
                            for (int j=0; j < sg_MapQueueing->Size(); j++)
                            {
                                tString item_name =  sg_MapQueueing->Get(j);
                                if (item_name == mapName)
                                {
                                    sg_MapQueueing->Remove(j);
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, stripMapNameOnly(mapName));
                                    Output.SetTemplateParameter(2, p->GetName());
                                    Output << "$map_queueing_file_removed";
                                    sn_ConsoleOut(Output);

                                    sg_LogQueue(p, command, argument, mapName);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output.SetTemplateParameter(2, searchFindings.Len());
                            Output << "$map_queueing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }

                    searchFindings.Clear();
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_mapqueueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$map_queueing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
    }
    else if (command == "/cq")
    {
        tString argument = params.ExtractNonBlankSubString(pos);
        if ((argument == "list") || (argument == "ls"))
        {
            tColoredString Output;
            Output << "0xaaffffList of configs in the que by order:\n";
            if (sg_ConfigQueueing->Size() > 0)
            {
                for (int i=0; i < sg_ConfigQueueing->Size(); i++)
                {
                    if (i == 0)
                    {
                        tString name = sg_ConfigQueueing->Get(i);
                        Output << "0xffaa00" << name;
                    }
                    else
                    {
                        tString name = sg_ConfigQueueing->Get(i);
                        Output << "0x00aaff, " << "0xffaa00" << name;
                    }
                }
            }
            else
            {
                Output << "0xddff00There are no items in the config queueing system.";
            }
            Output << "\n";
            sn_ConsoleOut(Output, p->Owner());
        }
        else if (argument == "add")
        {
            if (p->GetAccessLevel() <= se_configqueueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool configFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < configRotation->Size(); i++)
                    {
                        gRotationItem *configRotItem = configRotation->Get(i);
                        if (configRotItem)
                        {
                            tString filteredName, filteredSearch;
                            filteredName = ePlayerNetID::FilterName(configRotItem->Name());
                            filteredSearch = ePlayerNetID::FilterName(item);
                            if (filteredName.Contains(filteredSearch))
                            {
                                configFound = true;
                                searchFindings.Insert(configRotItem->Name());
                            }
                        }
                    }
                    if (!configFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$config_queueing_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            if (!gQueuePlayers::CanQueue(p))
                                return;

                            bool found = false;
                            tString configName = searchFindings[0];
                            for (int j=0; j < sg_ConfigQueueing->Size(); j++)
                            {
                                tString item_name =  sg_ConfigQueueing->Get(j);
                                if (item_name == configName)
                                {
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, configName);
                                    Output.SetTemplateParameter(2, j+1);
                                    Output << "$config_queueing_stored_file_found";
                                    sn_ConsoleOut(Output, p->Owner());
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                sg_ConfigQueueing->Insert(configName);
                                tOutput Output;
                                Output.SetTemplateParameter(1, configName);
                                Output.SetTemplateParameter(2, p->GetName());
                                Output << "config_queueing_file_stored";
                                sn_ConsoleOut(Output, p->Owner());

                                sg_LogQueue(p, command, argument, configName);
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output.SetTemplateParameter(2, searchFindings.Len());
                            Output << "$config_queueing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }

                    searchFindings.Clear();
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_configqueueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$config_queueing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
        else if (argument == "remove")
        {
            if (p->GetAccessLevel() <= se_configqueueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool configFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < sg_ConfigQueueing->Size(); i++)
                    {
                        tString configRotItem = sg_ConfigQueueing->Get(i);
                        if (configRotItem)
                        {
                            tString filteredName, filteredSearch;
                            filteredName = ePlayerNetID::FilterName(configRotItem);
                            filteredSearch = ePlayerNetID::FilterName(item);
                            if (filteredName.Contains(filteredSearch))
                            {
                                configFound = true;
                                searchFindings.Insert(configRotItem);
                            }
                        }
                    }
                    if (!configFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$config_queueing_que_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            tString configName = searchFindings[0];
                            for (int j=0; j < sg_ConfigQueueing->Size(); j++)
                            {
                                tString item_name =  sg_ConfigQueueing->Get(j);
                                if (item_name == configName)
                                {
                                    sg_ConfigQueueing->Remove(j);
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, configName);
                                    Output.SetTemplateParameter(2, p->GetName());
                                    Output << "$config_queueing_file_removed";
                                    sn_ConsoleOut(Output);

                                    sg_LogQueue(p, command, argument, configName);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output.SetTemplateParameter(2, searchFindings.Len());
                            Output << "$config_queueing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }

                    searchFindings.Clear();
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_configqueueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$config_queueing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
    }
    else if (command == "/ms")
    {
        tString argument = params.ExtractNonBlankSubString(pos);
        if ((argument == "list") || (argument == "ls"))
        {
            tColoredString Output;
            Output << "0xaaffffList of maps in the que by order:\n";
            if (sg_MapQueueing->Size() > 0)
            {
                for (int i=0; i < sg_MapQueueing->Size(); i++)
                {
                    if (i == 0)
                    {
                        tString name = sg_MapQueueing->Get(i);
                        Output << "0xffaa00" << name;
                    }
                    else
                    {
                        tString name = sg_MapQueueing->Get(i);
                        Output << "0x00aaff, " << "0xffaa00" << name;
                    }
                }
            }
            else
            {
                Output << "0xddff00There are no items currently in the map queueing system.";
                Output << "\n";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
        else if (argument == "add")
        {
            if (p->GetAccessLevel() <= se_mapqueueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool mapFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < mapStorage->Size(); i++)
                    {
                        gRotationItem *mapRotItem = mapStorage->Get(i);
                        if (mapRotItem)
                        {
                            tString filteredName, filteredSearch;
                            filteredName = ePlayerNetID::FilterName(mapRotItem->Name());
                            filteredSearch = ePlayerNetID::FilterName(item);
                            if (filteredName.Contains(filteredSearch))
                            {
                                mapFound = true;
                                searchFindings.Insert(mapRotItem->Name());
                            }
                        }
                    }
                    if (!mapFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$map_queueing_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            if (!gQueuePlayers::CanQueue(p))
                                return;

                            bool found = false;
                            tString mapName = searchFindings[0];
                            for (int j=0; j < sg_MapQueueing->Size(); j++)
                            {
                                tString item_name = sg_MapQueueing->Get(j);
                                if (item_name == mapName)
                                {
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, stripMapNameOnly(mapName));
                                    Output.SetTemplateParameter(2, j+1);
                                    Output << "$map_queueing_stored_file_found";
                                    sn_ConsoleOut(Output, p->Owner());
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                sg_MapQueueing->Insert(mapName);
                                tOutput Output;
                                Output.SetTemplateParameter(1, stripMapNameOnly(mapName));
                                Output.SetTemplateParameter(2, p->GetName());
                                Output << "$map_queueing_file_stored";
                                sn_ConsoleOut(Output);

                                sg_LogQueue(p, command, argument, mapName);
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output.SetTemplateParameter(2, searchFindings.Len());
                            Output << "$map_queueing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }

                    searchFindings.Clear();
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_mapqueueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$map_queueing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
        else if (argument == "remove")
        {
            if (p->GetAccessLevel() <= se_mapqueueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool mapFound = false;
                    tArray<tString> searchFindings;
                    for (int i = 0; i < sg_MapQueueing->Size(); i++)
                    {
                        tString mapQueuedItem = sg_MapQueueing->Get(1);
                        if (mapQueuedItem)
                        {
                            tString filteredName, filteredSearch;
                            filteredName = ePlayerNetID::FilterName(mapQueuedItem);
                            filteredSearch = ePlayerNetID::FilterName(item);
                            if (filteredName.Contains(filteredSearch))
                            {
                                mapFound = true;
                                searchFindings.Insert(mapQueuedItem);
                            }
                        }
                    }
                    if (!mapFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$map_queueing_que_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            tString mapName = searchFindings[0];
                            for (int j=0; j < sg_MapQueueing->Size(); j++)
                            {
                                tString item_name =  sg_MapQueueing->Get(j);
                                if (item_name == mapName)
                                {
                                    sg_MapQueueing->Remove(j);
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, mapName);
                                    Output.SetTemplateParameter(2, p->GetName());
                                    Output << "$map_queueing_file_removed";
                                    sn_ConsoleOut(Output);

                                    sg_LogQueue(p, command, argument, mapName);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output.SetTemplateParameter(2, searchFindings.Len());
                            Output << "$map_queueing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }

                    searchFindings.Clear();
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_mapqueueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$map_queueing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
    }
    else if (command == "/cs")
    {
        tString argument = params.ExtractNonBlankSubString(pos);
        if ((argument == "list") || (argument == "ls"))
        {
            tColoredString Output;
            Output << "0xaaffffList of configs in the que by order:\n";
            if (sg_ConfigQueueing->Size() > 0)
            {
                for (int i=0; i < sg_ConfigQueueing->Size(); i++)
                {
                    if (i == 0)
                    {
                        tString name = sg_ConfigQueueing->Get(i);
                        Output << "0xffaa00" << name;
                    }
                    else
                    {
                        tString name = sg_ConfigQueueing->Get(i);
                        Output << "0x00aaff, " << "0xffaa00" << name;
                    }
                }
            }
            else
            {
                Output << "0xddff00There are no items in the config queueing system.";
            }
            Output << "\n";
            sn_ConsoleOut(Output, p->Owner());
        }
        else if (argument == "add")
        {
            if (p->GetAccessLevel() <= se_configqueueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool configFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < configStorage->Size(); i++)
                    {
                        gRotationItem *configRotItem = configStorage->Get(i);
                        if (configRotItem)
                        {
                            tString filteredName, filteredSearch;
                            filteredName = ePlayerNetID::FilterName(configRotItem->Name());
                            filteredSearch = ePlayerNetID::FilterName(item);
                            if (filteredName.Contains(filteredSearch))
                            {
                                configFound = true;
                                searchFindings.Insert(configRotItem->Name());
                            }
                        }
                    }
                    if (!configFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$config_queueing_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            if (!gQueuePlayers::CanQueue(p))
                                return;

                            bool found = false;
                            tString configName = searchFindings[0];
                            for (int j=0; j < sg_ConfigQueueing->Size(); j++)
                            {
                                tString item_name =  sg_ConfigQueueing->Get(j);
                                if (item_name == configName)
                                {
                                    tOutput Output;
                                    Output.SetTemplateParameter(1, configName);
                                    Output.SetTemplateParameter(2, j+1);
                                    Output << "$config_queueing_stored_file_found";
                                    sn_ConsoleOut(Output, p->Owner());
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                sg_ConfigQueueing->Insert(configName);
                                tOutput Output;
                                Output.SetTemplateParameter(1, configName);
                                Output.SetTemplateParameter(2, p->GetName());
                                Output << "config_queueing_file_stored";
                                sn_ConsoleOut(Output);

                                sg_LogQueue(p, command, argument, configName);
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output.SetTemplateParameter(2, searchFindings.Len());
                            Output << "$config_queueing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }

                    searchFindings.Clear();
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_configqueueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$config_queueing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
        else if (argument == "remove")
        {
            if (p->GetAccessLevel() <= se_configqueueingAccessLevel)
            {
                tString items = params.ExtractNonBlankSubString(pos);
                if (items != "")
                {
                    int pos = 0;
                    tString item = items;
                    bool configFound = false;
                    tArray<tString> searchFindings;
                    for (int i=0; i < sg_ConfigQueueing->Size(); i++)
                    {
                        tString configRotItem = sg_ConfigQueueing->Get(i);
                        if (configRotItem)
                        {
                            tString filteredName, filteredSearch;
                            filteredName = ePlayerNetID::FilterName(configRotItem);
                            filteredSearch = ePlayerNetID::FilterName(item);
                            if (filteredName.Contains(filteredSearch))
                            {
                                configFound = true;
                                searchFindings.Insert(configRotItem);
                            }
                        }
                    }
                    if (!configFound)
                    {
                        tOutput Output;
                        Output.SetTemplateParameter(1, item);
                        Output << "$config_queueing_que_file_not_found";
                        sn_ConsoleOut(Output, p->Owner());
                    }
                    else
                    {
                        if (searchFindings.Len() == 1)
                        {
                            tString configName = searchFindings[0];
                            for (int j=0; j < sg_ConfigQueueing->Size(); j++)
                            {
                                tString item_name =  sg_ConfigQueueing->Get(j);
                                if (item_name == configName)
                                {
                                    sg_ConfigQueueing->Remove(j);

                                    tOutput Output;
                                    Output.SetTemplateParameter(1, configName);
                                    Output.SetTemplateParameter(2, p->GetName());
                                    Output << "$config_queueing_file_removed";
                                    sn_ConsoleOut(Output);

                                    sg_LogQueue(p, command, argument, configName);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            tOutput Output;
                            Output.SetTemplateParameter(1, item);
                            Output.SetTemplateParameter(2, searchFindings.Len());
                            Output << "$config_queueing_found_toomanyfiles";
                            sn_ConsoleOut(Output, p->Owner());
                        }
                    }

                    searchFindings.Clear();
                }
            }
            else
            {
                tOutput Output;
                Output.SetTemplateParameter(1, tCurrentAccessLevel::GetName(se_configqueueingAccessLevel));
                Output.SetTemplateParameter(2, argument);
                Output << "$config_queueing_required_level";
                sn_ConsoleOut(Output, p->Owner());
            }
        }
    }
}

void Orderedrotate()
{
    if ( mapRotation->Size() > 0 )
    {
        gRotationItem *mapRotItem = mapRotation->Current();
        if (mapRotItem)
        {
            LoadMap(mapRotItem->Name());
        }
        mapRotation->OrderedRotate();
    }

    if ( configRotation->Size() > 0 )
    {
        // transfer
        tCurrentAccessLevel level( tAccessLevel_Owner, true );

        if (sg_configRotationType == 0)
        {
            gRotationItem *configRotItem = configRotation->Current();
            if (configRotItem)
                st_Include( configRotItem->Name() );

            configRotation->OrderedRotate();
        }
        else if (sg_configRotationType == 1)
        {
            gRotationItem *configRotItem = configRotation->Current();
            if (configRotItem)
            {
                tString rclcl = tResourceManager::locateResource(NULL, configRotItem->Name());
                if ( rclcl ) {
                    std::ifstream rc(rclcl);
                    tConfItemBase::LoadAll(rc, false );
                    return;
                }

                con << tOutput( "$config_rinclude_not_found", configRotItem->Name() );
            }
            configRotation->OrderedRotate();
        }
    }
}

void Randomrotate()
{
    if ( mapRotation->Size() > 0 )
    {
        gRotationItem *mapRotItem = mapRotation->Current();
        if (mapRotItem)
        {
            LoadMap(mapRotItem->Name());
        }
        mapRotation->RandomRotate();
    }

    if ( configRotation->Size() > 0 )
    {
        // transfer
        tCurrentAccessLevel level( tAccessLevel_Owner, true );

        if (sg_configRotationType == 0)
        {
            gRotationItem *configRotItem = configRotation->Current();
            if (configRotItem)
                st_Include( configRotItem->Name() );

            configRotation->RandomRotate();
        }
        else if (sg_configRotationType == 1)
        {
            gRotationItem *configRotItem = configRotation->Current();
            if (configRotItem)
            {
                tString rclcl = tResourceManager::locateResource(NULL, configRotItem->Name());
                if ( rclcl ) {
                    std::ifstream rc(rclcl);
                    tConfItemBase::LoadAll(rc, false );
                    return;
                }

                con << tOutput( "$config_rinclude_not_found", configRotItem->Name() );
            }
            configRotation->RandomRotate();
        }
    }
}

void QueRotate()
{
    if (sg_MapQueueing->Size() > 0)
    {
        LoadMap(sg_MapQueueing->Current());
        sg_MapQueueing->Remove(sg_MapQueueing->CurrentID());
        if (sg_MapQueueing->Size() == 0)
        {
            tOutput Output;
            Output << "$map_queueing_is_empty";
            sn_ConsoleOut(Output);
        }
    }
    if (sg_ConfigQueueing->Size() > 0)
    {
        tCurrentAccessLevel level(tAccessLevel_Owner, true );
        if (sg_configRotationType == 0)
        {
            st_Include(sg_ConfigQueueing->Current());
            sg_ConfigQueueing->Remove(sg_ConfigQueueing->CurrentID());
            if (sg_ConfigQueueing->Size() == 0)
            {
                tOutput Output;
                Output << "$config_queueing_is_empty";
                sn_ConsoleOut(Output);
            }
        }
        else if (sg_configRotationType == 1)
        {
            tString rclcl = tResourceManager::locateResource(NULL, sg_ConfigQueueing->Current());
            sg_ConfigQueueing->Remove(sg_ConfigQueueing->CurrentID());
            if ( rclcl ) {
                std::ifstream rc(rclcl);
                tConfItemBase::LoadAll(rc, false );
                return;
            }

            con << tOutput( "$config_rinclude_not_found", sg_ConfigQueueing->Current() );
        }
    }
}

void gRotation::HandleNewRound(int rounds)
{
    if ((sg_MapQueueing->Size() == 0) && (sg_ConfigQueueing->Size() == 0))
    {
        //  resume normal rotation once queue is no longer active
        if (!queueActive_)
        {
            // rotate, if rotate is once per round
            if ( rotationtype == gROTATION_ORDERED_ROUND )
                Orderedrotate();
            else if (rotationtype == gROTATION_RANDOM_ROUND)
                Randomrotate();
            // rotate depending on how many times map remains the same per round/match
            else if (rotationtype == gROTATION_COUNTER)
            {
                //  add rotation counter
                gRotation::AddCounter();

                if (gRotation::Counter() >= sg_rotationMax)
                {
                    //  rotate orderly
                    Orderedrotate();

                    //  reset rotation counter
                    gRotation::ResetCounter();
                }
            }
            else if (rotationtype == gROTATION_ROUND)
            {
                //  load in the map for that round
                gRotationRoundSelection *mapRoundSelection = mapRoundRotation->Get(rounds);
                if (mapRoundSelection)
                {
                    gRotationItem *mapRotItem = mapRoundSelection->Current();
                    if (mapRotItem)
                    {
                        LoadMap(mapRotItem->Name());
                    }

                    //  finish off by rotating list
                    mapRoundSelection->Rotate();
                }

                //  load in the config for that round
                gRotationRoundSelection *configRoundSelection = configRoundRotation->Get(rounds);
                if (configRoundSelection)
                {
                    if (sg_configRotationType == 0)
                    {
                        gRotationItem *configRotItem = configRoundSelection->Current();
                        if (configRotItem)
                            st_Include( configRotItem->Name() );
                    }
                    else if (sg_configRotationType == 1)
                    {
                        gRotationItem *configRotItem = configRoundSelection->Current();
                        if (configRotItem)
                        {
                            tString rclcl = tResourceManager::locateResource(NULL, configRotItem->Name());
                            if ( rclcl ) {
                                std::ifstream rc(rclcl);
                                tConfItemBase::LoadAll(rc, false );
                                return;
                            }

                            con << tOutput( "$config_rinclude_not_found", configRotItem->Name());
                        }
                    }

                    //  finish off by rotating list
                    configRoundSelection->Rotate();
                }
            }
        }
        else
        {
            //!  reload where rotation last leftoff due to queue

            if (mapRotation->Size() > 0)
            {
                if ((mapRotation->ID() > 0) && (mapRotation->ID() < mapRotation->Size()))
                {
                    gRotationItem *mapRotItem =mapRotation->Get(mapRotation->ID() - 1);
                    if (mapRotItem)
                    {
                        LoadMap(mapRotItem->Name());
                    }
                }
            }

            if (configRotation->Size() > 0)
            {
                tCurrentAccessLevel level( tAccessLevel_Owner, true );

                int newID = configRotation->ID();
                if ((configRotation->ID() > 0) && (configRotation->ID() < configRotation->Size()))
                    newID = configRotation->ID() - 1;

                if (sg_configRotationType == 0)
                {
                    gRotationItem *configRotItem = configRotation->Get(newID);
                    if (configRotItem)
                        st_Include( configRotItem->Name() );
                }
                else if (sg_configRotationType == 1)
                {
                    gRotationItem *configRotItem = configRotation->Get(newID);
                    if (configRotItem)
                    {
                        tString rclcl = tResourceManager::locateResource(NULL, configRotItem->Name());
                        if ( rclcl ) {
                            std::ifstream rc(rclcl);
                            tConfItemBase::LoadAll(rc, false );
                            return;
                        }

                        con << tOutput( "$config_rinclude_not_found", configRotItem->Name());
                    }
                }
            }

            queueActive_ = false;
        }
    }
    else
    {
        queueActive_ = true;
        QueRotate();
    }

#ifdef HAVE_LIBRUBY
    gRoundEventRuby::DoRoundEvents();
#endif
}

void gRotation::HandleNewMatch()
{
    if ((sg_MapQueueing->Size() == 0) || (sg_ConfigQueueing->Size() == 0))
    {
        //check for map rotation, new match...
        if ( rotationtype == gROTATION_ORDERED_MATCH )
            Orderedrotate();
        else if ( rotationtype == gROTATION_RANDOM_MATCH)
            Randomrotate();
    }

#ifdef HAVE_LIBRUBY
    gMatchEventRuby::DoMatchEvents();
#endif
}

#ifdef HAVE_LIBRUBY

static tCallbackRuby *roundEventRuby_anchor;
gRoundEventRuby::gRoundEventRuby()
        :tCallbackRuby(roundEventRuby_anchor)
{
}

void gRoundEventRuby::DoRoundEvents()
{
    Exec(roundEventRuby_anchor);
}

static tCallbackRuby *matchEventRuby_anchor;

gMatchEventRuby::gMatchEventRuby()
        :tCallbackRuby(matchEventRuby_anchor)
{
}

void gMatchEventRuby::DoMatchEvents()
{
    Exec(matchEventRuby_anchor);
}
#endif
