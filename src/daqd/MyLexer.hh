/*
  $Id: MyLexer.hh,v 1.10 2009/04/28 22:42:20 aivanov Exp $
*/

#ifndef MYLEXER_H
#define MYLEXER_H
#include<iostream>
#include<iomanip>
#include <stdlib.h>
#include "io.h"
#include "net_writer.hh"
using namespace std;

#define YY_DECL int my_lexer::my_yylex (YYSTYPE *lvalp)
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif


class my_lexer : public yyFlexLexer {
private:
    std::istream* yyin_;
    std::ostream* yyout_;
public:
  my_lexer (std::ifstream* arg_yyin = 0, std::ofstream* arg_yyout = 0, int ifd = 0, int ofd = 0, int pstrict = 0, int pprompt = 0) 
    : yyFlexLexer (arg_yyin, arg_yyout), strict (pstrict), error_code (0), num_channels (0), prompt (pprompt),
#ifdef NETWORK_PRODUCER
    used_by_controller(0),
    connection_num(0),
#endif
    trend_channels (0), auth_ok (0), n_archive_channel_names(0) {
    this->ifd = ifd; this->ofd = ofd;
    prompt_lineno = 1;
    cptr = cmnd;
    yy_flex_debug=1;

#ifndef FLEX_USES_IOSTREAM_REF
    yyin_ = yyin;
    yyout_ = yyout;
#else
    yyin_ = &yyin;
    yyout_ = &yyout;
#endif
  };

  enum { max_channels = 128 };

  int ifd;
  int ofd;

  circ_buffer_t *cb;
  int my_yylex (YYSTYPE *lvalp);
  int my_yyerror (char *mesg) { 
    if (strict)
      *yyout_ << "0001" << std::flush;
    else
      *yyout_ << "yyerror: <" << yylineno << "> " << mesg << std::endl;
    return 0;
  }

  char cmnd [0xfff+1];
  char *cptr;

  int LexerInput (char* buf, int max_size) {
    // int ret = yyFlexLexer::LexerInput (buf, max_size);
    int ret = basic_io::readn( ifd, buf, 1 );
    if (ret > 0) {
      buf [ret] = 0;
      char *stp;
      if (stp = strpbrk (buf, ";\n\r")) {
	char cs = *stp;
	*stp = 0;
	*cptr = 0;
	if (strlen(cmnd) + strlen(buf))
	  system_log(2, "->%d: %s%s", ifd, cmnd, buf);
	*stp = cs;
	cptr = cmnd;
      } else {
	strncpy (cptr, buf, min(ret, cmnd+0xfff-cptr));
	cptr += min(ret,cmnd+0xff-cptr);
      }
      //      fputs (buf, stderr);
      //      system_log(5, "command: (%d) -->%s<--x", ret, buf);
    } else if (ret < 0) {
      system_log(1, "->%d: lexer input read returned -1; errno=%d", ifd, errno);
      ret = 0;
    }
    return ret;
  }

  std::istream *get_yyin () { return yyin_; }
  int get_ifd () { return ifd; }
  std::ostream *get_yyout () { return yyout_; }

  /*
    Set if the propmt should be displayed
    */
  int prompt;

  /*
    Used to avoid duplicate prompt display
  */
  int prompt_lineno;

  int lineno () { return yylineno; }

  /*
    Set if the responses from the server ought to be strict.
    This means we are talking to the client program, rather then the user
    connected with telnet.
  */
  int strict;

  /*
    Maintained by the strict parser code.
    Setting this will kill the parser
  */
  int error_code;

  /*
    Store channel configuration for a moment (used by the parser code)
  */
  long_channel_t channels [max_channels];
  int num_channels;
  int trend_channels; // Set for `start trend net-writer' request

  /*
    Session authenticated with the password
  */
  int auth_ok;

  /*
    Parser calculates how many channels in the request are archive channels
   */
  int n_archive_channel_names;

#ifdef NETWORK_PRODUCER
  /*
    Set if this connection is used by the controller to send the data
  */
  int used_by_controller;
  int connection_num; // Connection zero spawns main producer thread

#endif

  void outsync() { fsync(ofd); }
};

#endif
