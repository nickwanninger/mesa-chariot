/**************************************************************************
 * 
 * Copyright 2005 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef ST_CB_BUFFEROBJECTS_H
#define ST_CB_BUFFEROBJECTS_H

struct st_context;
struct gl_buffer_object;
struct pipe_buffer;

/**
 * State_tracker vertex/pixel buffer object, derived from Mesa's
 * gl_buffer_object.
 */
struct st_buffer_object
{
   struct gl_buffer_object Base;
   struct pipe_buffer *buffer;  
   GLsizeiptrARB size;
};


/* Are the obj->Name tests necessary?  Unfortunately yes, mesa
 * allocates a couple of gl_buffer_object structs statically, and the
 * Name == 0 test is the only way to identify them and avoid casting
 * them erroneously to our structs.
 */
static INLINE struct st_buffer_object *
st_buffer_object(struct gl_buffer_object *obj)
{
   if (obj->Name)
      return (struct st_buffer_object *) obj;
   else
      return NULL;
}


extern void
st_init_bufferobject_functions(struct dd_function_table *functions);


#endif
