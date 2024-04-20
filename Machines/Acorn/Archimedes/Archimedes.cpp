//
//  Archimedes.cpp
//  Clock Signal
//
//  Created by Thomas Harte on 04/03/2024.
//  Copyright © 2024 Thomas Harte. All rights reserved.
//

#include "Archimedes.hpp"

#include "HalfDuplexSerial.hpp"
#include "InputOutputController.hpp"
#include "Keyboard.hpp"
#include "KeyboardMapper.hpp"
#include "MemoryController.hpp"
#include "Sound.hpp"

#include "../../AudioProducer.hpp"
#include "../../KeyboardMachine.hpp"
#include "../../MediaTarget.hpp"
#include "../../MouseMachine.hpp"
#include "../../ScanProducer.hpp"
#include "../../TimedMachine.hpp"

#include "../../../Activity/Source.hpp"

#include "../../../InstructionSets/ARM/Disassembler.hpp"
#include "../../../InstructionSets/ARM/Executor.hpp"
#include "../../../Outputs/Log.hpp"
#include "../../../Components/I2C/I2C.hpp"

#include <algorithm>
#include <array>
#include <set>
#include <vector>

namespace Archimedes {

#ifndef NDEBUG
namespace {
Log::Logger<Log::Source::Archimedes> logger;
}

template <InstructionSet::ARM::Model model, typename Executor>
struct HackyDebugger {
	void notify(uint32_t address, uint32_t instruction, Executor &executor) {
		pc_history[pc_history_ptr] = address;
		pc_history_ptr = (pc_history_ptr + 1) % pc_history.size();

//			if(
//				executor_.pc() > 0x038021d0 &&
//					last_r1 != executor_.registers()[1]
//					 ||
//				(
//					last_link != executor_.registers()[14] ||
//					last_r0 != executor_.registers()[0] ||
//					last_r10 != executor_.registers()[10] ||
//					last_r1 != executor_.registers()[1]
//				)
//			) {
//				logger.info().append("%08x modified R14 to %08x; R0 to %08x; R10 to %08x; R1 to %08x",
//					last_pc,
//					executor_.registers()[14],
//					executor_.registers()[0],
//					executor_.registers()[10],
//					executor_.registers()[1]
//				);
//				logger.info().append("%08x modified R1 to %08x",
//					last_pc,
//					executor_.registers()[1]
//				);
//				last_link = executor_.registers()[14];
//				last_r0 = executor_.registers()[0];
//				last_r10 = executor_.registers()[10];
//				last_r1 = executor_.registers()[1];
//			}

//			if(instruction == 0xe8fd7fff) {
//				printf("At %08x [%d]; after last PC %08x and %zu ago was %08x\n",
//					address,
//					instr_count,
//					pc_history[(pc_history_ptr - 2 + pc_history.size()) % pc_history.size()],
//					pc_history.size(),
//					pc_history[pc_history_ptr]);
//			}
//			last_r9 = executor_.registers()[9];

//			log |= address == 0x038031c4;
//			log |= instr_count == 53552731 - 30;
//			log &= executor_.pc() != 0x000000a0;

//			log = (executor_.pc() == 0x038162afc) || (executor_.pc() == 0x03824b00);
//			log |= instruction & ;

		// The following has the effect of logging all taken SWIs and their return codes.
/*		if(
			(instruction & 0x0f00'0000) == 0x0f00'0000 &&
			executor.registers().test(InstructionSet::ARM::Condition(instruction >> 28))
		) {
			if(instruction & 0x2'0000) {
				swis.emplace_back();
				swis.back().count = swi_count++;
				swis.back().opcode = instruction;
				swis.back().address = executor.pc();
				swis.back().return_address = executor.registers().pc(4);
				for(int c = 0; c < 10; c++) swis.back().regs[c] = executor.registers()[uint32_t(c)];

				// Possibly capture more detail.
				//
				// Cf. http://productsdb.riscos.com/support/developers/prm_index/numswilist.html
				uint32_t pointer = 0;
				switch(instruction & 0xfd'ffff) {
					case 0x41501:
						swis.back().swi_name = "MessageTrans_OpenFile";

						// R0: pointer to file descriptor; R1: pointer to filename; R2: pointer to hold file data.
						// (R0 and R1 are in the RMA if R2 = 0)
						pointer = executor.registers()[1];
					break;
					case 0x41502:
						swis.back().swi_name = "MessageTrans_Lookup";
					break;
					case 0x41506:
						swis.back().swi_name = "MessageTrans_ErrorLookup";
					break;

					case 0x4028a:
						swis.back().swi_name = "Podule_EnumerateChunksWithInfo";
					break;

					case 0x4000a:
						swis.back().swi_name = "Econet_ReadLocalStationAndNet";
					break;
					case 0x4000e:
						swis.back().swi_name = "Econet_SetProtection";
					break;
					case 0x40015:
						swis.back().swi_name = "Econet_ClaimPort";
					break;

					case 0x40541:
						swis.back().swi_name = "FileCore_Create";
					break;

					case 0x80156:
					case 0x8015b:
						swis.back().swi_name = "PDriver_MiscOpForDriver";
					break;

					case 0x05:
						swis.back().swi_name = "OS_CLI";
						pointer = executor.registers()[0];
					break;
					case 0x0d:
						swis.back().swi_name = "OS_Find";
						if(executor.registers()[0] >= 0x40) {
							pointer = executor.registers()[1];
						}
					break;
					case 0x1d:
						swis.back().swi_name = "OS_Heap";
					break;
					case 0x1e:
						swis.back().swi_name = "OS_Module";
					break;

					case 0x20:
						swis.back().swi_name = "OS_Release";
					break;
					case 0x21:
						swis.back().swi_name = "OS_ReadUnsigned";
					break;
					case 0x23:
						swis.back().swi_name = "OS_ReadVarVal";

						// R0: pointer to variable name.
						pointer = executor.registers()[0];
					break;
					case 0x24:
						swis.back().swi_name = "OS_SetVarVal";

						// R0: pointer to variable name.
						pointer = executor.registers()[0];
					break;
					case 0x26:
						swis.back().swi_name = "OS_GSRead";
					break;
					case 0x27:
						swis.back().swi_name = "OS_GSTrans";
						pointer = executor.registers()[0];
					break;
					case 0x29:
						swis.back().swi_name = "OS_FSControl";
					break;
					case 0x2a:
						swis.back().swi_name = "OS_ChangeDynamicArea";
					break;

					case 0x4c:
						swis.back().swi_name = "OS_ReleaseDeviceVector";
					break;

					case 0x43057:
						swis.back().swi_name = "Territory_LowerCaseTable";
					break;
					case 0x43058:
						swis.back().swi_name = "Territory_UpperCaseTable";
					break;

					case 0x42fc0:
						swis.back().swi_name = "Portable_Speed";
					break;
					case 0x42fc1:
						swis.back().swi_name = "Portable_Control";
					break;
				}

				if(pointer) {
					while(true) {
						uint8_t next;
						executor.bus.template read<uint8_t>(pointer, next, InstructionSet::ARM::Mode::Supervisor, false);
						++pointer;

						if(next < 32) break;
						swis.back().value_name.push_back(static_cast<char>(next));
					}
				}

			}

			if(executor.registers().pc_status(0) & InstructionSet::ARM::ConditionCode::Overflow) {
				logger.error().append("SWI called with V set");
			}
		}
		if(!swis.empty() && executor.pc() == swis.back().return_address) {
			// Overflow set => SWI failure.
			auto &back = swis.back();
			if(executor.registers().pc_status(0) & InstructionSet::ARM::ConditionCode::Overflow) {
				auto info = logger.info();

				info.append("[%d] Failed swi ", back.count);
				if(back.swi_name.empty()) {
					info.append("&%x", back.opcode & 0xfd'ffff);
				} else {
					info.append("%s", back.swi_name.c_str());
				}

				if(!back.value_name.empty()) {
					info.append(" %s", back.value_name.c_str());
				}

				info.append(" @ %08x ", back.address);
				for(uint32_t c = 0; c < 10; c++) {
					info.append("r%d:%08x ", c, back.regs[c]);
				}
			}

			swis.pop_back();
		}*/

		if(log) {
			InstructionSet::ARM::Disassembler<model> disassembler;
			InstructionSet::ARM::dispatch<model>(instruction, disassembler);

			auto info = logger.info();
			info.append("[%d] %08x: %08x\t\t%s\t prior:[",
				instr_count,
				executor.pc(),
				instruction,
				disassembler.last().to_string(executor.pc()).c_str());
			for(uint32_t c = 0; c < 15; c++) {
				info.append("r%d:%08x ", c, executor.registers()[c]);
			}
			info.append("]");
		}
//		opcodes.insert(instruction);
//		if(accumulate) {
//			int c = 0;
//			for(auto instr : opcodes) {
//				printf("0x%08x, ", instr);
//				++c;
//				if(!(c&15)) printf("\n");
//			}
//			accumulate = false;
//		}

		++instr_count;
	}

private:
	std::array<uint32_t, 75> pc_history;
	std::size_t pc_history_ptr = 0;
	uint32_t instr_count = 0;
	uint32_t swi_count = 0;

