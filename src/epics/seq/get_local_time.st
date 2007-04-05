program get_local_time

%% #include <stdio.h>
%% #include <string.h>
%% #include <stdlib.h>
%% #include <math.h>

string localtimestring;
assign localtimestring to "{ifo}:{sys}-TIME_STRING";

ss get_local_time{

state init
{
  when() {
       strcpy(localtimestring,"Not Available");
       pvPut(localtimestring);
  }state run
}

state run {
  when(delay(1)){
%% time_t lt = time(0);
%% strcpy(localtimestring, ctime(&lt));
   pvPut(localtimestring);
  }state run
}
}
