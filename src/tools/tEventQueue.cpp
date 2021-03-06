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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  
***************************************************************************

*/

#include "tEventQueue.h"

/* ************************
   the Event Queue:
   ************************ */


tEventQueue::~tEventQueue(){
    for(int i=Len()-1;i>=0;i--)
        delete Events(i);
}

void tEventQueue::Timestep(REAL time){
    tEvent *e;
    // process the bottom of the heap as long as there is work to do there:
    while(Len() && (e=Events(0))->Val() < time){

    #ifdef EVENT_DEB
        CheckHeap();
    #endif

        if (e->Check(time))
            Replace(e); // location of e may have changed
        else
        {
            Remove(e);
            delete e;
        }
    }

#ifdef EVENT_DEB
    CheckHeap();
#endif
}



/* ************************
   Events:
   ************************ */

tEvent::~tEvent(){}

//tEventQueue *tEvent::Queue(){return static_cast<tEventQueue*>(Heap());}
// in wich queue are we?
