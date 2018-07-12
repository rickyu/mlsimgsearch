/* Copyright 2000-2008 MySQL AB, 2008 Sun Microsystems, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */



//#include "mysql_priv.h"


#include "log_event.h"
//#include "rpl_utility.h"
#define log_cs	&my_charset_latin1

#define FLAGSTR(V,F) ((V)&(F)?#F" ":"")


/*
  Size of buffer for printing a double in format %.<PREC>g

  optional '-' + optional zero + '.'  + PREC digits + 'e' + sign +
  exponent digits + '\0'
*/
#define FMT_G_BUFSIZE(PREC) (3 + (PREC) + 5 + 1)




#ifndef DBUG_OFF
uint debug_not_change_ts_if_art_event= 1; // bug#29309 simulation
#endif




/**************************************************************************
	Log_event methods (= the parent class of all events)
**************************************************************************/

/**
  @return
  returns the human readable name of the event's type
*/

const char* Log_event::get_type_str(Log_event_type type)
{
  switch(type) {
  case START_EVENT_V3:  return "Start_v3";
  case STOP_EVENT:   return "Stop";
  case QUERY_EVENT:  return "Query";
  case ROTATE_EVENT: return "Rotate";
  case INTVAR_EVENT: return "Intvar";
  case LOAD_EVENT:   return "Load";
  case NEW_LOAD_EVENT:   return "New_load";
  case SLAVE_EVENT:  return "Slave";
  case CREATE_FILE_EVENT: return "Create_file";
  case APPEND_BLOCK_EVENT: return "Append_block";
  case DELETE_FILE_EVENT: return "Delete_file";
  case EXEC_LOAD_EVENT: return "Exec_load";
  case RAND_EVENT: return "RAND";
  case XID_EVENT: return "Xid";
  case USER_VAR_EVENT: return "User var";
  case FORMAT_DESCRIPTION_EVENT: return "Format_desc";
  case TABLE_MAP_EVENT: return "Table_map";
  case PRE_GA_WRITE_ROWS_EVENT: return "Write_rows_event_old";
  case PRE_GA_UPDATE_ROWS_EVENT: return "Update_rows_event_old";
  case PRE_GA_DELETE_ROWS_EVENT: return "Delete_rows_event_old";
  case WRITE_ROWS_EVENT: return "Write_rows";
  case UPDATE_ROWS_EVENT: return "Update_rows";
  case DELETE_ROWS_EVENT: return "Delete_rows";
  case BEGIN_LOAD_QUERY_EVENT: return "Begin_load_query";
  case EXECUTE_LOAD_QUERY_EVENT: return "Execute_load_query";
  case INCIDENT_EVENT: return "Incident";
  default: return "Unknown";				/* impossible */
  }
}

const char* Log_event::get_type_str()
{
  return get_type_str(get_type_code());
}


/*
  Log_event::Log_event()
*/

/*
  Log_event::Log_event()
*/

Log_event::Log_event(const char* buf,
                     const Format_description_log_event* description_event)
  :temp_buf(0), cache_stmt(0)
{
  when = uint4korr(buf);
  server_id = uint4korr(buf + SERVER_ID_OFFSET);
  data_written= uint4korr(buf + EVENT_LEN_OFFSET);
  if (description_event->binlog_version==1)
  {
    log_pos= 0;
    flags= 0;
    return;
  }
  /* 4.0 or newer */
  log_pos= uint4korr(buf + LOG_POS_OFFSET);
  /*
    If the log is 4.0 (so here it can only be a 4.0 relay log read by
    the SQL thread or a 4.0 master binlog read by the I/O thread),
    log_pos is the beginning of the event: we transform it into the end
    of the event, which is more useful.
    But how do you know that the log is 4.0: you know it if
    description_event is version 3 *and* you are not reading a
    Format_desc (remember that mysqlbinlog starts by assuming that 5.0
    logs are in 4.0 format, until it finds a Format_desc).
  */
  if (description_event->binlog_version==3 &&
      buf[EVENT_TYPE_OFFSET]<FORMAT_DESCRIPTION_EVENT && log_pos)
  {
      /*
        If log_pos=0, don't change it. log_pos==0 is a marker to mean
        "don't change rli->group_master_log_pos" (see
        inc_group_relay_log_pos()). As it is unreal log_pos, adding the
        event len's is nonsense. For example, a fake Rotate event should
        not have its log_pos (which is 0) changed or it will modify
        Exec_master_log_pos in SHOW SLAVE STATUS, displaying a nonsense
        value of (a non-zero offset which does not exist in the master's
        binlog, so which will cause problems if the user uses this value
        in CHANGE MASTER).
      */
    log_pos+= data_written; /* purecov: inspected */
  }
  DBUG_PRINT("info", ("log_pos: %lu", (ulong) log_pos));

  flags= uint2korr(buf + FLAGS_OFFSET);
  if ((buf[EVENT_TYPE_OFFSET] == FORMAT_DESCRIPTION_EVENT) ||
      (buf[EVENT_TYPE_OFFSET] == ROTATE_EVENT))
  {
    /*
      These events always have a header which stops here (i.e. their
      header is FROZEN).
    */
    /*
      Initialization to zero of all other Log_event members as they're
      not specified. Currently there are no such members; in the future
      there will be an event UID (but Format_description and Rotate
      don't need this UID, as they are not propagated through
      --log-slave-updates (remember the UID is used to not play a query
      twice when you have two masters which are slaves of a 3rd master).
      Then we are done.
    */
    return;
  }
  /* otherwise, go on with reading the header from buf (nothing now) */
}



