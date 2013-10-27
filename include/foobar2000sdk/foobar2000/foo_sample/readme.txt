This component demonstrates:
* main.cpp :
  * Declaring your component's version information.
* input_raw.cpp :
  * Declaring your own "input" classes for decoding additional audio file formats.
  * Calling file system services.
* preferences.cpp :
  * Declaring your configuration variables.
  * Creating preferences pages using simple WTL dialogs.
  * Declaring advanced preferences entries.
* initquit.cpp :
  * Sample initialization/shutdown callback service.
* dsp.cpp :
  * Sample DSP.
* contextmenu.cpp :
  * Sample context menu command.
* decode.cpp :
  * Getting PCM data from arbitrary audio files.
  * Use of the threaded_process API to easily run time-consuming tasks in worker threads with progress dialogs.
* mainmenu.cpp :
  * Sample main menu command
* playback_state.cpp :
  * Use of playback callbacks.
  * Use of playback control.
* ui_element.cpp : 
  * Simple UI Element implementation.