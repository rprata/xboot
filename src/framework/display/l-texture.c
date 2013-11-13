/*
 * framework/display/l-texture.c
 *
 * Copyright(c) 2007-2013 jianjun jiang <jerryjianjun@gmail.com>
 * official site: http://xboot.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <cairo.h>
#include <cairoint.h>
#include <xfs/xfs.h>
#include <framework/display/l-display.h>

static cairo_status_t xfs_read_func(void * closure, unsigned char * data, unsigned int size)
{
	struct xfs_file_t * file = closure;
	size_t ret;

    while(size)
    {
    	ret = xfs_read(file, data, 1, size);
    	size -= ret;
    	data += ret;
    	if(size && xfs_eof(file))
    		return _cairo_error(CAIRO_STATUS_READ_ERROR);
    }
    return CAIRO_STATUS_SUCCESS;
}

static cairo_surface_t * cairo_image_surface_create_from_png_xfs(const char * filename)
{
	struct xfs_file_t * file;
	cairo_surface_t * surface;

	file = xfs_open_read(filename);
	if(!file)
		return _cairo_surface_create_in_error(_cairo_error(CAIRO_STATUS_FILE_NOT_FOUND));
	surface = cairo_image_surface_create_from_png_stream(xfs_read_func, file);
	xfs_close(file);
    return surface;
}

static inline int is_black_pixel(unsigned char * p)
{
	return (((p[0] == 0) && (p[1] == 0) && (p[2] == 0) && (p[3] == 0xff)) ? 1 : 0);
}

static bool_t fill_nine_patch(struct ltexture_t * texture)
{
	cairo_surface_t * cs;
	cairo_t * cr;
	unsigned char * data;
	int width, height;
	int stride;
	int w, h;
	int i;

	if(!texture || !texture->surface)
		return FALSE;

	width = cairo_image_surface_get_width(texture->surface);
	height = cairo_image_surface_get_height(texture->surface);
	if(width < 3 || height < 3)
		return FALSE;

	/* Nine patch chunk */
	cs = cairo_surface_create_similar_image(texture->surface, CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create(cs);
	cairo_set_source_surface(cr, texture->surface, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);

	data = cairo_image_surface_get_data(cs);
	stride = cairo_image_surface_get_stride(cs);

	/* Top stretch row line */
	texture->patch.stretch.a = 1;
	texture->patch.stretch.b = width - 2;
	for(i = 1; i < width - 1; i++)
	{
		if(is_black_pixel(&data[stride * 0 + i * 4]))
		{
			texture->patch.stretch.a = i;
			break;
		}
	}
	for(i = width - 2; i > 0; i--)
	{
		if(is_black_pixel(&data[stride * 0 + i * 4]))
		{
			texture->patch.stretch.b = i;
			break;
		}
	}

	/* Left stretch column line */
	texture->patch.stretch.c = 1;
	texture->patch.stretch.d = height - 2;
	for(i = 1; i < height - 1; i++)
	{
		if(is_black_pixel(&data[stride * i + 0 * 4]))
		{
			texture->patch.stretch.c = i;
			break;
		}
	}
	for(i = height - 2; i > 0; i--)
	{
		if(is_black_pixel(&data[stride * i + 0 * 4]))
		{
			texture->patch.stretch.d = i;
			break;
		}
	}

	/* Bottom content row line */
	texture->patch.content.a = 1;
	texture->patch.content.b = width - 2;
	for(i = 1; i < width - 1; i++)
	{
		if(is_black_pixel(&data[stride * (height - 1) + i * 4]))
		{
			texture->patch.content.a = i;
			break;
		}
	}
	for(i = width - 2; i > 0; i--)
	{
		if(is_black_pixel(&data[stride * (height - 1) + i * 4]))
		{
			texture->patch.content.b = i;
			break;
		}
	}

	/* Right content column line */
	texture->patch.content.c = 1;
	texture->patch.content.d = height - 2;
	for(i = 1; i < height - 1; i++)
	{
		if(is_black_pixel(&data[stride * i + (width - 1) * 4]))
		{
			texture->patch.content.c = i;
			break;
		}
	}
	for(i = height - 2; i > 0; i--)
	{
		if(is_black_pixel(&data[stride * i + (width - 1) * 4]))
		{
			texture->patch.content.d = i;
			break;
		}
	}
	cairo_surface_destroy(cs);

	/* Top left */
	w = texture->patch.stretch.a - 1;
	h = texture->patch.stretch.c - 1;
	if(w > 0 && h > 0)
	{
		cs = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
		cr = cairo_create(cs);
		cairo_set_source_surface(cr, texture->surface, -1, -1);
		cairo_paint(cr);
		cairo_destroy(cr);
		texture->patch.lt = cs;
	}
	else
	{
		texture->patch.lt = NULL;
	}

	/* Top Middle */
	w = texture->patch.stretch.b - texture->patch.stretch.a + 1;
	h = texture->patch.stretch.c - 1;
	if(w > 0 && h > 0)
	{
		cs = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
		cr = cairo_create(cs);
		cairo_set_source_surface(cr, texture->surface, -texture->patch.stretch.a, -1);
		cairo_paint(cr);
		cairo_destroy(cr);
		texture->patch.mt = cs;
	}
	else
	{
		texture->patch.mt = NULL;
	}

	/* Top Right */
	w = width -2 - texture->patch.stretch.b;
	h = texture->patch.stretch.c - 1;
	if(w > 0 && h > 0)
	{
		cs = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
		cr = cairo_create(cs);
		cairo_set_source_surface(cr, texture->surface, -texture->patch.stretch.b - 1, -1);
		cairo_paint(cr);
		cairo_destroy(cr);
		texture->patch.rt = cs;
	}
	else
	{
		texture->patch.rt = NULL;
	}

	/* Middle left */
	w = texture->patch.stretch.a - 1;
	h = texture->patch.stretch.d - texture->patch.stretch.c + 1;
	if(w > 0 && h > 0)
	{
		cs = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
		cr = cairo_create(cs);
		cairo_set_source_surface(cr, texture->surface, -1, -texture->patch.stretch.c);
		cairo_paint(cr);
		cairo_destroy(cr);
		texture->patch.lm = cs;
	}
	else
	{
		texture->patch.lm = NULL;
	}

	/* Middle Middle */
	w = texture->patch.stretch.b - texture->patch.stretch.a + 1;
	h = texture->patch.stretch.d - texture->patch.stretch.c + 1;
	if(w > 0 && h > 0)
	{
		cs = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
		cr = cairo_create(cs);
		cairo_set_source_surface(cr, texture->surface, -texture->patch.stretch.a, -texture->patch.stretch.c);
		cairo_paint(cr);
		cairo_destroy(cr);
		texture->patch.mm = cs;
	}
	else
	{
		texture->patch.mm = NULL;
	}

	/* Middle Right */
	w = width -2 - texture->patch.stretch.b;
	h = texture->patch.stretch.d - texture->patch.stretch.c + 1;
	if(w > 0 && h > 0)
	{
		cs = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
		cr = cairo_create(cs);
		cairo_set_source_surface(cr, texture->surface, -texture->patch.stretch.b - 1, -texture->patch.stretch.c);
		cairo_paint(cr);
		cairo_destroy(cr);
		texture->patch.rm = cs;
	}
	else
	{
		texture->patch.rm = NULL;
	}

	/* Bottom left */
	w = texture->patch.stretch.a - 1;
	h = height - 2 - texture->patch.stretch.d;
	if(w > 0 && h > 0)
	{
		cs = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
		cr = cairo_create(cs);
		cairo_set_source_surface(cr, texture->surface, -1, -texture->patch.stretch.d - 1);
		cairo_paint(cr);
		cairo_destroy(cr);
		texture->patch.lb = cs;
	}
	else
	{
		texture->patch.lb = NULL;
	}

	/* Bottom Middle */
	w = texture->patch.stretch.b - texture->patch.stretch.a + 1;
	h = height - 2 - texture->patch.stretch.d;
	if(w > 0 && h > 0)
	{
		cs = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
		cr = cairo_create(cs);
		cairo_set_source_surface(cr, texture->surface, -texture->patch.stretch.a, -texture->patch.stretch.d - 1);
		cairo_paint(cr);
		cairo_destroy(cr);
		texture->patch.mb = cs;
	}
	else
	{
		texture->patch.mb = NULL;
	}

	/* Bottom Right */
	w = width -2 - texture->patch.stretch.b;
	h = height - 2 - texture->patch.stretch.d;
	if(w > 0 && h > 0)
	{
		cs = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
		cr = cairo_create(cs);
		cairo_set_source_surface(cr, texture->surface, -texture->patch.stretch.b - 1, -texture->patch.stretch.d - 1);
		cairo_paint(cr);
		cairo_destroy(cr);
		texture->patch.rb = cs;
	}
	else
	{
		texture->patch.rb = NULL;
	}

	return TRUE;
}