Log_event* Log_event::read_log_event(FILE* fp,
                                     const Format_description_log_event
                                     *description_event)
{
   char head[LOG_EVENT_MINIMAL_HEADER_LEN];
  /*
    First we only want to read at most LOG_EVENT_MINIMAL_HEADER_LEN, just to
    check the event for sanity and to know its length; no need to really parse
    it. We say "at most" because this could be a 3.23 master, which has header
    of 13 bytes, whereas LOG_EVENT_MINIMAL_HEADER_LEN is 19 bytes (it's
    "minimal" over the set {MySQL >=4.0}).
  */
  uint header_size= min(description_event->common_header_len,
                        LOG_EVENT_MINIMAL_HEADER_LEN);

  if (fread((uchar *) head, 1,header_size,fp) != header_size)
  {
    /*
      No error here; it could be that we are at the file's end. However
      if the next my_b_read() fails (below), it will be an error as we
      were able to read the first bytes.
    */
    DBUG_RETURN(0);
  }
  uint data_len = uint4korr(head + EVENT_LEN_OFFSET);
  char *buf= 0;
  const char *error= 0;
  Log_event *res=  0;


  if (data_len < header_size)
  {
    error = "Event too small";
    goto err;
  }

  // some events use the extra byte to null-terminate strings
  if (!(buf = (char*) my_malloc(data_len+1, MYF(MY_WME))))
  {
    error = "Out of memory";
    goto err;
  }
  buf[data_len] = 0;
  memcpy(buf, head, header_size);
  if (fread((uchar*) buf + header_size, 1,data_len - header_size,fp) != data_len - header_size)
  {
    error = "read error";
    goto err;
  }
  if ((res= read_log_event(buf, data_len, &error, description_event)))
    res->register_temp_buf(buf);

err:
  if (!res)
  {
    DBUG_ASSERT(error != 0);
    my_free(buf, MYF(MY_ALLOW_ZERO_PTR));
  }
  DBUG_RETURN(res);
}


/**
  Binlog format tolerance is in (buf, event_len, description_event)
  constructors.
*/

Log_event* Log_event::read_log_event(const char* buf, uint event_len,
				     const char **error,
                     const Format_description_log_event *description_event) {
  Log_event* ev;
  /* Check the integrity */
  if (event_len < EVENT_LEN_OFFSET ||
      buf[EVENT_TYPE_OFFSET] >= ENUM_END_EVENT ||
      (uint) event_len != uint4korr(buf+EVENT_LEN_OFFSET))
  {
    *error="Sanity check failed";		// Needed to free buffer
    DBUG_RETURN(NULL); // general sanity check - will fail on a partial read
  }

  uint event_type= buf[EVENT_TYPE_OFFSET];
  if (event_type > description_event->number_of_event_types &&
      event_type != FORMAT_DESCRIPTION_EVENT)
  {
    /*
      It is unsafe to use the description_event if its post_header_len
      array does not include the event type.
    */
    DBUG_PRINT("error", ("event type %d found, but the current "
                         "Format_description_log_event supports only %d event "
                         "types", event_type,
                         description_event->number_of_event_types));
    ev= NULL;
  }
  else
  {
    /*
      In some previuos versions (see comment in
      Format_description_log_event::Format_description_log_event(char*,...)),
      event types were assigned different id numbers than in the
      present version. In order to replicate from such versions to the
      present version, we must map those event type id's to our event
      type id's.  The mapping is done with the event_type_permutation
      array, which was set up when the Format_description_log_event
      was read.
    */
    if (description_event->event_type_permutation)
    {
      IF_DBUG({
          int new_event_type=
            description_event->event_type_permutation[event_type];
          DBUG_PRINT("info",
                     ("converting event type %d to %d (%s)",
                      event_type, new_event_type,
                      get_type_str((Log_event_type)new_event_type)));
        });
      event_type= description_event->event_type_permutation[event_type];
    }

    switch(event_type) {
      case FORMAT_DESCRIPTION_EVENT:
        ev = new Format_description_log_event(buf, event_len, description_event);
        break;
      case START_EVENT_V3: /* this is sent only by MySQL <=4.x */
        ev = new Start_log_event_v3(buf, description_event);
        break;
    case QUERY_EVENT:
      ev  = new Query_log_event(buf, event_len, description_event, QUERY_EVENT);
      break;
    case UPDATE_ROWS_EVENT:
    case DELETE_ROWS_EVENT:
    case WRITE_ROWS_EVENT:
      ev = new Rows_log_event(buf,event_len,(Log_event_type)event_type,description_event);
      break;
    case TABLE_MAP_EVENT:
      ev = new Table_map_log_event(buf,event_len,description_event);
      break;
    case ROTATE_EVENT:
      ev = new Rotate_log_event(buf,event_len,description_event);
      break;
    case LOAD_EVENT:
    case NEW_LOAD_EVENT:
    case SLAVE_EVENT: /* can never happen (unused event) */
    case CREATE_FILE_EVENT:
    case APPEND_BLOCK_EVENT:
    case DELETE_FILE_EVENT:
    case EXEC_LOAD_EVENT:
    case STOP_EVENT:
    case INTVAR_EVENT:
    case XID_EVENT:
    case RAND_EVENT:
    case USER_VAR_EVENT:
    case PRE_GA_WRITE_ROWS_EVENT:
    case PRE_GA_UPDATE_ROWS_EVENT:
    case PRE_GA_DELETE_ROWS_EVENT:
    case BEGIN_LOAD_QUERY_EVENT:
    case EXECUTE_LOAD_QUERY_EVENT:
    case INCIDENT_EVENT:
      ev = new Dumb_log_event(buf, event_len, description_event,(Log_event_type)event_type);
      break;
    default:
      DBUG_PRINT("error",("Unknown event code: %d",
                          (int) buf[EVENT_TYPE_OFFSET]));
      ev= NULL;
      break;
    }
  }

 /*
    is_valid() are small event-specific sanity tests which are
    important; for example there are some my_malloc() in constructors
    (e.g. Query_log_event::Query_log_event(char*...)); when these
    my_malloc() fail we can't return an error out of the constructor
    (because constructor is "void") ; so instead we leave the pointer we
    wanted to allocate (e.g. 'query') to 0 and we test it in is_valid().
    Same for Format_description_log_event, member 'post_header_len'.

    SLAVE_EVENT is never used, so it should not be read ever.
  */
  if (!ev || !ev->is_valid() || (event_type == SLAVE_EVENT))
  {
    DBUG_PRINT("error",("Found invalid event in binary log"));

    delete ev;
    *error= "Found invalid event in binary log";
    DBUG_RETURN(0);
  }
  DBUG_RETURN(ev);  
}

/**************************************************************************
	Start_log_event_v3 methods
**************************************************************************/
/*
  Start_log_event_v3::Start_log_event_v3()
*/

Start_log_event_v3::Start_log_event_v3(const char* buf,
                                       const Format_description_log_event
                                       *description_event)
  :Log_event(buf, description_event)
{
  buf+= description_event->common_header_len;
  binlog_version= uint2korr(buf+ST_BINLOG_VER_OFFSET);
  memcpy(server_version, buf+ST_SERVER_VER_OFFSET,
	 ST_SERVER_VER_LEN);
  // prevent overrun if log is corrupted on disk
  server_version[ST_SERVER_VER_LEN-1]= 0;
  created= uint4korr(buf+ST_CREATED_OFFSET);
  dont_set_created= 1;
}
/***************************************************************************
       Format_description_log_event methods
****************************************************************************/
/**
  Format_description_log_event 1st ctor.

    Ctor. Can be used to create the event to write to the binary log (when the
    server starts or when FLUSH LOGS), or to create artificial events to parse
    binlogs from MySQL 3.23 or 4.x.
    When in a client, only the 2nd use is possible.

  @param binlog_version         the binlog version for which we want to build
                                an event. Can be 1 (=MySQL 3.23), 3 (=4.0.x
                                x>=2 and 4.1) or 4 (MySQL 5.0). Note that the
                                old 4.0 (binlog version 2) is not supported;
                                it should not be used for replication with
                                5.0.
*/

