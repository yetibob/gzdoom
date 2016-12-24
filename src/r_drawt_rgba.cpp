/*
**  Drawer commands for the RT family of drawers
**  Copyright (c) 2016 Magnus Norddahl
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
*/

#include "templates.h"
#include "doomtype.h"
#include "doomdef.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"
#include "r_draw_rgba.h"
#include "r_drawers.h"

namespace swrenderer
{
	WorkerThreadData DrawColumnRt1LLVMCommand::ThreadData(DrawerThread *thread)
	{
		WorkerThreadData d;
		d.core = thread->core;
		d.num_cores = thread->num_cores;
		d.pass_start_y = thread->pass_start_y;
		d.pass_end_y = thread->pass_end_y;
		d.temp = thread->dc_temp_rgba;
		return d;
	}

	DrawColumnRt1LLVMCommand::DrawColumnRt1LLVMCommand(int hx, int sx, int yl, int yh)
	{
		using namespace drawerargs;

		args.dest = (uint32_t*)dc_destorg + ylookup[yl] + sx;
		args.source = nullptr;
		args.source2 = nullptr;
		args.colormap = dc_colormap;
		args.translation = dc_translation;
		args.basecolors = (const uint32_t *)GPalette.BaseColors;
		args.pitch = dc_pitch;
		args.count = yh - yl + 1;
		args.dest_y = yl;
		args.iscale = dc_iscale;
		args.texturefrac = hx;
		args.light = LightBgra::calc_light_multiplier(dc_light);
		args.color = LightBgra::shade_pal_index_simple(dc_color, args.light);
		args.srccolor = dc_srccolor_bgra;
		args.srcalpha = dc_srcalpha >> (FRACBITS - 8);
		args.destalpha = dc_destalpha >> (FRACBITS - 8);
		args.light_red = dc_shade_constants.light_red;
		args.light_green = dc_shade_constants.light_green;
		args.light_blue = dc_shade_constants.light_blue;
		args.light_alpha = dc_shade_constants.light_alpha;
		args.fade_red = dc_shade_constants.fade_red;
		args.fade_green = dc_shade_constants.fade_green;
		args.fade_blue = dc_shade_constants.fade_blue;
		args.fade_alpha = dc_shade_constants.fade_alpha;
		args.desaturate = dc_shade_constants.desaturate;
		args.flags = 0;
		if (dc_shade_constants.simple_shade)
			args.flags |= DrawColumnArgs::simple_shade;
		if (args.source2 == nullptr)
			args.flags |= DrawColumnArgs::nearest_filter;

		DetectRangeError(args.dest, args.dest_y, args.count);
	}

	void DrawColumnRt1LLVMCommand::Execute(DrawerThread *thread)
	{
		WorkerThreadData d = ThreadData(thread);
		Drawers::Instance()->DrawColumnRt1(&args, &d);
	}

	FString DrawColumnRt1LLVMCommand::DebugInfo()
	{
		return "DrawColumnRt\n" + args.ToString();
	}

	/////////////////////////////////////////////////////////////////////////////

	RtInitColsRGBACommand::RtInitColsRGBACommand(BYTE *buff)
	{
		this->buff = buff;
	}

	void RtInitColsRGBACommand::Execute(DrawerThread *thread)
	{
		thread->dc_temp_rgba = buff == NULL ? thread->dc_temp_rgbabuff_rgba : (uint32_t*)buff;
	}

	FString RtInitColsRGBACommand::DebugInfo()
	{
		return "RtInitCols";
	}

	/////////////////////////////////////////////////////////////////////////////

	template<typename InputPixelType>
	DrawColumnHorizRGBACommand<InputPixelType>::DrawColumnHorizRGBACommand()
	{
		using namespace drawerargs;

		_count = dc_count;
		_iscale = dc_iscale;
		_texturefrac = dc_texturefrac;
		_source = (const InputPixelType *)dc_source;
		_x = dc_x;
		_yl = dc_yl;
		_yh = dc_yh;
	}

