/* Rts/Sys/stats.c.
 * Larceny run-time system -- run-time statistics.
 *
 * $Id: stats.c,v 1.10 1997/02/11 19:48:21 lth Exp lth $
 *
 * The stats module maintains run-time statistics.  Mainly, these are
 * statistics on memory use (bytes allocated and collected, amount of 
 * memory allocated to which parts the RTS, and so on) and running time
 * statistics (time spent in collector, time spent in any specific collector,
 * and so on).
 *
 * Statistics are calculated in two ways.  First, some parts of the RTS
 * are assumed to call the statistics module to signal events; for example,
 * the garbage collector calls the following callbacks:
 *
 *    stats_before_gc()      signals the start of a collection
 *    stats_gc_type()        reports the generation number and type
 *    stats_after_gc()       signals the end of a collection
 *
 * Currently, the listed procedures are the only such callbacks.
 * Second, procedures in the stats module will call on procedures
 * in other parts of the RTS (notably gc->stats() and gclib_stats()) to
 * obtain information about those parts.
 *
 * Scheme code makes a callout to UNIX_getresourceusage(), passing a Scheme
 * vector that is to be filled with resource data.  That call eventually
 * ends up in stats_fillvector(), which performs the filling.
 *
 * There are some static data in this file, and in the longer term
 * a better solution must be found.  The statistics data don't really
 * belong with the collector, as their scope extends beyond the collector.
 * Probably, what should be done is create a 'statistics' ADT (object)
 * that can be passed around as needed, and which will contain all the
 * static data.
 *
 * Still to be done:
 * - record/report remembered set memory usage statistics
 * - record/report number of times a remset was cleared
 * - record/report overall memory usage statistics
 */

#include <sys/time.h>

#ifdef SUNOS
/* For rusage() et al. */
#include <sys/resource.h>
#endif

#ifdef SOLARIS
/* For rusage() et al, which are obsolete. */
#include <sys/resource.h>
#include <sys/rusage.h>
#endif

#include <stdio.h>
#include <memory.h>
#include "larceny.h"
#include "macros.h"
#include "cdefs.h"
#include "gc.h"
#include "memmgr.h"
#include "gclib.h"
#include "assert.h"

/* These are hardwired limits in the system. They should possibly be 
 * defined somewhere else.
 */
#define MAX_GENERATIONS 32

typedef struct {
  word collections;      /* collections in this heap */
  word promotions;       /* promotions into this heap */
  word gctime;           /* total gc time in ms */
} gen_stat_t;

typedef struct {
  word hscanned_hi;      /* hash table */
  word hscanned_lo;      /*   entries scanned */
  word hrecorded_hi;     /* hash table */
  word hrecorded_lo;     /*   entries removed */
  word hremoved_hi;      /* hash table */
  word hremoved_lo;      /*   entries removed */
  word ssbrecorded_hi;   /* SSB transactions */
  word ssbrecorded_lo;   /*   recorded */
  word wscanned_hi;      /* Words of old */
  word wscanned_lo;      /*   objects scanned */
  word cleared;          /* Number of times remset was cleared */
} rem_stat_t;

/* The structure of type sys_stat_t keeps statistics information both
 * running and at the present time.
 * All numbers in this structure are represented as fixnums.
 */

typedef struct {
  /* GC stuff - overall */
  word wallocated_hi;      /* total words */
  word wallocated_lo;      /*  allocated */
  word wcollected_hi;      /* total words */
  word wcollected_lo;      /*  reclaimed */
  word wcopied_hi;         /* total words */
  word wcopied_lo;         /*  copied */
  word gctime;             /* total milliseconds GC time */

  /* GC stuff - overall (preliminary) */
  gen_stat_t gen_stat[MAX_GENERATIONS]; /* Information about each generation */

  /* GC stuff - snapshot */
  word wlive;              /* current words live */
  word lastcollection_gen; /* generation of last collection */
  word lastcollection_type;/* type of last collection */

  /* Remembered sets - overall (preliminary) */
  rem_stat_t rem_stat[MAX_GENERATIONS]; /* info about each remset */
  word hscanned_hi;        /* hash table */
  word hscanned_lo;        /*   entries scanned */
  word hrecorded_hi;       /* hash table */
  word hrecorded_lo;       /*   entries recorded */
  word ssbrecorded_hi;     /* SSB entries */
  word ssbrecorded_lo;     /*   recorded */
  word remset_cleared;     /* number of times remsets cleared */
  
  /* Stacks - overall */
  word fflushed_hi;        /* number of stack */
  word fflushed_lo;        /*   frames flushed */
  word wflushed_hi;        /* words of stack */
  word wflushed_lo;        /*   frames flushed or copied */
  word stacks_created;     /* number of stacks created */
  word frestored_hi;       /* number of stack */
  word frestored_lo;       /*   frames restored */

  /* RTS memory use - snapshot */
  word wallocated_heap;    /* words allocated to heap areas */
  word wallocated_remset;  /* words allocated to remembered sets */
  word wallocated_rts;     /* words allocated to RTS "other" */

#if SIMULATE_NEW_BARRIER
  simulated_barrier_stats_t swb;   /* simulated barrier statistics */
#endif
} sys_stat_t;


