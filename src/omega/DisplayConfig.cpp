/******************************************************************************
 * THE OMEGA LIB PROJECT
 *-----------------------------------------------------------------------------
 * Copyright 2010-2015		Electronic Visualization Laboratory, 
 *							University of Illinois at Chicago
 * Authors:										
 *  Alessandro Febretti		febret@gmail.com
 *-----------------------------------------------------------------------------
 * Copyright (c) 2010-2015, Electronic Visualization Laboratory,  
 * University of Illinois at Chicago
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this 
 * list of conditions and the following disclaimer. Redistributions in binary 
 * form must reproduce the above copyright notice, this list of conditions and 
 * the following disclaimer in the documentation and/or other materials provided 
 * with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR 
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-----------------------------------------------------------------------------
 * What's in this file
 *	Classes used to describe a display system configuration (network, screens, 
 *	system geometry etc.)
 ******************************************************************************/
#include "omega/DisplayConfig.h"
#include "omega/Engine.h"
#include "omega/CylindricalDisplayConfig.h"
#include "omega/PlanarDisplayConfig.h"

// For canvas change notifier
#include "omega/PythonInterpreter.h"

using namespace omega;
using namespace std;

//////////////////////////////////////////////////////////////////////////////
void DisplayConfig::LoadConfig(Setting& scfg, DisplayConfig& cfg)
{
    // Set a default tile resolution.
    cfg.tileResolution[0] = 640;
    cfg.tileResolution[1] = 480;

    String cfgType = Config::getStringValue("geometry", scfg, "ConfigPlanar");

    //cfg.numTiles = Config::getVector2iValue("numTiles", scfg);
    cfg.referenceTile = Config::getVector2iValue("referenceTile", scfg);
    cfg.referenceOffset = Config::getVector3fValue("referenceOffset", scfg);
    cfg.tileSize = Config::getVector2fValue("tileSize", scfg);
    cfg.bezelSize = Config::getVector2fValue("bezelSize", scfg);
    //ofmsg("Bezel size: %1%", %cfg.bezelSize);
    
    cfg.tileResolution = Config::getVector2iValue("tileResolution", scfg);
    cfg.windowOffset = Config::getVector2iValue("windowOffset", scfg);

    cfg.verbose = Config::getBoolValue("verbose", scfg, false);

    cfg.latency = Config::getIntValue("latency", scfg);
    
    String sm = Config::getStringValue("stereoMode", scfg, "default");
    StringUtils::toLowerCase(sm);
    if(sm == "default") cfg.stereoMode = DisplayTileConfig::Mono;
    else if(sm == "mono") cfg.stereoMode = DisplayTileConfig::Mono;
    // 'interleaved' defaults to row interleaved
    else if(sm == "interleaved") cfg.stereoMode = DisplayTileConfig::LineInterleaved;
    else if(sm == "rowinterleaved") cfg.stereoMode = DisplayTileConfig::LineInterleaved;
    else if(sm == "sidebyside") cfg.stereoMode = DisplayTileConfig::SideBySide;
    else if(sm == "columninterleaved") cfg.stereoMode = DisplayTileConfig::ColumnInterleaved;

    cfg.invertStereo = Config::getBoolValue("invertStereo", scfg);

    cfg.fullscreen = Config::getBoolValue("fullscreen", scfg);
    cfg.borderless = Config::getBoolValue("borderless", scfg, false);

    // deprecated
    cfg.panopticStereoEnabled = Config::getBoolValue("panopticStereoEnabled", scfg);

    cfg.disableConfigGenerator = Config::getBoolValue("disableConfigGenerator", scfg, false);
    
    //cfg.displayResolution = cfg.tileResolution.cwiseProduct(cfg.numTiles);
    //ofmsg("Total display resolution: %1%", %cfg.displayResolution);

    cfg.nodeLauncher = Config::getStringValue("nodeLauncher", scfg);
    cfg.nodeKiller = Config::getStringValue("nodeKiller", scfg);
    cfg.basePort = Config::getIntValue("basePort", scfg);

    cfg.launcherInterval = Config::getIntValue("launcherInterval", scfg, 500);

    const Setting& sTiles = scfg["tiles"];
    // Reset number of nodes and tiles. Will count them in the next loop.
    cfg.numNodes = 0;
    cfg.numTiles = 0;

    cfg.enableVSync= Config::getBoolValue("enableVSync", scfg, false);
    cfg.enableSwapSync = Config::getBoolValue("enableSwapSync", scfg, true);

    for(int i = 0; i < sTiles.getLength(); i++)
    {
        const Setting& sTileHost = sTiles[i];
        DisplayNodeConfig& ncfg = cfg.nodes[cfg.numNodes];
        ncfg.enabled = Config::getBoolValue("enabled", sTileHost, true);
        ncfg.hostname = sTileHost.getName();
        String alternHostname = Config::getStringValue("hostname", sTileHost);
        if(alternHostname != "") ncfg.hostname = alternHostname;
        ncfg.numTiles = 0;
        if(ncfg.hostname != "local")
        {
            ncfg.isRemote = true;
        }
        else
        {
            ncfg.isRemote = false;
        }
        ncfg.port = Config::getIntValue("port", sTileHost);

        for(int j = 0; j < sTileHost.getLength(); j++)
        {
            const Setting& sTile = sTileHost[j];
            if(sTile.getType() == Setting::TypeGroup)
            {
                // Create a new display tile and parse config.
                DisplayTileConfig* tc = new DisplayTileConfig(cfg);
                tc->node = &ncfg;
                cfg.tiles[sTile.getName()] = tc;
                tc->parseConfig(sTile, cfg);

                ncfg.tiles[ncfg.numTiles] = tc;
                tc->id = ncfg.numTiles;
                ncfg.numTiles++;
                cfg.numTiles++;
            }
        }
        cfg.numNodes++;
    }

    // Parse cylindrical display configurations.
    if(cfgType == "ConfigCylindrical")
    {
        cfg.configBuilder = new CylindricalDisplayConfig();
        cfg.configBuilder->buildConfig(cfg, scfg);
    }
    else if(cfgType == "ConfigPlanar")
    {
        cfg.configBuilder = new PlanarDisplayConfig();
        cfg.configBuilder->buildConfig(cfg, scfg);
    }

    // Initialization: Set the active rect for all tiles to be the tile 
    // size / pos.
    // Also set the initial canvas rect.
    int maxint = std::numeric_limits<int>::max();
    int minint = std::numeric_limits<int>::min();
    int cx = maxint;
    int cy = maxint;
    int cX = minint;
    int cY = minint;

    cfg.displayResolution = Vector2i::Zero();

    foreach(Tile t, cfg.tiles)
    {
        t->activeRect = Rect(t->position, t->position + t->pixelSize);
        if(t->enabled)
        {
            if(t->offset[0] < cx) cx = t->offset[0];
            if(t->offset[1] < cy) cy = t->offset[1];
            Vector2i endpoint = t->offset + t->pixelSize;
            if(endpoint[0] > cX) cX = endpoint[0];
            if(endpoint[1] > cY) cY = endpoint[1];

            // Update the full display resolution
            if(endpoint[0] > cfg.displayResolution[0]) cfg.displayResolution[0] = endpoint[0];
            if(endpoint[1] > cfg.displayResolution[1]) cfg.displayResolution[1] = endpoint[1];
        }
    }
    cfg._canvasRect = Rect(cx, cy, cX - cx, cY - cy);

    // If we have a canvasRect config entry, use it to set the initial canvas
    // rect.
    cfg._canvasRect.min = Config::getVector2iValue("canvasPosition", scfg, cfg._canvasRect.min);
    Vector2i csize = cfg._canvasRect.size();
    csize = Config::getVector2iValue("canvasSize", scfg, csize);
    cfg._canvasRect.max = cfg._canvasRect.min + csize;
    cfg.setCanvasRect(cfg._canvasRect);
}