	template<typename InputPixelType>
	void DrawColumnHorizRGBACommand<InputPixelType>::Execute(DrawerThread *thread)
	{
		int count = _count;
		uint32_t *dest;
		fixed_t fracstep;
		fixed_t frac;

		if (count <= 0)
			return;

		{
			int x = _x & 3;
			dest = &thread->dc_temp_rgba[x + 4 * _yl];
		}
		fracstep = _iscale;
		frac = _texturefrac;

		const InputPixelType *source = _source;

		if (count & 1) {
			*dest = source[frac >> FRACBITS]; dest += 4; frac += fracstep;
		}
		if (count & 2) {
			dest[0] = source[frac >> FRACBITS]; frac += fracstep;
			dest[4] = source[frac >> FRACBITS]; frac += fracstep;
			dest += 8;
		}
		if (count & 4) {
			dest[0] = source[frac >> FRACBITS]; frac += fracstep;
			dest[4] = source[frac >> FRACBITS]; frac += fracstep;
			dest[8] = source[frac >> FRACBITS]; frac += fracstep;
			dest[12] = source[frac >> FRACBITS]; frac += fracstep;
			dest += 16;
		}
		count >>= 3;
		if (!count) return;

		do
		{
			dest[0] = source[frac >> FRACBITS]; frac += fracstep;
			dest[4] = source[frac >> FRACBITS]; frac += fracstep;
			dest[8] = source[frac >> FRACBITS]; frac += fracstep;
			dest[12] = source[frac >> FRACBITS]; frac += fracstep;
			dest[16] = source[frac >> FRACBITS]; frac += fracstep;
			dest[20] = source[frac >> FRACBITS]; frac += fracstep;
			dest[24] = source[frac >> FRACBITS]; frac += fracstep;
			dest[28] = source[frac >> FRACBITS]; frac += fracstep;
			dest += 32;
		} while (--count);
	}

	template<typename InputPixelType>
	FString DrawColumnHorizRGBACommand<InputPixelType>::DebugInfo()
	{
		return "DrawColumnHoriz";
	}

	// Generate code for the versions we use:
	template class DrawColumnHorizRGBACommand<uint8_t>;
	template class DrawColumnHorizRGBACommand<uint32_t>;

	/////////////////////////////////////////////////////////////////////////////

	FillColumnHorizRGBACommand::FillColumnHorizRGBACommand()
	{
		using namespace drawerargs;

		_x = dc_x;
		_count = dc_count;
		_color = GPalette.BaseColors[dc_color].d | (uint32_t)0xff000000;
		_yl = dc_yl;
		_yh = dc_yh;
	}

	void FillColumnHorizRGBACommand::Execute(DrawerThread *thread)
	{
		int count = _count;
		uint32_t color = _color;
		uint32_t *dest;

		if (count <= 0)
			return;

		{
			int x = _x & 3;
			dest = &thread->dc_temp_rgba[x + 4 * _yl];
		}

		if (count & 1) {
			*dest = color;
			dest += 4;
		}
		if (!(count >>= 1))
			return;
		do {
			dest[0] = color; dest[4] = color;
			dest += 8;
		} while (--count);
	}

	FString FillColumnHorizRGBACommand::DebugInfo()
	{
		return "FillColumnHoriz";
	}

	/////////////////////////////////////////////////////////////////////////////