static sys_stat_t   memstats;             /* statistics */
static unsigned     rtstart;              /* time at startup (base time) */
static int          generations;          /* number of generations in gc */

static unsigned     time_before_gc;       /* timestamp when starting gc */
static unsigned     live_before_gc;       /* live when starting a gc */
static heap_stats_t *heapstats_before_gc; /* stats before gc */
static heap_stats_t *heapstats_after_gc;  /* stats after last gc */
static gc_t *gc;                          /* The garbage collector */

static FILE *dumpfile = 0;                /* For stats dump */

static void current_statistics( heap_stats_t *, sys_stat_t * );
static heap_stats_t *make_heapstats( heap_stats_t *base );
static void add( unsigned *hi, unsigned *lo, unsigned x );
static void print_heapstats( heap_stats_t *stats );
static void dump_stats( heap_stats_t *stats, sys_stat_t *ms );


/* Initialize the stats module.
 *
 * 'generations' is the number of generations in the gc.
 * 'show_heapstats' is 1 if heap statistics should be printed at startup.
 */
void
stats_init( gc_t *collector, int gen, int show_heapstats )
{
  int i;

  assert( gen <= MAX_GENERATIONS );

  gc = collector;
  rtstart = stats_rtclock();
  generations = gen;

  heapstats_before_gc = make_heapstats( 0 );
  heapstats_after_gc  = make_heapstats( 0 );
  memset( &memstats, 0, sizeof( memstats ) );

  current_statistics( heapstats_after_gc, &memstats );

  if (show_heapstats) 
    print_heapstats( heapstats_after_gc );
}


/* Collect statistics in anticipation of a garbage collection.
 *
 * Called by the memory manager at the very beginning of a collection.
 */
void stats_before_gc( void )
{
  word allocated;

  time_before_gc = stats_rtclock();

  current_statistics( heapstats_before_gc, &memstats );
  live_before_gc = nativeint( memstats.wlive );

  allocated = heapstats_before_gc[0].live-heapstats_after_gc[0].live;
  add( &memstats.wallocated_hi, &memstats.wallocated_lo,
      fixnum( allocated / sizeof(word) ) );
}


/* Record collection type.
 *
 * Called by the actual collector code when the type and location of a 
 * collection has been determined.
 *
 * 'generation' is the generation number doing the copying.
 * 'type' is STATS_PROMOTE if it's a promotion, STATS_COLLECT if 
 * it's a collection.
 */
void stats_gc_type( int generation, stats_gc_t type )
{
  /* Just record the values; stats_gc_type may be called multiple times
     during a collection, and only the last set of values should be
     remembered.
   */
  memstats.lastcollection_gen = fixnum(generation);
  memstats.lastcollection_type = fixnum((word)type);
}


/* Wrap up statistics gathering after a garbage collection.
 */
