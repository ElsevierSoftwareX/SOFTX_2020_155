program stepper

%% #include <stdio.h>
%% #include <string.h>
%% #include <stdlib.h>
%% #include <math.h>

/* Motor selection commands (1 or 2) */
int motor0;
int motor1;
int motor2;
int motor3;
assign motor0 to "{ifo}:{sys}-MOTOR_SEL0";
assign motor1 to "{ifo}:{sys}-MOTOR_SEL1";
assign motor2 to "{ifo}:{sys}-MOTOR_SEL2";
assign motor3 to "{ifo}:{sys}-MOTOR_SEL3";

ss stepper
{

state init
{
  when() {
       motor0 = 1;
       pvPut(motor0);
       motor1 = 1;
       pvPut(motor1);
       motor2 = 1;
       pvPut(motor2);
       motor3 = 1;
       pvPut(motor3);
  }state run
}

state run
{
  when(delay(1)) {
	;
  } state run
}
}
