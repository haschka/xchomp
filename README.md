### xchomp 

A Pac Man clone that was distributed with old linux distributions. 

I ( Thomas Haschka ) extracted it from old RedHat 5.0 source RPMS

The original code seems to have been written Jerry J. Shekhel, c.f. README

The original 1.0 version had the option to pass a frame delay option to 
the compilier in order to slow down the code for fast processors.
I removed that option and modified the code so that the main even loop
runs at a constant rate of 30 times per seconds. 

To compile this you will need X11-dev headers/libraries on your system and
a c compilier. I compilied this on gentoo with gcc like:

```
gcc -O2 -march=native -std=gnu89 main.c demo.c contact.c maze.c props.c resources.c drivers.c status.c -I. -I./bitmaps -I/usr/include -I/usr/include/X11 -o xchomp -lX11
```

The program uses very old style C function declarations. Maybe one day one
will port it to a newer C style. 