static bool_t match_extension(const char * filename, const char * ext)
{
	int pos, len;

	pos = strlen(filename);
	len = strlen(ext);

	if ((!pos) || (!len) || (len > pos))
		return FALSE;

	pos -= len;
	return (strcasecmp(filename + pos, ext) == 0);
}

static int l_texture_new(lua_State * L)
{
	const char * filename = luaL_checkstring(L, 1);
	if(match_extension(filename, ".png"))
	{
		struct ltexture_t * texture = lua_newuserdata(L, sizeof(struct ltexture_t));
		texture->surface = cairo_image_surface_create_from_png_xfs(filename);
		if(cairo_surface_status(texture->surface) != CAIRO_STATUS_SUCCESS)
			return 0;
		if(match_extension(filename, ".9.png") && fill_nine_patch(texture))
			texture->patch.valid = 1;
		else
			texture->patch.valid = 0;
		luaL_setmetatable(L, MT_NAME_TEXTURE);
		return 1;
	}
	return 0;
}

static const luaL_Reg l_texture[] = {
	{"new",	l_texture_new},
	{NULL,	NULL}
};

static int m_texture_gc(lua_State * L)
{
	struct ltexture_t * texture = luaL_checkudata(L, 1, MT_NAME_TEXTURE);
	cairo_surface_destroy(texture->surface);
	if(texture->patch.valid)
	{
		if(texture->patch.lt)
			cairo_surface_destroy(texture->patch.lt);
		if(texture->patch.mt)
			cairo_surface_destroy(texture->patch.mt);
		if(texture->patch.rt)
			cairo_surface_destroy(texture->patch.rt);
		if(texture->patch.lm)
			cairo_surface_destroy(texture->patch.lm);
		if(texture->patch.mm)
			cairo_surface_destroy(texture->patch.mm);
		if(texture->patch.rm)
			cairo_surface_destroy(texture->patch.rm);
		if(texture->patch.lb)
			cairo_surface_destroy(texture->patch.lb);
		if(texture->patch.mb)
			cairo_surface_destroy(texture->patch.mb);
		if(texture->patch.rb)
			cairo_surface_destroy(texture->patch.rb);
		texture->patch.valid = 0;
	}
	return 0;
}

