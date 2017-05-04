/* Copyright (C) 2014, Armin Biere, Johannes Kepler University, Linz */

#include "ipasir.h"

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
  printf ("c [genipasat] ");
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

static int vars, clauses, lineno = 1, closefile;
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

void learncb(void* state, int* cls) {
  printf("solver learned ");
  while (*cls) {printf("%d ", *cls); cls++;}
  printf("\n");
}

int main (int argc, char ** argv) {
  int res = 0, i, ch, c, l, lit;
  char * sig;
  for (i = 1; i < argc; i++) {
    if (!strcmp (argv[i], "-h")) usage ();
    else if (argv[i][0] == '-') die ("invalid '%s' (try '-h')", argv[i]);
    else if (name) die ("multiple files '%s' and '%s'", name, argv[i]);
    else name = argv[i];
  }
  solver = ipasir_init ();
  ipasir_set_learn(solver, 0, 2, learncb);
  sig = strdup (ipasir_signature (solver));
  msg ("GenIPASAT Generic IPASIR Solver");
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
  if (fscanf (file, " cnf %d %d", &vars, &clauses) != 2)
    perr ("invalid header");
  if (vars < 0) perr ("invalid negative number of variables %d", vars);
  if (clauses < 0) perr ("invalid negative number of clauses %d", clauses);
  msg ("found 'p cnf %d %d' header at line %d", vars, clauses, lineno);
  c = l = lit = 0;
  while (fscanf (file, "%d", &lit) == 1) {
    if (lit == INT_MIN || abs (lit) > vars) perr ("invalid literal %d", lit);
    ipasir_add (solver, lit);
    if (lit) l++; else c++;
  }
  if (!feof (file)) perr ("could not parse literal");
  if (lit) perr ("last literal %d was non zero", lit);
  msg ("parsed %d literals in %d clauses", l, c);
  if (c != clauses) perr ("expected %d clauses but got %d", clauses, c);
  if (closefile == 2) pclose (file);
  if (closefile == 1) fclose (file);
  msg ("calling SAT solver after %.2f seconds", getime ());
  res = ipasir_solve (solver);
  msg ("SAT solver returns %d after %.2f seconds", res, getime ());
  fflush (stderr);
  if (res == 10) {
    printf ("s SATISFIABLE\n");
    if (vars) {
      fflush (stdout);
      fputc ('v', stdout);
      for (i = 1; i <= vars; i++) {
	if (!(i % 8)) fputs ("\nv", stdout);
	printf (" %d", ipasir_val (solver, i));
      }
      if (vars) fputc ('\n', stdout);
    }
    printf ("v 0\n");
  } else if (res == 20) printf ("s UNSATISFIABLE\n");
  else printf ("s UNKNOWN\n");
  fflush (stdout);
  ipasir_release (solver);
  msg ("released '%s' after %.2f seconds", sig, getime ());
  msg ("exit %d", res);
  free (sig);
  return res;
}
