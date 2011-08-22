/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2009-2011  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "abc.h"
#include <limits>
#include "class.h"
#include "params.h"
#include "toplevel.h"

using namespace std;

namespace lightspark
{
    void unpackFlashParam (const char type, ASObject *obj, void *pointer)
    {
        switch (type)
        {
        case 'b':
            *(bool *) pointer = abstract_b (Boolean_concrete (obj));
            break;
        case 'i':
            *(int *) pointer = obj->toInt();
            break;
        case 'u':
            *(unsigned int *) pointer = obj->toUInt();
            break;
        case 'd':
            *(double *) pointer = obj->toNumber();
            break;
        case 's':
            pointer = Class<ASString>::getInstanceS(obj->toString(false));
            break;
        }
    }

    void unpackFlashParams (const char *format, ASObject * const *args, const unsigned int argslen, ...)
    {
        va_list out;
        va_start(out, argslen);
        unpackFlashParams(format, args, argslen, &out);
        va_end(out);
    }

    void unpackFlashParams (const char *format, ASObject * const *args, const unsigned int argslen, va_list *out)
    {
        unsigned int i;
        void *pointer;
        for (i = 0; i < argslen; i ++)
        {
            pointer = va_arg (out, void *);
            unpackFlashParam (format[i], args[i], pointer);
        }

        /* Fill in all the "unpassed" parameters. */
        for (; i < strlen(format); i ++)
        {
            pointer = va_arg (out, void*);
            switch (format[i])
            {
            case 'b':
                *((bool *) pointer) = false;
                break;
            case 'i':
                *((int *) pointer) = 0;
                break;
            case 'u':
                *((unsigned int *) pointer) = 0;
                break;
            case 'd':
                *((double *) pointer) = numeric_limits<double>::quiet_NaN();
                break;
            /* String is "undefined" as well. */
            default:
                pointer = new Undefined;
                break;
            }
        }
    }
};
