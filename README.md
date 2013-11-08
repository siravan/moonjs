Moonjs
======

Moonjs (http://svtsim.com/moonjs/agc.html) is an online Apollo Guidance Computer (AGC) simulator. It is a port of the Virtual AGC (http://www.ibiblio.org/apollo/) by Ronald Burkey from C to javascript/asm.js using the Emscripten compiler (https://github.com/kripken/emscripten/wiki). 

AGC was the main computer system of the Apollo program that successfully landed 12 astronauts on Moon. There was one AGC on each of the Apollo Command Modules and another one on each Lunar Module. There was also a second backup computer system called Abort Guidance System (AGS) on the Lunar Modules, which is simulated by Virtual AGC, but not the current version of Moonjs. 

Astronauts interacted with AGC by using DSKY, a combination of 7-segment numerical displays, indicator lights and a simple keypad, which is simulated on this page. The simulated DSKY communicates with a simulated AGC, which in turn runs a copy of Colossus 249, the flight software that flew on the Apollo 9 Command Module. 

Why Javascript?
===============

Recent advances in the javascript language - such as optimized engines, ahead-of-time (AOT) compilation, and asm.js - make it possible to write computationally extensive applications in javascript. My previous experience with online javascript-based simulation - svtsim(svtsimcom/svtsim.html) and hemosim (svtsim.com/hemosim.html) - was very positive and convinced me of the suitablity of the HTML5/javascript combination in writing portable, easy-to-use simulators. <b>Moonjs</b> is the logical next step that uses Emscripten compiler to convert
existing C code into asm.js, a highly-optimizable subset of javascript. 


Compiling
=========

To build Moonjs, you need to have Emscripten and its requirements (including Clang C compiler, python 2.7 and node.js) installed on your computer. 

Change the including Makefile by setting EMCC to the directory of the installed Emscripten (default is /usr/local/emsripten). Compile the source by 

    $ make

Do not use '.configure'. If compilation succeeds, it will generate 'agc.js' file which included the compiled asm.js and the required libraries. It also generate 'agc.data', which is a copy of the corerope image (provided as Core.bin) packaged for loading using AJAX.

Running
=======

The following files are needed to run Moonjs:

    agc.html                        the main html page and the javascript code simulating DSKY
    agc.js                          the compiled agc engine code
    agc.data                        a copy of the Core.bin which is loaded by agc.js during initialization
    digital-7-mono-italic.ttf       font file for seven-segment displays
    Apollo_DSKY_interface.svg.png   static graphic showing the DSKY interface (public domain image from NASA)


Point your browser to agc.html and run Moonjs! Note that you need to use a browser that supports HTML5 features, especially typed arrays. This includes recent versions of Firefox, Chrome, Safari and IE 9+. Note that the code does not run on older versions of IE. 

Also if you are testing the software from local disk, Chrome security prevents loading agc.data using AJAX over the local file system. If you are using Chrome, you need to run a static web server. For example, run the following command in the Moonjs directory:

    $ python -m SimpleHTTPServer 8000

and run Moonjs by pointing the browser to http://localhost:8000/agc.html.


    
    