	struct SWICall {
		uint32_t count;
		uint32_t opcode;
		uint32_t address;
		uint32_t regs[10];
		uint32_t return_address;
		std::string value_name;
		std::string swi_name;
	};
	std::vector<SWICall> swis;
	uint32_t last_pc = 0;
//	uint32_t last_r9 = 0;
	bool log = false;
	bool accumulate = true;

	std::set<uint32_t> opcodes;
};
#else
template <InstructionSet::ARM::Model model, typename Executor>
struct HackyDebugger {
	void notify(uint32_t, uint32_t, Executor &) {}
};
#endif

class ConcreteMachine:
	public Machine,
	public MachineTypes::AudioProducer,
	public MachineTypes::MappedKeyboardMachine,
	public MachineTypes::MediaTarget,
	public MachineTypes::MouseMachine,
	public MachineTypes::TimedMachine,
	public MachineTypes::ScanProducer,
	public Activity::Source
{
	private:
		// TODO: pick a sensible clock rate; this is just code for '24 MIPS, please'.
		static constexpr int ClockRate = 24'000'000;

		// Runs for 24 cycles, distributing calls to the various ticking subsystems
		// 'correctly' (i.e. correctly for the approximation in use).
		//
		// The implementation of this is coupled to the ClockRate above, hence its
		// appearance here.
		template <int video_divider>
		void macro_tick() {
			macro_counter_ -= 24;

			// This is a 24-cycle window, so at 24Mhz macro_tick() is called at 1Mhz.
			// Hence, required ticks are:
			//
			// 	* CPU: 24;
			//	* video: 24 / video_divider;
			//	* floppy: 8;
			//	* timers: 2;
			//	* sound: 1.

			tick_cpu_video<0, video_divider>();		tick_cpu_video<1, video_divider>();
			tick_cpu_video<2, video_divider>();		tick_floppy();
			tick_cpu_video<3, video_divider>();		tick_cpu_video<4, video_divider>();
			tick_cpu_video<5, video_divider>();		tick_floppy();
			tick_cpu_video<6, video_divider>();		tick_cpu_video<7, video_divider>();
			tick_cpu_video<8, video_divider>();		tick_floppy();
			tick_cpu_video<9, video_divider>();		tick_cpu_video<10, video_divider>();
			tick_cpu_video<11, video_divider>();	tick_floppy();
			tick_timers();

			tick_cpu_video<12, video_divider>();	tick_cpu_video<13, video_divider>();
			tick_cpu_video<14, video_divider>();	tick_floppy();
			tick_cpu_video<15, video_divider>();	tick_cpu_video<16, video_divider>();
			tick_cpu_video<17, video_divider>();	tick_floppy();
			tick_cpu_video<18, video_divider>();	tick_cpu_video<19, video_divider>();
			tick_cpu_video<20, video_divider>();	tick_floppy();
			tick_cpu_video<21, video_divider>();	tick_cpu_video<22, video_divider>();
			tick_cpu_video<23, video_divider>();	tick_floppy();
			tick_timers();
			tick_sound();
		}
		int macro_counter_ = 0;

		template <int offset, int video_divider>
		void tick_cpu_video() {
			if constexpr (!(offset % video_divider)) {
				tick_video();
			}

#ifndef NDEBUG
			// Debug mode: run CPU a lot slower. Actually at close to original advertised MIPS speed.
			if constexpr (offset & 7) return;
#endif
			if constexpr (offset & 1) return;
			tick_cpu();
		}

	public:
		ConcreteMachine(
			const Analyser::Static::Target &target,
			const ROMMachine::ROMFetcher &rom_fetcher
		) : executor_(*this, *this, *this) {
			set_clock_rate(ClockRate);

			constexpr ROM::Name risc_os = ROM::Name::AcornRISCOS311;
			ROM::Request request(risc_os);
			auto roms = rom_fetcher(request);
			if(!request.validate(roms)) {
				throw ROMMachine::Error::MissingROMs;
			}

			executor_.bus.set_rom(roms.find(risc_os)->second);
			insert_media(target.media);

			fill_pipeline(0);
		}

		void update_interrupts() {
			using Exception = InstructionSet::ARM::Registers::Exception;

			const int requests = executor_.bus.interrupt_mask();
			if((requests & InterruptRequests::FIQ) && executor_.registers().would_interrupt<Exception::FIQ>()) {
				pipeline_.reschedule(Pipeline::SWISubversion::FIQ);
				return;
			}
			if((requests & InterruptRequests::IRQ) && executor_.registers().would_interrupt<Exception::IRQ>()) {
				pipeline_.reschedule(Pipeline::SWISubversion::IRQ);
			}
		}

		void did_set_status() {
			// This might have been a change of mode, so...
			fill_pipeline(executor_.pc());
			update_interrupts();
		}

		void did_set_pc() {
			fill_pipeline(executor_.pc());
		}

		bool should_swi(uint32_t) {
			using Exception = InstructionSet::ARM::Registers::Exception;
			using SWISubversion = Pipeline::SWISubversion;

			switch(pipeline_.swi_subversion()) {
				case Pipeline::SWISubversion::None:
				return true;

				case SWISubversion::DataAbort:
//					executor_.set_pc(executor_.pc() - 4);
					executor_.registers().exception<Exception::DataAbort>();
				break;

				// FIQ and IRQ decrement the PC because their apperance in the pipeline causes
				// it to look as though they were fetched, but they weren't.
				case SWISubversion::FIQ:
					executor_.set_pc(executor_.pc() - 4);
					executor_.registers().exception<Exception::FIQ>();
				break;
				case SWISubversion::IRQ:
					executor_.set_pc(executor_.pc() - 4);
					executor_.registers().exception<Exception::IRQ>();
				break;
			}

			did_set_pc();
			return false;
		}

		void update_clock_rates() {
			video_divider_ = executor_.bus.video().clock_divider();
		}

	private:
		// MARK: - ScanProducer.
		void set_scan_target(Outputs::Display::ScanTarget *scan_target) override {
			executor_.bus.video().crt().set_scan_target(scan_target);
		}
		Outputs::Display::ScanStatus get_scaled_scan_status() const override {
			return executor_.bus.video().crt().get_scaled_scan_status() * video_divider_;
		}

		// MARK: - TimedMachine.
		void run_for(Cycles cycles) override {
			macro_counter_ += cycles.as<int>();

			while(macro_counter_ > 0) {
				switch(video_divider_) {
					default:	macro_tick<2>();	break;
					case 3:		macro_tick<3>();	break;
					case 4:		macro_tick<4>();	break;
					case 6:		macro_tick<6>();	break;
				}

			}
		}
		int video_divider_ = 1;

		void tick_cpu() {
			const uint32_t instruction = advance_pipeline(executor_.pc() + 8);
			debugger_.notify(executor_.pc(), instruction, executor_);
			InstructionSet::ARM::execute(instruction, executor_);
		}

		void tick_timers()	{	executor_.bus.tick_timers();	}
		void tick_sound()	{	executor_.bus.sound().tick();	}
		void tick_video()	{	executor_.bus.video().tick();	}
		void tick_floppy()	{	executor_.bus.tick_floppy();	}

		// MARK: - MediaTarget
		bool insert_media(const Analyser::Static::Media &media) override {
			size_t c = 0;
			for(auto &disk : media.disks) {
				executor_.bus.set_disk(disk, c);
				c++;
				if(c == 4) break;
			}
			return true;
		}

		// MARK: - AudioProducer
		Outputs::Speaker::Speaker *get_speaker() override {
			return executor_.bus.speaker();
		}

		// MARK: - Activity::Source.
		void set_activity_observer(Activity::Observer *observer) final {
			executor_.bus.set_activity_observer(observer);
		}

		// MARK: - MappedKeyboardMachine.
		MappedKeyboardMachine::KeyboardMapper *get_keyboard_mapper() override {
			return &keyboard_mapper_;
		}
		Archimedes::KeyboardMapper keyboard_mapper_;

		void set_key_state(uint16_t key, bool is_pressed) override {
			const int row = Archimedes::KeyboardMapper::row(key);
			const int column = Archimedes::KeyboardMapper::column(key);
			executor_.bus.keyboard().set_key_state(row, column, is_pressed);
		}

		// MARK: - MouseMachine.
		Inputs::Mouse &get_mouse() override {
			return executor_.bus.keyboard().mouse();
		}

		// MARK: - ARM execution.
		static constexpr auto arm_model = InstructionSet::ARM::Model::ARMv2;
		using Executor = InstructionSet::ARM::Executor<arm_model, MemoryController<ConcreteMachine, ConcreteMachine>, ConcreteMachine>;
		Executor executor_;

		void fill_pipeline(uint32_t pc) {
			if(pipeline_.interrupt_next()) return;
			advance_pipeline(pc);
			advance_pipeline(pc + 4);
		}

		uint32_t advance_pipeline(uint32_t pc) {
			uint32_t instruction;
			const bool did_read = executor_.bus.read(pc, instruction, executor_.registers().mode(), false);
			return pipeline_.exchange(
				instruction,
				did_read ? Pipeline::SWISubversion::None : Pipeline::SWISubversion::DataAbort);
		}

		struct Pipeline {
			enum SWISubversion: uint8_t {
				None,
				DataAbort,
				IRQ,
				FIQ,
			};

			uint32_t exchange(uint32_t next, SWISubversion subversion) {
				const uint32_t result = upcoming_[active_].opcode;
				latched_subversion_ = upcoming_[active_].subversion;

				upcoming_[active_].opcode = next;
				upcoming_[active_].subversion = subversion;
				active_ ^= 1;

				return result;
			}

			SWISubversion swi_subversion() const {
				return latched_subversion_;
			}

			// TODO: one day, possibly: schedule the subversion one slot further into the future
			// (i.e. active_ ^ 1) to allow one further instruction to occur as usual before the
			// action paplies. That is, if interrupts take effect one instruction later after a flags
			// change, which I don't yet know.
			//
			// In practice I got into a bit of a race condition between interrupt scheduling and
			// flags changes, so have backed off for now.
			void reschedule(SWISubversion subversion) {
				upcoming_[active_].opcode = 0xef'000000;
				upcoming_[active_].subversion = subversion;
			}

			bool interrupt_next() const {
				return upcoming_[active_].subversion == SWISubversion::IRQ || upcoming_[active_].subversion == SWISubversion::FIQ;
			}

		private:
			struct Stage {
				uint32_t opcode;
				SWISubversion subversion = SWISubversion::None;
			};
			Stage upcoming_[2];
			int active_ = 0;

			SWISubversion latched_subversion_;
		} pipeline_;

		// MARK: - Yucky, temporary junk.
		HackyDebugger<arm_model, Executor> debugger_;
};

}

using namespace Archimedes;

std::unique_ptr<Machine> Machine::Archimedes(const Analyser::Static::Target *target, const ROMMachine::ROMFetcher &rom_fetcher) {
	return std::make_unique<ConcreteMachine>(*target, rom_fetcher);
}