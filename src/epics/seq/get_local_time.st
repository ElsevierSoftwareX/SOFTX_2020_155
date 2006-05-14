program get_local_time

%% #include <stdio.h>
%% #include <string.h>
%% #include <stdlib.h>
%% #include <math.h>

string localtimestring;
assign localtimestring to "L0:TIM-PACIFIC_STRING";
string localtimestring1;
assign localtimestring1 to "M0:TIM-EASTERN_STRING";

ss get_local_time{

state init
{
  when() {
       strcpy(localtimestring,"Not Available");
       pvPut(localtimestring);
       strcpy(localtimestring1,"Not Available");
       pvPut(localtimestring1);
  }state run
}

state run {
  when(delay(1)){
%% time_t lt = time(0);
%% strcpy(localtimestring, ctime(&lt));
   pvPut(localtimestring);
%% strcpy(localtimestring1, ctime(&lt));
   pvPut(localtimestring1);
  }state run
}
}