static int m_texture_size(lua_State * L)
{
	struct ltexture_t * texture = luaL_checkudata(L, 1, MT_NAME_TEXTURE);
	int w = cairo_image_surface_get_width(texture->surface);
	int h = cairo_image_surface_get_height(texture->surface);
	if(texture->patch.valid)
	{
		lua_pushnumber(L, w - 2);
		lua_pushnumber(L, h - 2);
	}
	else
	{
		lua_pushnumber(L, w);
		lua_pushnumber(L, h);
	}
	return 2;
}

static int m_texture_region(lua_State * L)
{
	struct ltexture_t * texture = luaL_checkudata(L, 1, MT_NAME_TEXTURE);
	int x = luaL_optinteger(L, 2, 0);
	int y = luaL_optinteger(L, 3, 0);
	int w = luaL_optinteger(L, 4, cairo_image_surface_get_width(texture->surface));
	int h = luaL_optinteger(L, 5, cairo_image_surface_get_height(texture->surface));
	if(texture->patch.valid)
		return 0;
	struct ltexture_t * tex = lua_newuserdata(L, sizeof(struct ltexture_t));
	tex->surface = cairo_surface_create_similar(texture->surface, cairo_surface_get_content(texture->surface), w, h);
	tex->patch.valid = 0;
	cairo_t * cr = cairo_create(tex->surface);
	cairo_set_source_surface(cr, texture->surface, -x, -y);
	cairo_paint(cr);
	cairo_destroy(cr);
	luaL_setmetatable(L, MT_NAME_TEXTURE);
	return 1;
}

static const luaL_Reg m_texture[] = {
	{"__gc",		m_texture_gc},
	{"size",		m_texture_size},
	{"region",		m_texture_region},
	{NULL,			NULL}
};

int luaopen_texture(lua_State * L)
{
	luaL_newlib(L, l_texture);
	luahelper_create_metatable(L, MT_NAME_TEXTURE, m_texture);
	return 1;
}
