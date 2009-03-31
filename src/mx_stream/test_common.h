/*************************************************************************
 * The contents of this file are subject to the MYRICOM MYRINET          *
 * EXPRESS (MX) NETWORKING SOFTWARE AND DOCUMENTATION LICENSE (the       *
 * "License"); User may not use this file except in compliance with the  *
 * License.  The full text of the License can found in LICENSE.TXT       *
 *                                                                       *
 * Software distributed under the License is distributed on an "AS IS"   *
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See  *
 * the License for the specific language governing rights and            *
 * limitations under the License.                                        *
 *                                                                       *
 * Copyright 2003 - 2004 by Myricom, Inc.  All rights reserved.          *
 *************************************************************************/

#define insist(e) 							\
  do {									\
    if (!(e)) {								\
      printf ("%s %s %d\n", #e, __FILE__, __LINE__);			\
      abort();								\
    }									\
  } while (0)

void fill_data(char *buff, int len)
{
  int i;
  for (i = 0; i < len; i++)
    buff[i] = 'a' + i % 26;
}

int check_data(char *buff, int len)
{
  int i, err = 0;;
  for (i = 0; i < len; i++)
    if (buff[i] != 'a' + i % 26) {
      printf ("recv_buff[%d] should recv: %c (%d), but recv: %c (%d)\n", 
	      i, 'a' + i % 26, 'a' + i % 26, 
	      buff[i], buff[i]);
      fflush(stdout);
      err ++;
    }
  if (err) {
    printf ("Find %d errors.\n", err);
    return 0;
  }
  return 1;
}   

#define MAKE_MATCH(a,b) ((uint64_t)(a) << 32 | (b))