void stats_after_gc( void )
{
  unsigned gen;
  unsigned time;

  if (memstats.lastcollection_type == fixnum(STATS_COLLECT))
    memstats.gen_stat[nativeint(memstats.lastcollection_gen)].collections
      += fixnum(1);
  else
    memstats.gen_stat[nativeint(memstats.lastcollection_gen)].promotions
      += fixnum(1);

  current_statistics( heapstats_after_gc, &memstats );
  time = stats_rtclock() - time_before_gc;

  add( &memstats.wcollected_hi, &memstats.wcollected_lo,
       fixnum( live_before_gc - nativeint(memstats.wlive) ) );

  gen = nativeint(memstats.lastcollection_gen);
  if (nativeint(memstats.lastcollection_type) == STATS_COLLECT)
    add( &memstats.wcopied_hi, &memstats.wcopied_lo,
	fixnum(heapstats_after_gc[gen].live) );
  else
    add( &memstats.wcopied_hi, &memstats.wcopied_lo,
	fixnum(heapstats_after_gc[gen].live-heapstats_before_gc[gen].live) );

  memstats.gctime += fixnum( time );
  memstats.gen_stat[gen].gctime += fixnum( time );

  dump_stats( heapstats_after_gc, &memstats );
}


unsigned stats_rtclock( void )
{
  struct timeval t;
  struct timezone tz;

  if (gettimeofday( &t, &tz ) == -1)
    return -1;
  return (t.tv_sec * 1000 + t.tv_usec / 1000) - rtstart;
}


