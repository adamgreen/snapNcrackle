/*  Copyright (C) 2012  Adam Green (https://github.com/adamgreen)

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/
#ifndef _PARSE_CSV_H_
#define _PARSE_CSV_H_

#include "try_catch.h"


typedef struct ParseCSV ParseCSV;


__throws ParseCSV*    ParseCSV_Create(void);
         void         ParseCSV_Free(ParseCSV* pThis);
         
__throws void         ParseCSV_Parse(ParseCSV* pThis, char* pLine);
         size_t       ParseCSV_FieldCount(ParseCSV* pThis);
         const char** ParseCSV_FieldPointers(ParseCSV* pThis);
         
#endif /* _PARSE_CSV_H_ */
