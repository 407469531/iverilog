/*
 * Copyright (c) 2001-2008 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

# include  "config.h"
# include  "parse_misc.h"
# include  "compile.h"
# include  "schedule.h"
# include  "vpi_priv.h"
# include  "statistics.h"
# include  <stdio.h>
# include  <stdlib.h>
# include  <string.h>
# include  <unistd.h>

#if defined(HAVE_SYS_RESOURCE_H)
# include  <sys/time.h>
# include  <sys/resource.h>
#endif // defined(HAVE_SYS_RESOURCE_H)

#if defined(HAVE_GETOPT_H)
# include  <getopt.h>
#endif

#if defined(__MINGW32__)
# include  <windows.h>
#endif

ofstream debug_file;

#if defined(__MINGW32__) && !defined(HAVE_GETOPT_H)
extern "C" int getopt(int argc, char*argv[], const char*fmt);
extern "C" int optind;
extern "C" const char*optarg;
#endif

#if !defined(HAVE_LROUND)
/*
 * If the system doesn't provide the lround function, then we provide
 * it ourselves here. It is simply the nearest integer, rounded away
 * from zero.
 */
# include  <math.h>
extern "C" long int lround(double x)
{
      if (x >= 0.0)
	    return (long)floor(x+0.5);
      else
	    return (long)ceil(x-0.5);
}
#endif

bool verbose_flag = false;
static int vvp_return_value = 0;

void vpip_set_return_value(int value)
{
      vvp_return_value = value;
}

static char log_buffer[4096];

#if defined(HAVE_SYS_RESOURCE_H)
static void my_getrusage(struct rusage *a)
{
      getrusage(RUSAGE_SELF, a);

#     if defined(LINUX)
      {
	    FILE *statm;
	    unsigned siz, rss, shd;
	    long page_size = sysconf(_SC_PAGESIZE);
	    if (page_size==-1) page_size=0;
	    statm = fopen("/proc/self/statm", "r");
	    if (!statm) {
		  perror("/proc/self/statm");
		  return;
	    }
	    if (3<=fscanf(statm, "%u%u%u", &siz, &rss, &shd)) {
		  a->ru_maxrss = page_size * siz;
		  a->ru_idrss  = page_size * rss;
		  a->ru_ixrss  = page_size * shd;
	    }
	    fclose(statm);
      }
#     endif
}

static void print_rusage(struct rusage *a, struct rusage *b)
{
      double delta = a->ru_utime.tv_sec
	    +        a->ru_utime.tv_usec/1E6
	    +        a->ru_stime.tv_sec
	    +        a->ru_stime.tv_usec/1E6
	    -        b->ru_utime.tv_sec
	    -        b->ru_utime.tv_usec/1E6
	    -        b->ru_stime.tv_sec
	    -        b->ru_stime.tv_usec/1E6
	    ;

      vpi_mcd_printf(1,
	      " ... %G seconds,"
	      " %.1f/%.1f/%.1f KBytes size/rss/shared\n",
	      delta,
	      a->ru_maxrss/1024.0,
	      (a->ru_idrss+a->ru_isrss)/1024.0,
	      a->ru_ixrss/1024.0 );
}

#else // ! defined(HAVE_SYS_RESOURCE_H)

// Provide dummies
struct rusage { int x; };
inline static void my_getrusage(struct rusage *) { }
inline static void print_rusage(struct rusage *, struct rusage *){};

#endif // ! defined(HAVE_SYS_RESOURCE_H)


unsigned module_cnt = 0;
const char*module_tab[64];

extern void vpi_mcd_init(FILE *log);
extern void vvp_vpi_init(void);