word stats_fillvector(void)
{
  static heap_stats_t *hs = 0;

  struct rusage buf;
  unsigned usertime, systime, minflt, majflt, size;
  word allocated;
  sys_stat_t ms;
  word *vp, *p, *genv, *gv, *remv, *rv, *gv_base, *rv_base;
  int i;
  
  if (hs == 0) hs = make_heapstats( 0 );
  current_statistics( hs, &memstats );

  /* Now work on a temporary structure! */
  ms = memstats;
  allocated = hs[0].live-heapstats_after_gc[0].live;
  add( &ms.wallocated_hi, &ms.wallocated_lo, fixnum(allocated/sizeof(word)));

  getrusage( RUSAGE_SELF, &buf );

#ifdef SUNOS
  systime = fixnum( buf.ru_stime.tv_sec * 1000 + buf.ru_stime.tv_usec / 1000);
  usertime = fixnum( buf.ru_utime.tv_sec * 1000 + buf.ru_utime.tv_usec / 1000);
#endif

#ifdef SOLARIS
  systime = fixnum( buf.ru_stime.tv_sec*1000 + buf.ru_stime.tv_nsec/1000000);
  usertime = fixnum( buf.ru_utime.tv_sec*1000 + buf.ru_utime.tv_nsec/1000000);
#endif

  majflt = fixnum( buf.ru_majflt );
  minflt = fixnum( buf.ru_minflt );

  /* Allocate memory for the statistics vector
   *
   * We need the following memory:
   * - roundup2(STAT_VSIZE+1) words for the main vector
   * - roundup2(generations+1) for the vector to hold generation vectors
   * - roundup2(STAT_G_SIZE+1)*generations words for the generations
   * - roundup2(generations-1+1) for the vector to hold remset vectors
   * - roundup2(STAT_R_SIZE+1)*(generations-1) words for the remsets
   */
  size = roundup8((roundup2( STAT_VSIZE+1 ) +
		   roundup2( generations+1 ) +
		   roundup2( STAT_G_SIZE+1 )*generations +
		   roundup2( generations-1+1 ) +
		   roundup2( STAT_R_SIZE+1 )*(generations-1)
		   ) * sizeof(word) );
  p = alloc_from_heap( size );
  memset( (void*)p, 0, size );

  /* First initialize the main vector */
  *p = mkheader( STAT_VSIZE*sizeof(word), VECTOR_HDR );
  vp = p+1;

  vp[ STAT_WALLOCATED_HI ] = ms.wallocated_hi;
  vp[ STAT_WALLOCATED_LO ] = ms.wallocated_lo;
  vp[ STAT_WCOLLECTED_HI ] = ms.wcollected_hi;
  vp[ STAT_WCOLLECTED_LO ] = ms.wcollected_lo;
  vp[ STAT_WCOPIED_HI ]    = ms.wcopied_hi;
  vp[ STAT_WCOPIED_LO ]    = ms.wcopied_lo;
  vp[ STAT_GCTIME ]        = ms.gctime;
  vp[ STAT_WLIVE ]         = ms.wlive;
  vp[ STAT_LAST_GEN ]      = ms.lastcollection_gen;
  vp[ STAT_LAST_TYPE ]     = (ms.lastcollection_type == STATS_COLLECT 
			       ? fixnum(0) 
			       : fixnum(1));

  /* skipping generation info for now; see below */
  /* skipping remset info for now; see below */

  vp[ STAT_FFLUSHED_HI ]   = ms.fflushed_hi;
  vp[ STAT_FFLUSHED_LO ]   = ms.fflushed_lo;
  vp[ STAT_WFLUSHED_HI ]   = ms.wflushed_hi;
  vp[ STAT_WFLUSHED_LO ]   = ms.wflushed_lo;
  vp[ STAT_STK_CREATED ]   = ms.stacks_created;
  vp[ STAT_FRESTORED_HI ]  = ms.frestored_hi;
  vp[ STAT_FRESTORED_LO ]  = ms.frestored_lo;
  vp[ STAT_WORDS_HEAP ]    = ms.wallocated_heap;
  vp[ STAT_WORDS_REMSET ]  = ms.wallocated_remset;
  vp[ STAT_WORDS_RTS ]     = ms.wallocated_rts;

#if SIMULATE_NEW_BARRIER
  /* If we're not simulating the new barrier, the values will be 0 */
  vp[ STAT_SWB_ASSIGN ] = ms.swb.array_assignments;
  vp[ STAT_SWB_LHS_OK ] = ms.swb.lhs_young_or_remembered;
  vp[ STAT_SWB_RHS_CONST ] = ms.swb.rhs_constant;
  vp[ STAT_SWB_NOTXGEN ] = ms.swb.cross_gen_check;
  vp[ STAT_SWB_TRANS ] = ms.swb.transactions;
#endif

  vp[ STAT_RTIME ]         = fixnum( stats_rtclock() );
  vp[ STAT_STIME ]         = systime;
  vp[ STAT_UTIME ]         = usertime;
  vp[ STAT_MINFAULTS ]     = minflt;
  vp[ STAT_MAJFAULTS ]     = majflt;

  /* Now do generations */
  genv = p + roundup2( STAT_VSIZE + 1 ); /* points to generation metavector */
  *genv = mkheader( generations*sizeof(word), VECTOR_HDR );
  vp[ STAT_GENERATIONS ] = tagptr( genv, VEC_TAG );

  gv_base = genv + roundup2( generations+1 );
  for ( i=0 ; i < generations ; i++ ) {
    /* calculate offst of generation vector #i */
    gv = gv_base + i*roundup2( STAT_G_SIZE+1 );
    genv[ i+1 ] = tagptr( gv, VEC_TAG );
    *gv = mkheader( STAT_G_SIZE*sizeof(word), VECTOR_HDR );
    
    gv[ STAT_G_GC_COUNT+1 ] = ms.gen_stat[ i ].collections;
    gv[ STAT_G_PROM_COUNT+1 ] = ms.gen_stat[ i ].promotions;
    gv[ STAT_G_GCTIME+1 ] = ms.gen_stat[ i ].gctime;
    gv[ STAT_G_WLIVE+1 ] = fixnum(hs[i].live/4);
  }

  /* Now do remembered sets */
  remv = p + roundup2( STAT_VSIZE + 1 )
	   + roundup2( generations+1 )
           + roundup2( STAT_G_SIZE+1 )*generations;
  *remv = mkheader( (generations-1)*sizeof(word), VECTOR_HDR );
  vp[ STAT_REMSETS ] = tagptr( remv, VEC_TAG );
  rv_base = remv + roundup2( generations-1+1 );
  for ( i = 1 ; i < generations ; i++ ) {
    rv = rv_base + (i-1)*roundup2( STAT_R_SIZE+1 );
    remv[i] = tagptr( rv, VEC_TAG );
    *rv = mkheader( STAT_R_SIZE*sizeof(word), VECTOR_HDR );

    rv[ STAT_R_APOOL+1 ] = 0;  /* FIXME */
    rv[ STAT_R_UPOOL+1 ] = 0;  /* FIXME */
    rv[ STAT_R_AHASH+1 ] = 0;  /* FIXME */
    rv[ STAT_R_UHASH+1 ] = 0;  /* FIXME */
    rv[ STAT_R_HREC_HI+1 ] = ms.rem_stat[i].hrecorded_hi;
    rv[ STAT_R_HREC_LO+1 ] = ms.rem_stat[i].hrecorded_lo;
    rv[ STAT_R_HREM_HI+1 ] = ms.rem_stat[i].hremoved_hi;
    rv[ STAT_R_HREM_LO+1 ] = ms.rem_stat[i].hremoved_lo;
    rv[ STAT_R_HSCAN_HI+1 ] = ms.rem_stat[i].hscanned_hi;
    rv[ STAT_R_HSCAN_LO+1 ] = ms.rem_stat[i].hscanned_lo;
    rv[ STAT_R_WSCAN_HI+1 ] = ms.rem_stat[i].wscanned_hi;
    rv[ STAT_R_WSCAN_LO+1 ] = ms.rem_stat[i].wscanned_lo;
    rv[ STAT_R_SSBREC_HI+1 ] = ms.rem_stat[i].ssbrecorded_hi;
    rv[ STAT_R_SSBREC_LO+1 ] = ms.rem_stat[i].ssbrecorded_lo;

  }
  return tagptr( p, VEC_TAG );
}


