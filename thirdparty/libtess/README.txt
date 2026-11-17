This directory contains the polygon tessellator of GLU (src/libtess of Mesa GLU 9.0.2,
https://gitlab.freedesktop.org/mesa/glu), under the SGI Free Software License B 2.0 (see the
header of each file; the license is equivalent to the X11/MIT license).

It is built only with KICAD_HEADLESS_ONLY, where it replaces libGLU so that kicad-cli and its
libraries do not depend on OpenGL.

Changes to the original files:
- the sources include <kitess/glu.h> instead of <GL/glu.h>;
- include/kitess/glu.h is the tessellator part of the original glu.h, with the GL types taken
  from the glad header;
- src/error.c is gluErrorString() for the tessellator errors (from src/libutil/error.c and
  src/libutil/glue.c).