Format_description_log_event::
Format_description_log_event(uint8 binlog_ver, const char* server_ver)
  :Start_log_event_v3(), event_type_permutation(0)
{
  binlog_version= binlog_ver;
  switch (binlog_ver) {
  case 4: /* MySQL 5.0 */
    strmov(server_version, server_ver ? server_ver : "5.0.34");
    //memcpy(server_version, ::server_version, ST_SERVER_VER_LEN);
    DBUG_EXECUTE_IF("pretend_version_50034_in_binlog",
                    strmov(server_version, "5.0.34"););
    common_header_len= LOG_EVENT_HEADER_LEN;
    number_of_event_types= LOG_EVENT_TYPES;
    /* we'll catch my_malloc() error in is_valid() */
    post_header_len=(uint8*) my_malloc(number_of_event_types*sizeof(uint8),
                                       MYF(0));

    /*
      This long list of assignments is not beautiful, but I see no way to
      make it nicer, as the right members are #defines, not array members, so
      it's impossible to write a loop.
    */
    if (post_header_len)
    {
      // Allows us to sanity-check that all events initialized their
      // events (see the end of this 'if' block).
      IF_DBUG(memset(post_header_len, 255,
                     number_of_event_types*sizeof(uint8)););

      /* Note: all event types must explicitly fill in their lengths here. */
      post_header_len[START_EVENT_V3-1]= START_V3_HEADER_LEN;
      post_header_len[QUERY_EVENT-1]= QUERY_HEADER_LEN;
      post_header_len[STOP_EVENT-1]= STOP_HEADER_LEN;
      post_header_len[ROTATE_EVENT-1]= ROTATE_HEADER_LEN;
      post_header_len[INTVAR_EVENT-1]= INTVAR_HEADER_LEN;
      post_header_len[LOAD_EVENT-1]= LOAD_HEADER_LEN;
      post_header_len[SLAVE_EVENT-1]= SLAVE_HEADER_LEN;
      post_header_len[CREATE_FILE_EVENT-1]= CREATE_FILE_HEADER_LEN;
      post_header_len[APPEND_BLOCK_EVENT-1]= APPEND_BLOCK_HEADER_LEN;
      post_header_len[EXEC_LOAD_EVENT-1]= EXEC_LOAD_HEADER_LEN;
      post_header_len[DELETE_FILE_EVENT-1]= DELETE_FILE_HEADER_LEN;
      post_header_len[NEW_LOAD_EVENT-1]= NEW_LOAD_HEADER_LEN;
      post_header_len[RAND_EVENT-1]= RAND_HEADER_LEN;
      post_header_len[USER_VAR_EVENT-1]= USER_VAR_HEADER_LEN;
      post_header_len[FORMAT_DESCRIPTION_EVENT-1]= FORMAT_DESCRIPTION_HEADER_LEN;
      post_header_len[XID_EVENT-1]= XID_HEADER_LEN;
      post_header_len[BEGIN_LOAD_QUERY_EVENT-1]= BEGIN_LOAD_QUERY_HEADER_LEN;
      post_header_len[EXECUTE_LOAD_QUERY_EVENT-1]= EXECUTE_LOAD_QUERY_HEADER_LEN;
      /*
        The PRE_GA events are never be written to any binlog, but
        their lengths are included in Format_description_log_event.
        Hence, we need to be assign some value here, to avoid reading
        uninitialized memory when the array is written to disk.
      */
      post_header_len[PRE_GA_WRITE_ROWS_EVENT-1] = 0;
      post_header_len[PRE_GA_UPDATE_ROWS_EVENT-1] = 0;
      post_header_len[PRE_GA_DELETE_ROWS_EVENT-1] = 0;

      post_header_len[TABLE_MAP_EVENT-1]=    TABLE_MAP_HEADER_LEN;
      post_header_len[WRITE_ROWS_EVENT-1]=   ROWS_HEADER_LEN;
      post_header_len[UPDATE_ROWS_EVENT-1]=  ROWS_HEADER_LEN;
      post_header_len[DELETE_ROWS_EVENT-1]=  ROWS_HEADER_LEN;
      /*
        We here have the possibility to simulate a master of before we changed
        the table map id to be stored in 6 bytes: when it was stored in 4
        bytes (=> post_header_len was 6). This is used to test backward
        compatibility.
        This code can be removed after a few months (today is Dec 21st 2005),
        when we know that the 4-byte masters are not deployed anymore (check
        with Tomas Ulin first!), and the accompanying test (rpl_row_4_bytes)
        too.
      */
      DBUG_EXECUTE_IF("old_row_based_repl_4_byte_map_id_master",
                      post_header_len[TABLE_MAP_EVENT-1]=
                      post_header_len[WRITE_ROWS_EVENT-1]=
                      post_header_len[UPDATE_ROWS_EVENT-1]=
                      post_header_len[DELETE_ROWS_EVENT-1]= 6;);
      post_header_len[INCIDENT_EVENT-1]= INCIDENT_HEADER_LEN;

      // Sanity-check that all post header lengths are initialized.
      IF_DBUG({
          int i;
          for (i=0; i<number_of_event_types; i++)
            assert(post_header_len[i] != 255);
        });
    }
    break;

  case 1: /* 3.23 */
  case 3: /* 4.0.x x>=2 */
    /*
      We build an artificial (i.e. not sent by the master) event, which
      describes what those old master versions send.
    */
    if (binlog_ver==1)
      strmov(server_version, server_ver ? server_ver : "3.23");
    else
      strmov(server_version, server_ver ? server_ver : "4.0");
    common_header_len= binlog_ver==1 ? OLD_HEADER_LEN :
      LOG_EVENT_MINIMAL_HEADER_LEN;
    /*
      The first new event in binlog version 4 is Format_desc. So any event type
      after that does not exist in older versions. We use the events known by
      version 3, even if version 1 had only a subset of them (this is not a
      problem: it uses a few bytes for nothing but unifies code; it does not
      make the slave detect less corruptions).
    */
    number_of_event_types= FORMAT_DESCRIPTION_EVENT - 1;
    post_header_len=(uint8*) my_malloc(number_of_event_types*sizeof(uint8),
                                       MYF(0));
    if (post_header_len)
    {
      post_header_len[START_EVENT_V3-1]= START_V3_HEADER_LEN;
      post_header_len[QUERY_EVENT-1]= QUERY_HEADER_MINIMAL_LEN;
      post_header_len[STOP_EVENT-1]= 0;
      post_header_len[ROTATE_EVENT-1]= (binlog_ver==1) ? 0 : ROTATE_HEADER_LEN;
      post_header_len[INTVAR_EVENT-1]= 0;
      post_header_len[LOAD_EVENT-1]= LOAD_HEADER_LEN;
      post_header_len[SLAVE_EVENT-1]= 0;
      post_header_len[CREATE_FILE_EVENT-1]= CREATE_FILE_HEADER_LEN;
      post_header_len[APPEND_BLOCK_EVENT-1]= APPEND_BLOCK_HEADER_LEN;
      post_header_len[EXEC_LOAD_EVENT-1]= EXEC_LOAD_HEADER_LEN;
      post_header_len[DELETE_FILE_EVENT-1]= DELETE_FILE_HEADER_LEN;
      post_header_len[NEW_LOAD_EVENT-1]= post_header_len[LOAD_EVENT-1];
      post_header_len[RAND_EVENT-1]= 0;
      post_header_len[USER_VAR_EVENT-1]= 0;
    }
    break;
  default: /* Includes binlog version 2 i.e. 4.0.x x<=1 */
    post_header_len= 0; /* will make is_valid() fail */
    break;
  }
  calc_server_version_split();
}