static void
print_heapstats( heap_stats_t *stats )
{
  int i;

  consolemsg( "Heap statistics:");
  for ( i=0; i < generations ; i++ ) {
    consolemsg( "Generation %d", i );
    consolemsg( "  Size of semispace 1: %lu bytes", stats[i].semispace1 );
    consolemsg( "  Size of semispace 2: %lu bytes", stats[i].semispace2 );
    consolemsg( "  Live data: %lu bytes", stats[i].live );
    if (stats[i].stack || i==0)
      consolemsg( "  Live stack: %lu bytes", stats[i].stack );
    /* FIXME: also: text/static area, if there's one. */
  }
}


/* Dump the stats to an output stream, on an s-expression format.
 * Format: see the banner string below.
 */
static char *dump_banner =
"; RTS statistics, dumped by Larceny version %s\n;\n"
"; The output is an s-expression, one following each GC.  When some entry\n"
"; is advertised as xxx-hi and xxx-lo, then the -hi is the high 29 bits\n"
"; and the -lo is the low 29 bits of a 58-bit unsigned integer.\n"
"; All times are in milliseconds.\n;\n"
"; First there is some general information (running totals):\n"
";\n"
";  (words-allocated-hi words-allocated-lo\n"
";   words-reclaimed-hi words-reclaimed-lo\n"
";   words-copied-hi words-copied-lo\n"
";   gctime\n"
";\n"
"; The next entries are snapshots at the last GC; the gc type\n"
"; is a symbol, \"collect\" or \"promote\":\n"
";\n"
";   words-live generation-of-last-gc type-of-last-gc\n"
";\n"
"; Then follows a list of sublists of generation info,\n"
"; ordered from younger to older; words-live is snapshot data:\n"
";\n"
";   ((collections promotions-into gctime words-live) ...)\n"
";\n"
"; Then follows a list of sublists of remembered set info,\n"
"; ordered from younger to older (each remset is associated\n"
"; with one old generation).  First the snapshot entries:\n"
";\n"
";   ((words-allocated-to-pool words-used-in-pool\n"
";     hash-table-total-entries hash-table-non-null-entries\n"
";\n"
"; Then there are running totals:\n"
";\n"
";     hash-entries-recorded-hi hash-entries-recorded-lo\n"
";     hash-entries-removed-hi hash-entries-recorded-lo\n"
";     hash-entries-scanned-hi hash-entries-scanned-lo\n"
";     words-of-oldspace-scanned-hi words-of-oldspace-scanned-lo\n"
";     ssb-entries-recorded-hi ssb-entries-recorded-lo\n"
";    )...)\n"
";\n"
"; Then follows information about the stack cache (running totals):\n"
";\n"
";   frames-flushed-hi frames-flushed-lo words-flushed-hi words-flushed-lo\n"
";   stacks-created frames-restored-hi frames-restored-lo\n"
";\n"
"; Then there is information about overall memory use (snapshots):\n"
";\n"
";   words-allocated-heap words-allocated-remset words-allocated-rts-other\n"
#if SIMULATE_NEW_BARRIER
";\n"
"; Then there is the simulated write barrier profile:\n;\n"
";   array-assignments\n"
";   lhs-young-or-remembered rhs-constant not-cross-gen\n"
";   array-transactions\n"
#endif
";  )\n";

