nchanterm
---------

UTF-8 ANSI/vt102 text mode interface.

Features:
  UTF8 native
  Color, underline, bold
  Mouse and keyboard events
  Internally keeps track of terminal state and only does delta updates.
  Liberal license ISC which is equivalent to MIT but with a simplified language.
  Single .c file that can be dropped into a project to gain text mode
  capabilities.

The API provided is the smallest core needed to drive an application, higher
level abstractions can be provided on top.

Rationale
---------

Terminal emulators in use today mostly share the ANSI subset of control
commands, terminals this statement applies to include:

xterm, putty, linux console, vte (gnome-terminal etc), Eterm, screen and more.

Even though they support a wide range of different commands, manipulating the
character grid as well as initiating mouse-event can be achieved through a
common subset.

Documentation
-------------

Nchanterm can be installed systemwide can be used through pkg-config based
interface or to be dropped into the source tree of a project. See examples/
for some code examples using the API. For a list of supported key-codes look
for the array called /keycodes/ in the source, not all key-codes work on all
terminals (should perhaps remove the ones least likely to be portable).