/**
  The problem with this constructor is that the fixed header may have a
  length different from this version, but we don't know this length as we
  have not read the Format_description_log_event which says it, yet. This
  length is in the post-header of the event, but we don't know where the
  post-header starts.

  So this type of event HAS to:
  - either have the header's length at the beginning (in the header, at a
  fixed position which will never be changed), not in the post-header. That
  would make the header be "shifted" compared to other events.
  - or have a header of size LOG_EVENT_MINIMAL_HEADER_LEN (19), in all future
  versions, so that we know for sure.

  I (Guilhem) chose the 2nd solution. Rotate has the same constraint (because
  it is sent before Format_description_log_event).
*/

Format_description_log_event::
Format_description_log_event(const char* buf,
                             uint event_len,
                             const
                             Format_description_log_event*
                             description_event)
  :Start_log_event_v3(buf, description_event), event_type_permutation(0)
{
  DBUG_ENTER("Format_description_log_event::Format_description_log_event(char*,...)");
  buf+= LOG_EVENT_MINIMAL_HEADER_LEN;
  if ((common_header_len=buf[ST_COMMON_HEADER_LEN_OFFSET]) < OLD_HEADER_LEN)
    DBUG_VOID_RETURN; /* sanity check */
  number_of_event_types=
    event_len-(LOG_EVENT_MINIMAL_HEADER_LEN+ST_COMMON_HEADER_LEN_OFFSET+1);
  DBUG_PRINT("info", ("common_header_len=%d number_of_event_types=%d",
                      common_header_len, number_of_event_types));
  /* If alloc fails, we'll detect it in is_valid() */
  post_header_len= (uint8*) my_memdup((uchar*)buf+ST_COMMON_HEADER_LEN_OFFSET+1,
                                      number_of_event_types*
                                      sizeof(*post_header_len), MYF(0));
  calc_server_version_split();

  /*
    In some previous versions, the events were given other event type
    id numbers than in the present version. When replicating from such
    a version, we therefore set up an array that maps those id numbers
    to the id numbers of the present server.

    If post_header_len is null, it means malloc failed, and is_valid
    will fail, so there is no need to do anything.

    The trees in which events have wrong id's are:

    mysql-5.1-wl1012.old mysql-5.1-wl2325-5.0-drop6p13-alpha
    mysql-5.1-wl2325-5.0-drop6 mysql-5.1-wl2325-5.0
    mysql-5.1-wl2325-no-dd

    (this was found by grepping for two lines in sequence where the
    first matches "FORMAT_DESCRIPTION_EVENT," and the second matches
    "TABLE_MAP_EVENT," in log_event.h in all trees)

    In these trees, the following server_versions existed since
    TABLE_MAP_EVENT was introduced:

    5.1.1-a_drop5p3   5.1.1-a_drop5p4        5.1.1-alpha
    5.1.2-a_drop5p10  5.1.2-a_drop5p11       5.1.2-a_drop5p12
    5.1.2-a_drop5p13  5.1.2-a_drop5p14       5.1.2-a_drop5p15
    5.1.2-a_drop5p16  5.1.2-a_drop5p16b      5.1.2-a_drop5p16c
    5.1.2-a_drop5p17  5.1.2-a_drop5p4        5.1.2-a_drop5p5
    5.1.2-a_drop5p6   5.1.2-a_drop5p7        5.1.2-a_drop5p8
    5.1.2-a_drop5p9   5.1.3-a_drop5p17       5.1.3-a_drop5p17b
    5.1.3-a_drop5p17c 5.1.4-a_drop5p18       5.1.4-a_drop5p19
    5.1.4-a_drop5p20  5.1.4-a_drop6p0        5.1.4-a_drop6p1
    5.1.4-a_drop6p2   5.1.5-a_drop5p20       5.2.0-a_drop6p3
    5.2.0-a_drop6p4   5.2.0-a_drop6p5        5.2.0-a_drop6p6
    5.2.1-a_drop6p10  5.2.1-a_drop6p11       5.2.1-a_drop6p12
    5.2.1-a_drop6p6   5.2.1-a_drop6p7        5.2.1-a_drop6p8
    5.2.2-a_drop6p13  5.2.2-a_drop6p13-alpha 5.2.2-a_drop6p13b
    5.2.2-a_drop6p13c

    (this was found by grepping for "mysql," in all historical
    versions of configure.in in the trees listed above).

    There are 5.1.1-alpha versions that use the new event id's, so we
    do not test that version string.  So replication from 5.1.1-alpha
    with the other event id's to a new version does not work.
    Moreover, we can safely ignore the part after drop[56].  This
    allows us to simplify the big list above to the following regexes:

    5\.1\.[1-5]-a_drop5.*
    5\.1\.4-a_drop6.*
    5\.2\.[0-2]-a_drop6.*

    This is what we test for in the 'if' below.
  */
  if (post_header_len &&
      server_version[0] == '5' && server_version[1] == '.' &&
      server_version[3] == '.' &&
      strncmp(server_version + 5, "-a_drop", 7) == 0 &&
      ((server_version[2] == '1' &&
        server_version[4] >= '1' && server_version[4] <= '5' &&
        server_version[12] == '5') ||
       (server_version[2] == '1' &&
        server_version[4] == '4' &&
        server_version[12] == '6') ||
       (server_version[2] == '2' &&
        server_version[4] >= '0' && server_version[4] <= '2' &&
        server_version[12] == '6')))
  {
    if (number_of_event_types != 22)
    {
      DBUG_PRINT("info", (" number_of_event_types=%d",
                          number_of_event_types));
      /* this makes is_valid() return false. */
      my_free(post_header_len, MYF(MY_ALLOW_ZERO_PTR));
      post_header_len= NULL;
      DBUG_VOID_RETURN;
    }
    static const uint8 perm[23]=
      {
        UNKNOWN_EVENT, START_EVENT_V3, QUERY_EVENT, STOP_EVENT, ROTATE_EVENT,
        INTVAR_EVENT, LOAD_EVENT, SLAVE_EVENT, CREATE_FILE_EVENT,
        APPEND_BLOCK_EVENT, EXEC_LOAD_EVENT, DELETE_FILE_EVENT,
        NEW_LOAD_EVENT,
        RAND_EVENT, USER_VAR_EVENT,
        FORMAT_DESCRIPTION_EVENT,
        TABLE_MAP_EVENT,
        PRE_GA_WRITE_ROWS_EVENT,
        PRE_GA_UPDATE_ROWS_EVENT,
        PRE_GA_DELETE_ROWS_EVENT,
        XID_EVENT,
        BEGIN_LOAD_QUERY_EVENT,
        EXECUTE_LOAD_QUERY_EVENT,
      };
    event_type_permutation= perm;
    /*
      Since we use (permuted) event id's to index the post_header_len
      array, we need to permute the post_header_len array too.
    */
    uint8 post_header_len_temp[23];
    for (int i= 1; i < 23; i++)
      post_header_len_temp[perm[i] - 1]= post_header_len[i - 1];
    for (int i= 0; i < 22; i++)
      post_header_len[i] = post_header_len_temp[i];
  }
  DBUG_VOID_RETURN;
}
/**
   Splits the event's 'server_version' string into three numeric pieces stored
   into 'server_version_split':
   X.Y.Zabc (X,Y,Z numbers, a not a digit) -> {X,Y,Z}
   X.Yabc -> {X,Y,0}
   Xabc -> {X,0,0}
   'server_version_split' is then used for lookups to find if the server which
   created this event has some known bug.
*/
void Format_description_log_event::calc_server_version_split()
{
  char *p= server_version, *r;
  ulong number;
  for (uint i= 0; i<=2; i++)
  {
    number= strtoul(p, &r, 10);
    server_version_split[i]= (uchar)number;
    DBUG_ASSERT(number < 256); // fit in uchar
    p= r;
    DBUG_ASSERT(!((i == 0) && (*r != '.'))); // should be true in practice
    if (*r == '.')
      p++; // skip the dot
  }
  DBUG_PRINT("info",("Format_description_log_event::server_version_split:"
                     " '%s' %d %d %d", server_version,
                     server_version_split[0],
                     server_version_split[1], server_version_split[2]));
}
/**************************************************************************
	Query_log_event methods
**************************************************************************/
/**
   Macro to check that there is enough space to read from memory.

   @param PTR Pointer to memory
   @param END End of memory
   @param CNT Number of bytes that should be read.
 */