int stats_opendump( const char *filename )
{
  if (dumpfile != 0) stats_closedump();

  dumpfile = fopen( filename, "w" );

  if (dumpfile)
    fprintf( dumpfile, dump_banner, version );
  return dumpfile != 0;
}

void stats_closedump( void )
{
  if (dumpfile == 0) return;

  fclose( dumpfile );
  dumpfile = 0;
}

static void
dump_stats( heap_stats_t *stats, sys_stat_t *ms )
{
  int i;

  if (dumpfile == 0) return;

  fprintf( dumpfile, "(" );

  /* Print overall memory and GC information */
  fprintf( dumpfile, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %s ",
	   nativeint( ms->wallocated_hi ),
	   nativeint( ms->wallocated_lo ),
	   nativeint( ms->wcollected_hi ),
	   nativeint( ms->wcollected_lo ),
	   nativeint( ms->wcopied_hi ),
	   nativeint( ms->wcopied_lo ),
	   nativeint( ms->gctime ),
	   nativeint( ms->wlive ),
	   nativeint( ms->lastcollection_gen ),
	   nativeint( ms->lastcollection_type ) == STATS_COLLECT 
	     ? "collect"
	     : "promote" );

  /* Print generations information */
  fprintf( dumpfile, "(" );
  for ( i=0 ; i < generations ; i++ ) {
    fprintf( dumpfile, "(%lu %lu %lu %lu) ",
	     nativeint( ms->gen_stat[i].collections ),
	     nativeint( ms->gen_stat[i].promotions ),
	     nativeint( ms->gen_stat[i].gctime ),
	     stats[i].live/4 );
  }
  fprintf( dumpfile, ") " );

  /* Print remembered sets information */
  fprintf( dumpfile, "(" );
  for ( i = 1 ; i < generations ; i++ ) {
    fprintf( dumpfile, "(%lu %lu %lu %lu ",
	     0, /* FIXME */
	     0, /* FIXME */
	     0, /* FIXME */
	     0  /* FIXME */
	    );
    fprintf( dumpfile, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu) ",
	     nativeint( ms->rem_stat[i].hrecorded_hi ),
	     nativeint( ms->rem_stat[i].hrecorded_lo ),
	     nativeint( ms->rem_stat[i].hremoved_hi ),
	     nativeint( ms->rem_stat[i].hremoved_lo ),
	     nativeint( ms->rem_stat[i].hscanned_hi ),
	     nativeint( ms->rem_stat[i].hscanned_lo ),
	     nativeint( ms->rem_stat[i].wscanned_hi ),
	     nativeint( ms->rem_stat[i].wscanned_lo ),
	     nativeint( ms->rem_stat[i].ssbrecorded_hi ),
	     nativeint( ms->rem_stat[i].ssbrecorded_lo ) );
  }
  fprintf( dumpfile, ") " );

  /* Print stack information */
  fprintf( dumpfile, "%lu %lu %lu %lu %lu %lu %lu ",
	   nativeint( ms->fflushed_hi ), 
	   nativeint( ms->fflushed_lo ),
	   nativeint( ms->wflushed_hi ),
	   nativeint( ms->wflushed_lo ),
	   nativeint( ms->stacks_created ),
	   nativeint( ms->frestored_hi ),
	   nativeint( ms->frestored_lo ) );

  /* Print overal heap information */
  fprintf( dumpfile, "%lu %lu %lu ",
	   nativeint( ms->wallocated_heap ),
	   nativeint( ms->wallocated_remset ),
	   nativeint( ms->wallocated_rts ) );

#if SIMULATE_NEW_BARRIER
  fprintf( dumpfile, "%lu %lu %lu %lu %lu",
	   nativeint( ms->swb.array_assignments ),
	   nativeint( ms->swb.lhs_young_or_remembered ),
	   nativeint( ms->swb.rhs_constant ),
	   nativeint( ms->swb.cross_gen_check ),
	   nativeint( ms->swb.transactions ) );
#endif

  fprintf( dumpfile, ")\n" );
}


/* Get the current heap statistics */

