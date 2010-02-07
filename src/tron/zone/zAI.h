/*

*************************************************************************

ArmageTron -- Just another Tron Lightcycle Game in 3D.
Copyright (C) 2005  by 
and the AA DevTeam (see the file AUTHORS(.txt) in the main source directory)

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

#ifndef ArmageTron_ZONE_AI_H
#define ArmageTron_ZONE_AI_H

#include "gAINavigator.h"
#include "gAIBase.h"

class gCycle;
class zZone;

//! evaluator for zone defense
class zZoneEvaluator: public gAINavigator::FollowEvaluator
{
public:
    zZoneEvaluator( gCycle const & cycle, zZone const & zone, eCoord & random, REAL maxStep, gCycle * lastBlocker );
    ~zZoneEvaluator();

private:
    //! constructor helper
    void Init( gCycle const & cycle, zZone const & zone, eCoord & random, REAL maxStep, gCycle * lastBlocker );
};

//! state for zone defense   
class zStateDefend: public gAIPlayer::State
{
public:
    zStateDefend( gAIPlayer & ai );
    ~zStateDefend();

    virtual REAL Think( REAL maxStep );
private:
    eCoord randomPos;
    tJUST_CONTROLLED_PTR< gCycle > lastBlocker_;
};


#endif