#define CHECK_SPACE(PTR,END,CNT)                      \
  do {                                                \
    DBUG_PRINT("info", ("Read %s", code_name(pos[-1]))); \
    DBUG_ASSERT((PTR) + (CNT) <= (END));              \
    if ((PTR) + (CNT) > (END)) {                      \
      DBUG_PRINT("info", ("query= 0"));               \
      query= 0;                                       \
      DBUG_VOID_RETURN;                               \
    }                                                 \
  } while (0)

/* 2 utility functions for the next method */

/**
   Read a string with length from memory.

   This function reads the string-with-length stored at
   <code>src</code> and extract the length into <code>*len</code> and
   a pointer to the start of the string into <code>*dst</code>. The
   string can then be copied using <code>memcpy()</code> with the
   number of bytes given in <code>*len</code>.

   @param src Pointer to variable holding a pointer to the memory to
              read the string from.
   @param dst Pointer to variable holding a pointer where the actual
              string starts. Starting from this position, the string
              can be copied using @c memcpy().
   @param len Pointer to variable where the length will be stored.
   @param end One-past-the-end of the memory where the string is
              stored.

   @return    Zero if the entire string can be copied successfully,
              @c UINT_MAX if the length could not be read from memory
              (that is, if <code>*src >= end</code>), otherwise the
              number of bytes that are missing to read the full
              string, which happends <code>*dst + *len >= end</code>.
*/
static int
get_str_len_and_pointer(const Log_event::Byte **src,
                        const char **dst,
                        uint *len,
                        const Log_event::Byte *end)
{
  if (*src >= end)
    return -1;       // Will be UINT_MAX in two-complement arithmetics
  uint length= **src;
  if (length > 0)
  {
    if (*src + length >= end)
      return *src + length - end + 1;       // Number of bytes missing
    *dst= (char *)*src + 1;                    // Will be copied later
  }
  *len= length;
  *src+= length + 1;
  return 0;
}

static void copy_str_and_move(const char **src, 
                              Log_event::Byte **dst, 
                              uint len)
{
  memcpy(*dst, *src, len);
  *src= (const char *)*dst;
  (*dst)+= len;
  *(*dst)++= 0;
}


/**
  The simplest constructor that could possibly work.  This is used for
  creating static objects that have a special meaning and are invisible
  to the log.  
*/
Query_log_event::Query_log_event()
  :Log_event(), data_buf(0)
{
  memset(&user, 0, sizeof(user));
  memset(&host, 0, sizeof(host));
}