//////////////////////////////////////////////////////////////////////////////
int DisplayConfig::setupMultiInstance(MultiInstanceConfig* mic)
{
    // If entries in the mult instance config tiles are -1, do not reconfigure
    // enabled tiles, just set the application instance id.
    if(mic->tilex != -1 && mic->tiley != -1 && mic->tileh != -1 && mic->tilew != -1)
    {
        // By default set all tiles to disabled.
        typedef Dictionary<String, Ref<DisplayTileConfig> > DisplayTileDictionary;
        foreach(DisplayTileDictionary::Item dtc, tiles) dtc->enabled = false;

        // Enable tiles in the active viewport
        for(int y = mic->tiley; y < mic->tiley + mic->tileh; y++)
        {
            for(int x = mic->tilex; x < mic->tilex + mic->tilew; x++)
            {
                DisplayTileConfig* dtc = tileGrid[x][y];
                if(dtc != NULL) dtc->enabled = true;
                else ofwarn("DisplayConfig::setupMultiInstance: could not find tile %1% %2%", %x %y);
            }
        }
        
        // Disable nodes that have no active tiles.
        for(int n = 0; n < numNodes; n++)
        {
            DisplayNodeConfig& nc = nodes[n];
            bool enabled = false;
            for(int i = 0; i < nc.numTiles; i++) enabled |= nc.tiles[i]->enabled;
            
            // NOTE: if a node has been disabled through the config, it will stay
            // disabled here, regardless of the tile state.
            nc.enabled &= enabled;
        }
    }

    int offs = mic->id;
    if(offs == -1)
    {
        // Compute an offset to the base port based on the port pool and tile viewport
        offs = (mic->tiley * tileGridSize[0] + mic->tilex) * mic->portPool / numTiles;
    }
    basePort += offs;
    mic->id = offs;

    ofmsg("Grid size %1% %2% pool %3% numTimes %4%", %tileGridSize[0] %tileGridSize[1] %mic->portPool %numTiles);
    //ofmsg("Multi-Instance mode: instance id = %1% tile viewport (%2% %3% - %4% %5%) port %6%", 
    //    %mic->id %mic->tilex %mic->tiley %(mic->tilex + mic->tilew) %(mic->tiley + mic->tileh) %basePort);

    return offs;
}
    
