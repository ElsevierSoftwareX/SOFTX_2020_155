//
// Created by jonathan.hanks on 1/17/20.
//

#ifndef DAQD_TRUNK_ARGS_H
#define DAQD_TRUNK_ARGS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! @file
 * Args defines a simple wrapper around getopt_long.  The main idea is to
 * provide an api which makes it easier to keep command line argument
 * descriptions and parsers in sync.
 */

/*!
 * @brief An args_handle is an opaque handle to an argument parser.
 * @details It is created as via a call to args_create_parser and must
 * be destroyed by a call to args_destroy.
 *
 * The basic work flow is: to create a parser, add arguments to it, parse
 *  1. Create a parser via args_create_parser( ... )
 *  2. Add arguments via calls to args_add_[flag|int|string_ptr]( ... )
 *  3. Parse the arguments via args_parse( ... )
 *  4. Do your applications work
 *  5. Destroy the parser via args_destroy( ... )
 *
 *  For example to have the simple (and contrived) set of options
 *  -h|--help -> help
 *  -i|--int = <num> -> pass a number
 *  -s|--state = <state> -> pass in a state name
 *  --end = <state> -> pass in a state, long form only
 *  -v -> verbose output, short form only
 *
 *  The code would look somewhat like this:
 *
 *  int int_arg = 0;
 *  const char* state = "";
 *  const char* end_state = "";
 *  int verbose_flag = 0;
 *
 *  args_handle args = args_create_parser("A demonstration program");
 *  args_add_int(args, 'i', "int", "num", "Pass a number into the program",
 * &int_arg, 0); args_add_string_ptr(args, '-s', "state", "state", "Pass in a
 * state name", &state, "start"); args_add_string_ptr(args, ARGS_NO_SHORT,
 * "end", "state", "Pass in the end state name", &end_state, "end");
 *  args_add_flag(args, 'v', ARGS_NO_LONG, "Verbose output", &verbose_flag);
 *  if (args_parse(args, argc, argv))
 *  {
 *     ... do program logic here ...
 *  }
 *  args_destroy(&args);
 *
 * @note When creating arguments, a default value is always required.  This is
 * done to make sure default values are explicitly documented.
 */
typedef void* args_handle;

/*!
 * @brief Constant used when building arguments, denote that there is no short
 * form.
 */
extern const char ARGS_NO_SHORT;
/*!
 * @brief Constant used when building arguments, denote that there is no long
 * form.
 */
extern const char* ARGS_NO_LONG;

/*!
 * @brief Create an argument parser
 * @param program_description Descriptive text for the program.
 * @return On success a non-NULL args_handle.  Else on error NULL.
 * @note This will add a default option for -h|--help to show the help.
 */
extern args_handle args_create_parser( const char* program_description );

/*!
 * @brief Destroy an argument parser, releasing any resources owned by it.
 * @param handle A pointer to an argument handle returned by args_create_parser
 * @note This is safe to call with a null argument parser.
 */
extern void args_destroy( args_handle* handle );

/*!
 * @brief A helper function to print usage notes.
 * @note This is generally not needed as args_parse will call args_fprint_usage.
 * @note This is safe to call with a null args_handle
 * @param args The argument parser handle
 * @param program_name The name of the program (pass argv[0])
 * @param out The stream to send output to
 */
extern void
args_fprint_usage( args_handle args, const char* program_name, FILE* out );

/*!
 * @brief Add a flag argument to the parser (flags have no arguments)
 * @param args The argument parser handle
 * @param short_name Short character name or ARGS_NO_SHORT to denote that
 * there is no shart form of the option
 * @param long_name Long name of the flag or ARGS_NO_LONG to denote that there
 * is no long form name for this option
 * @param description Help text for this flag
 * @param destination A pointer to an int.  For every occurance in of the flag
 * in the parsed arguments this integer will be incremented.
 * @return non-zero if the argument could be added, else 0.
 * @note It is safe to pass a NULL for args.
 */
extern int args_add_flag( args_handle args,
                          char        short_name,
                          const char* long_name,
                          const char* description,
                          int*        destination );

/*!
 * @brief Add a string pointer argument to the parser
 * @param args The argument parser handle
 * @param short_name Short character name or ARGS_NO_SHORT to denote that
 * there is no shart form of the option
 * @param long_name Long name of the flag or ARGS_NO_LONG to denote that there
 * is no long form name for this option
 * @param units Units text to be used in the help (arg = <units>   Help)
 * @param description Help text for this flag
 * @param destination A pointer to the const char* to hold the options argument.
 * @param default_value A default value to set destination to.
 * @return non-zero if the argument could be added, else 0.
 * @note It is safe to pass a NULL for args.
 */
extern int args_add_string_ptr( args_handle  args,
                                char         short_name,
                                const char*  long_name,
                                const char*  units,
                                const char*  description,
                                const char** destination,
                                const char*  default_value );

/*!
 * @brief Add a int pointer argument to the parser
 * @param args The argument parser handle
 * @param short_name Short character name or ARGS_NO_SHORT to denote that
 * there is no shart form of the option
 * @param long_name Long name of the flag or ARGS_NO_LONG to denote that there
 * is no long form name for this option
 * @param units Units text to be used in the help (arg = <units>   Help)
 * @param description Help text for this flag
 * @param destination A pointer to the int to hold the options argument.
 * @param default_value A default value to set destination to.
 * @return non-zero if the argument could be added, else 0.
 * @note It is safe to pass a NULL for args.
 */
extern int args_add_int( args_handle args,
                         char        short_name,
                         const char* long_name,
                         const char* units,
                         const char* description,
                         int*        destination,
                         int         default_value );

/*!
 * @brief Parse an argument vector using an argument parser
 * @details This parses the argument vector using the provided parser.  If
 * unexpected (or incorrect) arguments are found, the help will be printed.
 * @param args The argument parser handle.
 * @param argc The length of argv.
 * @param argv The argument array.
 * @return >1 on success, 0  on error, <0 if help was requested (not an error
 * state). This is meant to be used such that the program should abort if the
 * return value is <= 0.  Discriminating the return code in this case can be
 * used to set a non-error code if help was explicitly requested.
 * @note this is safe to call with a NULL args parameter.
 */
extern int args_parse( args_handle args, int argc, char* argv[] );

#ifdef __cplusplus
}
#endif

#endif // DAQD_TRUNK_ARGS_H