/**
  This is used by the SQL slave thread to prepare the event before execution.
*/
Query_log_event::Query_log_event(const char* buf, uint event_len,
                                 const Format_description_log_event
                                 *description_event,
                                 Log_event_type event_type)
  :Log_event(buf, description_event), data_buf(0), query(NullS),
   db(NullS), catalog_len(0), status_vars_len(0),
   flags2_inited(0), sql_mode_inited(0), charset_inited(0),
   auto_increment_increment(1), auto_increment_offset(1),
   time_zone_len(0), lc_time_names_number(0), charset_database_number(0),
   table_map_for_update(0), master_data_written(0)
{
  ulong data_len;
  uint32 tmp;
  uint8 common_header_len, post_header_len;
  Log_event::Byte *start;
  const Log_event::Byte *end;
  bool catalog_nz= 1;
  DBUG_ENTER("Query_log_event::Query_log_event(char*,...)");

  memset(&user, 0, sizeof(user));
  memset(&host, 0, sizeof(host));
  common_header_len= description_event->common_header_len;
  post_header_len= description_event->post_header_len[event_type-1];
  DBUG_PRINT("info",("event_len: %u  common_header_len: %d  post_header_len: %d",
                     event_len, common_header_len, post_header_len));
  
  /*
    We test if the event's length is sensible, and if so we compute data_len.
    We cannot rely on QUERY_HEADER_LEN here as it would not be format-tolerant.
    We use QUERY_HEADER_MINIMAL_LEN which is the same for 3.23, 4.0 & 5.0.
  */
  if (event_len < (uint)(common_header_len + post_header_len))
    DBUG_VOID_RETURN;				
  data_len = event_len - (common_header_len + post_header_len);
  buf+= common_header_len;
  
  slave_proxy_id= thread_id = uint4korr(buf + Q_THREAD_ID_OFFSET);
  exec_time = uint4korr(buf + Q_EXEC_TIME_OFFSET);
  db_len = (uint)buf[Q_DB_LEN_OFFSET]; // TODO: add a check of all *_len vars
  error_code = uint2korr(buf + Q_ERR_CODE_OFFSET);

  /*
    5.0 format starts here.
    Depending on the format, we may or not have affected/warnings etc
    The remnent post-header to be parsed has length:
  */
  tmp= post_header_len - QUERY_HEADER_MINIMAL_LEN; 
  if (tmp)
  {
    status_vars_len= uint2korr(buf + Q_STATUS_VARS_LEN_OFFSET);
    /*
      Check if status variable length is corrupt and will lead to very
      wrong data. We could be even more strict and require data_len to
      be even bigger, but this will suffice to catch most corruption
      errors that can lead to a crash.
    */
    if (status_vars_len > min(data_len, MAX_SIZE_LOG_EVENT_STATUS))
    {
      DBUG_PRINT("info", ("status_vars_len (%u) > data_len (%lu); query= 0",
                          status_vars_len, data_len));
      query= 0;
      DBUG_VOID_RETURN;
    }
    data_len-= status_vars_len;
    DBUG_PRINT("info", ("Query_log_event has status_vars_len: %u",
                        (uint) status_vars_len));
    tmp-= 2;
  } 
  else
  {
    /*
      server version < 5.0 / binlog_version < 4 master's event is 
      relay-logged with storing the original size of the event in
      Q_MASTER_DATA_WRITTEN_CODE status variable.
      The size is to be restored at reading Q_MASTER_DATA_WRITTEN_CODE-marked
      event from the relay log.
    */
    DBUG_ASSERT(description_event->binlog_version < 4);
    master_data_written= data_written;
  }
  /*
    We have parsed everything we know in the post header for QUERY_EVENT,
    the rest of post header is either comes from older version MySQL or
    dedicated to derived events (e.g. Execute_load_query...)
  */

  /* variable-part: the status vars; only in MySQL 5.0  */
  
  start= (Log_event::Byte*) (buf+post_header_len);
  end= (const Log_event::Byte*) (start+status_vars_len);
  for (const Log_event::Byte* pos= start; pos < end;)
  {
    switch (*pos++) {
    case Q_FLAGS2_CODE:
      CHECK_SPACE(pos, end, 4);
      flags2_inited= 1;
      flags2= uint4korr(pos);
      DBUG_PRINT("info",("In Query_log_event, read flags2: %lu", (ulong) flags2));
      pos+= 4;
      break;
    case Q_SQL_MODE_CODE:
    {
#ifndef DBUG_OFF
      char buff[22];
#endif
      CHECK_SPACE(pos, end, 8);
      sql_mode_inited= 1;
      sql_mode= (ulong) uint8korr(pos); // QQ: Fix when sql_mode is ulonglong
      DBUG_PRINT("info",("In Query_log_event, read sql_mode: %s",
			 llstr(sql_mode, buff)));
      pos+= 8;
      break;
    }
    case Q_CATALOG_NZ_CODE:
      DBUG_PRINT("info", ("case Q_CATALOG_NZ_CODE; pos: 0x%lx; end: 0x%lx",
                          (ulong) pos, (ulong) end));
      if (get_str_len_and_pointer(&pos, &catalog, &catalog_len, end))
      {
        DBUG_PRINT("info", ("query= 0"));
        query= 0;
        DBUG_VOID_RETURN;
      }
      break;
    case Q_AUTO_INCREMENT:
      CHECK_SPACE(pos, end, 4);
      auto_increment_increment= uint2korr(pos);
      auto_increment_offset=    uint2korr(pos+2);
      pos+= 4;
      break;
    case Q_CHARSET_CODE:
    {
      CHECK_SPACE(pos, end, 6);
      charset_inited= 1;
      memcpy(charset, pos, 6);
      pos+= 6;
      break;
    }
    case Q_TIME_ZONE_CODE:
    {
      if (get_str_len_and_pointer(&pos, &time_zone_str, &time_zone_len, end))
      {
        DBUG_PRINT("info", ("Q_TIME_ZONE_CODE: query= 0"));
        query= 0;
        DBUG_VOID_RETURN;
      }
      break;
    }
    case Q_CATALOG_CODE: /* for 5.0.x where 0<=x<=3 masters */
      CHECK_SPACE(pos, end, 1);
      if ((catalog_len= *pos))
        catalog= (char*) pos+1;                           // Will be copied later
      CHECK_SPACE(pos, end, catalog_len + 2);
      pos+= catalog_len+2; // leap over end 0
      catalog_nz= 0; // catalog has end 0 in event
      break;
    case Q_LC_TIME_NAMES_CODE:
      CHECK_SPACE(pos, end, 2);
      lc_time_names_number= uint2korr(pos);
      pos+= 2;
      break;
    case Q_CHARSET_DATABASE_CODE:
      CHECK_SPACE(pos, end, 2);
      charset_database_number= uint2korr(pos);
      pos+= 2;
      break;
    case Q_TABLE_MAP_FOR_UPDATE_CODE:
      CHECK_SPACE(pos, end, 8);
      table_map_for_update= uint8korr(pos);
      pos+= 8;
      break;
    case Q_MASTER_DATA_WRITTEN_CODE:
      CHECK_SPACE(pos, end, 4);
      data_written= master_data_written= uint4korr(pos);
      pos+= 4;
      break;
    case Q_INVOKER:
    {
      CHECK_SPACE(pos, end, 1);
      user.length= *pos++;
      CHECK_SPACE(pos, end, user.length);
      user.str= (char *)pos;
      pos+= user.length;

      CHECK_SPACE(pos, end, 1);
      host.length= *pos++;
      CHECK_SPACE(pos, end, host.length);
      host.str= (char *)pos;
      pos+= host.length;
    }
    default:
      /* That's why you must write status vars in growing order of code */
      DBUG_PRINT("info",("Query_log_event has unknown status vars (first has\
 code: %u), skipping the rest of them", (uint) *(pos-1)));
      pos= (const uchar*) end;                         // Break loop
    }
  }
  
  if (!(start= data_buf = (Log_event::Byte*) my_malloc(catalog_len + 1 +
                                             time_zone_len + 1 +
                                             data_len + 1 +
                                             user.length + 1 +
                                             host.length + 1,
                                             MYF(MY_WME))))
      DBUG_VOID_RETURN;
  if (catalog_len)                                  // If catalog is given
  {
    /**
      @todo we should clean up and do only copy_str_and_move; it
      works for both cases.  Then we can remove the catalog_nz
      flag. /sven
    */
    if (likely(catalog_nz)) // true except if event comes from 5.0.0|1|2|3.
      copy_str_and_move(&catalog, &start, catalog_len);
    else
    {
      memcpy(start, catalog, catalog_len+1); // copy end 0
      catalog= (const char *)start;
      start+= catalog_len+1;
    }
  }
  if (time_zone_len)
    copy_str_and_move(&time_zone_str, &start, time_zone_len);

  if (user.length > 0)
    copy_str_and_move((const char **)&(user.str), &start, user.length);
  if (host.length > 0)
    copy_str_and_move((const char **)&(host.str), &start, host.length);

  /**
    if time_zone_len or catalog_len are 0, then time_zone and catalog
    are uninitialized at this point.  shouldn't they point to the
    zero-length null-terminated strings we allocated space for in the
    my_alloc call above? /sven
  */

  /* A 2nd variable part; this is common to all versions */ 
  memcpy((char*) start, end, data_len);          // Copy db and query
  start[data_len]= '\0';              // End query with \0 (For safetly)
  db= (char *)start;
  query= (char *)(start + db_len + 1);
  q_len= data_len - db_len -1;
  DBUG_VOID_RETURN;
}

