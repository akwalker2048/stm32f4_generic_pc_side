#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>

#include "cmd_handling_readline.h"


#include "pc_serial.h"
#include "generic_packet.h"
#include "gp_receive.h"
#include "gp_proj_thermal.h"
#include "gp_proj_universal.h"
#include "gp_proj_analog.h"
#include "gp_proj_sonar.h"
#include "gp_proj_motor.h"

int execute_line (char *line);

extern char *getwd ();
extern char *xmalloc ();

int motor_start(char *arg);
int motor_stop(char *arg);
int motor_home(char *arg);
int motor_set_pos(char *arg);
int motor_set_tilt_mult(char *arg);
int motor_set_pid(char *arg);
int motor_tmc260_query_status(char *arg);
int motor_tmc260_set_drvctrl_sdon(char *arg);
int motor_tmc260_set_chopconf(char *arg);
int motor_tmc260_set_smarten(char *arg);
int motor_tmc260_set_drvconf(char *arg);
int motor_tmc260_set_sgcsconf(char *arg);


typedef int (*command_func_ptr)(char *);

typedef struct {
  char *name;			/* User printable name of the function. */
  command_func_ptr func;		/* Function to call to do the job. */
  char *doc;			/* Documentation for this function.  */
} COMMAND;

#define NUM_COMMANDS 13
command_func_ptr null_func_ptr = NULL;

char *motor_start_name = "motor_start";
char *motor_start_doc = "Send MOTOR_START packet!";
command_func_ptr motor_start_ptr = (command_func_ptr)&motor_start;

char *motor_stop_name = "motor_stop";
char *motor_stop_doc = "Send MOTOR_STOP packet!";
command_func_ptr motor_stop_ptr = (command_func_ptr)&motor_stop;

char *motor_home_name = "motor_home";
char *motor_home_doc = "Send MOTOR_HOME packet!";
command_func_ptr motor_home_ptr = (command_func_ptr)&motor_home;

char *motor_set_pos_name = "motor_set_pos";
char *motor_set_pos_doc = "Send MOTOR_SET_POSITION packet!";
command_func_ptr motor_set_pos_ptr = (command_func_ptr)&motor_set_pos;

char *motor_set_tilt_mult_name = "motor_set_tilt_mult";
char *motor_set_tilt_mult_doc = "Send MOTOR_SET_TILT_MULTIPLIER packet!";
command_func_ptr motor_set_tilt_mult_ptr = (command_func_ptr)&motor_set_tilt_mult;

char *motor_set_pid_name = "motor_set_pid";
char *motor_set_pid_doc = "Send MOTOR_SET_PID (motor_set_pid 1.0f 0.05f 0.005f)";
command_func_ptr motor_set_pid_ptr = (command_func_ptr)&motor_set_pid;

char *motor_tmc260_query_status_name = "motor_tmc260_query_status";
char *motor_tmc260_query_status_doc = "Send MOTOR_TMC260_QUERY_STATUS";
command_func_ptr motor_tmc260_query_status_ptr = (command_func_ptr)&motor_tmc260_query_status;

char *motor_tmc260_set_drvctrl_sdon_name = "motor_tmc260_set_drvctrl_sdon";
char *motor_tmc260_set_drvctrl_sdon_doc = "Send MOTOR_TMC260_SET_DRVCTRL_SDON";
command_func_ptr motor_tmc260_set_drvctrl_sdon_ptr = (command_func_ptr)&motor_tmc260_set_drvctrl_sdon;

char *motor_tmc260_set_chopconf_name = "motor_tmc260_set_chopconf";
char *motor_tmc260_set_chopconf_doc = "Send MOTOR_TMC260_SET_CHOPCONF";
command_func_ptr motor_tmc260_set_chopconf_ptr = (command_func_ptr)&motor_tmc260_set_chopconf;

char *motor_tmc260_set_smarten_name = "motor_tmc260_set_smarten";
char *motor_tmc260_set_smarten_doc = "Send MOTOR_TMC260_SET_SMARTEN";
command_func_ptr motor_tmc260_set_smarten_ptr = (command_func_ptr)&motor_tmc260_set_smarten;

char *motor_tmc260_set_drvconf_name = "motor_tmc260_set_drvconf";
char *motor_tmc260_set_drvconf_doc = "Send MOTOR_TMC260_SET_DRVCONF";
command_func_ptr motor_tmc260_set_drvconf_ptr = (command_func_ptr)&motor_tmc260_set_drvconf;