static void
current_statistics( heap_stats_t *stats, sys_stat_t *ms )
{
  int i;
  word bytes_live = 0;
  word ssb_recorded = 0;
  word hash_recorded = 0;
  word hash_scanned = 0;
  word frames_flushed = 0;
  word frames_restored = 0;
  word bytes_flushed = 0;
  word stacks_created = 0;

  for ( i=0 ; i < generations ; i++ ) {
    gc->stats( gc, i, &stats[i] );
    bytes_live += stats[i].live;
    ssb_recorded += stats[i].ssb_recorded;
    hash_recorded += stats[i].hash_recorded;
    hash_scanned += stats[i].hash_scanned;
    frames_flushed += stats[i].frames_flushed;
    bytes_flushed += stats[i].bytes_flushed;
    stacks_created += stats[i].stacks_created;
    frames_restored += stats[i].frames_restored;
    add( &ms->rem_stat[i].hrecorded_hi, &ms->rem_stat[i].hrecorded_lo,
	 fixnum( stats[i].hash_recorded ));
    add( &ms->rem_stat[i].hscanned_hi, &ms->rem_stat[i].hscanned_lo,
	 fixnum( stats[i].hash_scanned ));
    add( &ms->rem_stat[i].hremoved_hi, &ms->rem_stat[i].hremoved_lo,
	 fixnum( stats[i].hash_removed ));
    add( &ms->rem_stat[i].ssbrecorded_hi, &ms->rem_stat[i].ssbrecorded_lo,
	 fixnum( stats[i].ssb_recorded ));
    add( &ms->rem_stat[i].wscanned_hi, &ms->rem_stat[i].wscanned_lo,
	 fixnum( stats[i].words_scanned ));
    ms->rem_stat[i].cleared += fixnum(0);  /* FIXME */
  }

  /* Overall statistics */
  ms->wlive = fixnum(bytes_live / sizeof(word));
  add( &ms->ssbrecorded_hi, &ms->ssbrecorded_lo, fixnum( ssb_recorded ) );
  add( &ms->hrecorded_hi, &ms->hrecorded_lo, fixnum( hash_recorded ) );
  add( &ms->hscanned_hi, &ms->hscanned_lo, fixnum( hash_scanned ) );
  add( &ms->fflushed_hi, &ms->fflushed_lo, fixnum( frames_flushed ) );
  add( &ms->wflushed_hi, &ms->wflushed_lo, fixnum(bytes_flushed/sizeof(word)));
  add( &ms->frestored_hi, &ms->frestored_lo, fixnum( frames_restored ) );
  ms->stacks_created += fixnum( stacks_created );
  gclib_stats( &ms->wallocated_heap,
	       &ms->wallocated_remset, 
	       &ms->wallocated_rts );

#if SIMULATE_NEW_BARRIER
  { simulated_barrier_stats_t s;
    simulated_barrier_stats( &s );
    ms->swb.array_assignments += fixnum(s.array_assignments);
    ms->swb.lhs_young_or_remembered += fixnum(s.lhs_young_or_remembered);
    ms->swb.rhs_constant += fixnum(s.rhs_constant);
    ms->swb.cross_gen_check += fixnum(s.cross_gen_check);
    ms->swb.transactions += fixnum(s.transactions);
  }
#endif
}

static heap_stats_t *
make_heapstats( heap_stats_t *base )
{
  heap_stats_t *p;

 again:
  p = (heap_stats_t*)malloc( generations*sizeof(heap_stats_t) );
  if (p == 0) {
    memfail( MF_MALLOC, "can't allocate heap statistics metadata." );
    goto again;
  }
  if (base == 0)
    memset( p, 0, sizeof( heap_stats_t )*generations );
  else
    memcpy( p, base, sizeof( heap_stats_t )*generations );
  return p;
}

/* Adds a word to a doubleword with carry propagation, both parts of
 * the doubleword are independently represented as fixnums, as is 'x'.
 */

#define LARGEST_FIXNUM (2147483644L)  /* ((2^29)-1)*4 */

static void add( unsigned *hi, unsigned *lo, unsigned x )
{
  assert((x & 3) == 0);
  *lo += x;
  if (*lo > LARGEST_FIXNUM) {
    printf( "OVERFLOW IN ADD\n" );
    *lo -= LARGEST_FIXNUM;
    *hi += 4;
  }
}


/* eof */
