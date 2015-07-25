//
//  CRT.hpp
//  Clock Signal
//
//  Created by Thomas Harte on 19/07/2015.
//  Copyright © 2015 Thomas Harte. All rights reserved.
//

#ifndef CRT_cpp
#define CRT_cpp

#include <stdint.h>
#include <stdarg.h>
#include <string>
#include <vector>

#include "CRTFrame.h"

namespace Outputs {

class CRT;
struct CRTFrameBuilder {
	CRTFrame frame;

	CRTFrameBuilder(int width, int height, int number_of_buffers, va_list buffer_sizes);
	~CRTFrameBuilder();

	private:
		std::vector<uint16_t> _all_runs;

		void reset();
		void complete();

		uint16_t *get_next_run();
		friend CRT;

		void allocate_write_area(int required_length);
		uint8_t *get_write_target_for_buffer(int buffer);

		// a pointer to the section of content buffer currently being
		// returned and to where the next section will begin
		int _write_x_position, _write_y_position;
		int _write_target_pointer;
};

static const int kCRTNumberOfFrames = 3;

class CRT {
	public:
		CRT(int cycles_per_line, int height_of_display, int number_of_buffers, ...);
		~CRT();

		void output_sync(int number_of_cycles);
		void output_blank(int number_of_cycles);
		void output_level(int number_of_cycles, const char *type);
		void output_data(int number_of_cycles, const char *type);

		class CRTDelegate {
			public:
				virtual void crt_did_end_frame(CRT *crt, CRTFrame *frame) = 0;
		};
		void set_delegate(CRTDelegate *delegate);
		void return_frame();

		void allocate_write_area(int required_length);
		uint8_t *get_write_target_for_buffer(int buffer);

	private:
		// fundamental creator-specified properties
		int _cycles_per_line;
		int _height_of_display;

		// properties directly derived from there
		int _hsync_error_window;			// the permitted window around the expected sync position in which a sync pulse will be recognised; calculated once at init

		// the current scanning position
		struct Vector {
			uint32_t x, y;
		} _rasterPosition, _scanSpeed, _retraceSpeed;

		// the run delegate and the triple buffer
		CRTFrameBuilder *_frame_builders[kCRTNumberOfFrames];
		CRTFrameBuilder *_current_frame_builder;
		int _frames_with_delegate;
		int _frame_read_pointer;
		CRTDelegate *_delegate;

		// outer elements of sync separation
		bool _is_receiving_sync;				// true if the CRT is currently receiving sync (i.e. this is for edge triggering of horizontal sync)
		bool _did_detect_hsync;					// true if horizontal sync was detected during this scanline (so, this affects flywheel adjustments)
		int _sync_capacitor_charge_level;		// this charges up during times of sync and depletes otherwise; needs to hit a required threshold to trigger a vertical sync
		int _sync_capacitor_charge_threshold;	// this charges up during times of sync and depletes otherwise; needs to hit a required threshold to trigger a vertical sync
		int _vretrace_counter;					// a down-counter for time during a vertical retrace
		int _vertical_retrace_time;

		// components of the flywheel sync
		int _horizontal_counter;			// time run since the _start_ of the last horizontal sync
		int _expected_next_hsync;			// our current expection of when the next horizontal sync will be encountered (which implies current flywheel velocity)
		int _horizontal_retrace_time;
		bool _is_in_hsync;					// true for the duration of a horizontal sync — used to determine beam running direction and speed

		// the outer entry point for dispatching output_sync, output_blank, output_level and output_data
		enum Type {
			Sync, Level, Data, Blank
		} type;
		void advance_cycles(int number_of_cycles, bool hsync_requested, bool vsync_charging, Type type, const char *data_type);

		// the inner entry point that determines whether and when the next sync event will occur within
		// the current output window
		enum SyncEvent {
			None,
			StartHSync, EndHSync,
			StartVSync, EndVSync
		};
		SyncEvent next_vertical_sync_event(bool vsync_is_charging, int cycles_to_run_for, int *cycles_advanced);
		SyncEvent next_horizontal_sync_event(bool hsync_is_requested, int cycles_to_run_for, int *cycles_advanced);
};

}


#endif /* CRT_cpp */