char *motor_tmc260_set_sgcsconf_name = "motor_tmc260_set_sgcsconf";
char *motor_tmc260_set_sgcsconf_doc = "Send MOTOR_TMC260_SET_SGCSCONF";
command_func_ptr motor_tmc260_set_sgcsconf_ptr = (command_func_ptr)&motor_tmc260_set_sgcsconf;


COMMAND commands[NUM_COMMANDS];

/* Forward declarations. */
char *stripwhite ();
COMMAND *find_command ();

/* /\* The name of this program, as taken from argv[0]. *\/ */
/* char *progname; */

/* When non-zero, this global means the user is done using this program. */
int done;

char *
dupstr (char *s)
     /* int s; */
{
  char *r;

  r = xmalloc (strlen (s) + 1);
  strcpy (r, s);
  return (r);
}

int cmd_handling_readline(void)
{
  char *line, *s;

  /* progname = argv[0]; */

  /* /\* Do this in main() before calling this function. *\/ */
  /* initialize_readline ();	/\* Bind our completer. *\/ */

  /* /\* Loop reading and executing lines until the user quits. *\/ */
  /* for ( ; done == 0; ) */
  /*   { */
  line = readline ("CMD>> ");

  if (!line)
     return 1;

  /* Remove leading and trailing whitespace from the line.
     Then, if there is anything left, add it to the history list
     and execute it. */
  s = stripwhite (line);

  if (*s)
  {
     add_history (s);
     execute_line (s);
  }

  free (line);

  return 0;
}

