/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#ifndef CHECKHEADER_SLIB_CORE_DL_LINUX_ALSA
#define CHECKHEADER_SLIB_CORE_DL_LINUX_ALSA

#include "definition.h"

#if defined(SLIB_PLATFORM_IS_LINUX) && defined(SLIB_PLATFORM_IS_DESKTOP)

#include "../core/dl.h"

#include "alsa/asoundlib.h"

namespace slib
{

	SLIB_IMPORT_LIBRARY_BEGIN(alsa, "libasound.so.2")
		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_open,
			int, ,
			snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode
		)
		#define snd_pcm_open slib::alsa::getApi_snd_pcm_open()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_close,
			int, ,
			snd_pcm_t *pcm
		)
		#define snd_pcm_close slib::alsa::getApi_snd_pcm_close()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_nonblock,
			int, ,
			snd_pcm_t *pcm, int nonblock
		)
		#define snd_pcm_nonblock slib::alsa::getApi_snd_pcm_nonblock()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_device_name_hint,
			int, ,
			int, const char*, void*** hints
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(			
			snd_device_name_get_hint,
			char*, ,
			void* hint, const char* prop
		)

		SLIB_IMPORT_LIBRARY_FUNCTION(			
			snd_device_name_free_hint,
			void, ,
			void** hints
		)
		
		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_card_get_name,
			int, ,
			int card, char **name
		)
		#define snd_card_get_name slib::alsa::getApi_snd_card_get_name()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_card_get_longname,
			int, ,
			int card, char **name
		)
		#define snd_card_get_longname slib::alsa::getApi_snd_card_get_longname()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_sizeof,
			size_t, ,
		)
		#define snd_pcm_hw_params_sizeof slib::alsa::getApi_snd_pcm_hw_params_sizeof()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_any,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params
		)
		#define snd_pcm_hw_params_any slib::alsa::getApi_snd_pcm_hw_params_any()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_set_rate_resample,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val
		)
		#define snd_pcm_hw_params_set_rate_resample slib::alsa::getApi_snd_pcm_hw_params_set_rate_resample()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_set_access,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access
		)
		#define snd_pcm_hw_params_set_access slib::alsa::getApi_snd_pcm_hw_params_set_access()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_set_format,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val
		)
		#define snd_pcm_hw_params_set_format slib::alsa::getApi_snd_pcm_hw_params_set_format()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_set_channels,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val
		)
		#define snd_pcm_hw_params_set_channels slib::alsa::getApi_snd_pcm_hw_params_set_channels()
		
		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_set_rate,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val, int dir
		)
		#define snd_pcm_hw_params_set_rate slib::alsa::getApi_snd_pcm_hw_params_set_rate()
		
		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_set_buffer_time_near,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir
		)
		#define snd_pcm_hw_params_set_buffer_time_near slib::alsa::getApi_snd_pcm_hw_params_set_buffer_time_near()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_set_period_time_near,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir
		)
		#define snd_pcm_hw_params_set_period_time_near slib::alsa::getApi_snd_pcm_hw_params_set_period_time_near()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params_set_periods_near,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir
		)
		#define snd_pcm_hw_params_set_periods_near slib::alsa::getApi_snd_pcm_hw_params_set_periods_near()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_hw_params,
			int, ,
			snd_pcm_t *pcm, snd_pcm_hw_params_t *params
		)
		#define snd_pcm_hw_params slib::alsa::getApi_snd_pcm_hw_params()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_sw_params_sizeof,
			size_t, ,
		)
		#define snd_pcm_sw_params_sizeof slib::alsa::getApi_snd_pcm_sw_params_sizeof()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_sw_params_current,
			int, ,
			snd_pcm_t *pcm, snd_pcm_sw_params_t *params
		)
		#define snd_pcm_sw_params_current slib::alsa::getApi_snd_pcm_sw_params_current()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_sw_params_set_start_threshold,
			int, ,
			snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val
		)
		#define snd_pcm_sw_params_set_start_threshold slib::alsa::getApi_snd_pcm_sw_params_set_start_threshold()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_sw_params_set_stop_threshold,
			int, ,
			snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val
		)
		#define snd_pcm_sw_params_set_stop_threshold slib::alsa::getApi_snd_pcm_sw_params_set_stop_threshold()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_sw_params_set_avail_min,
			int, ,
			snd_pcm_t *pcm, snd_pcm_sw_params_t *params, snd_pcm_uframes_t val
		)
		#define snd_pcm_sw_params_set_avail_min slib::alsa::getApi_snd_pcm_sw_params_set_avail_min()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_sw_params,
			int, ,
			snd_pcm_t *pcm, snd_pcm_sw_params_t *params
		)
		#define snd_pcm_sw_params slib::alsa::getApi_snd_pcm_sw_params()
		
		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_frames_to_bytes,
			ssize_t, ,
			snd_pcm_t *pcm, snd_pcm_sframes_t frames
		)
		#define snd_pcm_frames_to_bytes slib::alsa::getApi_snd_pcm_frames_to_bytes()
		
		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_prepare,
			int, ,
			snd_pcm_t *pcm
		)
		#define snd_pcm_prepare slib::alsa::getApi_snd_pcm_prepare()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_start,
			int, ,
			snd_pcm_t *pcm
		)
		#define snd_pcm_start slib::alsa::getApi_snd_pcm_start()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_drop,
			int, ,
			snd_pcm_t *pcm
		)
		#define snd_pcm_drop slib::alsa::getApi_snd_pcm_drop()
		
		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_drain,
			int, ,
			snd_pcm_t *pcm
		)
		#define snd_pcm_drain slib::alsa::getApi_snd_pcm_drain()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_pause,
			int, ,
			snd_pcm_t *pcm, int enable
		)
		#define snd_pcm_pause slib::alsa::getApi_snd_pcm_pause()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_readi,
			snd_pcm_sframes_t, ,
			snd_pcm_t *pcm, void *buffer, snd_pcm_uframes_t size
		)
		#define snd_pcm_readi slib::alsa::getApi_snd_pcm_readi()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_writei,
			snd_pcm_sframes_t, ,
			snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size
		)
		#define snd_pcm_writei slib::alsa::getApi_snd_pcm_writei()

		SLIB_IMPORT_LIBRARY_FUNCTION(
			snd_pcm_avail_update,
			snd_pcm_sframes_t, ,
			snd_pcm_t *pcm
		)
		#define snd_pcm_avail_update slib::alsa::getApi_snd_pcm_avail_update()

	SLIB_IMPORT_LIBRARY_END

}

#endif

#endif
