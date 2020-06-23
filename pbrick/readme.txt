PBrickLib, P{alm|rogrammable}Brick Library
------------------------------------------

Copyright (C) 2002 Till Harbaum <t.harbaum@web.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

WHAT IS THE PBRICK LIBRARY?
---------------------------

The PBrick Library is a shared library that provides access to the
Lego Mindstorms RCX controller brick (pbrick). It uses the palms
IrDA interface to provide an interface to send and receive data to
and from the RCX allowing for applications from simple remote controls
(like e.g. PBRemote.prc in this package) to complex applications
using the Palm as a robots main processor and using the RCX as an 
intelligent IO controller that drives and controls the motors and
sensors of the robot.

The library consists of two parts, the library itself and a
programming interface description (c header file). The end user
only needs the library and of course at least one application using 
the library. The advantage of putting the whole interface code into
a separate library is:

- The application developer doesn't need to care about the low
  level RCX communication at all
- Updates of the interface routines can be performed by replacing the
  library only. There's no need to re-compile or re-install the
  applications themselves.
- The interface can be adopted to new palm hardware by providing
  a new library. The user land applications will very likely work
  on future palms, while the interface code is very palm hardware
  dependant and may require updates to work on future palm models.

INSTALLATION / THE DEMO APPLICATIONS
------------------------------------

The PBrick Library comes with two example programs:

  The PBrick Remote (PBRemote.prc) and
  the PBrick Demo (PBDemo.prc) 

In order to use one or both of these programs install them on your
palm the usual way together with the PBrick Library (PBrickLib.prc).

PBRICK REMOTE
-------------

The PBrick Remote program demonstrates simple one-way communication
from the palm to the rcx. It provides all the functions and keys of
the original mindstorms remote control. This software does not analyse
any reply messages sent by the RCX, it will e.g. therefore not
recognize the fact, that the RCX is out of range or switched off.  

The PBrick Remote program does not offer additional functionality over
various other existing palm/RCX related software like e.g. Omniremote
and RMover besides the fact, that PBrick Remote ist open source and,
hence, can be used as a basis of your own palm/RCX programming
experiments.

PBRICK DEMO
-----------

The PBrick Demo makes use of the PBrick Libraries capability to read
data from the RCX back into the palm. This demo displays various
library internal settings regarding communication speed and
accuracy. It uses the bi-directional communication to verify the
execution of several operations like starting sounds. Other functions
of this demo rely on the second channel to read RCX internal settings
back into the palm like e.g. the firmware and ROM versions or der
battery state. These functions are based on easy-to-use high level
interface routines provided by the Library as well as powerful low
level routines that allow for unlimited communication between palm and
RCX.

Have fun,
  Till


