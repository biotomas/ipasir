/* Copyright (C) 2014, Armin Biere, Johannes Kepler University, Linz */

#include "ipasir.h"

#define _GNU_SOURCE

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <limits.h>

static void msg (const char * fmt, ...) {
  va_list ap;
  fflush (stderr);
  printf ("c [icnf] ");
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

static void die (const char * fmt, ...) {
  va_list ap;
  fflush (stderr);
  printf ("*** genipasat: ");
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
  exit (1);
}

static void usage () {
  printf ("usage: genipasat [-h] [ <dimacs> ]\n");
  exit (0);
}

static int hasuffix (const char * str, const char * suffix) {
  if (strlen (str) < strlen (suffix)) return 0;
  return !strcmp (str + strlen (str) - strlen (suffix), suffix);
}

static FILE * pipeit (const char * name, const char * fmt) {
  char * cmd = malloc (strlen (name) + strlen (fmt) + 20);
  FILE * res;
  sprintf (cmd, fmt, name);
  res = popen (cmd, "r");
  free (cmd);
  return res;
}

static int next (FILE * file, int * linenoptr) {
  int res;
  if ((res = getc (file)) == '\n') *linenoptr += 1;
  return res;
}

static int lineno = 1, closefile;
static const char * name = 0;
static void * solver;
static FILE * file;

static void perr (const char * fmt, ...) {
  va_list ap;
  fflush (stderr);
  printf ("*** genipasat: parse error in '%s' line '%d': ", name, lineno);
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
  exit (1);
}

#define NEXT() (ch = next (file, &lineno))

static double getime (void) {
  struct rusage u;
  double res;
  if (getrusage (RUSAGE_SELF, &u)) return 0;
  res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
  res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
  return res;
}

int main (int argc, char ** argv) {
  int res = 0, i, ch, lit, max_var = 0, assumption;
  char *line = NULL, *line_content;
  size_t len = 0, parsed, solves = 0, parsed_assumptions = 0, core_size = 0;
  ssize_t read;
  char * sig;
  double pre_time, post_time;
  int * current_assumptions = NULL;

  for (i = 1; i < argc; i++) {
    if (!strcmp (argv[i], "-h")) usage ();
    else if (argv[i][0] == '-') die ("invalid '%s' (try '-h')", argv[i]);
    else if (name) die ("multiple files '%s' and '%s'", name, argv[i]);
    else name = argv[i];
  }

  solver = ipasir_init ();

  sig = strdup (ipasir_signature (solver));
  msg ("icnf generic IPASIR Solver");
  msg ("initialized '%s'", sig);

  if (name) {
    closefile = 2;
    if (hasuffix (name, ".bz2")) file = pipeit (name, "bzcat %s");
    else if (hasuffix (name, ".gz")) file = pipeit (name, "gunzip -c %s");
    else file = fopen (name, "r"), closefile = 1;
    if (!file) die ("can not read '%s'", name);
  } else name = "<stdin>", file = stdin;

  msg ("reading '%s'", name);

  while (NEXT () == 'c') {
    while (NEXT () != '\n')
      if (ch == EOF)
    perr ("unexpected end-of-file in comment");
  }
  if (ch != 'p') perr ("expected 'p' or 'c'");
  if (fscanf (file, " inccnf") != 0)
    perr ("invalid header");

  msg ("found 'p inccnf' header at line %d", lineno);
  lit = 0;

  while ((read = getline(&line, &len, file)) != -1) {
    lineno ++;
    /* skip line breaks */
    if( line[0] == '\n') continue;

    /* skip short lines */
    if( read < 1 ) die ("found an empty line in the file");

    /* skip comments */
    if( line[0] == 'c' ) continue;

    line_content = line;
    parsed = 0;
    assumption = 0;

    if( line[0] == 'a' ) {
      if(read < 2) die("expected assumption vector to solve in '%s' (line %d)", line, lineno);
      parsed = 2;
      line_content = &(line[parsed]);
      assumption = 1;
      parsed_assumptions = 0;
    }

    while ( (i = sscanf (line_content, "%d", &lit)) == 1) {

      /* track highest variable */
      if (lit < 0) max_var = max_var > -lit ? max_var : -lit;
      else max_var = max_var > lit ? max_var : lit;

      /* in case we're on the assumption line, assume the literals and solver */
      if (!assumption) {
        ipasir_add (solver, lit);
      }
      else if (lit)
      {
        ipasir_assume(solver, lit);
        /* track variables to be assumed */
        if (current_assumptions == NULL || parsed_assumptions >= max_var)
          current_assumptions = realloc(current_assumptions, sizeof(int) * (max_var > parsed_assumptions ? max_var : parsed_assumptions));
        current_assumptions[parsed_assumptions] = lit;
        parsed_assumptions ++;
      }

      /* stop at the end of the clause, do not read the rest of the line */
      if (!lit) break;
      while (parsed < read && line[parsed] != ' ') ++parsed;
      /* actually skip space */
       ++parsed;
      if (parsed >= read) break;
      line_content = &(line[parsed]);
    }

    if (i != 1) die ("expected literals or a clause delimiter on line %s (line %d), but did not find any\n", lineno);

    if(assumption) {
      core_size = 0;
      solves ++;
      msg ("solved assumptions %lu (%d assumptions) on line %d ...", solves, parsed_assumptions, lineno);
      pre_time = getime ();
      res = ipasir_solve(solver);
      post_time = getime ();

      /* print model, or unsat core */
      if (res == 10)
      {
        if (max_var) {
          fflush (stdout);
          printf ("s SATISFIABLE\n");
          fputc ('v', stdout);
          for (i = 1; i <= max_var; i++) {
          if (!(i % 8)) fputs ("\nv", stdout);
            printf (" %d", ipasir_val (solver, i));
          }
          fputc ('\n', stdout);
        }
        printf ("v 0\n");
      } else if (res == 20) {
        core_size = 0;
        fflush (stdout);
        printf ("s UNSATISFIABLE\n");
        fputc ('v', stdout);
        for( i=0; i < parsed_assumptions; ++i )
        {
          if (ipasir_failed(solver, current_assumptions[i] < 0 ? -current_assumptions[i] : current_assumptions[i]))
          {
            core_size ++;
            printf (" %d", current_assumptions[i]);
          }
        }
        fputc ('\n', stdout);
        printf ("v 0\n");
      }

      msg ("solved assumptions %lu with results %d and unsat core of %u in %.2f seconds", solves, res, core_size, post_time - pre_time);
    }
  }

  if (!feof (file)) perr ("end of file is not reached yet");
  if (current_assumptions) free(current_assumptions);
  if (closefile == 2) pclose (file);
  if (closefile == 1) fclose (file);

  /* cleanup */
  fflush (stdout);
  ipasir_release (solver);
  msg ("released '%s' after %.2f seconds", sig, getime ());
  msg ("exit %d", res);
  free (sig);
  return res;
}
