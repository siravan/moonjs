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

  Filename:	queue.c
  Purpose:	This file is a modification of yaAGC SocketAPI.c and NullAPI.c. 
        It simplifies ChannelOutput and ChannelInput functions by removing 
        socket functions and implementing two simple FIFI queues for 
        communication with the advance() function and ultimately 
        hand-written javascript. 

  Compiler:	GNU gcc.
  Contact:	Shahriar Iravanian <siravan@svtsim.com>
  Reference:	http://svtsim.com/moonjs/agc.html
*/


//#include <errno.h>
#include <stdlib.h>
//#include <stdio.h>
#include "yaAGC.h"
#include "agc_engine.h"
#include "queue.h"

static int ChannelMasks[256];   // In place of Client->ChannelMask in SocketAPI

queue qout;     // queue containing packets from AGC to DSKY
queue qin;      // queue containing packets from DSKY to AGC

/*
    Initializes the queue data structure.
*/
void qinit(queue *q)    
{
  q->head = q->tail = 0;
}

/*
    Adds a packet to the back of the queue.
    This function assumes that packets are 4 characters long.
    Returns 4 (size of packet) on success and 0 on failure (if the queue is full).
*/
int qsend(queue *q, unsigned char *packet)
{
  int i = (q->tail + 4) & 0x0fff;
  if(i == q->head){ // full
    return 0;
  }
  q->data[q->tail] = packet[0];
  q->data[q->tail+1] = packet[1];
  q->data[q->tail+2] = packet[2];
  q->data[q->tail+3] = packet[3];
  q->tail = i;
  return 4;
}


/*
    Remove and return a packet from the head of the queue.
    This function assumes that packets are 4 characters long.
    Returns 4 (size of packet) on success and 0 if the queue is empty.
*/
int qrecv(queue *q, unsigned char *packet)
{
  if(q->head == q->tail){ // empty
    return 0;
  }
  packet[0] = q->data[q->head];
  packet[1] = q->data[q->head+1];
  packet[2] = q->data[q->head+2];
  packet[3] = q->data[q->head+3];
  q->head = (q->head + 4) & 0x0fff;
  return 4;
}


int initialized = 0;

/*
    Initializes the queues and ChannelMasks
*/
void Initialize()
{
  int i;
  if(initialized) return;
  initialized = 1;
  
  qinit(&qin);
  qinit(&qout);

  for(i=0; i<256; i++)
    ChannelMasks[i] = 077777;
}

//-----------------------------------------------------------------------------
// Function for transmitting "output channel" data to the client
// This is a simplified version of the ChannelOutput function in SocketAPI.c

void
ChannelOutput (agc_t * State, int Channel, int Value)
{
  unsigned char Packet[4];
  
  Initialize();
  // Some output channels have purposes within the CPU, so we have to
  // account for those separately.
  if (Channel == 7)
    {
      State->InputChannel[7] = State->OutputChannel7 = (Value & 0160);
      return;
    }
  // Stick data into the RHCCTR registers, if bits 8,9 of channel 013 are set.
  if (Channel == 013 && 0600 == (0600 & Value) && !CmOrLm)
    {
      State->Erasable[0][042] = LastRhcPitch;
      State->Erasable[0][043] = LastRhcYaw;
      State->Erasable[0][044] = LastRhcRoll;
    }
  // Most output channels are simply transmitted to clients representing
  // hardware simulations.
  if (FormIoPacket (Channel, Value, Packet))
    return;
  qsend (&qout, Packet);
}


//----------------------------------------------------------------------
// A function for handling reception of "input channel" data from
// the client. The function returns 0 if there is no input, or non-zero if
// there is input.  If the input causes an interrupt (as, for example,
// a keystroke from the DSKY), then the function should set the
// appropriate request in State->InterruptRequests[].
//
// The function also handles unprogrammed counter increments caused
// by inputs to the CPU.  The function returns 1 if a counter-increment
// was performed, and returns zero otherwise.
//
// This is a simplified version of the ChannelInput function in SocketAPI.c

int
ChannelInput (agc_t *State)
{
    static int SocketInterlace = 0;
    int Channel, Value, uBit;
    unsigned char Packet[4];
   
    //We use SocketInterlace to slow down the number
    // of polls of the sockets.
    if (SocketInterlace > 0){
        SocketInterlace--;
        return 0;
    } 
    SocketInterlace = SocketInterlaceReload;

    Initialize();

    if(!qrecv(&qin, Packet)){   // input queue is empty 
        return 0;
    }
    
    // Process a received packet.
    
    if (!ParseIoPacket (Packet, &Channel, &Value, &uBit)) {    
        Value &= 077777;    // Convert to AGC format (upper 15 bits).
        if (uBit) {
            ChannelMasks[Channel] = Value;
        } else if (Channel & 0x80) {
            // In this case we're dealing with a counter increment.
	        // So increment the counter.
	        UnprogrammedIncrement (State, Channel, Value);
	        return 1;
        } else {
            Value &= ChannelMasks[Channel];
            Value |= ReadIO (State, Channel) & ~ChannelMasks[Channel];
	        WriteIO (State, Channel, Value);
	        // If this is a keystroke from the DSKY, generate an interrupt req.
	        if (Channel == 015){
                State->InterruptRequests[5] = 1;
	        }
	        // If this is on fictitious input channel 0173, then the data
	        // should be placed in the INLINK counter register, and an
	        // UPRUPT interrupt request should be set.
	        else if (Channel == 0173) {
                State->Erasable[0][RegINLINK] = (Value & 077777);
	            State->InterruptRequests[7] = 1;
	        }
	        // Fictitious registers for rotational hand controller (RHC).
	        // Note that the RHC angles are not immediately used, but
	        // merely squirreled away for later.  They won't actually
	        // go into the counter registers until the RHC counters are
	        // enabled and the data requested (bits 8,9 of channel 13).
	        else if (Channel == 0166) {
                LastRhcPitch = Value;
	            ChannelOutput (State, Channel, Value);	// echo
	        } else if (Channel == 0167) {
                LastRhcYaw = Value;
	            ChannelOutput (State, Channel, Value);	// echo
	        } else if (Channel == 0170) {
                LastRhcRoll = Value;
	            ChannelOutput (State, Channel, Value);	// echo
	        } else if (Channel == 031) {
                static int LastInDetent = 040000;
	            int InDetent;
	            ChannelOutput (State, Channel, Value);
	            // If the RHC stick has moved out of detent,
	            // generate a RUPT10 interrupt.
	            InDetent = (040000 & Value);
	            if (LastInDetent && !InDetent){
                    State->InterruptRequests[10] = 1;
                }
	            LastInDetent = InDetent;
            } 
        }       
    }
    return 0;
}

//----------------------------------------------------------------------
// A function for handling anything routinely needed (i.e., executed on
// a regular schedule) by the i/o channel model of ChannelInput and
// ChannelOutput.  There are no good reasons that I know of why this
// would be needed, other than by my reference model (see SocketAPI.c),
// so you might just want to let this empty.

void
ChannelRoutine (agc_t *State)
{
}

//----------------------------------------------------------------------
// This function is useful only for debugging the socket interface, and
// so can be left as-is.

void
ShiftToDeda (agc_t *State, int Data)
{
}