int main(int argc, char*argv[])
{
      int opt;
      unsigned flag_errors = 0;
      const char*design_path = 0;
      struct rusage cycles[3];
      const char *logfile_name = 0x0;
      FILE *logfile = 0x0;
      extern void vpi_set_vlog_info(int, char**);
      extern bool stop_is_finish;

#ifdef __MINGW32__
	/* In the Windows world, we get the first module path
	   component relative the location where the binary lives. */
      { char path[4096], *s;
        GetModuleFileName(NULL,path,1024);
	  /* Get to the end.  Search back twice for backslashes */
	s = path + strlen(path);
	while (*s != '\\') s--; s--;
	while (*s != '\\') s--;
	strcpy(s,"\\lib\\ivl");
	vpip_module_path[0] = strdup(path);
      }
#endif

        /* For non-interactive runs we do not want to run the interactive
         * debugger, so make $stop just execute a $finish. */
      stop_is_finish = false;
      while ((opt = getopt(argc, argv, "+hl:M:m:nsv")) != EOF) switch (opt) {
         case 'h':
           fprintf(stderr,
                   "Usage: vvp [options] input-file [+plusargs...]\n"
                   "Options:\n"
                   " -h             Print this help message.\n"
                   " -l file        Logfile, '-' for <stderr>\n"
                   " -M path        VPI module directory\n"
		   " -M -           Clear VPI module path\n"
                   " -m module      Load vpi module.\n"
		   " -n             Non-interctive ($stop = $finish).\n"
		   " -s             $stop right away.\n"
                   " -v             Verbose progress messages.\n" );
           exit(0);
	  case 'l':
	    logfile_name = optarg;
	    break;
	  case 'M':
	    if (strcmp(optarg,"-") == 0) {
		  vpip_module_path_cnt = 0;
		  vpip_module_path[0] = 0;
	    } else {
		  vpip_module_path[vpip_module_path_cnt++] = optarg;
	    }
	    break;
	  case 'm':
	    module_tab[module_cnt++] = optarg;
	    break;
	  case 'n':
	    stop_is_finish = true;
	    break;
	  case 's':
	    schedule_stop(0);
	    break;
	  case 'v':
	    verbose_flag = true;
	    break;
	  default:
	    flag_errors += 1;
      }

      if (flag_errors)
	    return flag_errors;

      if (optind == argc) {
	    fprintf(stderr, "%s: no input file.\n", argv[0]);
	    return -1;
      }

	/* If the VVP_DEBUG variable is set, then it contains the path
	   to the vvp debug file. Open it for output. */

      if (char*path = getenv("VVP_DEBUG")) {
	    debug_file.open(path, ios_base::out);
      }

      design_path = argv[optind];

	/* This is needed to get the MCD I/O routines ready for
	   anything. It is done early because it is plausible that the
	   compile might affect it, and it is cheap to do. */

      if (logfile_name) {
	    if (!strcmp(logfile_name, "-"))
		  logfile = stderr;
	    else {
		  logfile = fopen(logfile_name, "w");
		  if (!logfile) {
		        perror(logfile_name);
		        exit(1);
		  }
		  setvbuf(logfile, log_buffer, _IOLBF, sizeof(log_buffer));
	    }
      }

      vpi_mcd_init(logfile);

      if (verbose_flag) {
	    my_getrusage(cycles+0);
	    vpi_mcd_printf(1, "Compiling VVP ...\n");
      }

      vvp_vpi_init();

	/* Make the extended arguments available to the simulation. */
      vpi_set_vlog_info(argc-optind, argv+optind);

      compile_init();

      for (unsigned idx = 0 ;  idx < module_cnt ;  idx += 1)
	    vpip_load_module(module_tab[idx]);

      if (int rc = compile_design(design_path))
	    return rc;

      if (verbose_flag) {
	    vpi_mcd_printf(1, "Compile cleanup...\n");
      }

      compile_cleanup();

      if (compile_errors > 0) {
	    vpi_mcd_printf(1, "%s: Program not runnable, %u errors.\n",
		    design_path, compile_errors);
	    return compile_errors;
      }

      if (verbose_flag) {
	    vpi_mcd_printf(1, " ... %8lu functors\n", count_functors);
	    vpi_mcd_printf(1, "           %8lu logic\n",  count_functors_logic);
	    vpi_mcd_printf(1, "           %8lu bufif\n",  count_functors_bufif);
	    vpi_mcd_printf(1, "           %8lu resolv\n",count_functors_resolv);
	    vpi_mcd_printf(1, "           %8lu signals\n", count_functors_sig);
	    vpi_mcd_printf(1, " ... %8lu opcodes (%zu bytes)\n",
		    count_opcodes, (unsigned long)size_opcodes);
	    vpi_mcd_printf(1, " ... %8lu nets\n",     count_vpi_nets);
	    vpi_mcd_printf(1, " ... %8lu vvp_nets (%zu bytes)\n",
			   count_vvp_nets, size_vvp_nets);
	    vpi_mcd_printf(1, " ... %8lu memories\n", count_vpi_memories);
	    vpi_mcd_printf(1, " ... %8lu scopes\n",   count_vpi_scopes);
      }

      if (verbose_flag) {
	    my_getrusage(cycles+1);
	    print_rusage(cycles+1, cycles+0);
	    vpi_mcd_printf(1, "Running ...\n");
      }


      schedule_simulate();

      if (verbose_flag) {
	    my_getrusage(cycles+2);
	    print_rusage(cycles+2, cycles+1);

	    vpi_mcd_printf(1, "Event counts:\n");
	    vpi_mcd_printf(1, "    %8lu time steps (pool=%lu)\n",
			   count_time_events, count_time_pool);
	    vpi_mcd_printf(1, "    %8lu thread schedule events\n",
		    count_thread_events);
	    vpi_mcd_printf(1, "    %8lu assign events\n",
		    count_assign_events);
	    vpi_mcd_printf(1, "    %8lu other events\n",
		    count_gen_events);
      }

      return vvp_return_value;
}
