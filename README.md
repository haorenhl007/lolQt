# lolQt

**Merge animated GIFs and background music into video**

## Prerequisites

  * [Microsoft Visual C++ 2010 SP1 Redistributable Package (x86)](http://www.microsoft.com/de-de/download/details.aspx?id=8328)
  * [MEncoder](http://oss.netfarm.it/mplayer-win32.php): Choose the correct build for your architecture.

## Installation

Download [lolQt-1.0.1-setup.exe](https://drive.google.com/folderview?id=0B3S-OBO0P8GMMlJZNzFLVDY2bFE&usp=sharing), run it, and follow the instructions.

## Usage in a nutshell

  * Go to Extras/Settings:
    - Choose path to MEncoder binary.
    - Choose output file (AVI).
    - Change path to temporary directory if necessary.
    - Change audio bitrate if necessary.
  * Drop an animated GIF onto the GUI.
  * Drop a music file (MP3) onto the GUI.
  * Tap on "Beat me!" according to the rhythm to compute beats per minute or choose bpm in the spin box.
  * Click "Save frames" to write the output file to disk. An AVI will be written with the sequence of the GIF's frames repeated as long as the music lasts. The sequence will be in synch with the music.