/**
  Dump_log_event methods.
 **/
Dumb_log_event:: Dumb_log_event(const char* buf,
                     uint event_len,
                     const Format_description_log_event* description_event,
                     Log_event_type type_code)
  :Log_event(buf, description_event), type_code_(type_code) {
  DBUG_VOID_RETURN;
}


/******************************************
  Rows_log_event methods.
  ******************************************/
Rows_log_event::Rows_log_event(const char *buf, uint event_len,
                               Log_event_type event_type,
                               const Format_description_log_event
                               *description_event)
  : Log_event(buf, description_event),
    m_row_count(0),
    m_table_id(0), m_rows_buf(0), m_rows_cur(0), m_rows_end(0)
{
  type_ = event_type;
  DBUG_ENTER("Rows_log_event::Rows_log_event(const char*,...)");
  uint8 const common_header_len= description_event->common_header_len;
  uint8 const post_header_len= description_event->post_header_len[event_type-1];

  DBUG_PRINT("enter",("event_len: %u  common_header_len: %d  "
		      "post_header_len: %d",
		      event_len, common_header_len,
		      post_header_len));

  const char *post_start= buf + common_header_len;
  post_start+= RW_MAPID_OFFSET;
  if (post_header_len == 6)
  {
    /* Master is of an intermediate source tree before 5.1.4. Id is 4 bytes */
    m_table_id= uint4korr(post_start);
    post_start+= 4;
  }
  else
  {
    m_table_id= (ulong) uint6korr(post_start);
    post_start+= RW_FLAGS_OFFSET;
  }

  m_flags= uint2korr(post_start);

  uchar const *const var_start=
    (const uchar *)buf + common_header_len + post_header_len;
  uchar const *const ptr_width= var_start;
  uchar *ptr_after_width= (uchar*) ptr_width;
  DBUG_PRINT("debug", ("Reading from %p", ptr_after_width));
  m_width = net_field_length(&ptr_after_width);
  DBUG_PRINT("debug", ("m_width=%lu", m_width));
  /* if bitmap_init fails, catched in is_valid() */
  if (likely(!bitmap_init(&m_cols,
                          m_width <= sizeof(m_bitbuf)*8 ? m_bitbuf : NULL,
                          m_width,
                          false)))
  {
    DBUG_PRINT("debug", ("Reading from %p", ptr_after_width));
    memcpy(m_cols.bitmap, ptr_after_width, (m_width + 7) / 8);
    create_last_word_mask(&m_cols);
    ptr_after_width+= (m_width + 7) / 8;
    DBUG_DUMP("m_cols", (uchar*) m_cols.bitmap, no_bytes_in_map(&m_cols));
  }
  else
  {
    // Needed because bitmap_init() does not set it to null on failure
    m_cols.bitmap= NULL;
    DBUG_VOID_RETURN;
  }

  m_cols_ai.bitmap= m_cols.bitmap; /* See explanation in is_valid() */

  if (event_type == UPDATE_ROWS_EVENT)
  {
    DBUG_PRINT("debug", ("Reading from %p", ptr_after_width));

    /* if bitmap_init fails, caught in is_valid() */
    if (likely(!bitmap_init(&m_cols_ai,
                            m_width <= sizeof(m_bitbuf_ai)*8 ? m_bitbuf_ai : NULL,
                            m_width,
                            false)))
    {
      DBUG_PRINT("debug", ("Reading from %p", ptr_after_width));
      memcpy(m_cols_ai.bitmap, ptr_after_width, (m_width + 7) / 8);
      create_last_word_mask(&m_cols_ai);
      ptr_after_width+= (m_width + 7) / 8;
      DBUG_DUMP("m_cols_ai", (uchar*) m_cols_ai.bitmap,
                no_bytes_in_map(&m_cols_ai));
    }
    else
    {
      // Needed because bitmap_init() does not set it to null on failure
      m_cols_ai.bitmap= 0;
      DBUG_VOID_RETURN;
    }
  }

  const uchar* const ptr_rows_data= (const uchar*) ptr_after_width;

  size_t const data_size= event_len - (ptr_rows_data - (const uchar *) buf);
  DBUG_PRINT("info",("m_table_id: %lu  m_flags: %d  m_width: %lu  data_size: %lu",
                     m_table_id, m_flags, m_width, (ulong) data_size));

  m_rows_buf= (uchar*) my_malloc(data_size, MYF(MY_WME));
  if (likely((bool)m_rows_buf))
  {
   m_rows_end= m_rows_buf + data_size;
    m_rows_cur= m_rows_end;
    memcpy(m_rows_buf, ptr_rows_data, data_size);
  }
  else
    m_cols.bitmap= 0; // to not free it

  DBUG_VOID_RETURN;
}