//////////////////////////////////////////////////////////////////////////////
bool DisplayConfig::isHostInTileSection(const String& hostname, int tilex, int tiley, int tilew, int tileh)
{
    // find host node.
    for(int i = 0; i < numNodes; i++)
    {
        if(nodes[i].hostname == hostname)
        {
            // If at least one tile is in section, return true.
            for(int j = 0; j < nodes[i].numTiles; j++)
            {
                DisplayTileConfig* dtc = nodes[i].tiles[j];
                if(dtc->isInGrid && 
                    dtc->gridX >= tilex && dtc->gridX < tilex + tilew &&
                    dtc->gridY >= tiley && dtc->gridY < tiley + tileh) return true;
            }
            return false;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
std::pair<bool, Vector3f> DisplayConfig::getPixelPosition(int x, int y)
{
    DisplayTileConfig* dtc = getTileFromPixel(x, y);
    if(dtc != NULL)
    {
        Vector3f res = dtc->getPixelPosition(x - dtc->offset[0], y - dtc->offset[1]);
        return std::pair<bool, Vector3f>(true, res);
    }
    return std::pair<bool, Vector3f>(false, Vector3f::Zero());
}

//////////////////////////////////////////////////////////////////////////////
DisplayTileConfig* DisplayConfig::getTileFromPixel(int x, int y)
{
    // Find the tile containing this pixel
    typedef KeyValue<String, Ref<DisplayTileConfig> > TileItem;
    foreach(TileItem t, this->tiles)
    {
        if(x >= t->offset[0] &&
            y >= t->offset[1] &&
            x < (t->offset[0] + t->pixelSize[0]) &&
            y < (t->offset[1] + t->pixelSize[1]))
        {
            return t.getValue();
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////////
void DisplayConfig::setTilesEnabled(int tilex, int tiley, int tilew, int tileh, bool enabled)
{
    foreach(Tile t, tiles)
    {
        if(t->gridX >= tilex &&
            t->gridX < tilex + tilew &&
            t->gridY >= tiley &&
            t->gridY < tiley + tileh)
        {
            t->enabled = enabled;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void DisplayConfig::setTilesEnabled(const String& tileNames)
{
    // First disable all tiles
    foreach(Tile t, tiles) t->enabled = false;

    // Then enable tiles in tileNames.
    Vector<String> vtileNames = StringUtils::split(tileNames, " ");
    foreach(String tileName, vtileNames)
    {
        if(tiles.find(tileName) != tiles.end()) tiles[tileName]->enabled = true;
    }
}


///////////////////////////////////////////////////////////////////////////////
void DisplayConfig::setCanvasRect(const Rect& cr)
{
    _canvasRect = cr;
    foreach(Tile t, tiles) t->updateActiveRect(_canvasRect);
    
    // Notify config builder of canvas change
    if(configBuilder != NULL) configBuilder->onCanvasChange(*this);

    // Notify listeners of canvas change.
    if(canvasListener != NULL) canvasListener->onCanvasChange();
    if(canvasChangedCommand.size() > 0)
    {
        PythonInterpreter* pi = SystemManager::instance()->getScriptInterpreter();
        pi->queueCommand(canvasChangedCommand);
    }
    
    // Notify default camera of canvas change
    Engine* e = Engine::instance();
    if(e != NULL && e->getDefaultCamera() != NULL)
    {
        e->getDefaultCamera()->setCanvasTransform(canvasPosition, canvasOrientation, canvasScale);
    }
}

//////////////////////////////////////////////////////////////////////////////
Vector3f DisplayConfig::defaultComputeEyePosition(
    const Vector3f headSpaceEyePosition, 
    const AffineTransform3& headTransform,
    const DrawContext& dc)
{
    return headTransform * headSpaceEyePosition;
}