/* Execute a command line. */
int execute_line (char *line)
     /* char *line; */
{
  register int i;
  COMMAND *command;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while (line[i] && whitespace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !whitespace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  command = find_command (word);

  if (!command)
    {
      fprintf (stderr, "%s: No such command for FileMan.\n", word);
      return (-1);
    }

  /* Get argument to command, if any. */
  while (whitespace (line[i]))
    i++;

  word = line + i;

  /* Call the function. */
  return ((*(command->func)) (word));
}

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
COMMAND *
find_command (name)
     char *name;
{
  register int i;

  for (i = 0; commands[i].name; i++)
    if (strcmp (name, commands[i].name) == 0)
      return (&commands[i]);

  return ((COMMAND *)NULL);
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char *
stripwhite (string)
     char *string;
{
  register char *s, *t;

  for (s = string; whitespace (*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}

/* **************************************************************** */
/*                                                                  */
/*                  Interface to Readline Completion                */
/*                                                                  */
/* **************************************************************** */

char *command_generator ();
/* char **fileman_completion (); */

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
int initialize_readline(void)
{

   int cmd_num = 0;

   /* Allow conditional parsing of the ~/.inputrc file. */
   rl_readline_name = "main_pc_comm";

   /* Load Command Structure */
   commands[cmd_num].name = motor_start_name;
   commands[cmd_num].func = motor_start_ptr;
   commands[cmd_num].doc = motor_start_doc;

   cmd_num++;
   commands[cmd_num].name = motor_stop_name;
   commands[cmd_num].func = motor_stop_ptr;
   commands[cmd_num].doc = motor_stop_doc;

   cmd_num++;
   commands[cmd_num].name = motor_home_name;
   commands[cmd_num].func = motor_home_ptr;
   commands[cmd_num].doc = motor_home_doc;

   cmd_num++;
   commands[cmd_num].name = motor_set_pos_name;
   commands[cmd_num].func = motor_set_pos_ptr;
   commands[cmd_num].doc = motor_set_pos_doc;

   cmd_num++;
   commands[cmd_num].name = motor_set_tilt_mult_name;
   commands[cmd_num].func = motor_set_tilt_mult_ptr;
   commands[cmd_num].doc = motor_set_tilt_mult_doc;

   cmd_num++;
   commands[cmd_num].name = motor_set_pid_name;
   commands[cmd_num].func = motor_set_pid_ptr;
   commands[cmd_num].doc = motor_set_pid_doc;

   cmd_num++;
   commands[cmd_num].name = motor_tmc260_query_status_name;
   commands[cmd_num].func = motor_tmc260_query_status_ptr;
   commands[cmd_num].doc = motor_tmc260_query_status_doc;

   cmd_num++;
   commands[cmd_num].name = motor_tmc260_set_drvctrl_sdon_name;
   commands[cmd_num].func = motor_tmc260_set_drvctrl_sdon_ptr;
   commands[cmd_num].doc = motor_tmc260_set_drvctrl_sdon_doc;

   cmd_num++;
   commands[cmd_num].name = motor_tmc260_set_chopconf_name;
   commands[cmd_num].func = motor_tmc260_set_chopconf_ptr;
   commands[cmd_num].doc = motor_tmc260_set_chopconf_doc;

   cmd_num++;
   commands[cmd_num].name = motor_tmc260_set_smarten_name;
   commands[cmd_num].func = motor_tmc260_set_smarten_ptr;
   commands[cmd_num].doc = motor_tmc260_set_smarten_doc;

   cmd_num++;
   commands[cmd_num].name = motor_tmc260_set_drvconf_name;
   commands[cmd_num].func = motor_tmc260_set_drvconf_ptr;
   commands[cmd_num].doc = motor_tmc260_set_drvconf_doc;

   cmd_num++;
   commands[cmd_num].name = motor_tmc260_set_sgcsconf_name;
   commands[cmd_num].func = motor_tmc260_set_sgcsconf_ptr;
   commands[cmd_num].doc = motor_tmc260_set_sgcsconf_doc;

   /* Next command goes here! */

   commands[NUM_COMMANDS - 1].name = NULL;
   commands[NUM_COMMANDS - 1].func = null_func_ptr;
   commands[NUM_COMMANDS - 1].doc = NULL;

   /* Tell the completer that we want a crack first. */
   /* rl_attempted_completion_function = (CPPFunction *)fileman_completion; */
}

/* /\* Attempt to complete on the contents of TEXT.  START and END show the */
/*    region of TEXT that contains the word to complete.  We can use the */
/*    entire line in case we want to do some simple parsing.  Return the */
/*    array of matches, or NULL if there aren't any. *\/ */
/* char ** */
/* fileman_completion (text, start, end) */
/*      char *text; */
/*      int start, end; */
/* { */
/*   char **matches; */

/*   matches = (char **)NULL; */

/*   /\* If this word is at the start of the line, then it is a command */
/*      to complete.  Otherwise it is the name of a file in the current */
/*      directory. *\/ */
/*   if (start == 0) */
/*     matches = rl_completion_matches (text, command_generator); */

/*   return (matches); */
/* } */

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *
command_generator (text, state)
     char *text;
     int state;
{
  static int list_index, len;
  char *name;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  /* Return the next name which partially matches from the command list. */
  while (name = commands[list_index].name)
    {
      list_index++;

      if (strncmp (name, text, len) == 0)
        return (dupstr(name));
    }

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

/* **************************************************************** */
/*                                                                  */
/*                       FileMan Commands                           */
/*                                                                  */
/* **************************************************************** */

/* List the file(s) named in arg. */
int motor_start(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   ssize_t bytes_written;

   if (!arg)
      arg = "";

   retval = create_motor_start(&gp);
   printf("Send MOTOR_START\n");
   serial_write_array(gp.gp, gp.packet_length, &bytes_written);

  return 0;
}

int motor_stop(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   ssize_t bytes_written;

   if (!arg)
      arg = "";

   retval = create_motor_stop(&gp);
   printf("Send MOTOR_STOP\n");
   serial_write_array(gp.gp, gp.packet_length, &bytes_written);

  return 0;

}

int motor_home(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   ssize_t bytes_written;

   if (!arg)
      arg = "";

   retval = create_motor_home(&gp);
   printf("Send MOTOR_HOME\n");
   serial_write_array(gp.gp, gp.packet_length, &bytes_written);

  return 0;
}

int motor_set_pos(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   ssize_t bytes_written;
   float pos;
   int num_args;

   num_args = sscanf(arg, "%f", &pos);

   if(num_args != 1)
   {

   }
   else
   {
      retval = create_motor_set_position(&gp, pos);
      printf("Send MOTOR_SET_POSITION\n");
      serial_write_array(gp.gp, gp.packet_length, &bytes_written);
   }

}


int motor_set_tilt_mult(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   ssize_t bytes_written;
   float mult;
   int num_args;

   num_args = sscanf(arg, "%f", &mult);

   if(num_args != 1)
   {

   }
   else
   {
      if((mult < 0.01f)||(mult > 100.0f))
      {
         mult = 1.0f;
      }

      retval = create_motor_set_tilt_multiplier(&gp, mult);
      printf("Send MOTOR_SET_TILT_MULTIPLIER\n");
      serial_write_array(gp.gp, gp.packet_length, &bytes_written);
   }

}


int motor_set_pid(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   ssize_t bytes_written;

   float p, i, d;

   int num_args;

   if(arg != NULL)
   {
      printf("motor_set_pid %s\n", arg);
   }

   num_args = sscanf(arg, "%f %f %f", &p, &i, &d);
   if(num_args == 3)
   {
      retval = create_motor_set_pid(&gp, p, i, d);
      printf("Send MOTOR_SET_PID\n");
      serial_write_array(gp.gp, gp.packet_length, &bytes_written);
   }
   else
   {
      printf("motor_set_pid requires 3 arguments!\n");
      printf("motor_set_pid 1.0 0.2 0.05\n");
   }


  return 0;

}


int motor_tmc260_query_status(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   ssize_t bytes_written;

   uint8_t status_type;
   unsigned int status_type_temp;

   int num_args;

   if(arg != NULL)
   {
      printf("motor_tmc260_query_status %s\n", arg);
   }

   num_args = sscanf(arg, "%u", &status_type_temp);
   status_type = 0xFF&status_type_temp;
   if(status_type > 2)
   {
      printf("Warning status_type can only be 0, 1, 0r 2!\n");
      status_type = 0;
   }

   if(num_args == 1)
   {
      retval = create_motor_tmc260_query_status(&gp, status_type);
      printf("Send MOTOR_TMC260_QUERY_STATUS type(%u)\n", status_type);
      serial_write_array(gp.gp, gp.packet_length, &bytes_written);
   }
   else
   {
      printf("motor_tmc260_query_status requires 1 arguments!\n");
      printf("motor_tmc260_query_status 0 /* Position Feedback */\n");
      printf("motor_tmc260_query_status 1 /* Stallgaurd Feedback */\n");
      printf("motor_tmc260_query_status 2 /* Stallgaurd + Current Feedback */\n");
   }


  return 0;

}



int motor_tmc260_set_drvctrl_sdon(char *arg)
{
   GenericPacket gp;
   ssize_t bytes_written;
   uint8_t retval;
   int num_args;

   unsigned int intpol_temp, dedge_temp, mres_temp;
   uint8_t intpol, dedge, mres;

   num_args = sscanf(arg, "%u %u %u", &intpol_temp, &dedge_temp, &mres_temp);
   if(num_args != 3)
   {
      printf("Proper command will have three args.  Example:\n");
      printf(">>motor_tmc260_set_drvctrl_sdon intpol dedge mres\n");
      printf("  intpol is 0 or 1 depending on whether you wish to interpolate steps\n");
      printf("  dedge is 0 or 1 depending on whether you want to step on one or both edges\n");
      printf("  mres is between 0 and 8 and represents the microstep configuration\n");
      printf("\n");
   }
   else
   {
      intpol = (uint8_t)intpol_temp;
      dedge = (uint8_t)dedge_temp;
      mres = (uint8_t)mres_temp;
      retval = create_motor_tmc260_set_drvctrl_sdon(&gp, intpol, dedge, mres);
      printf("Send MOTOR_TMC260_SET_DRVCTRL_SDON\n");
      serial_write_array(gp.gp, gp.packet_length, &bytes_written);
   }

   return 0;
}


int motor_tmc260_set_chopconf(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   int num_args;
   ssize_t bytes_written;

   unsigned int tbl_temp, chm_temp, rndtf_temp, hdec_temp, hend_temp, hstrt_temp, toff_temp;
   uint8_t tbl, chm, rndtf, hdec, hend, hstrt, toff;

   num_args = sscanf(arg, "%u %u %u %u %u %u %u", &tbl_temp, &chm_temp, &rndtf_temp, &hdec_temp, &hend_temp, &hstrt_temp, &toff_temp);
   if(num_args != 7)
   {
      printf("Proper command will have 7 args.  Example:\n");
      printf(">>motor_tmc260_set_chopconf tbl chm rndtf hdec hend hstrt toff\n");
      printf("  Individual arg description goes here...\n");
      printf("\n");
   }
   else
   {
      tbl = (uint8_t)tbl_temp;
      chm = (uint8_t)chm_temp;
      rndtf = (uint8_t)rndtf_temp;
      hdec = (uint8_t)hdec_temp;
      hend = (uint8_t)hend_temp;
      hstrt = (uint8_t)hstrt_temp;
      toff = (uint8_t)toff_temp;

      retval = create_motor_tmc260_set_chopconf(&gp, tbl, chm, rndtf, hdec, hend, hstrt, toff);
      printf("Send MOTOR_TMC260_SET_CHOPCONF\n");
      serial_write_array(gp.gp, gp.packet_length, &bytes_written);
   }

   return 0;
}

int motor_tmc260_set_smarten(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   int num_args;
   ssize_t bytes_written;

   unsigned int seimin_temp, sedn_temp, semax_temp, seup_temp, semin_temp;
   uint8_t seimin, sedn, semax, seup, semin;

   num_args = sscanf(arg, "%u %u %u %u %u", &seimin_temp, &sedn_temp, &semax_temp, &seup_temp, &semin_temp);
   if(num_args != 5)
   {
      printf("Proper command will have 5 args.  Example:\n");
      printf(">>motor_tmc260_set_smarten seimin, sedn, semax, seup, semin\n");
      printf("  Individual arg description goes here...\n");
      printf("\n");
   }
   else
   {
      seimin = (uint8_t)seimin_temp;
      sedn = (uint8_t)sedn_temp;
      semax = (uint8_t)semax_temp;
      seup = (uint8_t)seup_temp;
      semin = (uint8_t)semin_temp;
      retval = create_motor_tmc260_set_smarten(&gp, seimin, sedn, semax, seup, semin);
      printf("Send MOTOR_TMC260_SET_SMARTEN\n");
      serial_write_array(gp.gp, gp.packet_length, &bytes_written);
   }

   return 0;
}

int motor_tmc260_set_drvconf(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   int num_args;
   ssize_t bytes_written;

   unsigned int tst_temp, slph_temp, slpl_temp, diss2g_temp, ts2g_temp, sdoff_temp, vsense_temp, rdsel_temp;
   uint8_t tst, slph, slpl, diss2g, ts2g, sdoff, vsense, rdsel;

   num_args = sscanf(arg, "%u %u %u %u %u %u %u %u", &tst_temp, &slph_temp, &slpl_temp, &diss2g_temp, &ts2g_temp, &sdoff_temp, &vsense_temp, &rdsel_temp);
   if(num_args != 8)
   {
      printf("Proper command will have 8 args.  Example:\n");
      printf(">>motor_tmc260_set_drvconf tst slph slpl diss2g ts2g sdoff vsense rdsel\n");
      printf("  Individual arg description goes here...\n");
      printf("\n");
   }
   else
   {
      tst = (uint8_t)tst_temp;
      slph = (uint8_t)slph_temp;
      slpl = (uint8_t)slpl_temp;
      diss2g = (uint8_t)diss2g_temp;
      ts2g = (uint8_t)ts2g_temp;
      sdoff = (uint8_t)sdoff_temp;
      vsense = (uint8_t)vsense_temp;
      rdsel = (uint8_t)rdsel_temp;

      retval = create_motor_tmc260_set_drvconf(&gp, tst, slph, slpl, diss2g, ts2g, sdoff, vsense, rdsel);
      printf("Send MOTOR_TMC260_SET_DRVCONF\n");
      serial_write_array(gp.gp, gp.packet_length, &bytes_written);
   }

   return 0;
}

int motor_tmc260_set_sgcsconf(char *arg)
{
   GenericPacket gp;
   uint8_t retval;
   int num_args;
   ssize_t bytes_written;

   unsigned int sfilt_temp, sgt_temp, cs_temp;
   uint8_t sfilt, sgt, cs;

   num_args = sscanf(arg, "%u %u %u", &sfilt_temp, &sgt_temp, &cs_temp);
   if(num_args != 3)
   {
      printf("Proper command will have 3 args.  Example:\n");
      printf(">>motor_tmc260_set_sgcsconf sfilt sgt cs\n");
      printf("  Individual arg description goes here...\n");
      printf("\n");
   }
   else
   {
      sfilt = (uint8_t)sfilt_temp;
      sgt = (uint8_t)sgt_temp;
      cs = (uint8_t)cs_temp;

      retval = create_motor_tmc260_set_sgcsconf(&gp, sfilt, sgt, cs);
      printf("Send MOTOR_TMC260_SET_SGCSCONF\n");
      serial_write_array(gp.gp, gp.packet_length, &bytes_written);
   }

   return 0;
}



/* Return non-zero if ARG is a valid argument for CALLER, else print
   an error message and return zero. */
int
valid_argument (caller, arg)
     char *caller, *arg;
{
  if (!arg || !*arg)
    {
      fprintf (stderr, "%s: Argument required.\n", caller);
      return (0);
    }

  return (1);
}
