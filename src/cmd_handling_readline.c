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

/* The names of functions that actually do the manipulation. */
int motor_start();
int motor_stop();

/* A structure which contains information on the commands this program
   can understand. */

typedef struct {
  char *name;			/* User printable name of the function. */
  Function *func;		/* Function to call to do the job. */
  char *doc;			/* Documentation for this function.  */
} COMMAND;

COMMAND commands[] = {
  { "motor_start", motor_start, "Send MOTOR_START packet!" },
  { "motor_stop", motor_stop, "Send MOTOR_STOP packet!" },
  { (char *)NULL, (Function *)NULL, (char *)NULL }
};

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
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = "main_pc_comm";

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
int motor_start (char *arg)
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

int motor_stop (arg)
     char *arg;
{
   GenericPacket gp;
   uint8_t retval;
   ssize_t bytes_written;

   if (!arg)
      arg = "";

   retval = create_motor_stop(&gp);
   printf("Send MOTOR_START\n");
   serial_write_array(gp.gp, gp.packet_length, &bytes_written);

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