	namespace
	{
		static uint32_t particle_texture[16 * 16] =
		{
			1*1, 2*1, 3*1, 4*1, 5*1, 6*1, 7*1, 8*1, 8*1, 7*1, 6*1, 5*1, 4*1, 3*1, 2*1, 1*1,
			1*2, 2*2, 3*2, 4*2, 5*2, 6*2, 7*2, 8*2, 8*2, 7*2, 6*2, 5*2, 4*2, 3*2, 2*2, 1*2,
			1*3, 2*3, 3*3, 4*3, 5*3, 6*3, 7*3, 8*3, 8*3, 7*3, 6*3, 5*3, 4*3, 3*3, 2*3, 1*3,
			1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4, 8*4, 7*4, 6*4, 5*4, 4*4, 3*4, 2*4, 1*4,
			1*5, 2*5, 3*5, 4*5, 5*5, 6*5, 7*5, 8*5, 8*5, 7*5, 6*5, 5*5, 4*5, 3*5, 2*5, 1*5,
			1*6, 2*6, 3*6, 4*6, 5*6, 6*6, 7*6, 8*6, 8*6, 7*6, 6*6, 5*6, 4*6, 3*6, 2*6, 1*6,
			1*7, 2*7, 3*7, 4*7, 5*7, 6*7, 7*7, 8*7, 8*7, 7*7, 6*7, 5*7, 4*7, 3*7, 2*7, 1*7,
			1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8,
			1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 8*8, 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8,
			1*7, 2*7, 3*7, 4*7, 5*7, 6*7, 7*7, 8*7, 8*7, 7*7, 6*7, 5*7, 4*7, 3*7, 2*7, 1*7,
			1*6, 2*6, 3*6, 4*6, 5*6, 6*6, 7*6, 8*6, 8*6, 7*6, 6*6, 5*6, 4*6, 3*6, 2*6, 1*6,
			1*5, 2*5, 3*5, 4*5, 5*5, 6*5, 7*5, 8*5, 8*5, 7*5, 6*5, 5*5, 4*5, 3*5, 2*5, 1*5,
			1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4, 8*4, 8*4, 7*4, 6*4, 5*4, 4*4, 3*4, 2*4, 1*4,
			1*3, 2*3, 3*3, 4*3, 5*3, 6*3, 7*3, 8*3, 8*3, 7*3, 6*3, 5*3, 4*3, 3*3, 2*3, 1*3,
			1*2, 2*2, 3*2, 4*2, 5*2, 6*2, 7*2, 8*2, 8*2, 7*2, 6*2, 5*2, 4*2, 3*2, 2*2, 1*2,
			1*1, 2*1, 3*1, 4*1, 5*1, 6*1, 7*1, 8*1, 8*1, 7*1, 6*1, 5*1, 4*1, 3*1, 2*1, 1*1
		};
	}

	DrawParticleColumnRGBACommand::DrawParticleColumnRGBACommand(uint32_t *dest, int dest_y, int pitch, int count, uint32_t fg, uint32_t alpha, uint32_t fracposx)
	{
		_dest = dest;
		_pitch = pitch;
		_count = count;
		_fg = fg;
		_alpha = alpha;
		_fracposx = fracposx;
		_dest_y = dest_y;
	}

	void DrawParticleColumnRGBACommand::Execute(DrawerThread *thread)
	{
		int count = thread->count_for_thread(_dest_y, _count);
		if (count <= 0)
			return;

		uint32_t *dest = thread->dest_for_thread(_dest_y, _pitch, _dest);
		int pitch = _pitch * thread->num_cores;

		const uint32_t *source = &particle_texture[(_fracposx >> FRACBITS) * 16];
		uint32_t particle_alpha = _alpha;

		uint32_t fracstep = 16 * FRACUNIT / _count;
		uint32_t fracpos = fracstep * thread->skipped_by_thread(_dest_y) + fracstep / 2;
		fracstep *= thread->num_cores;

		uint32_t fg_red = (_fg >> 16) & 0xff;
		uint32_t fg_green = (_fg >> 8) & 0xff;
		uint32_t fg_blue = _fg & 0xff;

		for (int y = 0; y < count; y++)
		{
			uint32_t alpha = (source[fracpos >> FRACBITS] * particle_alpha) >> 6;
			uint32_t inv_alpha = 256 - alpha;

			uint32_t bg_red = (*dest >> 16) & 0xff;
			uint32_t bg_green = (*dest >> 8) & 0xff;
			uint32_t bg_blue = (*dest) & 0xff;

			uint32_t red = (fg_red * alpha + bg_red * inv_alpha) / 256;
			uint32_t green = (fg_green * alpha + bg_green * inv_alpha) / 256;
			uint32_t blue = (fg_blue * alpha + bg_blue * inv_alpha) / 256;

			*dest = 0xff000000 | (red << 16) | (green << 8) | blue;
			dest += pitch;
			fracpos += fracstep;
		}
	}

	FString DrawParticleColumnRGBACommand::DebugInfo()
	{
		return "DrawParticle";
	}
}