Rows_log_event::~Rows_log_event() {
  if (m_cols.bitmap == m_bitbuf) // no my_malloc happened
    m_cols.bitmap= 0; // so no my_free in bitmap_free
  bitmap_free(&m_cols); // To pair with bitmap_init().
  my_free((uchar*)m_rows_buf, MYF(MY_ALLOW_ZERO_PTR));
}

int Rows_log_event::get_data_size() {
  int const type_code= get_type_code();

  uchar buf[sizeof(m_width) + 1];
  uchar *end= net_store_length(buf, m_width);

  DBUG_EXECUTE_IF("old_row_based_repl_4_byte_map_id_master",
                  return 6 + no_bytes_in_map(&m_cols) + (end - buf) +
                  (type_code == UPDATE_ROWS_EVENT ? no_bytes_in_map(&m_cols_ai) : 0) +
                  (m_rows_cur - m_rows_buf););
  int data_size= ROWS_HEADER_LEN;
  data_size+= no_bytes_in_map(&m_cols);
  data_size+= (uint) (end - buf);

  if (type_code == UPDATE_ROWS_EVENT)
    data_size+= no_bytes_in_map(&m_cols_ai);

  data_size+= (uint) (m_rows_cur - m_rows_buf);
  return data_size; 
}

/*
  Constructor used by slave to read the event from the binary log.
 */
Table_map_log_event::Table_map_log_event(const char *buf, uint event_len,
                                         const Format_description_log_event
                                         *description_event)

  : Log_event(buf, description_event),
    columns_cnt_(0),
    table_id_(ULONG_MAX),
    table_ptr_(NULL)
{
  Parse(buf,event_len,description_event);
}

Table_map_log_event::~Table_map_log_event()
{
  if(table_ptr_){
    delete table_ptr_;
    table_ptr_ = NULL;
  }
}

int Table_map_log_event::Parse(const char *buf, uint event_len, 
            const Format_description_log_event *description_event){

  unsigned int bytes_read= 0;
  uint8 common_header_len= description_event->common_header_len;
  uint8 post_header_len= description_event->post_header_len[TABLE_MAP_EVENT-1];

  /* Read the post-header */
  const char *post_start= buf + common_header_len;

  post_start+= TM_MAPID_OFFSET;
  if (post_header_len == 6) {
    /* Master is of an intermediate source tree before 5.1.4. Id is 4 bytes */
    table_id_ = uint4korr(post_start);
    post_start+= 4;
  }
  else {
    DBUG_ASSERT(post_header_len == TABLE_MAP_HEADER_LEN);
    table_id_= (ulong) uint6korr(post_start);
    post_start+= TM_FLAGS_OFFSET;
  }

  DBUG_ASSERT(table_id_ != ~0UL);

  flags_ = uint2korr(post_start);

  /* Read the variable part of the event */
  const char *const vpart= buf + common_header_len + post_header_len;

  /* Extract the length of the various parts from the buffer */
  uchar const *const ptr_dblen= (uchar const*)vpart + 0;
  size_t dblen= *(uchar*) ptr_dblen;
  db_name_.assign((const char*)ptr_dblen + 1,(const char*) ptr_dblen + 1 + dblen);

  /* Length of database name + counter + terminating null */
  uchar const *const ptr_tbllen= ptr_dblen + dblen + 2;
  size_t tbllen= *(uchar*) ptr_tbllen;
  table_name_.assign((const char*)ptr_tbllen + 1, (const char*)ptr_tbllen + 1 + tbllen);

  /* Length of table name + counter + terminating null */
  uchar const *const ptr_colcnt= ptr_tbllen + tbllen + 2;
  uchar *ptr_after_colcnt= (uchar*) ptr_colcnt;
  columns_cnt_ = net_field_length(&ptr_after_colcnt);
  const uchar* columns_type_ptr = ptr_after_colcnt;


  ptr_after_colcnt= ptr_after_colcnt + columns_cnt_;
  bytes_read= (uint) (ptr_after_colcnt - (uchar *)buf);
  if (bytes_read < event_len) {
    ulong metadata_size= net_field_length(&ptr_after_colcnt);
    const uchar* metadata_ptr = ptr_after_colcnt;
    if(metadata_size > (columns_cnt_ * 2)){
      return -1;
    }
    
    uint num_null_bytes= (columns_cnt_ + 7) / 8;
    ptr_after_colcnt= (uchar*)ptr_after_colcnt + metadata_size;
    table_ptr_ = new table_def((uchar*)columns_type_ptr,columns_cnt_,(uchar*)metadata_ptr,
        metadata_size,ptr_after_colcnt,flags_);
  }
  return 0;
}


Rotate_log_event::Rotate_log_event(const char* buf, uint event_len,
                                   const Format_description_log_event* description_event)
  :Log_event(buf, description_event),pos_(0) 
{
  // The caller will ensure that event_len is what we have at EVENT_LEN_OFFSET
  uint8 header_size= description_event->common_header_len;
  uint8 post_header_len= description_event->post_header_len[ROTATE_EVENT-1];
  uint ident_offset;
  uint ident_len = 0;
  if (event_len < header_size)
    DBUG_VOID_RETURN;
  buf += header_size;
  pos_ = post_header_len ? uint8korr(buf + R_POS_OFFSET) : 4;
  ident_len = (uint)(event_len -
                     (header_size+post_header_len)); 
  ident_offset = post_header_len; 
  set_if_smaller(ident_len,FN_REFLEN-1);
  new_logname_.assign(buf + ident_offset, buf + ident_offset + ident_len);
  DBUG_VOID_RETURN;
}

