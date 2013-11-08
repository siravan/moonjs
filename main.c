/*
  Copyright 2013 Shahriar Iravanian <siravan@svtsim.com>

  This file is part of moonjs, a port of the Apollo Guidance Cmoputer (AGC)
  simulator (yaAGC) written by Ronald S. Burkey <info@sandroid.org> to 
  javascript using the Emscripten compiler 
  (https://github.com/kripken/emscripten/wiki).

  Both moonjs and yaAGC are free softwares; you can redistribute then and/or 
  modify them under the terms of the GNU General Public License as published 
  by the Free Software Foundation; either version 2 of the License, or (at 
  your option) any later version.

  Moonjs is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with yaAGC; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  

  Filename:	main.c
  Purpose:	The main entry point for moonjs software. 
        It contains the main() function which initializes
        the agc_engine. It also contains advance(), which
        is the interface of moonjs for hand-wirtten javascript.

  Compiler:	GNU gcc.
  Contact:	Shahriar Iravanian <siravan@svtsim.com>
  Reference:	http://svtsim.com/moonjs/agc.html
*/

#include <stdio.h>      // added for rfopen

#include "yaAGC.h"
#include "agc_engine.h"
#include "agc_symtab.h"
#include "queue.h"

agc_t State;
#define CORE_SIZE (044 * 02000)
#define TENMS  (int)(10000/10.7)  // 935 = the number of CPU instructions in 10 ms

int main (void)
{
    int i;

    agc_engine_init (&State, "Core.bin", NULL, 0);

    // runs the engine for 10 ms to finish initialization
    for(i=0; i<TENMS; i++){
        agc_engine (&State);    
    }
}

/*
    'advance', 'sendPort', 'scanPort' and 'readPort' are the main interface functions for communication with hand-written javascript:

        'advance' runs the agc engine for 10 ms
    
        'sendPort' makes a data packet from port and val and send it to the input channels

        'scanPort' scan the output queue for the first packet with a port number 
            matching the bit-mask (ports 0 to 31). Note that it discards other packets.

        'readPort' returns the next packet in the output queue
*/
int advance()
{
    int i;    

    for(i=0; i<TENMS; i++){ // runs the engine for 10 ms
        agc_engine (&State);    
    }    
    return 0;
}

int sendPort(unsigned int port, unsigned int val){
    unsigned char Packet[4];

    if (FormIoPacket (port, val, Packet)) 
        return 0;
    return qsend (&qin, Packet);    
}

int scanPort(unsigned int mask){
    unsigned char Packet[4];
    int chan, val, bit;    

    // loop while there is more packet in the output queue
    while(qrecv(&qout, Packet)){ 
        ParseIoPacket (Packet, &chan, &val, &bit);        
        if(mask & (1<<chan))
            return (chan<<16) + val;  // note: we use this construct for returning data to hand-written javascript
    }
    return 0;
}

int readPort(){
    unsigned char Packet[4];
    int chan, val, bit;    

    if(qrecv(&qout, Packet)){ 
        ParseIoPacket (Packet, &chan, &val, &bit);        
        return (chan<<16) + val;  // note: we use this construct for returning data to hand-written javascript
    }
    return 0;
}



/*
   This stub-function is here to keep agc_engine from slowing itself down by
   saving backtrace information, which is useful only for a debugger we're not
   building into the code anyway.
*/
void BacktraceAdd (agc_t *State, int Cause)
{
  // Keep this empty.
}


/*
    Simplified rfopen function instead of using the one in rfopen.c,
    since there is no /usr/local/bin or similar directories in the
    emscripten environment.
*/
FILE *rfopen (const char *Filename, const char *mode)
{  
    return fopen(Filename, mode);
}




